/* d3d10_1.dll -- Direct3D 10.1 -> DXMT shim.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Additional permission: Madeira Converter Exception, version 1
 * (testrepos/Madeira/LICENSE-EXCEPTION.md)
 *
 * D3D10CreateDevice1 maps the 10.1 feature level onto DXMT's
 * D3D10CoreCreateDevice and queries the resulting device for
 * ID3D10Device1 (which DXMT's D3D10 device implements). Every other
 * entry point is identical to d3d10.dll's, so it forwards there --
 * the same split Wine's own d3d10_1 uses.
 */

#include "d3d10_shim.h"

/* ------------------------------------------------------------------ */
/* Device creation                                                    */
/* ------------------------------------------------------------------ */

static D3D_FEATURE_LEVEL feature_level_1(D3D10_FEATURE_LEVEL1 level) {
  switch (level) {
  case D3D10_FEATURE_LEVEL_10_0:
    return D3D_FEATURE_LEVEL_10_0;
  case D3D10_FEATURE_LEVEL_10_1:
    return D3D_FEATURE_LEVEL_10_1;
  default:
    return (D3D_FEATURE_LEVEL)0;
  }
}

HRESULT WINAPI D3D10CreateDevice1(IDXGIAdapter *pAdapter,
                                  D3D10_DRIVER_TYPE DriverType, HMODULE Software,
                                  UINT Flags, D3D10_FEATURE_LEVEL1 HardwareLevel,
                                  UINT SDKVersion, ID3D10Device1 **ppDevice) {
  ID3D10Device *device = NULL;
  D3D_FEATURE_LEVEL level = feature_level_1(HardwareLevel);
  HRESULT hr;

  if (!ppDevice)
    return E_INVALIDARG;
  *ppDevice = NULL;
  if (level == (D3D_FEATURE_LEVEL)0)
    return E_INVALIDARG;

  hr = shim_create_d3d10_device(pAdapter, DriverType, Software, Flags,
                                SDKVersion, level, &device);
  if (FAILED(hr))
    return hr;

  hr = ID3D10Device_QueryInterface(device, &IID_ID3D10Device1,
                                   (void **)ppDevice);
  ID3D10Device_Release(device);
  return hr;
}

HRESULT WINAPI D3D10CreateDeviceAndSwapChain1(
    IDXGIAdapter *pAdapter, D3D10_DRIVER_TYPE DriverType, HMODULE Software,
    UINT Flags, D3D10_FEATURE_LEVEL1 HardwareLevel, UINT SDKVersion,
    DXGI_SWAP_CHAIN_DESC *pSwapChainDesc, IDXGISwapChain **ppSwapChain,
    ID3D10Device1 **ppDevice) {
  ID3D10Device *device = NULL;
  D3D_FEATURE_LEVEL level = feature_level_1(HardwareLevel);
  HRESULT hr;

  if (!ppDevice)
    return E_INVALIDARG;
  *ppDevice = NULL;
  if (level == (D3D_FEATURE_LEVEL)0)
    return E_INVALIDARG;

  hr = shim_create_device_and_swapchain(pAdapter, DriverType, Software, Flags,
                                        SDKVersion, level, pSwapChainDesc,
                                        ppSwapChain, &device);
  if (FAILED(hr))
    return hr;

  hr = ID3D10Device_QueryInterface(device, &IID_ID3D10Device1,
                                   (void **)ppDevice);
  ID3D10Device_Release(device);
  if (FAILED(hr) && ppSwapChain && *ppSwapChain) {
    IDXGISwapChain_Release(*ppSwapChain);
    *ppSwapChain = NULL;
  }
  return hr;
}

/* ------------------------------------------------------------------ */
/* Forward everything else to d3d10.dll (same split as Wine).          */
/* ------------------------------------------------------------------ */

static HMODULE forward_module(void) {
  static HMODULE mod = NULL;
  if (!mod)
    mod = LoadLibraryA("d3d10.dll");
  return mod;
}

/* Failure value per return type: error codes for HRESULT-likes, NULL/FALSE
 * for pointers and BOOL. */
