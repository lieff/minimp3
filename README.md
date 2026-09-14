# minimp3-fixed

A fixed-point fork of [minimp3](https://github.com/lieff/minimp3), forked at
`ea99364` and rewritten to decode MPEG Layer III in 32-bit integer arithmetic
with no FPU, no dynamic allocation, and a bounded stack — while holding
accuracy far above the ISO floor.

It is the decoder that ships in [FastLED](https://github.com/FastLED/FastLED),
and this repository is where it gets optimised. FastLED vendors five files from
here verbatim; `tools/check_vendor_sync.py` proves they have not drifted.

## Where it got to

Measured on an ESP32-C6 (RISC-V RV32IMC, 160 MHz), decoding a Layer III stream,
against [Helix](helix/) — the reference this work was benchmarked against.

| | decode time | vs Helix | accuracy |
|---|---|---|---|
| at the start | 142,647 µs | 3.99× slower | 123.24 dB |
| **now** | **39,385 µs** | **1.13×** | **123.24 dB** |

**−72% decode time. The gap to a hand-tuned assembly-era reference went from 4×
to about 13%, and not one dB of accuracy was spent to get there.** The decoder
runs at 5.2× real time on a 160 MHz RISC-V core.

For scale on that accuracy figure: the ISO limited-accuracy floor is 60 dB,
Helix itself scores 102.87 dB, and a 16-bit-multiply variant that was tried and
rejected scored 44.37 dB. 123.24 dB is not a rounding of "good enough".

### How it was won

| change | device effect |
|---|---|
| force-inline the hot leaf helpers | −50% |
| drop the DCT/IMDCT butterfly saturation | −28% |
| restructure the polyphase lane loop | −12% |
| lower DCT-32's multiplies through the high half | −2.5% |
| fold the rounding into the low half | −1.1% |

The two arithmetic entries are the interesting ones. Both come from the same
observation: on a 32-bit target, `(a*b) >> 32` is a *single instruction*
(`mulh` on RISC-V, `smull` on ARM), so any rounding you can express in the high
half of the product is free. They live in
[`port/platforms/int_asm.h`](port/platforms/int_asm.h) with the identities
written out, and are pinned by a codegen test — because the idiom is only fast
while the compiler recognises it, and when recognition lapses it lapses
silently: 2 instructions became 40, with the right answer every time.

## Benchmarks

Published on every change to the decoder, from the
[`benchmark-stats`](https://github.com/zackees/minimp3-fixed/tree/benchmark-stats)
branch. **[Full dashboard →](https://zackees.github.io/minimp3-fixed/)**

### Accuracy

[![PSNR against the ISO reference for the fixed and floating point builds](https://raw.githubusercontent.com/zackees/minimp3-fixed/benchmark-stats/benchmark-accuracy.svg)](https://zackees.github.io/minimp3-fixed/#accuracy)

The fixed-point build tracks the floating-point one to within a fraction of a
dB on every vector, roughly 50 dB above the ISO floor and 20 above the Helix
reference. That is the claim "no accuracy was spent" as a picture: none of the
speed work moved these bars.

### Instructions executed

[![Instructions executed on riscv32](https://raw.githubusercontent.com/zackees/minimp3-fixed/benchmark-stats/benchmark-opcount-riscv32.svg)](https://zackees.github.io/minimp3-fixed/#opcount)

[![Instructions executed on arm](https://raw.githubusercontent.com/zackees/minimp3-fixed/benchmark-stats/benchmark-opcount-arm.svg)](https://zackees.github.io/minimp3-fixed/#opcount)

Guest instructions actually executed on the target ISA, counted by a QEMU TCG
plugin ([`tools/qemu-plugin/opcount.c`](tools/qemu-plugin/opcount.c)). This is
the metric this repository optimises against, and it is **exact**: the same
binary on the same input returns the same count on any machine, every time. A
0.5% move is a real 0.5%, not something to average over fifteen runs.

The fixed-point build executes more instructions than the float build, which is
the expected trade and the entire point — the targets it ships to have no FPU,
so the float build's instruction count is not available to them at any price.

### Code size

[![Decoder .text size on riscv32](https://raw.githubusercontent.com/zackees/minimp3-fixed/benchmark-stats/benchmark-size-riscv32.svg)](https://zackees.github.io/minimp3-fixed/#size)

### History

[![Instruction count over published runs](https://raw.githubusercontent.com/zackees/minimp3-fixed/benchmark-stats/benchmark-history.svg)](https://zackees.github.io/minimp3-fixed/#history)

Every published run is appended to
[`history.jsonl`](https://zackees.github.io/minimp3-fixed/history.jsonl); the most recent is
[`latest.json`](https://zackees.github.io/minimp3-fixed/latest.json), and
[`manifest.json`](https://zackees.github.io/minimp3-fixed/manifest.json) carries a sha256 for every artifact so
a chart can be tied back to the envelope that produced it.

### What these can and cannot tell you

They rank candidates. They do not predict hardware time: an instruction count
knows nothing about the target's cache, pipeline or multiplier latency, and
wall clock under `qemu-user` measures the JIT rather than the guest. Timing on
real silicon stays in FastLED. See the doctrine section below for the four
occasions when a host measurement got the sign or the magnitude wrong.

## Correctness

83 conformance bitstreams ship in [`vectors/`](vectors/) with the fork.

```
python3 tools/conformance.py                  # fixed point
python3 tools/conformance.py --float          # the float build, for comparison
python3 tools/conformance.py --qemu riscv32   # cross-checked, no hardware
python3 tools/conformance.py --qemu arm
```

73 pass, 0 fail, on x86-64, riscv32 and arm — and the PCM checksum is
**identical on all three**, which is what "bit-exact" means here and how a
change is shown to have altered speed and nothing else. The remaining vectors
are declared, never skipped silently: 16 carry a recorded length exception with
its exact bound, 8 ship no reference PCM at all, and 1 needs VBR-tag handling
that belongs to the container layer rather than the codec.

Exact bounds rather than a name allowlist is deliberate. A decoder can emit a
correct prefix and stop, and a PSNR taken over the shared prefix will not
notice — so every exception records how many samples, and that number is
identical on the float build, which is the evidence it is a property of the
vector and not of the fixed-point port.

## Measuring a change

```
nix shell nixpkgs#zig nixpkgs#qemu nixpkgs#llvm      # or install them yourself
python3 tools/conformance.py --qemu riscv32          # correctness, cross-ISA
python3 tools/codesize.py --baseline HEAD            # riscv32 + arm .text delta
python3 tools/check_vendor_sync.py --fastled ../fastled
```

### Doctrine: what these measurements can and cannot tell you

This is the hardest-won part of the repository and the part most likely to be
ignored.

**Static size bounds a win. It does not rank changes.** Fewer instructions in
the hot path cannot make things slower, and a change costing 20% of text for 3%
of speed is visible before anyone flashes anything. But size says nothing about
how often a block runs — this decoder is now *smaller than Helix in every stage
but one* and still measurably slower.

**Never quote wall-clock time under `qemu-user`.** It measures the JIT. QEMU
here is a correctness instrument: it proves the arithmetic is bit-exact on
another ISA without hardware. That is worth a great deal and it is not a
stopwatch.

**A 64-bit host cannot rank changes for a 32-bit target.** This is not a
caution, it is a record:

| change | host said | device said |
|---|---|---|
| `FL_NO_INLINE` on `mp3d_synth` | −4.8% | ~0% |
| polyphase lane restructure | **+3.2%, a regression** | **−9.7%** |
| widening the accumulators to int64 | 0% | **+78%** |
| folding the rounding into the low half | 0.00% | **−1.14%** |

Every one of those host numbers was correctly measured. They were measured over
the wrong scope. On x86-64 the whole 64-bit product lives in one register, so
the operations this decoder spends its life on are free there and expensive
everywhere it actually ships.

**Hardware remains the timing authority.** It lives in FastLED (`bash
mp3measure`), which flashes an ESP32-C6 and reports microseconds and the Helix
ratio. This repository deliberately does not carry that: it would drag in a
whole build and deploy stack. What it carries is everything that can be checked
without a board.

## Layout

| path | |
|---|---|
| `minimp3.h` | the decoder — replaces upstream's, so `git diff master` is the fork's whole diff |
| `minimp3_synth_fixed.h` | DCT-32, polyphase and the integer SIMD kernels: 57% of a decode, and where optimisation work goes |
| `minimp3_fixed_tables.h` | Q-format coefficient tables |
| `port/` | minimal shims standing in for FastLED's headers, so the shared files stay byte-identical |
| `helix/` | the benchmark reference — **separately licensed, read [helix/README.md](helix/README.md)** |
| `vectors/` | the conformance suite, from upstream |
| `tools/` | conformance, cross-target size, vendor sync |

The decoder is C++ (templates and force-inline attributes are load-bearing), so
upstream's C `minimp3_test.c` and `scripts/` no longer build against it. They
are kept for reference; `tools/conformance.py` is the test entry point.

## Licence

CC0, inherited from upstream minimp3 — **except `helix/`**, which is
RealNetworks RPSL/RCSL and is present only as a benchmark reference. See
[helix/README.md](helix/README.md).
