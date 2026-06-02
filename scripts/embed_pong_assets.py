#!/usr/bin/env python3
"""Embed 8-bit indexed pong BMP sprites into generated pong_assets.{h,c}."""
from __future__ import annotations

import struct
import sys
from pathlib import Path


def read_indexed_bmp(path: Path) -> tuple[int, int, list[tuple[int, int, int]], bytes]:
    data = path.read_bytes()
    if data[:2] != b"BM":
        raise SystemExit(f"{path}: not a BMP")
    offset = struct.unpack_from("<I", data, 10)[0]
    w = struct.unpack_from("<i", data, 18)[0]
    h = struct.unpack_from("<i", data, 22)[0]
    bpp = struct.unpack_from("<H", data, 28)[0]
    if bpp != 8:
        raise SystemExit(f"{path}: expected 8 bpp, got {bpp}")
    width = abs(w)
    height = abs(h)
    palette = []
    for i in range(256):
        b, g, r, _ = struct.unpack_from("<BBBB", data, 54 + i * 4)
        palette.append((r, g, b))
    row_size = ((width + 3) // 4) * 4
    pixels = bytearray(width * height)
    for row in range(height):
        src_y = height - 1 - row if h > 0 else row
        src_off = offset + src_y * row_size
        dst_off = row * width
        pixels[dst_off : dst_off + width] = data[src_off : src_off + width]
    return width, height, palette, bytes(pixels)


def emit_array(name: str, values: bytes | list[int]) -> list[str]:
    if isinstance(values, bytes):
        values = list(values)
    lines = [f"const uint8_t {name}[] = {{"]
    row: list[str] = []
    for i, v in enumerate(values):
        row.append(str(v))
        if len(row) == 16:
            lines.append("    " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("    " + ", ".join(row) + ",")
    lines.append("};")
    return lines


def main() -> None:
    if len(sys.argv) > 2:
        print("usage: embed_pong_assets.py [out_dir]", file=sys.stderr)
        raise SystemExit(2)

    root = Path(__file__).resolve().parents[1]
    sprite_dir = root / "pong" / "include" / "sprites"
    out_dir = Path(sys.argv[1]) if len(sys.argv) == 2 else root / "build" / "pong" / "generated"
    out_dir.mkdir(parents=True, exist_ok=True)
    out_c = out_dir / "pong_assets.c"
    out_h = out_dir / "pong_assets.h"

    bg_w, bg_h, palette, bg_px = read_indexed_bmp(sprite_dir / "background.bmp")
    pad_w, pad_h, _, pad_px = read_indexed_bmp(sprite_dir / "paddle.bmp")
    ball_w, ball_h, _, ball_px = read_indexed_bmp(sprite_dir / "ball.bmp")

    if bg_w != 160 or bg_h != 120:
        raise SystemExit(f"background must be 160x120, got {bg_w}x{bg_h}")

    rgb_flat: list[int] = []
    for r, g, b in palette:
        rgb_flat.extend((r, g, b))

    h_lines = [
        "#ifndef PONG_ASSETS_H",
        "#define PONG_ASSETS_H",
        "",
        "#include <stdint.h>",
        "",
        "#define PONG_TRANSPARENT_INDEX 166u",
        "",
        f"#define PONG_BG_WIDTH  {bg_w}u",
        f"#define PONG_BG_HEIGHT {bg_h}u",
        f"#define PONG_PADDLE_WIDTH  {pad_w}u",
        f"#define PONG_PADDLE_HEIGHT {pad_h}u",
        f"#define PONG_BALL_WIDTH  {ball_w}u",
        f"#define PONG_BALL_HEIGHT {ball_h}u",
        "",
        "extern const uint8_t pong_palette_rgb888[768];",
        "extern const uint8_t pong_background[];",
        "extern const uint8_t pong_paddle[];",
        "extern const uint8_t pong_ball[];",
        "",
        "#endif",
    ]

    c_lines = [
        '#include "pong_assets.h"',
        "",
        *emit_array("pong_palette_rgb888", rgb_flat),
        "",
        *emit_array("pong_background", bg_px),
        "",
        *emit_array("pong_paddle", pad_px),
        "",
        *emit_array("pong_ball", ball_px),
        "",
    ]

    out_h.write_text("\n".join(h_lines) + "\n")
    out_c.write_text("\n".join(c_lines) + "\n")
    print(f"Wrote {out_h} and {out_c}")


if __name__ == "__main__":
    main()
