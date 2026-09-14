#!/usr/bin/env python3
"""Prove this repository and FastLED hold byte-identical decoder sources.

    python3 tools/check_vendor_sync.py --fastled ~/dev/fastled

This repository is where the fixed-point decoder is optimised; FastLED vendors
it. Those two copies drifting apart is the failure mode that would make every
measurement here meaningless for the thing that ships, and it is silent -- the
code would still compile and still decode. So it is checked rather than
trusted.

Five files carry the whole decoder and the arithmetic primitives it depends on.
Nothing else needs to match: FastLED has its own build system, and this
repository has its own port shims precisely so the shared files can stay
identical instead of being forked.
"""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# here -> FastLED
PAIRS = {
    "minimp3.h": "src/third_party/minimp3/minimp3.h",
    "minimp3_synth_fixed.h": "src/third_party/minimp3/minimp3_synth_fixed.h",
    "minimp3_fixed_tables.h": "src/third_party/minimp3/minimp3_fixed_tables.h",
    "port/fl/math/int_asm.h": "src/fl/math/int_asm.h",
    "port/platforms/int_asm.h": "src/platforms/int_asm.h",
}


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fastled", type=Path, required=True,
                        help="path to a FastLED checkout")
    parser.add_argument("--print", action="store_true", dest="show",
                        help="print this side's hashes and exit, for CI")
    args = parser.parse_args(argv)

    if args.show:
        for local in PAIRS:
            print(f"{digest(ROOT / local)}  {local}")
        return 0

    drift = []
    for local, remote in PAIRS.items():
        here, there = ROOT / local, args.fastled / remote
        if not there.is_file():
            drift.append(f"{remote}: not found in {args.fastled}")
            continue
        a, b = digest(here), digest(there)
        mark = "ok " if a == b else "DIFF"
        print(f"  [{mark}] {local}")
        if a != b:
            drift.append(f"{local}\n         here    {a}\n         fastled {b}")

    print()
    if drift:
        print("VENDOR SYNC: DRIFTED")
        for item in drift:
            print(f"  {item}")
        print("\nReconcile before measuring: a number taken here describes the "
              "code here,\nand FastLED is what ships.")
        return 1
    print(f"VENDOR SYNC: OK, {len(PAIRS)} files byte-identical")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
