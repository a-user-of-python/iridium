/* Host unit tests for d3d10_1.c (the d3d10_1.dll shim).
 *
 * Compiles the REAL shim source against minimal stub headers and checks
 * the 10.1 feature-level mapping plus the argument validation that runs
 * before any DXMT call.
 */
#include <assert.h>
#include <stdio.h>

#include "../d3d10_1.c"

static int checks = 0;
#define CHECK(cond)                                                            \
  do {                                                                         \
    checks++;                                                                  \
    if (!(cond)) {                                                             \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                   \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static int test_feature_level_mapping(void) {
  CHECK(feature_level_1(D3D10_FEATURE_LEVEL_10_0) == D3D_FEATURE_LEVEL_10_0);
  CHECK(feature_level_1(D3D10_FEATURE_LEVEL_10_1) == D3D_FEATURE_LEVEL_10_1);
  CHECK(feature_level_1((D3D10_FEATURE_LEVEL1)0) == (D3D_FEATURE_LEVEL)0);
  CHECK(feature_level_1((D3D10_FEATURE_LEVEL1)0xB000) == (D3D_FEATURE_LEVEL)0);
  return 0;
}

static int test_create_device1_validation(void) {
  ID3D10Device1 *dev = (ID3D10Device1 *)0x1234;

  /* NULL out-pointer */
  CHECK(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
                           D3D10_FEATURE_LEVEL_10_1, D3D10_SDK_VERSION,
                           NULL) == E_INVALIDARG);
  /* Unknown feature level */
  CHECK(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
                           (D3D10_FEATURE_LEVEL1)0xB000, D3D10_SDK_VERSION,
                           &dev) == E_INVALIDARG);
  CHECK(dev == NULL);
  /* Bad SDK version passes validation order: feature level is checked
   * first in this entry point, then the shared validator rejects the SDK. */
  CHECK(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
                           D3D10_FEATURE_LEVEL_10_1, 0,
                           &dev) == DXGI_ERROR_SDK_COMPONENT_MISSING);
  /* Non-hardware driver */
  CHECK(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_WARP, NULL, 0,
                           D3D10_FEATURE_LEVEL_10_1, D3D10_SDK_VERSION,
                           &dev) == DXGI_ERROR_UNSUPPORTED);
  /* Without DXMT deployed: the missing backend is reported, not a crash */
  CHECK(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
                           D3D10_FEATURE_LEVEL_10_1, D3D10_SDK_VERSION,
                           &dev) == E_NOINTERFACE);
  CHECK(dev == NULL);
  return 0;
}

static int test_create_device_and_swapchain1_validation(void) {
  ID3D10Device1 *dev = (ID3D10Device1 *)0x1234;
  IDXGISwapChain *chain = (IDXGISwapChain *)0x1234;
  DXGI_SWAP_CHAIN_DESC desc;
  memset(&desc, 0, sizeof(desc));

  CHECK(D3D10CreateDeviceAndSwapChain1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL,
                                       0, D3D10_FEATURE_LEVEL_10_1,
                                       D3D10_SDK_VERSION, NULL, &chain,
                                       &dev) == E_INVALIDARG);
  CHECK(D3D10CreateDeviceAndSwapChain1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL,
                                       0, D3D10_FEATURE_LEVEL_10_1,
                                       D3D10_SDK_VERSION, &desc, NULL,
                                       &dev) == E_INVALIDARG);
  CHECK(D3D10CreateDeviceAndSwapChain1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL,
                                       0, (D3D10_FEATURE_LEVEL1)0,
                                       D3D10_SDK_VERSION, &desc, &chain,
                                       &dev) == E_INVALIDARG);
  (void)chain;
  return 0;
}

static int test_forwards_fail_clean(void) {
  /* d3d10.dll is absent in the stub environment: forwards must return
   * E_NOINTERFACE, and the LPCSTR ones must return NULL (not a bogus
   * error-code-as-pointer). */
  ID3D10Blob *blob = NULL;
  CHECK(D3D10CreateBlob(16, &blob) == E_NOINTERFACE);
  CHECK(D3D10GetVertexShaderProfile(NULL) == NULL);
  CHECK(D3D10GetPixelShaderProfile(NULL) == NULL);
  CHECK(D3D10GetGeometryShaderProfile(NULL) == NULL);
  CHECK(D3D10StateBlockMaskDisableAll(NULL) == E_NOINTERFACE);
  return 0;
}

int main(void) {
  if (test_feature_level_mapping())
    return 1;
  if (test_create_device1_validation())
    return 1;
  if (test_create_device_and_swapchain1_validation())
    return 1;
  if (test_forwards_fail_clean())
    return 1;
  printf("d3d10_1 shim logic tests: ALL %d CHECKS PASSED\n", checks);
  return 0;
}
