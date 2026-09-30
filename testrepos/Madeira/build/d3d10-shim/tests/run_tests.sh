#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Additional permission: Madeira Converter Exception, version 1
# (testrepos/Madeira/LICENSE-EXCEPTION.md)
# Host-side tests for the d3d10/d3d10_1 shims.
#
# Compiles the REAL shim sources against minimal stub headers (layout- and
# signature-compatible with the Windows SDK) and runs:
#   1. logic unit tests (argument validation, mask helpers, forwards)
#   2. .def <-> implementation export consistency
#   3. build.sh syntax check
#
# This does NOT replace a real mingw-w64 compile or on-device testing; it
# catches logic and signature mistakes on any host with gcc + python3.
set -euo pipefail

TESTS_DIR="$(cd "$(dirname "$0")" && pwd)"
SHIM_DIR="$(dirname "$TESTS_DIR")"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

echo "== d3d10 logic =="
gcc -std=c11 -Wall -Wextra -I "$TESTS_DIR/stubs" -I "$SHIM_DIR" \
    -o "$OUT/test_d3d10" "$TESTS_DIR/test_d3d10.c"
"$OUT/test_d3d10"

echo "== d3d10_1 logic =="
gcc -std=c11 -Wall -Wextra -I "$TESTS_DIR/stubs" -I "$SHIM_DIR" \
    -o "$OUT/test_d3d10_1" "$TESTS_DIR/test_d3d10_1.c"
"$OUT/test_d3d10_1"

echo "== export consistency =="
python3 "$TESTS_DIR/test_def_consistency.py"

echo "== build.sh syntax =="
bash -n "$SHIM_DIR/build.sh"
echo "build.sh syntax OK"

echo
echo "HOST TESTS: ALL PASSED"
