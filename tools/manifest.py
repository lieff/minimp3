#!/usr/bin/env python3
"""Seal the site directory: every artifact with its role, size and sha256.

    python3 tools/manifest.py --site site/

The manifest is what makes a published run auditable after the fact. The charts
are regenerated on every publication and the branch is force-updated, so
without digests there would be no way to tell whether a given SVG is the one
that a given latest.json describes.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import mimetypes
from pathlib import Path

ROLES = {
    ".nojekyll": "github-pages-marker",
    "index.html": "dashboard",
    "latest.json": "publication-envelope",
    "history.jsonl": "publication-history",
    "benchmark-accuracy.svg": "accuracy-panel",
    "benchmark-history.svg": "history-chart",
}


def role_for(name: str) -> str:
    if name in ROLES:
        return ROLES[name]
    if name.startswith("benchmark-opcount-"):
        return f"opcount-panel-{name[18:-4]}"
    if name.startswith("benchmark-size-"):
        return f"size-panel-{name[15:-4]}"
    return "artifact"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--site", type=Path, required=True)
    args = parser.parse_args(argv)

    files = []
    for path in sorted(args.site.rglob("*")):
        if not path.is_file() or path.name == "manifest.json":
            continue
        data = path.read_bytes()
        rel = path.relative_to(args.site).as_posix()
        files.append({
            "path": rel,
            "role": role_for(rel),
            "size": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
            "media_type": mimetypes.guess_type(rel)[0]
                          or "application/octet-stream",
        })
    (args.site / "manifest.json").write_text(
        json.dumps({"files": files}, indent=1, sort_keys=True) + "\n")
    print(f"manifest: {len(files)} artifacts sealed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
