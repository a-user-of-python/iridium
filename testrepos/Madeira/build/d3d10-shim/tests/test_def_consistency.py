#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Additional permission: Madeira Converter Exception, version 1
# (testrepos/Madeira/LICENSE-EXCEPTION.md)
"""Check that each d3d10-shim .def export list exactly matches the WINAPI
functions implemented in the corresponding .c file.

Usage: test_def_consistency.py
Exit 0 when every .def name has one implementation and every implemented
export is listed; otherwise prints the mismatch and exits 1.
"""
import re
import sys
from pathlib import Path

SHIM_DIR = Path(__file__).resolve().parent.parent

PAIRS = [("d3d10.def", "d3d10.c"), ("d3d10_1.def", "d3d10_1.c")]

def def_exports(path):
    names = []
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith(";") or line.upper().startswith("LIBRARY") \
                or line.upper().startswith("EXPORTS"):
            continue
        names.append(line.split()[0])
    return names

def implemented_exports(path):
    # Matches: "<ret> WINAPI <Name>(" at top level, plus the
    # FORWARD<n>(<ret>, <Name>, ...) macro invocations in d3d10_1.c.
    text = path.read_text()
    direct = re.findall(r"^[A-Za-z_][\w ]*?\s+WINAPI\s+(\w+)\s*\(", text, re.M)
    macro = re.findall(r"^FORWARD\d+\s*\(\s*\w[\w ]*,\s*(\w+)", text, re.M)
    return direct + macro

def main():
    failures = 0
    for def_name, c_name in PAIRS:
        want = def_exports(SHIM_DIR / def_name)
        have = implemented_exports(SHIM_DIR / c_name)
        # DllMain is implemented but deliberately not exported.
        have = [h for h in have if h != "DllMain"]
        missing = [w for w in want if w not in have]
        extra = [h for h in have if h not in want]
        dupes = [w for w in set(want) if want.count(w) > 1]
        print(f"{def_name}: {len(want)} exports, {len(have)} implemented")
        for w in missing:
            print(f"  MISSING implementation for exported {w}")
            failures += 1
        for h in extra:
            print(f"  UNLISTED implementation {h} (add to {def_name} or drop)")
            failures += 1
        for w in dupes:
            print(f"  DUPLICATE export entry {w}")
            failures += 1
    if failures:
        print(f"def consistency: {failures} problem(s)")
        return 1
    print("def consistency: ALL EXPORTS MATCH IMPLEMENTATIONS")
    return 0

if __name__ == "__main__":
    sys.exit(main())
