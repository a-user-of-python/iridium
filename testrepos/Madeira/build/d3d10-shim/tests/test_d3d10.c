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
  CHECK(D3D10CompileEffectFromMemory(NULL, 0, 0, NULL, NULL, &blob, &blob) ==
        E_NOTIMPL);
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
   * 504 bytes total, IAPrimitiveTopology present at offset 495. */
  CHECK(sizeof(D3D10_STATE_BLOCK_MASK) == 504);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, IAPrimitiveTopology) == 495);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, Predication) == 503);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, VSShaderResources) == 17);
  CHECK(offsetof(D3D10_STATE_BLOCK_MASK, PSConstantBuffers) == 463);

  CHECK(D3D10StateBlockMaskDisableAll(NULL) == E_INVALIDARG);
  CHECK(D3D10StateBlockMaskEnableAll(NULL) == E_INVALIDARG);

  memset(&m, 0xAA, sizeof(m));
  CHECK(D3D10StateBlockMaskDisableAll(&m) == S_OK);
  for (size_t i = 0; i < sizeof(m); i++)
    CHECK(((uint8_t *)&m)[i] == 0);

  CHECK(D3D10StateBlockMaskEnableAll(&m) == S_OK);
  for (size_t i = 0; i < sizeof(m); i++)
    CHECK(((uint8_t *)&m)[i] == 0xFF);

  /* Capture ranges address the right fields */
  CHECK(D3D10StateBlockMaskDisableAll(&m) == S_OK);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_VS_SAMPLERS, 4, 3) ==
        S_OK);
  CHECK(m.VSSamplers[3] == 0);
  CHECK(m.VSSamplers[4] == 0xFF);
  CHECK(m.VSSamplers[6] == 0xFF);
  CHECK(m.VSSamplers[7] == 0);
  CHECK(m.PSSamplers[4] == 0); /* neighboring field untouched */

  CHECK(D3D10StateBlockMaskDisableCapture(&m, D3D10_DST_VS_SAMPLERS, 4, 3) ==
        S_OK);
  CHECK(m.VSSamplers[5] == 0);

  /* Range clamping at the field end */
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_SO_BUFFERS, 0, 99) ==
        S_OK);
  CHECK(m.SOBuffers == 0xFF);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_SO_BUFFERS, 5, 4) ==
        E_INVALIDARG); /* starts past the 1-byte field */

  /* GetSetting */
  CHECK(D3D10StateBlockMaskDisableAll(&m) == S_OK);
  CHECK(D3D10StateBlockMaskGetSetting(&m, D3D10_DST_PS, 0) == FALSE);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_PS, 0, 1) == S_OK);
  CHECK(D3D10StateBlockMaskGetSetting(&m, D3D10_DST_PS, 0) == TRUE);
  CHECK(D3D10StateBlockMaskGetSetting(&m, D3D10_DST_PS, 9) == FALSE);
  CHECK(D3D10StateBlockMaskGetSetting(NULL, D3D10_DST_PS, 0) == FALSE);
  CHECK(D3D10StateBlockMaskGetSetting(&m, (D3D10_DEVICE_STATE_TYPES)99, 0) ==
        FALSE);

  /* Newly added state types (24 total) hit their own fields */
  CHECK(D3D10StateBlockMaskDisableAll(&m) == S_OK);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_VS_SHADER_RESOURCES, 0,
                                         128) == S_OK);
  CHECK(m.VSShaderResources[0] == 0xFF && m.VSShaderResources[127] == 0xFF);
  CHECK(m.VSSamplers[0] == 0); /* neighbor untouched */
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_PS_CONSTANT_BUFFERS, 13,
                                         1) == S_OK);
  CHECK(m.PSConstantBuffers[13] == 0xFF);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_IA_PRIMITIVE_TOPOLOGY,
                                         0, 1) == S_OK);
  CHECK(m.IAPrimitiveTopology == 0xFF);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_RS_VIEWPORTS, 0, 1) ==
        S_OK);
  CHECK(m.RSViewports == 0xFF);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_RS_SCISSOR_RECTS, 0,
                                         1) == S_OK);
  CHECK(m.RSScissorRects == 0xFF);
  CHECK(D3D10StateBlockMaskEnableCapture(&m, D3D10_DST_RS_RASTERIZER_STATE, 0,
                                         1) == S_OK);
  CHECK(m.RSRasterizerState == 0xFF);
  CHECK(D3D10StateBlockMaskGetSetting(&m, D3D10_DST_IA_PRIMITIVE_TOPOLOGY, 0) ==
        TRUE);
  CHECK(D3D10StateBlockMaskGetSetting(&m, D3D10_DST_PREDICATION, 0) == FALSE);

  /* Boolean algebra over whole masks */
  memset(&a, 0x0F, sizeof(a));
  memset(&b, 0xF0, sizeof(b));
  CHECK(D3D10StateBlockMaskUnion(&a, &b, &r) == S_OK);
  CHECK(r.VS == 0xFF && r.Predication == 0xFF);
  CHECK(D3D10StateBlockMaskIntersect(&a, &b, &r) == S_OK);
  CHECK(r.VS == 0x00 && r.Predication == 0x00);
  CHECK(D3D10StateBlockMaskDifference(&a, &b, &r) == S_OK);
  CHECK(r.VS == 0x0F);
  CHECK(D3D10StateBlockMaskDifference(&b, &a, &r) == S_OK);
  CHECK(r.VS == 0xF0);
  CHECK(D3D10StateBlockMaskUnion(NULL, &b, &r) == E_INVALIDARG);
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
