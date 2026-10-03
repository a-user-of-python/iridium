/* d3d10.dll -- Direct3D 10 -> DXMT shim.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Additional permission: Madeira Converter Exception, version 1
 * (testrepos/Madeira/LICENSE-EXCEPTION.md)
 *
 * Device creation forwards to DXMT's d3d10core.dll (D3D10CoreCreateDevice);
 * shader-introspection helpers forward to d3dcompiler_43.dll (Wine ships
 * it, and games commonly bundle it). Effect-framework entry points are
 * honest stubs: the Effects10 framework is outside DXMT's scope.
 */

#include "d3d10_shim.h"

/* ------------------------------------------------------------------ */
/* d3dcompiler_43.dll forwards                                        */
/* ------------------------------------------------------------------ */

typedef HRESULT(WINAPI *PFN_D3DCreateBlob)(SIZE_T Size, ID3DBlob **ppBlob);
typedef HRESULT(WINAPI *PFN_D3DCompile)(LPCVOID pSrcData, SIZE_T SrcDataSize,
                                       LPCSTR pSourceName,
                                       const D3D_SHADER_MACRO *pDefines,
                                       ID3DInclude *pInclude, LPCSTR pEntrypoint,
                                       LPCSTR pTarget, UINT Flags1, UINT Flags2,
                                       ID3DBlob **ppCode, ID3DBlob **ppErrorMsgs);
typedef HRESULT(WINAPI *PFN_D3DDisassemble)(LPCVOID pSrcData, SIZE_T SrcDataSize,
                                            UINT Flags, LPCSTR szComments,
                                            ID3DBlob **ppDisassembly);
typedef HRESULT(WINAPI *PFN_D3DGetSignatureBlob)(LPCVOID pSrcData,
                                                 SIZE_T SrcDataSize,
                                                 ID3DBlob **ppSignatureBlob);
typedef HRESULT(WINAPI *PFN_D3DReflect)(LPCVOID pSrcData, SIZE_T SrcDataSize,
                                       REFIID pInterface, void **ppReflector);
typedef HRESULT(WINAPI *PFN_D3DPreprocess)(LPCVOID pSrcData, SIZE_T SrcDataSize,
                                          LPCSTR pSourceName,
                                          const D3D_SHADER_MACRO *pDefines,
                                          ID3DInclude *pInclude,
                                          ID3DBlob **ppCodeText,
                                          ID3DBlob **ppErrorMsgs);
typedef HRESULT(WINAPI *PFN_D3DGetDebugInfo)(LPCVOID pSrcData, SIZE_T SrcDataSize,
                                            ID3DBlob **ppDebugInfo);

HRESULT WINAPI D3D10CreateBlob(SIZE_T NumBytes, LPD3D10BLOB *ppBuffer) {
  PFN_D3DCreateBlob p = (PFN_D3DCreateBlob)shim_d3dcompiler_proc("D3DCreateBlob");
  if (!p || !ppBuffer)
    return E_NOINTERFACE;
  return p(NumBytes, (ID3DBlob **)ppBuffer);
}

HRESULT WINAPI D3D10CompileShader(LPCSTR pSrcData, SIZE_T SrcDataLen,
                                  LPCSTR pFileName,
                                  const D3D10_SHADER_MACRO *pDefines,
                                  LPD3D10INCLUDE pInclude, LPCSTR pFunctionName,
                                  LPCSTR pProfile, UINT Flags,
                                  ID3D10Blob **ppShader,
                                  ID3D10Blob **ppErrorMsgs) {
  PFN_D3DCompile p = (PFN_D3DCompile)shim_d3dcompiler_proc("D3DCompile");
  if (!p)
    return E_NOINTERFACE;
  /* D3D10_SHADER_MACRO and D3D_SHADER_MACRO are layout-identical. */
  return p(pSrcData, SrcDataLen, pFileName, (const D3D_SHADER_MACRO *)pDefines,
           (ID3DInclude *)pInclude, pFunctionName, pProfile, Flags, 0,
           (ID3DBlob **)ppShader, (ID3DBlob **)ppErrorMsgs);
}

HRESULT WINAPI D3D10DisassembleShader(const void *pShader, SIZE_T BytecodeLength,
                                      BOOL EnableColorCode, LPCSTR pComments,
                                      ID3D10Blob **ppDisassembly) {
  PFN_D3DDisassemble p =
      (PFN_D3DDisassemble)shim_d3dcompiler_proc("D3DDisassemble");
  if (!p)
    return E_NOINTERFACE;
  return p(pShader, BytecodeLength,
           EnableColorCode ? D3D_DISASM_ENABLE_COLOR_CODE : 0, pComments,
           (ID3DBlob **)ppDisassembly);
}

