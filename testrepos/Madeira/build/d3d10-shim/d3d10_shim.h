/* D3D10 / D3D10.1 -> DXMT shim: shared helpers.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Additional permission: Madeira Converter Exception, version 1
 * (testrepos/Madeira/LICENSE-EXCEPTION.md)
 *
 * Why this exists: DXMT implements Direct3D 10 inside d3d10core.dll, which
 * exports only D3D10CoreCreateDevice. Real D3D10 games never touch that
 * symbol -- they import D3D10CreateDevice from d3d10.dll (or
 * D3D10CreateDevice1 from d3d10_1.dll). Without these two DLLs, a D3D10
 * title falls back to Wine's wined3d-based d3d10, which has no Metal
 * backend on iOS. These shims bridge the gap so D3D10 games run on the
 * same DXMT -> Metal path as D3D11 games.
 *
 * Both DLLs resolve d3d10core.dll / dxgi.dll / d3dcompiler_43.dll at
 * runtime with LoadLibrary/GetProcAddress, so they link against nothing
 * but kernel32. That keeps them independent of link order and lets the
 * same binaries work wherever DXMT's dxgi/d3d10core are deployed
 * (system32 in the Wine prefix on iOS).
 *
 * GUID note: this header is included by exactly one translation unit per
 * DLL, so defining INITGUID here gives each DLL its own copy of the
 * DirectX interface IDs without duplicate-symbol risk.
 */

#ifndef D3D10_SHIM_H
#define D3D10_SHIM_H

#define INITGUID
#define COBJMACROS
#include <windows.h>
#include <d3d10.h>
#include <d3d10_1.h>
#include <dxgi.h>

/* D3D10CoreCreateDevice is DXMT's entry point (see
 * research/dxmt/src/d3d10/d3d10core.cpp). It takes the DXGI factory and
 * adapter explicitly because DXMT's device needs the Metal-backed
 * IMTLDXGIAdapter -- it cannot pick one itself. */
typedef HRESULT(WINAPI *PFN_D3D10CoreCreateDevice)(
    IDXGIFactory *pFactory, IDXGIAdapter *pAdapter, UINT Flags,
    D3D_FEATURE_LEVEL FeatureLevel, ID3D10Device **ppDevice);

typedef HRESULT(WINAPI *PFN_CreateDXGIFactory)(REFIID riid, void **ppFactory);

/* Race-free lazy module loading. LoadLibrary is idempotent, but two
 * threads racing the naive `if (!mod) mod = LoadLibraryA(...)` can both
 * publish a handle. Publish exactly one via an interlocked
 * compare-exchange; the loser's handle was never visible to any other
 * thread, so freeing it is safe. A failed load (NULL) is never published,
 * so the next call retries instead of caching the failure. */
static HMODULE shim_load_module_once(volatile HMODULE *slot, LPCSTR name) {
  HMODULE mod = *slot;
  if (!mod) {
    HMODULE loaded = LoadLibraryA(name);
    if (InterlockedCompareExchangePointer((PVOID volatile *)slot,
                                          (PVOID)loaded, NULL) != NULL) {
      /* Another thread published first; drop our private handle. */
      if (loaded)
        FreeLibrary(loaded);
    }
    mod = *slot;
  }
  return mod;
}

/* Lazily loaded module handles. The extra reference is never freed, which
 * is correct for process-lifetime DLLs. */
static HMODULE shim_d3d10core_module(void) {
  static volatile HMODULE mod = NULL;
  return shim_load_module_once(&mod, "d3d10core.dll");
}

static HMODULE shim_dxgi_module(void) {
  static volatile HMODULE mod = NULL;
  return shim_load_module_once(&mod, "dxgi.dll");
}

static HMODULE shim_d3dcompiler_module(void) {
  static volatile HMODULE mod = NULL;
  return shim_load_module_once(&mod, "d3dcompiler_43.dll");
}

static PFN_D3D10CoreCreateDevice shim_core_create_device(void) {
  HMODULE mod = shim_d3d10core_module();
  if (!mod)
    return NULL;
  return (PFN_D3D10CoreCreateDevice)GetProcAddress(mod, "D3D10CoreCreateDevice");
}

static PFN_CreateDXGIFactory shim_create_dxgi_factory(void) {
  HMODULE mod = shim_dxgi_module();
  if (!mod)
    return NULL;
  return (PFN_CreateDXGIFactory)GetProcAddress(mod, "CreateDXGIFactory");
}

/* Resolve the factory/adapter pair DXMT needs.
 *
 * If the caller supplied an adapter, use it and recover its factory via
 * GetParent (DXMT's adapter implements it). Otherwise create DXMT's
 * factory and take adapter 0 (the Metal device).
 *
 * On success the caller owns one reference on each out pointer. */
static HRESULT shim_resolve_factory_adapter(IDXGIAdapter *pAdapter,
                                            IDXGIFactory **ppFactory,
                                            IDXGIAdapter **ppAdapter) {
  PFN_CreateDXGIFactory createFactory = shim_create_dxgi_factory();
  IDXGIFactory *factory = NULL;
  IDXGIAdapter *adapter = NULL;
  HRESULT hr;

  if (!createFactory)
    return E_NOINTERFACE; /* DXMT's dxgi.dll is not deployed */

  if (pAdapter) {
    adapter = pAdapter;
    IDXGIAdapter_AddRef(adapter);
    /* GetParent is declared on IDXGIObject; IDXGIAdapter inherits its vtbl
     * slot, so call through the IDXGIObject macro. */
    hr = IDXGIObject_GetParent((IDXGIObject *)adapter, &IID_IDXGIFactory,
                               (void **)&factory);
    if (FAILED(hr)) {
      /* The adapter came from somewhere unexpected; fall back to a fresh
       * DXMT factory rather than failing outright. */
      hr = createFactory(&IID_IDXGIFactory, (void **)&factory);
      if (FAILED(hr)) {
        IDXGIAdapter_Release(adapter);
        return hr;
      }
    }
  } else {
    hr = createFactory(&IID_IDXGIFactory, (void **)&factory);
    if (FAILED(hr))
      return hr;
    hr = IDXGIFactory_EnumAdapters(factory, 0, &adapter);
    if (FAILED(hr)) {
      IDXGIFactory_Release(factory);
      return hr;
    }
  }

  *ppFactory = factory;
  *ppAdapter = adapter;
  return S_OK;
}

