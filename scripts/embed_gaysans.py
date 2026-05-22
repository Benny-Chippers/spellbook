#!/usr/bin/env python3
"""Backward-compatible wrapper around embed_vga_image.py for gaysans.txt sources."""
from __future__ import annotations

import sys
from pathlib import Path

# Allow `python3 scripts/embed_gaysans.py ...` without installing as a package.
sys.path.insert(0, str(Path(__file__).resolve().parent))

from embed_vga_image import load_payload, write_outputs  # noqa: E402


def main() -> None:
    if len(sys.argv) != 4:
        print(
            "usage: embed_gaysans.py <gaysans.txt> <out.c> <array_name>",
            file=sys.stderr,
        )
        print(
            "note: also writes <out.h> alongside <out.c> (same basename)",
            file=sys.stderr,
        )
        raise SystemExit(2)

    src = Path(sys.argv[1])
    dst_c = Path(sys.argv[2])
    array_name = sys.argv[3]
    dst_h = dst_c.with_suffix(".h")
    payload = load_payload(src)
    write_outputs(src, dst_c, dst_h, array_name, payload, "scripts/embed_gaysans.py")
    print(
        f"wrote {dst_c} and {dst_h} ({len(payload)} bytes, symbol {array_name})",
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