HRESULT WINAPI D3D10GetInputSignatureBlob(const void *pShaderBytecode,
                                          SIZE_T BytecodeLength,
                                          ID3D10Blob **ppSignatureBlob) {
  PFN_D3DGetSignatureBlob p =
      (PFN_D3DGetSignatureBlob)shim_d3dcompiler_proc("D3DGetInputSignatureBlob");
  if (!p)
    return E_NOINTERFACE;
  return p(pShaderBytecode, BytecodeLength, (ID3DBlob **)ppSignatureBlob);
}

HRESULT WINAPI D3D10GetOutputSignatureBlob(const void *pShaderBytecode,
                                           SIZE_T BytecodeLength,
                                           ID3D10Blob **ppSignatureBlob) {
  PFN_D3DGetSignatureBlob p =
      (PFN_D3DGetSignatureBlob)shim_d3dcompiler_proc("D3DGetOutputSignatureBlob");
  if (!p)
    return E_NOINTERFACE;
  return p(pShaderBytecode, BytecodeLength, (ID3DBlob **)ppSignatureBlob);
}

HRESULT WINAPI D3D10GetInputAndOutputSignatureBlob(const void *pShaderBytecode,
                                                   SIZE_T BytecodeLength,
                                                   ID3D10Blob **ppSignatureBlob) {
  PFN_D3DGetSignatureBlob p =
      (PFN_D3DGetSignatureBlob)shim_d3dcompiler_proc(
          "D3DGetInputAndOutputSignatureBlob");
  if (!p)
    return E_NOINTERFACE;
  return p(pShaderBytecode, BytecodeLength, (ID3DBlob **)ppSignatureBlob);
}

HRESULT WINAPI D3D10ReflectShader(const void *pShaderBytecode,
                                  SIZE_T BytecodeLength,
                                  ID3D10ShaderReflection **ppReflector) {
  PFN_D3DReflect p = (PFN_D3DReflect)shim_d3dcompiler_proc("D3DReflect");
  if (!p)
    return E_NOINTERFACE;
  return p(pShaderBytecode, BytecodeLength, &IID_ID3D10ShaderReflection,
           (void **)ppReflector);
}

HRESULT WINAPI D3D10PreprocessShader(LPCSTR pSrcData, SIZE_T SrcDataSize,
                                     LPCSTR pFileName,
                                     const D3D10_SHADER_MACRO *pDefines,
                                     LPD3D10INCLUDE pInclude,
                                     ID3D10Blob **ppShaderText,
                                     ID3D10Blob **ppErrorMsgs) {
  PFN_D3DPreprocess p =
      (PFN_D3DPreprocess)shim_d3dcompiler_proc("D3DPreprocess");
  if (!p)
    return E_NOINTERFACE;
  return p(pSrcData, SrcDataSize, pFileName,
           (const D3D_SHADER_MACRO *)pDefines, (ID3DInclude *)pInclude,
           (ID3DBlob **)ppShaderText, (ID3DBlob **)ppErrorMsgs);
}

HRESULT WINAPI D3D10GetShaderDebugInfo(const void *pShaderBytecode,
                                       SIZE_T BytecodeLength,
                                       ID3D10Blob **ppDebugInfo) {
  PFN_D3DGetDebugInfo p =
      (PFN_D3DGetDebugInfo)shim_d3dcompiler_proc("D3DGetDebugInfo");
  if (!p)
    return E_NOINTERFACE;
  return p(pShaderBytecode, BytecodeLength, (ID3DBlob **)ppDebugInfo);
}

/* ------------------------------------------------------------------ */
/* Shader profile helpers: DXMT accepts shader model 4.x bytecode.    */
/* ------------------------------------------------------------------ */

LPCSTR WINAPI D3D10GetVertexShaderProfile(ID3D10Device *pDevice) {
  (void)pDevice;
  return "vs_4_0";
}

LPCSTR WINAPI D3D10GetGeometryShaderProfile(ID3D10Device *pDevice) {
  (void)pDevice;
  return "gs_4_0";
}

LPCSTR WINAPI D3D10GetPixelShaderProfile(ID3D10Device *pDevice) {
  (void)pDevice;
  return "ps_4_0";
}

/* ------------------------------------------------------------------ */
/* Device creation -- the reason this DLL exists.                     */
/* ------------------------------------------------------------------ */