/* Validate the arguments shared by every CreateDevice entry point. */
static HRESULT shim_validate_create_args(D3D10_DRIVER_TYPE DriverType,
                                         HMODULE Software, UINT SDKVersion,
                                         void **ppDevice) {
  if (!ppDevice)
    return E_INVALIDARG;
  *ppDevice = NULL;
  if (SDKVersion != D3D10_SDK_VERSION)
    return DXGI_ERROR_SDK_COMPONENT_MISSING;
  /* DXMT is a hardware Metal renderer. Reference / null / software / WARP
   * drivers have no backend here. */
  if (DriverType != D3D10_DRIVER_TYPE_HARDWARE)
    return DXGI_ERROR_UNSUPPORTED;
  if (Software)
    return E_INVALIDARG;
  return S_OK;
}

/* Core device creation shared by d3d10.dll and d3d10_1.dll. */
static HRESULT shim_create_d3d10_device(IDXGIAdapter *pAdapter,
                                        D3D10_DRIVER_TYPE DriverType,
                                        HMODULE Software, UINT Flags,
                                        UINT SDKVersion,
                                        D3D_FEATURE_LEVEL featureLevel,
                                        ID3D10Device **ppDevice) {
  HRESULT hr = shim_validate_create_args(DriverType, Software, SDKVersion,
                                         (void **)ppDevice);
  IDXGIFactory *factory = NULL;
  IDXGIAdapter *adapter = NULL;
  PFN_D3D10CoreCreateDevice coreCreate;

  if (FAILED(hr))
    return hr;

  coreCreate = shim_core_create_device();
  if (!coreCreate)
    return E_NOINTERFACE; /* DXMT's d3d10core.dll is not deployed */

  hr = shim_resolve_factory_adapter(pAdapter, &factory, &adapter);
  if (FAILED(hr))
    return hr;

  hr = coreCreate(factory, adapter, Flags, featureLevel, ppDevice);

  IDXGIAdapter_Release(adapter);
  IDXGIFactory_Release(factory);
  return hr;
}

/* Shared CreateDeviceAndSwapChain: create the device, then build the
 * swap chain through the same DXGI factory. */
static HRESULT shim_create_device_and_swapchain(
    IDXGIAdapter *pAdapter, D3D10_DRIVER_TYPE DriverType, HMODULE Software,
    UINT Flags, UINT SDKVersion, D3D_FEATURE_LEVEL featureLevel,
    DXGI_SWAP_CHAIN_DESC *pSwapChainDesc, IDXGISwapChain **ppSwapChain,
    ID3D10Device **ppDevice) {
  ID3D10Device *device = NULL;
  IDXGIDevice *dxgiDevice = NULL;
  IDXGIFactory *factory = NULL;
  IDXGIAdapter *adapter = NULL;
  PFN_D3D10CoreCreateDevice coreCreate;
  HRESULT hr;

  if (!pSwapChainDesc || !ppSwapChain)
    return E_INVALIDARG;
  *ppSwapChain = NULL;

  /* Keep our own factory/adapter pair alive across both calls so the
   * swap chain is created from the factory that owns the adapter. */
  hr = shim_validate_create_args(DriverType, Software, SDKVersion,
                                 (void **)ppDevice);
  if (FAILED(hr))
    return hr;
  coreCreate = shim_core_create_device();
  if (!coreCreate)
    return E_NOINTERFACE; /* DXMT's d3d10core.dll is not deployed */
  hr = shim_resolve_factory_adapter(pAdapter, &factory, &adapter);
  if (FAILED(hr))
    return hr;

  hr = coreCreate(factory, adapter, Flags, featureLevel, &device);
  if (FAILED(hr))
    goto out;

  /* DXMT's D3D10 device is a D3D11 device underneath, so IDXGIDevice is
   * available for CreateSwapChain. */
  hr = ID3D10Device_QueryInterface(device, &IID_IDXGIDevice,
                                   (void **)&dxgiDevice);
  if (FAILED(hr))
    goto out;

  hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)dxgiDevice,
                                    pSwapChainDesc, ppSwapChain);
  if (FAILED(hr))
    goto out;

  *ppDevice = device;
  device = NULL; /* ownership transferred */

out:
  if (dxgiDevice)
    IDXGIDevice_Release(dxgiDevice);
  if (device)
    ID3D10Device_Release(device);
  IDXGIAdapter_Release(adapter);
  IDXGIFactory_Release(factory);
  return hr;
}

/* Forward one function to d3dcompiler_43.dll (which Wine provides and
 * games commonly ship). Returns the callee's address or NULL. */
static FARPROC shim_d3dcompiler_proc(const char *name) {
  HMODULE mod = shim_d3dcompiler_module();
  if (!mod)
    return NULL;
  return GetProcAddress(mod, name);
}

#endif /* D3D10_SHIM_H */
