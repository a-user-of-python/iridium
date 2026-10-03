/* Stub d3d10.h/dxgi.h/d3d10_1.h for host unit tests. Only the pieces
 * the shim sources reference. Struct layouts match the real headers for
 * the types under test. */
#ifndef TEST_STUB_D3D10_H
#define TEST_STUB_D3D10_H

#include "windows.h"

/* --- enums --- */
typedef enum D3D10_DRIVER_TYPE {
  D3D10_DRIVER_TYPE_HARDWARE = 0,
  D3D10_DRIVER_TYPE_REFERENCE = 1,
  D3D10_DRIVER_TYPE_NULL = 2,
  D3D10_DRIVER_TYPE_SOFTWARE = 3,
  /* Wine's include/d3d10misc.h:33 -- was incorrectly 4 in an earlier stub. */
  D3D10_DRIVER_TYPE_WARP = 5
} D3D10_DRIVER_TYPE;

typedef enum D3D10_FEATURE_LEVEL1 {
  D3D10_FEATURE_LEVEL_10_0 = 0xa000,
  D3D10_FEATURE_LEVEL_10_1 = 0xa100
} D3D10_FEATURE_LEVEL1;

typedef enum D3D_FEATURE_LEVEL {
  D3D_FEATURE_LEVEL_9_1 = 0x9100,
  D3D_FEATURE_LEVEL_10_0 = 0xa000,
  D3D_FEATURE_LEVEL_10_1 = 0xa100,
  D3D_FEATURE_LEVEL_11_0 = 0xb000
} D3D_FEATURE_LEVEL;

typedef enum D3D10_DEVICE_STATE_TYPES {
  D3D10_DST_SO_BUFFERS = 1,
  D3D10_DST_OM_RENDER_TARGETS = 2,
  D3D10_DST_OM_DEPTH_STENCIL_STATE = 3,
  D3D10_DST_OM_BLEND_STATE = 4,
  D3D10_DST_VS = 5,
  D3D10_DST_VS_SAMPLERS = 6,
  D3D10_DST_VS_SHADER_RESOURCES = 7,
  D3D10_DST_VS_CONSTANT_BUFFERS = 8,
  D3D10_DST_GS = 9,
  D3D10_DST_GS_SAMPLERS = 10,
  D3D10_DST_GS_SHADER_RESOURCES = 11,
  D3D10_DST_GS_CONSTANT_BUFFERS = 12,
  D3D10_DST_PS = 13,
  D3D10_DST_PS_SAMPLERS = 14,
  D3D10_DST_PS_SHADER_RESOURCES = 15,
  D3D10_DST_PS_CONSTANT_BUFFERS = 16,
  D3D10_DST_IA_VERTEX_BUFFERS = 17,
  D3D10_DST_IA_INDEX_BUFFER = 18,
  D3D10_DST_IA_INPUT_LAYOUT = 19,
  D3D10_DST_IA_PRIMITIVE_TOPOLOGY = 20,
  D3D10_DST_RS_VIEWPORTS = 21,
  D3D10_DST_RS_SCISSOR_RECTS = 22,
  D3D10_DST_RS_RASTERIZER_STATE = 23,
  D3D10_DST_PREDICATION = 24
} D3D10_DEVICE_STATE_TYPES;

#define D3D10_SDK_VERSION 29
#define DXGI_ERROR_SDK_COMPONENT_MISSING ((HRESULT)(int32_t)0x887A002D)
#define DXGI_ERROR_UNSUPPORTED ((HRESULT)(int32_t)0x887A002B)
#define D3D_DISASM_ENABLE_COLOR_CODE 0x2

typedef struct _D3D10_STATE_BLOCK_MASK {
  /* Bit-packed, exactly like the Windows SDK and Wine's
   * include/d3d10effect.idl:107-133. Array fields hold one bit per slot
   * (D3D10_BYTES_FROM_BITS(n) == ((n) + 7) >> 3); scalar fields use bit 0.
   * Total: 76 bytes. */
  uint8_t VS;
  uint8_t VSSamplers[2];         /* 16 sampler slots */
  uint8_t VSShaderResources[16]; /* 128 resource slots */
  uint8_t VSConstantBuffers[2];  /* 14 constant-buffer slots */
  uint8_t GS;
  uint8_t GSSamplers[2];
  uint8_t GSShaderResources[16];
  uint8_t GSConstantBuffers[2];
  uint8_t PS;
  uint8_t PSSamplers[2];
  uint8_t PSShaderResources[16];
  uint8_t PSConstantBuffers[2];
  uint8_t IAVertexBuffers[2]; /* 16 vertex-buffer slots */
  uint8_t IAIndexBuffer;
  uint8_t IAInputLayout;
  uint8_t IAPrimitiveTopology;
  uint8_t OMRenderTargets;
  uint8_t OMDepthStencilState;
  uint8_t OMBlendState;
  uint8_t RSViewports;
  uint8_t RSScissorRects;
  uint8_t RSRasterizerState;
  uint8_t SOBuffers;
  uint8_t Predication;
} D3D10_STATE_BLOCK_MASK;