HRESULT WINAPI D3D10CreateDevice(IDXGIAdapter *pAdapter,
                                 D3D10_DRIVER_TYPE DriverType, HMODULE Software,
                                 UINT Flags, UINT SDKVersion,
                                 ID3D10Device **ppDevice) {
  /* A NULL feature-level list means "the best you have"; DXMT maps that
   * to 10_1 on capable Metal hardware. */
  return shim_create_d3d10_device(pAdapter, DriverType, Software, Flags,
                                  SDKVersion, D3D_FEATURE_LEVEL_10_1, ppDevice);
}

HRESULT WINAPI D3D10CreateDeviceAndSwapChain(
    IDXGIAdapter *pAdapter, D3D10_DRIVER_TYPE DriverType, HMODULE Software,
    UINT Flags, UINT SDKVersion, DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
    IDXGISwapChain **ppSwapChain, ID3D10Device **ppDevice) {
  return shim_create_device_and_swapchain(
      pAdapter, DriverType, Software, Flags, SDKVersion,
      D3D_FEATURE_LEVEL_10_1, pSwapChainDesc, ppSwapChain, ppDevice);
}

HRESULT WINAPI D3D10CreateStateBlock(ID3D10Device *pDevice,
                                     D3D10_STATE_BLOCK_MASK *pStateBlockMask,
                                     ID3D10StateBlock **ppStateBlock) {
  if (!pDevice || !ppStateBlock)
    return E_INVALIDARG;
  return ID3D10Device_CreateStateBlock(pDevice, pStateBlockMask, ppStateBlock);
}

/* ------------------------------------------------------------------ */
/* State-block mask helpers. The D3D10_STATE_BLOCK_MASK is a 76-byte    */
/* bit-packed struct (Windows SDK / Wine include/d3d10effect.idl):     */
/* array fields hold one bit per slot, scalar fields use bit 0. Bit    */
/* `i` of a field lives in byte (i >> 3) at bit (i & 7), the same      */
/* packing Wine's dlls/d3d10/stateblock.c uses.                        */
/* ------------------------------------------------------------------ */

struct mask_field {
  D3D10_DEVICE_STATE_TYPES type;
  size_t offset; /* byte offset of the field within the mask */
  UINT bits;     /* number of meaningful bits in the field */
};

#ifndef D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT
#define D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT 16
#endif
#ifndef D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT
#define D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT 128
#endif
#ifndef D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT
#define D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT 14
#endif
#ifndef D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT
#define D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT 16
#endif

#define MASK_FIELD(t, f, b)                                                    \
  { t, offsetof(D3D10_STATE_BLOCK_MASK, f), b }

static const struct mask_field mask_fields[] = {
    MASK_FIELD(D3D10_DST_SO_BUFFERS, SOBuffers, 1),
    MASK_FIELD(D3D10_DST_OM_RENDER_TARGETS, OMRenderTargets, 1),
    MASK_FIELD(D3D10_DST_OM_DEPTH_STENCIL_STATE, OMDepthStencilState, 1),
    MASK_FIELD(D3D10_DST_OM_BLEND_STATE, OMBlendState, 1),
    MASK_FIELD(D3D10_DST_VS, VS, 1),
    MASK_FIELD(D3D10_DST_VS_SAMPLERS, VSSamplers,
               D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT),
    MASK_FIELD(D3D10_DST_VS_SHADER_RESOURCES, VSShaderResources,
               D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT),
    MASK_FIELD(D3D10_DST_VS_CONSTANT_BUFFERS, VSConstantBuffers,
               D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT),
    MASK_FIELD(D3D10_DST_GS, GS, 1),
    MASK_FIELD(D3D10_DST_GS_SAMPLERS, GSSamplers,
               D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT),
    MASK_FIELD(D3D10_DST_GS_SHADER_RESOURCES, GSShaderResources,
               D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT),
    MASK_FIELD(D3D10_DST_GS_CONSTANT_BUFFERS, GSConstantBuffers,
               D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT),
    MASK_FIELD(D3D10_DST_PS, PS, 1),
    MASK_FIELD(D3D10_DST_PS_SAMPLERS, PSSamplers,
               D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT),
    MASK_FIELD(D3D10_DST_PS_SHADER_RESOURCES, PSShaderResources,
               D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT),
    MASK_FIELD(D3D10_DST_PS_CONSTANT_BUFFERS, PSConstantBuffers,
               D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT),
    MASK_FIELD(D3D10_DST_IA_VERTEX_BUFFERS, IAVertexBuffers,
               D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT),
    MASK_FIELD(D3D10_DST_IA_INDEX_BUFFER, IAIndexBuffer, 1),
    MASK_FIELD(D3D10_DST_IA_INPUT_LAYOUT, IAInputLayout, 1),
    MASK_FIELD(D3D10_DST_IA_PRIMITIVE_TOPOLOGY, IAPrimitiveTopology, 1),
    MASK_FIELD(D3D10_DST_RS_VIEWPORTS, RSViewports, 1),
    MASK_FIELD(D3D10_DST_RS_SCISSOR_RECTS, RSScissorRects, 1),
    MASK_FIELD(D3D10_DST_RS_RASTERIZER_STATE, RSRasterizerState, 1),
    MASK_FIELD(D3D10_DST_PREDICATION, Predication, 1),
};

