/* Host unit tests for d3d10.c (the d3d10.dll shim).
 *
 * Compiles the REAL shim source against minimal stub headers and
 * exercises the logic that does not need DXMT deployed: argument
 * validation, profile strings, stub return codes, and the full
 * D3D10_STATE_BLOCK_MASK helper set.
 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "../d3d10.c"

static int checks = 0;
#define CHECK(cond)                                                            \
  do {                                                                         \
    checks++;                                                                  \
    if (!(cond)) {                                                             \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                   \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static int test_validate_args(void) {
  ID3D10Device *dev = (ID3D10Device *)0x1234;

  /* NULL out-pointer */
  CHECK(shim_validate_create_args(D3D10_DRIVER_TYPE_HARDWARE, NULL,
                                  D3D10_SDK_VERSION, NULL) == E_INVALIDARG);
  /* Wrong SDK version */
  CHECK(shim_validate_create_args(D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
                                  (void **)&dev) ==
        DXGI_ERROR_SDK_COMPONENT_MISSING);
  CHECK(dev == NULL); /* cleared before the SDK check */
  /* Every non-hardware driver type is unsupported (no WARP backend) */
  CHECK(shim_validate_create_args(D3D10_DRIVER_TYPE_REFERENCE, NULL,
                                  D3D10_SDK_VERSION,
                                  (void **)&dev) == DXGI_ERROR_UNSUPPORTED);
  CHECK(shim_validate_create_args(D3D10_DRIVER_TYPE_NULL, NULL,
                                  D3D10_SDK_VERSION,
                                  (void **)&dev) == DXGI_ERROR_UNSUPPORTED);
  CHECK(shim_validate_create_args(D3D10_DRIVER_TYPE_SOFTWARE, NULL,
                                  D3D10_SDK_VERSION,
                                  (void **)&dev) == DXGI_ERROR_UNSUPPORTED);
  CHECK(shim_validate_create_args(D3D10_DRIVER_TYPE_WARP, NULL,
                                  D3D10_SDK_VERSION,
                                  (void **)&dev) == DXGI_ERROR_UNSUPPORTED);
  /* Software module handle is meaningless without a software rasterizer */
  CHECK(shim_validate_create_args(D3D10_DRIVER_TYPE_HARDWARE, (HMODULE)0x1,
                                  D3D10_SDK_VERSION,
                                  (void **)&dev) == E_INVALIDARG);
  /* Happy path */
  dev = (ID3D10Device *)0x1234;
  CHECK(shim_validate_create_args(D3D10_DRIVER_TYPE_HARDWARE, NULL,
                                  D3D10_SDK_VERSION,
                                  (void **)&dev) == S_OK);
  CHECK(dev == NULL);
  return 0;
}

static int test_profiles_and_stubs(void) {
  ID3D10Blob *blob = NULL;

  CHECK(strcmp(D3D10GetVertexShaderProfile(NULL), "vs_4_0") == 0);
  CHECK(strcmp(D3D10GetGeometryShaderProfile(NULL), "gs_4_0") == 0);
  CHECK(strcmp(D3D10GetPixelShaderProfile(NULL), "ps_4_0") == 0);

  /* Effect framework is honestly unimplemented */
  CHECK(D3D10CompileEffectFromMemory(NULL, 0, NULL, NULL, NULL, 0, 0, &blob,
                                     &blob) == E_NOTIMPL);
  CHECK(D3D10CreateEffectFromMemory(NULL, 0, 0, NULL, NULL, NULL) == E_NOTIMPL);
  CHECK(D3D10CreateEffectPoolFromMemory(NULL, 0, 0, NULL, NULL) == E_NOTIMPL);
  CHECK(D3D10DisassembleEffect(NULL, FALSE, &blob) == E_NOTIMPL);
  CHECK(D3D10RegisterLayers() == E_NOTIMPL);
  CHECK(RevertToOldImplementation() == E_NOTIMPL);

  /* d3dcompiler_43 is absent in the stub environment: forwards must fail
   * with E_NOINTERFACE, not crash. */
  CHECK(D3D10CreateBlob(64, &blob) == E_NOINTERFACE);
  CHECK(D3D10CompileShader("x", 1, "f", NULL, NULL, "m", "ps_4_0", 0, &blob,
                           &blob) == E_NOINTERFACE);
  CHECK(D3D10ReflectShader("x", 1, NULL) == E_NOINTERFACE);

  /* Device creation without DXMT deployed reports the missing backend */
  {
    ID3D10Device *dev = NULL;
    CHECK(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
                            D3D10_SDK_VERSION, &dev) == E_NOINTERFACE);
    CHECK(dev == NULL);
  }
  return 0;
}

