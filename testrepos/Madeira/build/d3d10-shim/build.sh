#!/bin/bash
# Build the D3D10/D3D10.1 -> DXMT shims (d3d10.dll, d3d10_1.dll) as
# ARM64EC and AArch64 Windows PE DLLs, using the same pinned LLVM-MinGW
# toolchain the DXMT PE modules are built with.
#
# Output: out/<arch>/d3d10.dll, out/<arch>/d3d10_1.dll
set -euo pipefail

SHIM_DIR="$(cd "$(dirname "$0")" && pwd)"
MADEIRA="$(cd "$SHIM_DIR/../.." && pwd)"
MINGW_BIN="$MADEIRA/toolchains/llvm-mingw-20260421-ucrt-macos-universal/bin"
OUT="$SHIM_DIR/out"

if [[ ! -x "$MINGW_BIN/arm64ec-w64-mingw32-clang" ]]; then
    echo "d3d10-shim: missing LLVM-MinGW toolchain at $MINGW_BIN" >&2
    echo "d3d10-shim: run the runtime input preparation first (ci/prepare-native-runtime.sh)" >&2
    exit 69
fi

build_one() {
    local arch="$1" triple="$2" src="$3" def="$4" dll="$5"
    local dir="$OUT/$arch"
    mkdir -p "$dir"
    echo "d3d10-shim: building $dll for $arch"
    "$MINGW_BIN/$triple-clang" \
        -target "$triple" \
        -O2 -std=c11 -Wall -Wextra \
        -shared \
        -o "$dir/$dll" "$src" \
        -Wl,--def,"$def" \
        -Wl,--out-implib,"$dir/lib${dll%.dll}.a" \
        -lkernel32
    # Strip debug data like the DXMT staging step does; keep code, exports,
    # resources, relocations, and unwind info intact.
    "$MINGW_BIN/$triple-strip" --strip-debug "$dir/$dll"
    echo "d3d10-shim: wrote $dir/$dll ($(wc -c < "$dir/$dll" | tr -d ' ') bytes)"
}

for arch in arm64ec aarch64; do
    if [[ "$arch" == "arm64ec" ]]; then triple="arm64ec-w64-mingw32"; else triple="aarch64-w64-mingw32"; fi
    build_one "$arch" "$triple" "$SHIM_DIR/d3d10.c"   "$SHIM_DIR/d3d10.def"   "d3d10.dll"
    build_one "$arch" "$triple" "$SHIM_DIR/d3d10_1.c" "$SHIM_DIR/d3d10_1.def" "d3d10_1.dll"
done

echo "d3d10-shim: done"