static const struct mask_field *mask_lookup(D3D10_DEVICE_STATE_TYPES type) {
  size_t i;
  for (i = 0; i < sizeof(mask_fields) / sizeof(mask_fields[0]); i++) {
    if (mask_fields[i].type == type)
      return &mask_fields[i];
  }
  return NULL;
}

static BOOL mask_get_bit(const BYTE *field, UINT bits, UINT idx) {
  if (idx >= bits)
    return FALSE;
  return (field[idx >> 3] & (BYTE)(1u << (idx & 7))) != 0;
}

static void mask_set_bit_range(BYTE *field, UINT start, UINT count,
                               BOOL value) {
  /* Caller guarantees start + count <= field bit count (mask_capture). */
  UINT i;
  for (i = start; i < start + count; i++) {
    if (value)
      field[i >> 3] |= (BYTE)(1u << (i & 7));
    else
      field[i >> 3] &= (BYTE)(~(1u << (i & 7)));
  }
}

HRESULT WINAPI D3D10StateBlockMaskDisableAll(D3D10_STATE_BLOCK_MASK *pMask) {
  if (!pMask)
    return E_INVALIDARG;
  memset(pMask, 0, sizeof(*pMask));
  return S_OK;
}

HRESULT WINAPI D3D10StateBlockMaskEnableAll(D3D10_STATE_BLOCK_MASK *pMask) {
  if (!pMask)
    return E_INVALIDARG;
  memset(pMask, 0xFF, sizeof(*pMask));
  return S_OK;
}

static HRESULT mask_capture(D3D10_STATE_BLOCK_MASK *pMask,
                            D3D10_DEVICE_STATE_TYPES StateType, UINT RangeStart,
                            UINT RangeLength, BOOL value) {
  const struct mask_field *f = mask_lookup(StateType);
  if (!pMask || !f)
    return E_INVALIDARG;
  if (RangeStart >= f->bits)
    return E_INVALIDARG;
  /* Subtraction first: RangeStart + RangeLength can wrap past 2^32, which
   * used to bypass the clamp and drive the bit loop out of bounds. */
  if (RangeLength > f->bits - RangeStart)
    RangeLength = f->bits - RangeStart;
  /* NOTE: Wine returns E_INVALIDARG instead of clamping an overlong range.
   * We clamp (previous shim behavior); revisit if a game probes the
   * difference. */
  mask_set_bit_range((BYTE *)pMask + f->offset, RangeStart, RangeLength,
                     value);
  return S_OK;
}

HRESULT WINAPI D3D10StateBlockMaskDisableCapture(D3D10_STATE_BLOCK_MASK *pMask,
                                                 D3D10_DEVICE_STATE_TYPES StateType,
                                                 UINT RangeStart, UINT RangeLength) {
  return mask_capture(pMask, StateType, RangeStart, RangeLength, FALSE);
}

HRESULT WINAPI D3D10StateBlockMaskEnableCapture(D3D10_STATE_BLOCK_MASK *pMask,
                                                D3D10_DEVICE_STATE_TYPES StateType,
                                                UINT RangeStart, UINT RangeLength) {
  return mask_capture(pMask, StateType, RangeStart, RangeLength, TRUE);
}

BOOL WINAPI D3D10StateBlockMaskGetSetting(D3D10_STATE_BLOCK_MASK *pMask,
                                          D3D10_DEVICE_STATE_TYPES StateType,
                                          UINT Entry) {
  const struct mask_field *f = mask_lookup(StateType);
  if (!pMask || !f)
    return FALSE;
  return mask_get_bit((const BYTE *)pMask + f->offset, f->bits, Entry);
}

