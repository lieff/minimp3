#!/usr/bin/env python3
"""Run the MPEG audio conformance suite against the fixed-point decoder.

    python3 tools/conformance.py              # fixed point
    python3 tools/conformance.py --float      # the float build, for comparison
    python3 tools/conformance.py --qemu riscv32   # cross-checked under QEMU

Every vector shipping a reference must clear the ISO limited-accuracy floor of
60 dB. Exceptions are listed explicitly with their exact bound, never skipped
silently and never allowed by name alone -- a name-only allowlist would let a
listed vector truncate or overrun by any amount, which is the hole the length
check exists to close.

The vectors live in vectors/ and came with the fork from upstream.
"""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VECTORS = ROOT / "vectors"
BUILD = ROOT / ".build"

ISO_FLOOR_DB = 60.0

# A decoder can emit a correct prefix and then stop, and a PSNR over the shared
# prefix would not notice. So a shortfall is a failure unless it is listed here
# with its exact size. Every entry is a Layer I or Layer II vector whose
# reference is a fixed-size power-of-two buffer rather than a frame-exact dump,
# so "produced < reference" does not mean dropped audio. The numbers are
# identical on the fixed and float builds, which is the evidence that they are
# a property of the reference files and not of the fixed-point port.
KNOWN_SHORT_OUTPUT = {
    "l1-fl1": 27904, "l1-fl2": 27904, "l1-fl3": 27904, "l1-fl4": 13952,
    "l1-fl5": 27904, "l1-fl6": 27904, "l1-fl7": 17152, "l1-fl8": 27904,
    "l2-fl10": 18176, "l2-fl11": 18176, "l2-fl12": 18176, "l2-fl13": 9088,
    "l2-fl14": 28672, "l2-fl15": 28672, "l2-fl16": 18688,

}

# tools/decode.cpp strips ID3v2, ID3v1 and APE containers, so those vectors
# decode normally. The LAME/Xing VBR tag is different: it occupies a real MPEG
# frame slot rather than sitting outside the stream, so a decoder that does not
# recognise it emits it as audio and shifts the whole signal by one frame
# against the reference. Scores identically on the fixed and float builds.
# FastLED recognises the tag (its VBR handling, #4129) and scores this vector
# normally; this repository ships the codec, not the container layer.
KNOWN_TAG_FRAME_AS_AUDIO = {"l3-nonstandard-sin1k0db_lame_vbrtag"}

# Decoders legitimately run a frame or two past the reference (encoder delay
# and the final granule). Beyond that, surplus output is a defect unless listed.
STANDARD_LENGTH_ALLOWANCE = 2 * 1152 * 2

KNOWN_LONG_OUTPUT = {
    # Sample rate switches mid-stream (44.1 -> 48 kHz). The decoder follows the
    # switch; the reference captures only one rate.
    "l3-nonstandard-he_44_48khz": 172800,
}

RESULT_RE = re.compile(
    r"frames=(\d+) samples=(\d+) hz=(\d+) channels=(\d+) fnv1a=0x([0-9a-f]+) "
    r"produced=(-?\d+) reference=(-?\d+) psnr=([-\d.]+)"
)

# QEMU runs the decoder built for another ISA. It proves the arithmetic is
# bit-exact on that target without hardware -- it does NOT measure speed. Wall
# clock under qemu-user measures the JIT, not the target.
QEMU_TARGETS = {
    "riscv32": ("riscv32-linux-musl", "qemu-riscv32"),
    "arm": ("arm-linux-musleabihf", "qemu-arm"),
}


