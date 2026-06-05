#!/usr/bin/env python3
"""Embed 8-bit indexed Tetris BMP assets into generated tetris_assets.{h,c}."""
from __future__ import annotations

import struct
import sys
from pathlib import Path


TRANSPARENT_INDEX = 166


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


def first_solid_tile(width: int, height: int, pixels: bytes) -> bytes:
    if width == 4 and height == 4:
        return pixels

    for y in range(0, height - 3):
        for x in range(0, width - 3):
            tile = bytearray()
            solid = True
            for ty in range(4):
                row = pixels[(y + ty) * width + x : (y + ty) * width + x + 4]
                if TRANSPARENT_INDEX in row:
                    solid = False
                    break
                tile.extend(row)
            if solid:
                return bytes(tile)

    raise SystemExit("could not find a solid 4x4 tile")


def emit_array(name: str, values: bytes | list[int]) -> list[str]:
    if isinstance(values, bytes):
        values = list(values)

    lines = [f"const uint8_t {name}[] = {{"]
    row: list[str] = []
    for v in values:
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
        print("usage: embed_tetris_assets.py [out_dir]", file=sys.stderr)
        raise SystemExit(2)

    root = Path(__file__).resolve().parents[1]
    sprite_dir = root / "Tetris" / "include" / "sprites"
    out_dir = Path(sys.argv[1]) if len(sys.argv) == 2 else root / "build" / "tetris" / "generated"
    out_dir.mkdir(parents=True, exist_ok=True)
    out_c = out_dir / "tetris_assets.c"
    out_h = out_dir / "tetris_assets.h"

    bg_w, bg_h, palette, bg_px = read_indexed_bmp(sprite_dir / "Tetris_Background.bmp")
    if bg_w != 160 or bg_h != 120:
        raise SystemExit(f"background must be 160x120, got {bg_w}x{bg_h}")

    block_specs = [
        ("empty", None),
        ("red", "Pieces/Red_Block.bmp"),
        ("orange", "Pieces/Orange_Block.bmp"),
        ("yellow", "Pieces/Yellow_Block.bmp"),
        ("green", "Pieces/Green_Block.bmp"),
        ("teal", "Pieces/Teal_Block.bmp"),
        ("blue", "Pieces/Blue_Block.bmp"),
        ("purple", "Pieces/Purple_Block.bmp"),
    ]

    blocks: list[tuple[str, bytes]] = []
    for name, rel in block_specs:
        if rel is None:
            blocks.append((name, bytes([TRANSPARENT_INDEX] * 16)))
            continue
        w, h, _, px = read_indexed_bmp(sprite_dir / rel)
        blocks.append((name, first_solid_tile(w, h, px)))

    rgb_flat: list[int] = []
    for r, g, b in palette:
        rgb_flat.extend((r, g, b))

    h_lines = [
        "#ifndef TETRIS_ASSETS_H",
        "#define TETRIS_ASSETS_H",
        "",
        "#include <stdint.h>",
        "",
        "#define TETRIS_TRANSPARENT_INDEX 166u",
        "#define TETRIS_BG_WIDTH  160u",
        "#define TETRIS_BG_HEIGHT 120u",
        "#define TETRIS_TILE_SIZE 4u",
        "#define TETRIS_BLOCK_COUNT 8u",
        "",
        "extern const uint8_t tetris_palette_rgb888[768];",
        "extern const uint8_t tetris_background[];",
        "extern const uint8_t tetris_block_empty[16];",
        "extern const uint8_t tetris_block_red[16];",
        "extern const uint8_t tetris_block_orange[16];",
        "extern const uint8_t tetris_block_yellow[16];",
        "extern const uint8_t tetris_block_green[16];",
        "extern const uint8_t tetris_block_teal[16];",
        "extern const uint8_t tetris_block_blue[16];",
        "extern const uint8_t tetris_block_purple[16];",
        "extern const uint8_t *const tetris_block_pixels[TETRIS_BLOCK_COUNT];",
        "",
        "#endif",
    ]

    c_lines = [
        '#include "tetris_assets.h"',
        "",
        *emit_array("tetris_palette_rgb888", rgb_flat),
        "",
        *emit_array("tetris_background", bg_px),
        "",
    ]
    for name, block in blocks:
        c_lines.extend(emit_array(f"tetris_block_{name}", block))
        c_lines.append("")
    c_lines.extend(
        [
            "const uint8_t *const tetris_block_pixels[TETRIS_BLOCK_COUNT] = {",
            "    tetris_block_empty,",
            "    tetris_block_red,",
            "    tetris_block_orange,",
            "    tetris_block_yellow,",
            "    tetris_block_green,",
            "    tetris_block_teal,",
            "    tetris_block_blue,",
            "    tetris_block_purple,",
            "};",
            "",
        ]
    )

    out_h.write_text("\n".join(h_lines) + "\n")
    out_c.write_text("\n".join(c_lines) + "\n")
    print(f"Wrote {out_h} and {out_c}")


if __name__ == "__main__":
    main()