/* Slot counts from Wine's include/d3d10.idl (same as the Windows SDK). */
#define D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT 16
#define D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT 128
#define D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT 14
#define D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT 16

/* --- COM interface stubs (vtbl layout: only used slots) --- */
typedef struct IUnknown IUnknown;
typedef struct IUnknown_vtbl {
  HRESULT(WINAPI *QueryInterface)(void *, REFIID, void **);
  UINT(WINAPI *AddRef)(void *);
  UINT(WINAPI *Release)(void *);
} IUnknown_vtbl;

typedef struct IDXGIObject {
  struct IDXGIObject_vtbl *lpVtbl;
} IDXGIObject;
typedef struct IDXGIObject_vtbl {
  IUnknown_vtbl base;
  HRESULT(WINAPI *GetParent)(IDXGIObject *, REFIID, void **);
} IDXGIObject_vtbl;
typedef struct IDXGIAdapter {
  struct IDXGIAdapter_vtbl *lpVtbl;
} IDXGIAdapter;
typedef struct IDXGIAdapter_vtbl {
  IDXGIObject_vtbl base;
} IDXGIAdapter_vtbl;

typedef struct IDXGIFactory {
  struct IDXGIFactory_vtbl *lpVtbl;
} IDXGIFactory;
typedef struct DXGI_SWAP_CHAIN_DESC {
  uint8_t bytes[64];
} DXGI_SWAP_CHAIN_DESC;
typedef struct IDXGISwapChain {
  struct IDXGISwapChain_vtbl *lpVtbl;
} IDXGISwapChain;
typedef struct IDXGISwapChain_vtbl {
  IUnknown_vtbl base;
} IDXGISwapChain_vtbl;
typedef struct IDXGIFactory_vtbl {
  IUnknown_vtbl base;
  HRESULT(WINAPI *EnumAdapters)(IDXGIFactory *, UINT, IDXGIAdapter **);
  HRESULT(WINAPI *CreateSwapChain)(IDXGIFactory *, void *, DXGI_SWAP_CHAIN_DESC *,
                                   IDXGISwapChain **);
} IDXGIFactory_vtbl;

typedef struct ID3D10Device {
  struct ID3D10Device_vtbl *lpVtbl;
} ID3D10Device;
typedef struct ID3D10StateBlock {
  int dummy;
} ID3D10StateBlock;
typedef struct ID3D10Device_vtbl {
  IUnknown_vtbl base;
  HRESULT(WINAPI *CreateStateBlock)(ID3D10Device *, D3D10_STATE_BLOCK_MASK *,
                                    ID3D10StateBlock **);
} ID3D10Device_vtbl;

typedef struct IDXGIDevice {
  struct IDXGIDevice_vtbl *lpVtbl;
} IDXGIDevice;
typedef struct IDXGIDevice_vtbl {
  IUnknown_vtbl base;
} IDXGIDevice_vtbl;

typedef struct ID3D10Device1 {
  int dummy;
} ID3D10Device1;
typedef struct ID3D10Blob {
  int dummy;
} ID3D10Blob;
typedef ID3D10Blob ID3DBlob;
typedef ID3D10Blob *LPD3D10BLOB;
typedef struct ID3D10ShaderReflection {
  int dummy;
} ID3D10ShaderReflection;
typedef struct ID3D10Effect {
  int dummy;
} ID3D10Effect;
typedef struct ID3D10EffectPool {
  int dummy;
} ID3D10EffectPool;
typedef struct D3D10_SHADER_MACRO {
  LPCSTR Name;
  LPCSTR Definition;
} D3D10_SHADER_MACRO;
typedef D3D10_SHADER_MACRO D3D_SHADER_MACRO;
typedef struct ID3D10Include {
  int dummy;
} ID3D10Include;
typedef ID3D10Include *LPD3D10INCLUDE;
typedef ID3D10Include ID3DInclude;

/* COBJMACROS the shim uses */
#define IDXGIAdapter_AddRef(p) (p)->lpVtbl->base.base.AddRef((void *)(p))
#define IDXGIAdapter_Release(p) (p)->lpVtbl->base.base.Release((void *)(p))
#define IDXGIObject_GetParent(p, a, b) (p)->lpVtbl->GetParent((p), (a), (b))
#define IDXGIFactory_Release(p) (p)->lpVtbl->base.Release((void *)(p))
#define IDXGIFactory_EnumAdapters(p, a, b) (p)->lpVtbl->EnumAdapters((p), (a), (b))
#define IDXGIFactory_CreateSwapChain(p, a, b, c)                               \
  (p)->lpVtbl->CreateSwapChain((p), (a), (b), (c))
#define ID3D10Device_QueryInterface(p, a, b)                                    \
  (p)->lpVtbl->base.QueryInterface((void *)(p), (a), (b))
#define ID3D10Device_Release(p) (p)->lpVtbl->base.Release((void *)(p))
#define ID3D10Device_CreateStateBlock(p, a, b)                                  \
  (p)->lpVtbl->CreateStateBlock((p), (a), (b))
#define IDXGIDevice_Release(p) (p)->lpVtbl->base.Release((void *)(p))
#define IDXGISwapChain_Release(p) (p)->lpVtbl->base.Release((void *)(p))

#endif