static HRESULT mask_combine(const D3D10_STATE_BLOCK_MASK *pA,
                            const D3D10_STATE_BLOCK_MASK *pB,
                            D3D10_STATE_BLOCK_MASK *pResult, int op) {
  /* op: 0 = union (OR), 1 = intersect (AND), 2 = difference (XOR, matching
   * Wine's d3d10 -- its test suite asserts 0x33 ^ 0x55 == 0x66). Byte-wise
   * ops are bit-correct under any packing. */
  const BYTE *a, *b;
  BYTE *r;
  size_t i;
  if (!pA || !pB || !pResult)
    return E_INVALIDARG;
  a = (const BYTE *)pA;
  b = (const BYTE *)pB;
  r = (BYTE *)pResult;
  for (i = 0; i < sizeof(D3D10_STATE_BLOCK_MASK); i++) {
    if (op == 0)
      r[i] = (BYTE)(a[i] | b[i]);
    else if (op == 1)
      r[i] = (BYTE)(a[i] & b[i]);
    else
      r[i] = (BYTE)(a[i] ^ b[i]);
  }
  return S_OK;
}

HRESULT WINAPI D3D10StateBlockMaskUnion(D3D10_STATE_BLOCK_MASK *pA,
                                        D3D10_STATE_BLOCK_MASK *pB,
                                        D3D10_STATE_BLOCK_MASK *pResult) {
  return mask_combine(pA, pB, pResult, 0);
}

HRESULT WINAPI D3D10StateBlockMaskIntersect(D3D10_STATE_BLOCK_MASK *pA,
                                            D3D10_STATE_BLOCK_MASK *pB,
                                            D3D10_STATE_BLOCK_MASK *pResult) {
  return mask_combine(pA, pB, pResult, 1);
}

HRESULT WINAPI D3D10StateBlockMaskDifference(D3D10_STATE_BLOCK_MASK *pA,
                                             D3D10_STATE_BLOCK_MASK *pB,
                                             D3D10_STATE_BLOCK_MASK *pResult) {
  return mask_combine(pA, pB, pResult, 2);
}

/* ------------------------------------------------------------------ */
/* Honest stubs: not part of DXMT's scope.                            */
/* ------------------------------------------------------------------ */

/* Signature per Wine's dlls/d3d10/d3d10.spec:
 * D3D10CompileEffectFromMemory(ptr long str ptr ptr long long ptr ptr). */
HRESULT WINAPI D3D10CompileEffectFromMemory(void *pData, SIZE_T DataLength,
                                            LPCSTR pSrcFileName,
                                            const D3D_SHADER_MACRO *pDefines,
                                            ID3D10Include *pInclude,
                                            UINT HLSLFlags, UINT FXFlags,
                                            ID3D10Blob **ppEffectBuffer,
                                            ID3D10Blob **ppErrors) {
  (void)pData;
  (void)DataLength;
  (void)pSrcFileName;
  (void)pDefines;
  (void)pInclude;
  (void)HLSLFlags;
  (void)FXFlags;
  (void)ppEffectBuffer;
  (void)ppErrors;
  return E_NOTIMPL; /* Effects10 framework: use d3dx10_xx.dll */
}

HRESULT WINAPI D3D10CreateEffectFromMemory(void *pData, SIZE_T DataLength,
                                           UINT FXFlags, ID3D10Device *pDevice,
                                           ID3D10EffectPool *pPool,
                                           ID3D10Effect **ppEffect) {
  (void)pData;
  (void)DataLength;
  (void)FXFlags;
  (void)pDevice;
  (void)pPool;
  (void)ppEffect;
  return E_NOTIMPL;
}

HRESULT WINAPI D3D10CreateEffectPoolFromMemory(void *pData, SIZE_T DataLength,
                                               UINT FXFlags, ID3D10Device *pDevice,
                                               ID3D10EffectPool **ppEffectPool) {
  (void)pData;
  (void)DataLength;
  (void)FXFlags;
  (void)pDevice;
  (void)ppEffectPool;
  return E_NOTIMPL;
}

HRESULT WINAPI D3D10DisassembleEffect(ID3D10Effect *pEffect, BOOL EnableColorCode,
                                      ID3D10Blob **ppDisassembly) {
  (void)pEffect;
  (void)EnableColorCode;
  (void)ppDisassembly;
  return E_NOTIMPL;
}

HRESULT WINAPI D3D10RegisterLayers(void) { return E_NOTIMPL; }

DWORD WINAPI D3D10GetVersion(void) { return 0; }

HRESULT WINAPI RevertToOldImplementation(void) { return E_NOTIMPL; }

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  (void)hinstDLL;
  (void)fdwReason;
  (void)lpvReserved;
  return TRUE;
}
