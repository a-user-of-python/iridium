/* Minimal Windows/D3D stub headers for host-side unit tests of the
 * d3d10-shim logic. These declare ONLY what the shim sources use, with
 * layout-compatible definitions for the types the tests exercise
 * (D3D10_STATE_BLOCK_MASK, enums, HRESULT codes). They are NOT a Windows
 * SDK replacement; the real compile check happens with mingw-w64. */
#ifndef TEST_STUB_WINDOWS_H
#define TEST_STUB_WINDOWS_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef long HRESULT;
typedef unsigned int UINT;
typedef int BOOL;
typedef uint64_t SIZE_T;
typedef const char *LPCSTR;
typedef void *LPVOID;
typedef const void *LPCVOID;
typedef uint8_t BYTE;
typedef unsigned long DWORD;
typedef void *HMODULE;
typedef void *HINSTANCE;
typedef void *FARPROC;
typedef const void *REFIID;

#define WINAPI
#define TRUE 1
#define FALSE 0
#define S_OK ((HRESULT)0)
#define S_FALSE ((HRESULT)1)
#define E_INVALIDARG ((HRESULT)(int32_t)0x80070057)  /* sign-extended: real windows.h has 32-bit long */
#define E_NOTIMPL ((HRESULT)(int32_t)0x80004001)  /* sign-extended: real windows.h has 32-bit long */
#define E_NOINTERFACE ((HRESULT)(int32_t)0x80004002)  /* sign-extended: real windows.h has 32-bit long */
#define E_FAIL ((HRESULT)(int32_t)0x80004005)  /* sign-extended: real windows.h has 32-bit long */
#define FAILED(hr) ((HRESULT)(hr) < 0)
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)

/* Fake GUIDs: distinct addresses are all the tests need. */
typedef struct { int tag; } TEST_GUID;
static const TEST_GUID TEST_IID_IDXGIFactory = {1};
static const TEST_GUID TEST_IID_IDXGIDevice = {2};
static const TEST_GUID TEST_IID_ID3D10Device1 = {3};
static const TEST_GUID TEST_IID_ID3D10ShaderReflection = {4};
#define IID_IDXGIFactory TEST_IID_IDXGIFactory
#define IID_IDXGIDevice TEST_IID_IDXGIDevice
#define IID_ID3D10Device1 TEST_IID_ID3D10Device1
#define IID_ID3D10ShaderReflection TEST_IID_ID3D10ShaderReflection

static HMODULE stub_LoadLibraryA(LPCSTR n) {
  (void)n;
  return (HMODULE)0x1;
}
static FARPROC stub_GetProcAddress(HMODULE m, LPCSTR n) {
  (void)m;
  (void)n;
  return (FARPROC)0;
}
#define LoadLibraryA stub_LoadLibraryA
#define GetProcAddress stub_GetProcAddress

#endif
