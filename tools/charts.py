#!/usr/bin/env python3
"""Render the benchmark envelope as SVG charts for the README and the site.

    python3 tools/charts.py --site site/

The SVGs are written by hand rather than by a plotting library, for two
reasons that both matter here. CI needs no dependency at all -- the whole
publication path is the standard library. And a hand-written SVG can carry its
own `prefers-color-scheme` stylesheet, so the same file is legible on GitHub in
light and dark mode; a rasterised or library-generated chart bakes in one
background and looks broken in the other.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

W, H = 760, 300
PAD_L, PAD_R, PAD_T, PAD_B = 210, 80, 46, 34

# Deliberately colour-blind safe and distinguishable in greyscale: the fixed
# series is always the darker of the pair.
SERIES = {
    "minimp3-fixed": ("#1f4e79", "fixed point"),
    "minimp3-float": ("#7fb3d5", "floating point"),
    "helix": ("#c0504d", "Helix reference"),
}

STYLE = """
  <style>
    .bg   { fill: #ffffff; }
    .ax   { stroke: #c8c8c8; stroke-width: 1; }
    .grid { stroke: #ececec; stroke-width: 1; }
    text  { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI",
            Helvetica, Arial, sans-serif; fill: #24292f; }
    .title  { font-size: 15px; font-weight: 600; }
    .sub    { font-size: 11px; fill: #57606a; }
    .lbl    { font-size: 11.5px; }
    .val    { font-size: 11px; font-weight: 600; }
    .ref    { stroke: #d1242f; stroke-width: 1.25; stroke-dasharray: 4 3; }
    .reftxt { font-size: 10px; fill: #d1242f; }
    @media (prefers-color-scheme: dark) {
      .bg   { fill: #0d1117; }
      .ax   { stroke: #30363d; }
      .grid { stroke: #21262d; }
      text  { fill: #e6edf3; }
      .sub  { fill: #8b949e; }
      .ref    { stroke: #ff7b72; }
      .reftxt { fill: #ff7b72; }
    }
  </style>
"""


def esc(text: str) -> str:
    return (text.replace("&", "&amp;").replace("<", "&lt;")
            .replace(">", "&gt;"))


def pick(rows: list[dict], **where) -> list[dict]:
    return [r for r in rows
            if all(r.get(k) == v for k, v in where.items())]


def value(row: dict) -> float:
    return float(row["summary"]["median"])


def bar_chart(path: Path, title: str, subtitle: str, groups: list[str],
              series: dict[str, list[float | None]], unit: str,
              refs: list[tuple[float, str]] | None = None,
              fmt: str = "{:,.0f}") -> None:
    """Horizontal grouped bars. Horizontal because the group labels are vector
    names, which do not fit under vertical bars without rotating them."""
    rows = len(groups)
    names = [n for n in series if any(v is not None for v in series[n])]
    per = max(1, len(names))
    plot_h = H - PAD_T - PAD_B
    band = plot_h / max(rows, 1)
    bar_h = min(17.0, (band - 8) / per)

    finite = [v for vals in series.values() for v in vals if v is not None]
    top = max(finite + [r[0] for r in (refs or [])]) if finite else 1.0
    top *= 1.18
    span = W - PAD_L - PAD_R

    def x(v: float) -> float:
        return PAD_L + (v / top) * span

    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
           f'viewBox="0 0 {W} {H}" role="img" aria-label="{esc(title)}">',
           STYLE,
           f'<rect class="bg" width="{W}" height="{H}"/>',
           f'<text class="title" x="16" y="24">{esc(title)}</text>',
           f'<text class="sub" x="16" y="40">{esc(subtitle)}</text>']

    for frac in (0.25, 0.5, 0.75, 1.0):
        gx = PAD_L + frac * span
        out.append(f'<line class="grid" x1="{gx:.1f}" y1="{PAD_T}" '
                   f'x2="{gx:.1f}" y2="{PAD_T + plot_h:.1f}"/>')
    out.append(f'<line class="ax" x1="{PAD_L}" y1="{PAD_T}" x2="{PAD_L}" '
               f'y2="{PAD_T + plot_h:.1f}"/>')

    for level, label in (refs or []):
        rx = x(level)
        out.append(f'<line class="ref" x1="{rx:.1f}" y1="{PAD_T - 6}" '
                   f'x2="{rx:.1f}" y2="{PAD_T + plot_h:.1f}"/>')
        out.append(f'<text class="reftxt" x="{rx + 4:.1f}" y="{PAD_T - 9}">'
                   f'{esc(label)}</text>')

    for i, group in enumerate(groups):
        base = PAD_T + i * band
        out.append(f'<text class="lbl" x="{PAD_L - 8}" '
                   f'y="{base + band / 2 + 4:.1f}" text-anchor="end">'
                   f'{esc(group)}</text>')
        for j, name in enumerate(names):
            v = series[name][i]
            if v is None:
                continue
            colour = SERIES.get(name, ("#888888", name))[0]
            y = base + (band - bar_h * per) / 2 + j * bar_h
            bw = max(1.0, x(v) - PAD_L)
            out.append(f'<rect x="{PAD_L}" y="{y:.1f}" width="{bw:.1f}" '
                       f'height="{bar_h - 2.5:.1f}" fill="{colour}" rx="1.5">'
                       f'<title>{esc(name)}: {fmt.format(v)} {esc(unit)}</title>'
                       f'</rect>')
            out.append(f'<text class="val" x="{PAD_L + bw + 5:.1f}" '
                       f'y="{y + bar_h / 2 + 1.5:.1f}">{fmt.format(v)}</text>')

    lx = 16
    ly = H - 10
    for name in names:
        colour, label = SERIES.get(name, ("#888888", name))
        out.append(f'<rect x="{lx}" y="{ly - 9}" width="10" height="10" '
                   f'fill="{colour}" rx="1.5"/>')
        out.append(f'<text class="sub" x="{lx + 15}" y="{ly}">{esc(label)}</text>')
        lx += 22 + 7 * len(label)
    out.append(f'<text class="sub" x="{W - 16}" y="{ly}" text-anchor="end">'
               f'{esc(unit)}</text>')
    out.append("</svg>")
    path.write_text("\n".join(out) + "\n")


def history_chart(path: Path, history: Path, target: str = "riscv32") -> None:
    points: list[tuple[str, float]] = []
    if history.is_file():
        for line in history.read_text().splitlines():
            if not line.strip():
                continue
            envelope = json.loads(line)
            rows = pick(envelope["absolute_summaries"],
                        decoder_id="minimp3-fixed",
                        metric_id="opcount-instructions",
                        scenario_id="corpus", target=target)
            if rows:
                points.append((envelope["provenance"].get("commit_short", "?"),
                               value(rows[0])))
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="240" '
           f'viewBox="0 0 {W} 240" role="img" aria-label="opcount history">',
           STYLE, f'<rect class="bg" width="{W}" height="240"/>',
           '<text class="title" x="16" y="24">Instruction count over time</text>',
           f'<text class="sub" x="16" y="40">minimp3-fixed, whole corpus, '
           f'{esc(target)} &#183; lower is better</text>']
    if len(points) < 2:
        out.append('<text class="sub" x="16" y="130">Not enough published runs '
                   'yet &#8212; this chart fills in as history accumulates.</text>')
    else:
        values = [v for _, v in points]
        lo, hi = min(values) * 0.98, max(values) * 1.02
        px = W - PAD_L - PAD_R
        ph = 240 - PAD_T - PAD_B

        def sx(i: int) -> float:
            return PAD_L + px * i / max(1, len(points) - 1)

        def sy(v: float) -> float:
            return PAD_T + ph * (1 - (v - lo) / (hi - lo or 1))

        path_d = " ".join(("M" if i == 0 else "L") +
                          f"{sx(i):.1f},{sy(v):.1f}"
                          for i, (_, v) in enumerate(points))
        out.append(f'<line class="ax" x1="{PAD_L}" y1="{PAD_T + ph:.1f}" '
                   f'x2="{PAD_L + px}" y2="{PAD_T + ph:.1f}"/>')
        out.append(f'<path d="{path_d}" fill="none" stroke="#1f4e79" '
                   f'stroke-width="2"/>')
        for i, (label, v) in enumerate(points):
            out.append(f'<circle cx="{sx(i):.1f}" cy="{sy(v):.1f}" r="3" '
                       f'fill="#1f4e79"><title>{esc(label)}: {v:,.0f}</title>'
                       f'</circle>')
        # Without a y-scale the line is unreadable: a flat series and a
        # halved one look identical when the axis auto-fits.
        for level in (lo, hi):
            out.append(f'<text class="sub" x="{PAD_L - 8}" '
                       f'y="{sy(level) + 4:.1f}" text-anchor="end">'
                       f'{level:,.0f}</text>')
            out.append(f'<line class="grid" x1="{PAD_L}" y1="{sy(level):.1f}" '
                       f'x2="{PAD_L + px}" y2="{sy(level):.1f}"/>')
        latest_label = f'{points[-1][1]:,.0f}'
        out.append(f'<text class="val" x="{sx(len(points) - 1) - 6:.1f}" '
                   f'y="{sy(points[-1][1]) - 9:.1f}" text-anchor="end">'
                   f'{latest_label}</text>')
        out.append(f'<text class="sub" x="{PAD_L}" y="{PAD_T + ph + 16:.1f}">'
                   f'{esc(points[0][0])}</text>')
        out.append(f'<text class="sub" x="{PAD_L + px}" '
                   f'y="{PAD_T + ph + 16:.1f}" text-anchor="end">'
                   f'{esc(points[-1][0])}</text>')
    out.append("</svg>")
    path.write_text("\n".join(out) + "\n")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--site", type=Path, default=ROOT / "site")
    args = parser.parse_args(argv)

    envelope = json.loads((args.site / "latest.json").read_text())
    rows = envelope["absolute_summaries"]
    decoders = ["minimp3-fixed", "minimp3-float"]

    # --- accuracy
    scenarios = sorted({r["scenario_id"] for r in rows
                        if r["metric_id"] == "accuracy-psnr-db"})
    if scenarios:
        series = {d: [next((value(r) for r in pick(
                        rows, decoder_id=d, metric_id="accuracy-psnr-db",
                        scenario_id=s)), None) for s in scenarios]
                  for d in decoders}
        bar_chart(
            args.site / "benchmark-accuracy.svg",
            "Accuracy against the ISO reference",
            "PSNR over the decoded signal · higher is better",
            scenarios, series, "dB",
            refs=[(60.0, "ISO floor 60 dB"), (102.87, "Helix 102.87 dB")],
            fmt="{:,.2f}")

    # --- opcount and size, per target
    for target in sorted({r["target"] for r in rows if r["target"] != "any"}):
        ops = sorted({r["scenario_id"] for r in rows
                      if r["metric_id"] == "opcount-instructions"
                      and r["target"] == target})
        if ops:
            series = {d: [next((value(r) for r in pick(
                            rows, decoder_id=d,
                            metric_id="opcount-instructions",
                            scenario_id=s, target=target)), None)
                          for s in ops] for d in decoders}
            bar_chart(
                args.site / f"benchmark-opcount-{target}.svg",
                f"Instructions executed on {target}",
                "Counted exactly under QEMU, zero run-to-run variance "
                "· lower is better",
                ops, series, "instructions")

        sizes = {d: [next((value(r) for r in pick(
                        rows, decoder_id=d, metric_id="code-size-text-bytes",
                        target=target)), None)] for d in decoders}
        if any(v[0] is not None for v in sizes.values()):
            bar_chart(
                args.site / f"benchmark-size-{target}.svg",
                f"Code size on {target}",
                ".text of the decoder built -Os · lower is better",
                ["decoder .text"], sizes, "bytes")

    history_chart(args.site / "benchmark-history.svg",
                  args.site / "history.jsonl")

    written = sorted(p.name for p in args.site.glob("*.svg"))
    print(f"{len(written)} charts: {', '.join(written)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
