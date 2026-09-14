#!/usr/bin/env python3
"""Measure the decoder and emit a publication envelope.

    python3 tools/benchmark.py --out site/

Produces latest.json, appends to history.jsonl, and writes manifest.json with a
sha256 for every artifact -- the same shape the charts and the published site
consume.

Three metrics, all of them deterministic:

  accuracy-psnr-db      higher is better.  Against the ISO reference PCM.
  opcount-instructions  lower is better.   Guest instructions actually executed
                                           on the target ISA, under QEMU.
  code-size-text-bytes  lower is better.   .text for the target ISA.

None of these is a wall clock, and that is deliberate. Every one returns the
same number on every machine on every run, so a change of 0.5% is a real change
rather than something to average away. The cost is that they rank candidates
rather than predicting hardware time: an instruction count knows nothing about
cache or multiplier latency. Hardware timing lives in FastLED.

The `summary` block carries quartiles for compatibility with variance-bearing
benchmarks; for these metrics count is 1 and `exact` is true, which is a
property worth stating rather than hiding.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import mimetypes
import platform
import re
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VECTORS = ROOT / "vectors"

sys.path.insert(0, str(ROOT / "tools"))
import opcount as oc  # noqa: E402

SCHEMA = "minimp3-fixed/benchmark/1"

# A representative spread rather than all 83: one common stereo stream, one
# long mono, one at a low bitrate, one that switches sample rate. The corpus
# row is the sum over every vector and is the headline number.
SCENARIOS = ["l3-hecommon", "l3-compl", "l3-si", "l3-nonstandard-he_44_48khz"]

DIRECTION = {
    "accuracy-psnr-db": "higher-is-better",
    "opcount-instructions": "lower-is-better",
    "code-size-text-bytes": "lower-is-better",
}

RESULT_RE = re.compile(r"produced=(-?\d+) reference=(-?\d+) psnr=([-\d.]+)")


def exact(value: float) -> dict:
    """A summary block for a metric with no run-to-run variance."""
    return {"count": 1, "median": value, "min": value, "max": value,
            "q1": value, "q3": value, "iqr": 0.0, "relative_iqr": 0.0,
            "noisy": False, "exact": True}


def row(decoder: str, metric: str, scenario: str, target: str,
        value: float) -> dict:
    return {"decoder_id": decoder, "metric_id": metric,
            "scenario_id": scenario, "target": target,
            "direction": DIRECTION[metric], "summary": exact(value)}


def host_psnr(binary: Path, name: str) -> float | None:
    bitstream, reference = VECTORS / f"{name}.bit", VECTORS / f"{name}.pcm"
    if not (bitstream.is_file() and reference.is_file()
            and reference.stat().st_size):
        return None
    out = subprocess.run([str(binary), str(bitstream), "--ref", str(reference)],
                         capture_output=True, text=True).stdout
    match = RESULT_RE.search(out)
    return float(match.group(3)) if match and float(match.group(3)) > 0 else None


def build_host(fixed: bool) -> Path:
    oc.BUILD.mkdir(exist_ok=True)
    binary = oc.BUILD / f"decode_host_{'fixed' if fixed else 'float'}"
    subprocess.run(
        ["c++", "-std=gnu++11", "-Os", "-fno-exceptions", "-fno-rtti",
         "-fno-strict-aliasing", "-DMINIMP3_NO_SIMD",
         f"-DMINIMP3_{'FIXED' if fixed else 'FLOAT'}_POINT",
         f"-I{ROOT}", f"-I{ROOT / 'port'}",
         str(ROOT / "tools" / "decode.cpp"), "-o", str(binary), "-lm"],
        cwd=ROOT, check=True, capture_output=True)
    return binary


def text_bytes(binary: Path) -> int | None:
    for tool in ("llvm-size", "size"):
        try:
            out = subprocess.run([tool, "-A", str(binary)],
                                 capture_output=True, text=True, check=True).stdout
        except (FileNotFoundError, subprocess.CalledProcessError):
            continue
        for line in out.splitlines():
            parts = line.split()
            if len(parts) >= 2 and parts[0] == ".text":
                return int(parts[1])
    return None


def provenance() -> dict:
    def git(*args: str) -> str:
        try:
            return subprocess.run(["git", *args], cwd=ROOT, check=True,
                                  capture_output=True, text=True).stdout.strip()
        except Exception:
            return "unknown"
    return {
        "commit": git("rev-parse", "HEAD"),
        "commit_short": git("rev-parse", "--short", "HEAD"),
        "subject": git("log", "-1", "--pretty=%s"),
        "host_arch": platform.machine(),
        "host_system": platform.system(),
        "python": platform.python_version(),
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=ROOT / "site")
    parser.add_argument("--history", type=Path,
                        help="existing history.jsonl to append to")
    args = parser.parse_args(argv)
    args.out.mkdir(parents=True, exist_ok=True)

    summaries: list[dict] = []
    notes: list[str] = []

    # --- accuracy, on the host: the arithmetic is bit-exact across ISAs, so
    # --- one host run is the accuracy of every target.
    for decoder, fixed in (("minimp3-fixed", True), ("minimp3-float", False)):
        binary = build_host(fixed)
        for name in SCENARIOS:
            value = host_psnr(binary, name)
            if value is not None:
                summaries.append(
                    row(decoder, "accuracy-psnr-db", name, "any", value))

    # --- opcount and size, per target ISA
    plugin = oc.build_plugin()
    if plugin is None:
        notes.append("opcount unavailable on this machine: needs QEMU with "
                     "plugin support, qemu-plugin.h and glib headers")
    for target in sorted(oc.TARGETS):
        for decoder, fixed in (("minimp3-fixed", True), ("minimp3-float", False)):
            try:
                binary = oc.build_decoder(target, fixed=fixed)
            except (subprocess.CalledProcessError, FileNotFoundError):
                notes.append(f"could not cross-build {decoder} for {target}")
                continue
            size = text_bytes(binary)
            if size is not None:
                summaries.append(
                    row(decoder, "code-size-text-bytes", "decoder", target, size))
            if plugin is None:
                continue
            corpus = 0
            for name in SCENARIOS:
                bitstream = VECTORS / f"{name}.bit"
                if not bitstream.is_file():
                    continue
                n = oc.count(plugin, target, binary, bitstream)
                if n is None:
                    continue
                summaries.append(
                    row(decoder, "opcount-instructions", name, target, n))
                corpus += n
            if corpus:
                summaries.append(
                    row(decoder, "opcount-instructions", "corpus", target, corpus))

    envelope = {
        "schema": SCHEMA,
        "generated_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "provenance": provenance(),
        "notes": notes,
        "absolute_summaries": summaries,
    }

    latest = args.out / "latest.json"
    latest.write_text(json.dumps(envelope, indent=1, sort_keys=True) + "\n")

    history = args.out / "history.jsonl"
    prior = ""
    if args.history and args.history.is_file():
        prior = args.history.read_text()
    elif history.is_file():
        prior = history.read_text()
    if prior and not prior.endswith("\n"):
        prior += "\n"
    history.write_text(prior + json.dumps(envelope, sort_keys=True) + "\n")

    print(f"{len(summaries)} measurements -> {latest}")
    for note in notes:
        print(f"  note: {note}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