#define SHIM_FAIL(ret)                                                         \
  _Generic((ret)0, HRESULT: (ret)E_NOINTERFACE, DWORD: (ret)E_NOINTERFACE,      \
                      BOOL: (ret)FALSE, default: (ret)0)

#define FORWARD0(ret, name)                                                    \
  ret WINAPI name(void) {                                                      \
    static ret(WINAPI * p)(void) = NULL;                                       \
    if (!p) {                                                                  \
      HMODULE m = forward_module();                                            \
      if (!m)                                                                  \
        return SHIM_FAIL(ret);                                             \
      p = (ret(WINAPI *)(void))GetProcAddress(m, #name);                       \
      if (!p)                                                                  \
        return SHIM_FAIL(ret);                                             \
    }                                                                          \
    return p();                                                                \
  }

/* Typed forwards: one macro per arity keeps the signatures exact. */
#define FORWARD1(ret, name, t1)                                                \
  ret WINAPI name(t1 a1) {                                                     \
    static ret(WINAPI * p)(t1) = NULL;                                         \
    if (!p) {                                                                  \
      HMODULE m = forward_module();                                            \
      if (!m)                                                                  \
        return SHIM_FAIL(ret);                                             \
      p = (ret(WINAPI *)(t1))GetProcAddress(m, #name);                          \
      if (!p)                                                                  \
        return SHIM_FAIL(ret);                                             \
    }                                                                          \
    return p(a1);                                                              \
  }

#define FORWARD2(ret, name, t1, t2)                                            \
  ret WINAPI name(t1 a1, t2 a2) {                                              \
    static ret(WINAPI * p)(t1, t2) = NULL;                                      \
    if (!p) {                                                                  \
      HMODULE m = forward_module();                                            \
      if (!m)                                                                  \
        return SHIM_FAIL(ret);                                             \
      p = (ret(WINAPI *)(t1, t2))GetProcAddress(m, #name);                      \
      if (!p)                                                                  \
        return SHIM_FAIL(ret);                                             \
    }                                                                          \
    return p(a1, a2);                                                          \
  }

#define FORWARD3(ret, name, t1, t2, t3)                                        \
  ret WINAPI name(t1 a1, t2 a2, t3 a3) {                                       \
    static ret(WINAPI * p)(t1, t2, t3) = NULL;                                  \
    if (!p) {                                                                  \
      HMODULE m = forward_module();                                            \
      if (!m)                                                                  \
        return SHIM_FAIL(ret);                                             \
      p = (ret(WINAPI *)(t1, t2, t3))GetProcAddress(m, #name);                  \
      if (!p)                                                                  \
        return SHIM_FAIL(ret);                                             \
    }                                                                          \
    return p(a1, a2, a3);                                                      \
  }

#define FORWARD4(ret, name, t1, t2, t3, t4)                                    \
  ret WINAPI name(t1 a1, t2 a2, t3 a3, t4 a4) {                                \
    static ret(WINAPI * p)(t1, t2, t3, t4) = NULL;                              \
    if (!p) {                                                                  \
      HMODULE m = forward_module();                                            \
      if (!m)                                                                  \
        return SHIM_FAIL(ret);                                             \
      p = (ret(WINAPI *)(t1, t2, t3, t4))GetProcAddress(m, #name);               \
      if (!p)                                                                  \
        return SHIM_FAIL(ret);                                             \
    }                                                                          \
    return p(a1, a2, a3, a4);                                                  \
  }

#define FORWARD5(ret, name, t1, t2, t3, t4, t5)                                \
  ret WINAPI name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5) {                         \
    static ret(WINAPI * p)(t1, t2, t3, t4, t5) = NULL;                          \
    if (!p) {                                                                  \
      HMODULE m = forward_module();                                            \
      if (!m)                                                                  \
        return SHIM_FAIL(ret);                                             \
      p = (ret(WINAPI *)(t1, t2, t3, t4, t5))GetProcAddress(m, #name);           \
      if (!p)                                                                  \
        return SHIM_FAIL(ret);                                             \
    }                                                                          \
    return p(a1, a2, a3, a4, a5);                                              \
  }

#define FORWARD6(ret, name, t1, t2, t3, t4, t5, t6)                            \
  ret WINAPI name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6) {                  \
    static ret(WINAPI * p)(t1, t2, t3, t4, t5, t6) = NULL;                      \
    if (!p) {                                                                  \
      HMODULE m = forward_module();                                            \
      if (!m)                                                                  \
        return SHIM_FAIL(ret);                                             \
      p = (ret(WINAPI *)(t1, t2, t3, t4, t5, t6))GetProcAddress(m, #name);       \
      if (!p)                                                                  \
        return SHIM_FAIL(ret);                                             \
    }                                                                          \
    return p(a1, a2, a3, a4, a5, a6);                                          \
  }

#define FORWARD7(ret, name, t1, t2, t3, t4, t5, t6, t7)                        \
  ret WINAPI name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7) {          \
    static ret(WINAPI * p)(t1, t2, t3, t4, t5, t6, t7) = NULL;                   \
    if (!p) {                                                                  \
      HMODULE m = forward_module();                                            \
      if (!m)                                                                  \
        return SHIM_FAIL(ret);                                             \
      p = (ret(WINAPI *)(t1, t2, t3, t4, t5, t6, t7))GetProcAddress(m, #name);   \
      if (!p)                                                                  \
        return SHIM_FAIL(ret);                                             \
    }                                                                          \
    return p(a1, a2, a3, a4, a5, a6, a7);                                      \
  }

#define FORWARD8(ret, name, t1, t2, t3, t4, t5, t6, t7, t8)                    \
  ret WINAPI name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8) {  \
    static ret(WINAPI * p)(t1, t2, t3, t4, t5, t6, t7, t8) = NULL;               \
    if (!p) {                                                                  \
      HMODULE m = forward_module();                                            \
      if (!m)                                                                  \
        return SHIM_FAIL(ret);                                             \
      p = (ret(WINAPI *)(t1, t2, t3, t4, t5, t6, t7, t8))GetProcAddress(m,       \
                                                                        #name); \
      if (!p)                                                                  \
        return SHIM_FAIL(ret);                                             \
    }                                                                          \
    return p(a1, a2, a3, a4, a5, a6, a7, a8);                                  \
  }

#define FORWARD9(ret, name, t1, t2, t3, t4, t5, t6, t7, t8, t9)                    \
  ret WINAPI name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8,       \
                  t9 a9) {                                                    \
    static ret(WINAPI * p)(t1, t2, t3, t4, t5, t6, t7, t8, t9) = NULL;          \
    if (!p) {                                                                  \
      HMODULE m = forward_module();                                            \
      if (!m)                                                                  \
        return SHIM_FAIL(ret);                                             \
      p = (ret(WINAPI *)(t1, t2, t3, t4, t5, t6, t7, t8, t9))GetProcAddress(    \
          m, #name);                                                           \
      if (!p)                                                                  \
        return SHIM_FAIL(ret);                                             \
    }                                                                          \
    return p(a1, a2, a3, a4, a5, a6, a7, a8, a9);                              \
  }

#define FORWARD10(ret, name, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10)           \
  ret WINAPI name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8,       \
                  t9 a9, t10 a10) {                                            \
    static ret(WINAPI * p)(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10) = NULL;     \
    if (!p) {                                                                  \
      HMODULE m = forward_module();                                            \
      if (!m)                                                                  \
        return SHIM_FAIL(ret);                                             \
      p = (ret(WINAPI *)(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10))              \
          GetProcAddress(m, #name);                                            \
      if (!p)                                                                  \
        return SHIM_FAIL(ret);                                             \
    }                                                                          \
    return p(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);                         \
  }

FORWARD7(HRESULT, D3D10CompileEffectFromMemory, void *, SIZE_T, UINT,
         ID3D10Include *, ID3D10EffectPool *, ID3D10Blob **, ID3D10Blob **)
FORWARD10(HRESULT, D3D10CompileShader, LPCSTR, SIZE_T, LPCSTR,
          const D3D10_SHADER_MACRO *, LPD3D10INCLUDE, LPCSTR, LPCSTR, UINT,
          ID3D10Blob **, ID3D10Blob **)
FORWARD2(HRESULT, D3D10CreateBlob, SIZE_T, LPD3D10BLOB *)
FORWARD6(HRESULT, D3D10CreateEffectFromMemory, void *, SIZE_T, UINT,
         ID3D10Device *, ID3D10EffectPool *, ID3D10Effect **)
FORWARD5(HRESULT, D3D10CreateEffectPoolFromMemory, void *, SIZE_T, UINT,
         ID3D10Device *, ID3D10EffectPool **)
FORWARD3(HRESULT, D3D10CreateStateBlock, ID3D10Device *,
         D3D10_STATE_BLOCK_MASK *, ID3D10StateBlock **)
FORWARD3(HRESULT, D3D10DisassembleEffect, ID3D10Effect *, BOOL, ID3D10Blob **)
FORWARD5(HRESULT, D3D10DisassembleShader, const void *, SIZE_T, BOOL, LPCSTR,
         ID3D10Blob **)
FORWARD1(LPCSTR, D3D10GetGeometryShaderProfile, ID3D10Device *)
FORWARD3(HRESULT, D3D10GetInputAndOutputSignatureBlob, const void *, SIZE_T,
         ID3D10Blob **)
FORWARD3(HRESULT, D3D10GetInputSignatureBlob, const void *, SIZE_T,
         ID3D10Blob **)
FORWARD3(HRESULT, D3D10GetOutputSignatureBlob, const void *, SIZE_T,
         ID3D10Blob **)
FORWARD1(LPCSTR, D3D10GetPixelShaderProfile, ID3D10Device *)
FORWARD3(HRESULT, D3D10GetShaderDebugInfo, const void *, SIZE_T, ID3D10Blob **)
FORWARD1(LPCSTR, D3D10GetVertexShaderProfile, ID3D10Device *)
FORWARD7(HRESULT, D3D10PreprocessShader, LPCSTR, SIZE_T, LPCSTR,
         const D3D10_SHADER_MACRO *, LPD3D10INCLUDE, ID3D10Blob **,
         ID3D10Blob **)
FORWARD3(HRESULT, D3D10ReflectShader, const void *, SIZE_T,
         ID3D10ShaderReflection **)
FORWARD0(HRESULT, D3D10RegisterLayers)
FORWARD0(DWORD, D3D10GetVersion)
FORWARD3(HRESULT, D3D10StateBlockMaskDifference, D3D10_STATE_BLOCK_MASK *,
         D3D10_STATE_BLOCK_MASK *, D3D10_STATE_BLOCK_MASK *)
FORWARD1(HRESULT, D3D10StateBlockMaskDisableAll, D3D10_STATE_BLOCK_MASK *)
FORWARD4(HRESULT, D3D10StateBlockMaskDisableCapture, D3D10_STATE_BLOCK_MASK *,
         D3D10_DEVICE_STATE_TYPES, UINT, UINT)
FORWARD1(HRESULT, D3D10StateBlockMaskEnableAll, D3D10_STATE_BLOCK_MASK *)
FORWARD4(HRESULT, D3D10StateBlockMaskEnableCapture, D3D10_STATE_BLOCK_MASK *,
         D3D10_DEVICE_STATE_TYPES, UINT, UINT)
FORWARD3(BOOL, D3D10StateBlockMaskGetSetting, D3D10_STATE_BLOCK_MASK *,
         D3D10_DEVICE_STATE_TYPES, UINT)
FORWARD3(HRESULT, D3D10StateBlockMaskIntersect, D3D10_STATE_BLOCK_MASK *,
         D3D10_STATE_BLOCK_MASK *, D3D10_STATE_BLOCK_MASK *)
FORWARD3(HRESULT, D3D10StateBlockMaskUnion, D3D10_STATE_BLOCK_MASK *,
         D3D10_STATE_BLOCK_MASK *, D3D10_STATE_BLOCK_MASK *)
FORWARD0(HRESULT, RevertToOldImplementation)

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  (void)hinstDLL;
  (void)fdwReason;
  (void)lpvReserved;
  return TRUE;
}
