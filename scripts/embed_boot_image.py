#!/usr/bin/env python3
"""Embed a 160x120 24-bit BMP as indexed RGB444 boot image assets."""
from __future__ import annotations

import struct
import sys
from pathlib import Path

WIDTH = 160
HEIGHT = 120
MAX_COLORS = 256


def read_bmp_rgb444(path: Path) -> tuple[list[tuple[int, int, int]], bytes]:
    data = path.read_bytes()
    if len(data) < 54 or data[:2] != b"BM":
        raise SystemExit(f"{path}: not a BMP")

    offset = struct.unpack_from("<I", data, 10)[0]
    width = struct.unpack_from("<i", data, 18)[0]
    height_raw = struct.unpack_from("<i", data, 22)[0]
    planes = struct.unpack_from("<H", data, 26)[0]
    bpp = struct.unpack_from("<H", data, 28)[0]
    if width != WIDTH or abs(height_raw) != HEIGHT or planes != 1 or bpp != 24:
        raise SystemExit(f"{path}: expected {WIDTH}x{HEIGHT} 24bpp BMP")

    height = abs(height_raw)
    top_down = height_raw < 0
    row_stride = ((width * 3 + 3) // 4) * 4
    palette: list[tuple[int, int, int]] = []
    palette_index: dict[tuple[int, int, int], int] = {}
    pixels = bytearray()

    for screen_y in range(height):
        file_y = screen_y if top_down else height - 1 - screen_y
        row_off = offset + file_y * row_stride
        for x in range(width):
            blue, green, red = data[row_off + x * 3 : row_off + x * 3 + 3]
            color = ((red >> 4) & 0x0F, (green >> 4) & 0x0F, (blue >> 4) & 0x0F)
            idx = palette_index.get(color)
            if idx is None:
                if len(palette) >= MAX_COLORS:
                    raise SystemExit(f"{path}: more than {MAX_COLORS} RGB444 colors")
                idx = len(palette)
                palette_index[color] = idx
                palette.append(color)
            pixels.append(idx)

    return palette, bytes(pixels)


def emit_array(name: str, values: list[int] | bytes) -> list[str]:
    if isinstance(values, bytes):
        values = list(values)
    lines = [f"const uint8_t {name}[] = {{"]
    for i in range(0, len(values), 16):
        lines.append("    " + ", ".join(str(v) for v in values[i : i + 16]) + ",")
    lines.append("};")
    return lines


def main() -> None:
    if len(sys.argv) != 4:
        print("usage: embed_boot_image.py <source.bmp> <out_dir> <asset_name>", file=sys.stderr)
        raise SystemExit(2)

    src = Path(sys.argv[1])
    out_dir = Path(sys.argv[2])
    asset_name = sys.argv[3]
    if not asset_name.isidentifier():
        raise SystemExit(f"asset_name must be a C identifier, got {asset_name!r}")

    out_dir.mkdir(parents=True, exist_ok=True)
    out_h = out_dir / f"{asset_name}.h"
    out_c = out_dir / f"{asset_name}.c"
    guard = f"{asset_name.upper()}_H"
    palette, pixels = read_bmp_rgb444(src)

    palette_flat: list[int] = []
    for r, g, b in palette:
        palette_flat.extend((r, g, b))

    h_lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "#include <stdint.h>",
        "",
        f"#define {asset_name.upper()}_WIDTH {WIDTH}u",
        f"#define {asset_name.upper()}_HEIGHT {HEIGHT}u",
        f"#define {asset_name.upper()}_PALETTE_COLORS {len(palette)}u",
        "",
        f"extern const uint8_t {asset_name}_palette_rgb444[];",
        f"extern const uint8_t {asset_name}_indices[];",
        "",
        "#endif",
    ]

    c_lines = [
        f'#include "{asset_name}.h"',
        "",
        *emit_array(f"{asset_name}_palette_rgb444", palette_flat),
        "",
        *emit_array(f"{asset_name}_indices", pixels),
        "",
    ]

    out_h.write_text("\n".join(h_lines) + "\n")
    out_c.write_text("\n".join(c_lines) + "\n")
    print(f"Wrote {out_h} and {out_c} ({len(palette)} colors)")


if __name__ == "__main__":
    main()
