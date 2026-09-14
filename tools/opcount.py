#!/usr/bin/env python3
"""Count guest instructions executed by a decode, via a QEMU TCG plugin.

    python3 tools/opcount.py --target riscv32 vectors/l3-hecommon.bit

This is the repository's cross-target performance metric, and the one nothing
else here provides. Static .text bounds a win but cannot rank changes. The
host's Callgrind counts x86-64 instructions for a decoder that ships to 32-bit
targets, and on this decoder it has been wrong about both the sign and the
magnitude. Wall clock under qemu-user measures the JIT.

An executed instruction count is exact, reproducible bit for bit, and free of
timing noise: two runs of the same binary on the same input return the same
number on any machine. That is a real advantage over a throughput benchmark,
and it is why this repository can gate on it rather than merely report it.

It is not a cycle count. It does not know the target's pipeline, cache or
multiplier latency, so it ranks candidates rather than predicting hardware
time. Real silicon remains the timing authority (FastLED's `bash mp3measure`).
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / ".build"
PLUGIN_SRC = ROOT / "tools" / "qemu-plugin" / "opcount.c"

TARGETS = {
    "riscv32": ("riscv32-linux-musl", "qemu-riscv32"),
    "arm": ("arm-linux-musleabihf", "qemu-arm"),
}

OPCOUNT_RE = re.compile(r"OPCOUNT (\d+)")


def qemu_include_dir() -> Path | None:
    """Where qemu-plugin.h lives. It ships inside the QEMU package rather than
    a separate -dev, so it is found relative to the binary on the PATH.
    MINIMP3_QEMU_INCLUDE overrides, which is how CI points at a header fetched
    to match the distribution's QEMU -- Debian and Ubuntu package the emulator
    without it."""
    override = os.environ.get("MINIMP3_QEMU_INCLUDE")
    if override and (Path(override) / "qemu-plugin.h").is_file():
        return Path(override)
    binary = shutil.which("qemu-riscv32") or shutil.which("qemu-arm")
    if not binary:
        return None
    for candidate in (Path(binary).resolve().parent.parent / "include",
                      Path("/usr/include")):
        if (candidate / "qemu-plugin.h").is_file():
            return candidate
    return None


def build_plugin() -> Path | None:
    """Compile the counter. Returns None when the toolchain cannot -- callers
    then report the metric as unavailable rather than reporting a wrong one."""
    BUILD.mkdir(exist_ok=True)
    plugin = BUILD / "libopcount.so"
    include = qemu_include_dir()
    if include is None:
        return None
    cflags: list[str] = []
    if shutil.which("pkg-config"):
        probe = subprocess.run(["pkg-config", "--cflags", "glib-2.0"],
                               capture_output=True, text=True)
        if probe.returncode == 0:
            cflags = probe.stdout.split()
    if not cflags:
        return None  # qemu-plugin.h includes glib.h; without it, no plugin.
    result = subprocess.run(
        ["cc", "-shared", "-fPIC", "-O2", f"-I{include}", *cflags,
         str(PLUGIN_SRC), "-o", str(plugin)],
        capture_output=True, text=True)
    return plugin if result.returncode == 0 else None


def build_decoder(target: str, fixed: bool = True) -> Path:
    BUILD.mkdir(exist_ok=True)
    triple, _ = TARGETS[target]
    variant = "fixed" if fixed else "float"
    binary = BUILD / f"decode_{variant}_{target}"
    subprocess.run(
        ["zig", "c++", "-target", triple, "-static", "-nostdlib++",
         "-std=gnu++11", "-Os", "-fno-exceptions", "-fno-rtti",
         "-fno-strict-aliasing", "-DMINIMP3_NO_SIMD",
         f"-DMINIMP3_{'FIXED' if fixed else 'FLOAT'}_POINT",
         f"-I{ROOT}", f"-I{ROOT / 'port'}",
         str(ROOT / "tools" / "decode.cpp"), "-o", str(binary)],
        cwd=ROOT, check=True, capture_output=True)
    return binary


def count(plugin: Path, target: str, binary: Path, bitstream: Path) -> int | None:
    _, runner = TARGETS[target]
    result = subprocess.run(
        [runner, "-plugin", str(plugin), str(binary), str(bitstream)],
        capture_output=True, text=True)
    match = OPCOUNT_RE.search(result.stderr)
    return int(match.group(1)) if match else None


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bitstreams", nargs="*", type=Path)
    parser.add_argument("--target", default="riscv32", choices=sorted(TARGETS))
    parser.add_argument("--float", action="store_true", dest="float_variant")
    args = parser.parse_args(argv)

    plugin = build_plugin()
    if plugin is None:
        print("opcount unavailable: needs QEMU with plugin support, its "
              "qemu-plugin.h, and glib development headers.", file=sys.stderr)
        return 2
    binary = build_decoder(args.target, fixed=not args.float_variant)
    streams = args.bitstreams or sorted((ROOT / "vectors").glob("*.bit"))
    total = 0
    for bitstream in streams:
        n = count(plugin, args.target, binary, bitstream)
        if n is None:
            print(f"  {bitstream.stem:40s} FAILED")
            continue
        total += n
        if len(streams) <= 12:
            print(f"  {bitstream.stem:40s} {n:>14,}")
    print(f"  {'TOTAL (' + args.target + ')':40s} {total:>14,}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
