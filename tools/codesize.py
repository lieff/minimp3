#!/usr/bin/env python3
"""Cross-target code size for the fixed-point decoder, with a baseline diff.

    python3 tools/codesize.py                    # riscv32 and arm .text
    python3 tools/codesize.py --baseline HEAD    # against the last commit
    python3 tools/codesize.py --functions        # per-function breakdown

Deterministic, needs no hardware, and runs in seconds. What it is good for and
what it is not:

  - It BOUNDS a win. Fewer instructions in the hot path cannot make the decoder
    slower, and a change that adds 20% of text for 3% of speed is visible here
    before anyone flashes anything.
  - It does NOT rank changes. Static size says nothing about how many times a
    block executes. On this decoder, minimp3 is now smaller than the Helix
    reference in every stage but one and still measurably slower.

Read tools/qemu_check.py for correctness across ISAs, and see the doctrine
section of README.md for why neither of these replaces a stopwatch on real
silicon.
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# (label, zig target, objdump-capable llvm tool arch)
TARGETS = [
    ("riscv32", "riscv32-linux-musl"),
    ("arm", "arm-linux-musleabihf"),
]

FLAGS = [
    "-std=gnu++11", "-Os", "-fno-exceptions", "-fno-rtti",
    "-fno-strict-aliasing", "-DMINIMP3_FIXED_POINT", "-DMINIMP3_NO_SIMD",
    "-DMINIMP3_ONLY_MP3",
]


def build_object(target: str, tree: Path, out: Path) -> Path:
    obj = out / f"decoder_{target}.o"
    subprocess.run(
        ["zig", "c++", "-target", target, "-nostdlib++", "-c",
         f"-I{tree}", f"-I{tree / 'port'}", *FLAGS,
         str(ROOT / "tools" / "decode.cpp"), "-o", str(obj)],
        check=True, capture_output=True,
    )
    return obj


def sections(obj: Path) -> dict[str, int]:
    out = subprocess.run(["llvm-size", "-A", str(obj)],
                         capture_output=True, text=True, check=True).stdout
    found: dict[str, int] = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) >= 2 and parts[0].startswith("."):
            try:
                found[parts[0]] = int(parts[1])
            except ValueError:
                pass
    return found


def functions(obj: Path) -> list[tuple[int, str]]:
    out = subprocess.run(["llvm-nm", "--print-size", "--size-sort",
                          "--reverse-sort", str(obj)],
                         capture_output=True, text=True, check=True).stdout
    rows = []
    for line in out.splitlines():
        parts = line.split()
        if len(parts) >= 4 and parts[2].lower() == "t":
            rows.append((int(parts[1], 16), parts[3]))
    return rows


def checkout(ref: str, into: Path) -> Path:
    tree = into / "baseline"
    tree.mkdir()
    tar = subprocess.run(["git", "archive", ref], cwd=ROOT,
                         check=True, capture_output=True).stdout
    subprocess.run(["tar", "-x", "-C", str(tree)], input=tar, check=True)
    return tree


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", help="git ref to compare against")
    parser.add_argument("--functions", action="store_true")
    args = parser.parse_args(argv)

    for tool in ("zig", "llvm-size", "llvm-nm"):
        if not shutil.which(tool):
            raise SystemExit(
                f"{tool} not found. nix shell nixpkgs#zig nixpkgs#llvm")

    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp)
        base_tree = checkout(args.baseline, out) if args.baseline else None
        for label, target in TARGETS:
            obj = build_object(target, ROOT, out)
            text = sections(obj).get(".text", 0)
            print(f"\n{label}")
            print(f"  .text   {text:>9,} bytes")
            if base_tree:
                base = sections(build_object(target, base_tree, out)).get(".text", 0)
                delta = text - base
                pct = (delta / base * 100) if base else 0.0
                print(f"  {args.baseline:<7} {base:>9,} bytes")
                print(f"  delta   {delta:>+9,} bytes ({pct:+.2f}%)")
            if args.functions:
                print("  per function:")
                for size, name in functions(obj)[:12]:
                    print(f"    {size:>7,}  {name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