def build(float_variant: bool, qemu: str | None) -> tuple[list[str], Path]:
    BUILD.mkdir(exist_ok=True)
    name = "harness_float" if float_variant else "harness_fixed"
    if qemu:
        name += f"_{qemu}"
    binary = BUILD / name
    defines = ["-DMINIMP3_NO_SIMD"]
    defines.append("-DMINIMP3_FLOAT_POINT" if float_variant
                   else "-DMINIMP3_FIXED_POINT")
    common = ["-std=gnu++11", "-Os", "-fno-exceptions", "-fno-rtti",
              "-fno-strict-aliasing", f"-I{ROOT}", f"-I{ROOT / 'port'}",
              *defines, str(ROOT / "tools" / "decode.cpp"), "-o", str(binary)]
    if qemu:
        triple, runner = QEMU_TARGETS[qemu]
        if not shutil.which("zig"):
            raise SystemExit(
                "zig is required to cross-compile for QEMU (nix shell nixpkgs#zig)")
        subprocess.run(["zig", "c++", "-target", triple, "-static", "-nostdlib++", *common],
                       cwd=ROOT, check=True)
        return [runner, str(binary)], binary
    compiler = shutil.which("c++") or shutil.which("g++")
    if not compiler:
        raise SystemExit("no host C++ compiler found")
    subprocess.run([compiler, *common, "-lm"], cwd=ROOT, check=True)
    return [str(binary)], binary


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--float", action="store_true", dest="float_variant")
    parser.add_argument("--qemu", choices=sorted(QEMU_TARGETS))
    parser.add_argument("--checksums", action="store_true",
                        help="print each vector's PCM checksum, for comparing "
                             "one build against another")
    args = parser.parse_args(argv)

    pairs = sorted((b, b.with_suffix(".pcm")) for b in VECTORS.glob("*.bit")
                   if b.with_suffix(".pcm").is_file())
    streams = sorted(VECTORS.glob("*.bit"))
    if not pairs:
        raise SystemExit(f"no vectors found under {VECTORS}")
    print(f"{len(streams)} bitstreams, {len(pairs)} with references")

    runner, _ = build(args.float_variant, args.qemu)
    passed = failed = excepted = unreferenced = container = 0
    checksums: dict[str, str] = {}

    for bitstream, reference in pairs:
        name = bitstream.stem
        proc = subprocess.run([*runner, str(bitstream), "--ref", str(reference)],
                              capture_output=True, text=True)
        match = RESULT_RE.search(proc.stdout)
        if not match:
            print(f"  FAIL {name:38s} produced no result "
                  f"({proc.stdout.strip()[:60] or proc.stderr.strip()[:60]})")
            failed += 1
            continue
        checksums[name] = match.group(5)
        produced, expected = int(match.group(6)), int(match.group(7))
        psnr = float(match.group(8))

        # A zero-byte .pcm means the suite ships no reference for this stream.
        # They are the malformed and edge-case vectors, and they are the whole
        # reason the sanitizer pass runs over every bitstream rather than only
        # the reference-backed ones: the stream that found the polyphase
        # overflow (l3-nonstandard-big-iscf) is one of these. Decoding without
        # crashing is the whole check here; there is nothing to score against.
        if expected <= 0:
            unreferenced += 1
            continue

        shortfall = expected - produced
        if shortfall > 0 and shortfall != KNOWN_SHORT_OUTPUT.get(name):
            print(f"  FAIL {name:38s} {shortfall} samples short "
                  f"(expected exception {KNOWN_SHORT_OUTPUT.get(name, 'none')})")
            failed += 1
            continue
        surplus = produced - expected
        if surplus > STANDARD_LENGTH_ALLOWANCE and \
                surplus != KNOWN_LONG_OUTPUT.get(name):
            print(f"  FAIL {name:38s} {surplus} samples long "
                  f"(expected exception {KNOWN_LONG_OUTPUT.get(name, 'none')})")
            failed += 1
            continue
        if psnr < ISO_FLOOR_DB and name in KNOWN_TAG_FRAME_AS_AUDIO:
            container += 1
            continue
        if psnr < ISO_FLOOR_DB:
            print(f"  FAIL {name:38s} {psnr:8.2f} dB, below the "
                  f"{ISO_FLOOR_DB:.0f} dB floor")
            failed += 1
            continue
        if name in KNOWN_SHORT_OUTPUT or name in KNOWN_LONG_OUTPUT:
            excepted += 1
        passed += 1

    if args.checksums:
        print()
        for name in sorted(checksums):
            print(f"  {name:40s} 0x{checksums[name]}")

    print()
    variant = "float" if args.float_variant else "fixed"
    where = f" under qemu-{args.qemu}" if args.qemu else ""
    print(f"CONFORMANCE ({variant}{where}): {passed} passed, {failed} failed, "
          f"{excepted} with a recorded length exception, "
          f"{unreferenced} without a comparable reference, "
          f"{container} needing container support this repo does not ship, "
          f"of {len(pairs)}")
    if failed:
        print("CONFORMANCE:FAIL")
        return 1
    print("CONFORMANCE:PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