static int test_mask_helpers(void) {
  D3D10_STATE_BLOCK_MASK m, a, b, r;

  /* Layout must match the Windows SDK / Wine d3d10effect.idl exactly:
   * 76 bytes total, bit-packed (one bit per array slot). */
  CHECK(sizeof(D3D10_STATE_BLOCK_MASK) == 76);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, VS) == 0);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, VSSamplers) == 1);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, VSShaderResources) == 3);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, VSConstantBuffers) == 19);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, GS) == 21);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, PS) == 42);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, PSConstantBuffers) == 61);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, IAVertexBuffers) == 63);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, IAIndexBuffer) == 65);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, IAPrimitiveTopology) == 67);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, OMRenderTargets) == 68);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, SOBuffers) == 74);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, Predication) == 75);

  CHECK(D3D10StateBlockMaskDisableAll(NULL) == E_INVALIDARG);
  CHECK(D3D10StateBlockMaskEnableAll(NULL) == E_INVALIDARG);

  memset(&m, 0xAA, sizeof(m));
  CHECK(D3D10StateBlockMaskDisableAll(&m) == S_OK);
  for (size_t i = 0; i < sizeof(m); i++)
    CHECK(((uint8_t *)&m)[i] == 0);

  CHECK(D3D10StateBlockMaskEnableAll(&m) == S_OK);
  for (size_t i = 0; i < sizeof(m); i++)
    CHECK(((uint8_t *)&m)[i] == 0xFF);

  /* Capture ranges set individual BITS (Wine's dlls/d3d10/tests/device.c
   * vectors: {start, count} -> expected bytes). */
  CHECK(D3D10StateBlockMaskDisableAll(&m) == S_OK);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_VS_SAMPLERS, 8, 4) ==
        S_OK);
  CHECK(m.VSSamplers[0] == 0x00);
  CHECK(m.VSSamplers[1] == 0x0F); /* bits 8..11 */
  CHECK(m.PSSamplers[1] == 0x00); /* neighboring field untouched */

  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_VS_SAMPLERS, 9, 4) ==
        S_OK);
  CHECK(m.VSSamplers[1] == 0x1F); /* bits 8..12 */

  CHECK(D3D10StateBlockMaskDisableAll(&m) == S_OK);
  CHECK(D3D10StateBlockMaskDisableCapture(&m, D3D10_DST_VS_SAMPLERS, 9, 4) ==
        S_OK);
  CHECK(m.VSSamplers[0] == 0x00);
  CHECK(m.VSSamplers[1] == 0x00);

  memset(&m, 0xFF, sizeof(m));
  CHECK(D3D10StateBlockMaskDisableCapture(&m, D3D10_DST_VS_SAMPLERS, 9, 4) ==
        S_OK);
  CHECK(m.VSSamplers[1] == 0xE1); /* 0xFF with bits 9..12 cleared */

  /* Cross-byte range: bits 8..19 of the 128-bit resource field */
  CHECK(D3D10StateBlockMaskDisableAll(&m) == S_OK);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_VS_SHADER_RESOURCES, 8,
                                         12) == S_OK);
  CHECK(m.VSShaderResources[0] == 0x00);
  CHECK(m.VSShaderResources[1] == 0xFF);
  CHECK(m.VSShaderResources[2] == 0x0F);
  CHECK(m.VSShaderResources[3] == 0x00);

  /* Scalar (single-bit) fields */
  CHECK(D3D10StateBlockMaskDisableAll(&m) == S_OK);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_PS, 0, 1) == S_OK);
  CHECK(m.PS == 0x01);
  memset(&m, 0xFF, sizeof(m));
  CHECK(D3D10StateBlockMaskDisableCapture(&m, D3D10_DST_PS, 0, 1) == S_OK);
  CHECK(m.PS == 0xFE);

  /* Overlong ranges clamp to the field end (no E_INVALIDARG, no wrap) */
  CHECK(D3D10StateBlockMaskDisableAll(&m) == S_OK);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_SO_BUFFERS, 0, 99) ==
        S_OK);
  CHECK(m.SOBuffers == 0x01); /* one bit, not one byte */
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_SO_BUFFERS, 5, 4) ==
        E_INVALIDARG); /* starts past the 1-bit field */

  /* D2 regression: huge RangeLength must clamp, not wrap and overrun */
  CHECK(D3D10StateBlockMaskDisableAll(&m) == S_OK);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_VS_SAMPLERS, 0,
                                          ~0u) == S_OK);
  CHECK(m.VSSamplers[0] == 0xFF && m.VSSamplers[1] == 0xFF);
  CHECK(D3D10StateBlockMaskDisableAll(&m) == S_OK);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_VS_SAMPLERS, 15,
                                          ~0u) == S_OK);
  CHECK(m.VSSamplers[0] == 0x00 && m.VSSamplers[1] == 0x80);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_VS_SAMPLERS, ~0u, 1) ==
        E_INVALIDARG);

  /* GetSetting reads bits */
  CHECK(D3D10StateBlockMaskDisableAll(&m) == S_OK);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_VS_SAMPLERS, 8, 4) ==
        S_OK);
  CHECK(D3D10StateBlockMaskGetSetting(&m, D3D10_DST_VS_SAMPLERS, 8) == TRUE);
  CHECK(D3D10StateBlockMaskGetSetting(&m, D3D10_DST_VS_SAMPLERS, 11) == TRUE);
  CHECK(D3D10StateBlockMaskGetSetting(&m, D3D10_DST_VS_SAMPLERS, 7) == FALSE);
  CHECK(D3D10StateBlockMaskGetSetting(&m, D3D10_DST_VS_SAMPLERS, 12) == FALSE);
  CHECK(D3D10StateBlockMaskGetSetting(&m, D3D10_DST_VS_SAMPLERS, 16) == FALSE);
  CHECK(D3D10StateBlockMaskGetSetting(&m, D3D10_DST_PS, 0) == FALSE);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_PS, 0, 1) == S_OK);
  CHECK(D3D10StateBlockMaskGetSetting(&m, D3D10_DST_PS, 0) == TRUE);
  CHECK(D3D10StateBlockMaskGetSetting(&m, D3D10_DST_PS, 9) == FALSE);
  CHECK(D3D10StateBlockMaskGetSetting(NULL, D3D10_DST_PS, 0) == FALSE);
  CHECK(D3D10StateBlockMaskGetSetting(&m, (D3D10_DEVICE_STATE_TYPES)99, 0) ==
        FALSE);

  /* All 24 state types address their own fields */
  CHECK(D3D10StateBlockMaskDisableAll(&m) == S_OK);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_PS_CONSTANT_BUFFERS,
                                         13, 1) == S_OK);
  CHECK((m.PSConstantBuffers[1] & 0x20) != 0); /* bit 13 */
  CHECK((m.PSConstantBuffers[1] & 0xDF) == 0);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_IA_PRIMITIVE_TOPOLOGY,
                                         0, 1) == S_OK);
  CHECK(m.IAPrimitiveTopology == 0x01);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_RS_VIEWPORTS, 0, 1) ==
        S_OK);
  CHECK(m.RSViewports == 0x01);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_RS_SCISSOR_RECTS, 0,
                                         1) == S_OK);
  CHECK(m.RSScissorRects == 0x01);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_RS_RASTERIZER_STATE, 0,
                                         1) == S_OK);
  CHECK(m.RSRasterizerState == 0x01);
  CHECK(D3D10StateBlockMaskGetSetting(&m, D3D10_DST_IA_PRIMITIVE_TOPOLOGY, 0) ==
        TRUE);
  CHECK(D3D10StateBlockMaskGetSetting(&m, D3D10_DST_PREDICATION, 0) == FALSE);

  /* Boolean algebra over whole masks; Difference is XOR (Wine parity:
   * its test suite asserts 0x33 ^ 0x55 == 0x66). */
  memset(&a, 0x0F, sizeof(a));
  memset(&b, 0xF0, sizeof(b));
  CHECK(D3D10StateBlockMaskUnion(&a, &b, &r) == S_OK);
  CHECK(r.VS == 0xFF && r.Predication == 0xFF);
  CHECK(D3D10StateBlockMaskIntersect(&a, &b, &r) == S_OK);
  CHECK(r.VS == 0x00 && r.Predication == 0x00);
  CHECK(D3D10StateBlockMaskDifference(&a, &b, &r) == S_OK);
  CHECK(r.VS == 0xFF);
  a.VS = 0x33;
  b.VS = 0x55;
  CHECK(D3D10StateBlockMaskDifference(&a, &b, &r) == S_OK);
  CHECK(r.VS == 0x66);
  CHECK(D3D10StateBlockMaskUnion(NULL, &b, &r) == E_INVALIDARG);
  CHECK(D3D10StateBlockMaskIntersect(&a, NULL, &r) == E_INVALIDARG);
  CHECK(D3D10StateBlockMaskDifference(&a, &b, NULL) == E_INVALIDARG);
  return 0;
}

int main(void) {
  if (test_validate_args())
    return 1;
  if (test_profiles_and_stubs())
    return 1;
  if (test_mask_helpers())
    return 1;
  printf("d3d10 shim logic tests: ALL %d CHECKS PASSED\n", checks);
  return 0;
}
