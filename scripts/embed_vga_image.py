#!/usr/bin/env python3
"""
Convert a VGA frame source into embedded C (+ header) for tests/render_image.c.

Sources:
  - .bmp  24-bit BMP, exactly 160x120 (matches VGA resolution)
  - .txt  gaysans bracket format (9600 interleaved or 28800 planar lines)

Output layout (28800 bytes): row-major, BMP bottom row first (row 0 = screen
bottom). Each row is 80 column-bytes × RGB interleaved (R,G,B per x_byte).
Each channel byte packs two 4-bit pixels (even X in low nibble, odd X high).

Usage:
  embed_vga_image.py <source> <out.c> <out.h> <array_name>
"""
from __future__ import annotations

import re
import struct
import sys
from pathlib import Path

VGA_WIDTH = 160
VGA_HEIGHT = 120
VGA_WIDTH_BYTES = VGA_WIDTH // 2
VGA_FRAME_BYTES = VGA_WIDTH_BYTES * VGA_HEIGHT * 3

TRIPLE = re.compile(
    r"\[([0-9A-Fa-f]{1,2})\s+[0-9A-Fa-f]{1,2}\]\s*"
    r"\[([0-9A-Fa-f]{1,2})\s+[0-9A-Fa-f]{1,2}\]\s*"
    r"\[([0-9A-Fa-f]{1,2})\s+[0-9A-Fa-f]{1,2}\]"
)
SINGLE = re.compile(r"\[([0-9A-Fa-f]{1,2})\s+[0-9A-Fa-f]{1,2}\]")


def byte_from_bracket_first(value: str) -> int:
    return int(value, 16) & 0xFF


def pack_nibbles(even: int, odd: int) -> int:
    return ((odd & 0x0F) << 4) | (even & 0x0F)


def rgb888_to_nibble(channel: int) -> int:
    return (channel >> 4) & 0x0F


def parse_gaysans_interleaved(lines: list[str]) -> bytes:
    out = bytearray()
    for i, line in enumerate(lines):
        match = TRIPLE.match(line.strip())
        if not match:
            raise SystemExit(
                f"line {i + 1}: expected [hh hh] [hh hh] [hh hh], got: {line!r}"
            )
        out.extend(
            (
                byte_from_bracket_first(match.group(1)),
                byte_from_bracket_first(match.group(2)),
                byte_from_bracket_first(match.group(3)),
            )
        )
    return bytes(out)


def parse_gaysans_planar(lines: list[str]) -> bytes:
    singles: list[int] = []
    for i, line in enumerate(lines):
        match = SINGLE.match(line.strip())
        if not match:
            raise SystemExit(f"line {i + 1}: expected [hh hh], got: {line!r}")
        singles.append(byte_from_bracket_first(match.group(1)))
    count = len(singles) // 3
    if count * 3 != len(singles):
        raise SystemExit("planar file: line count not divisible by 3")
    r_plane = singles[0:count]
    g_plane = singles[count : 2 * count]
    b_plane = singles[2 * count : 3 * count]
    out = bytearray()
    for i in range(count):
        out.extend((r_plane[i], g_plane[i], b_plane[i]))
    return bytes(out)


def parse_gaysans_text(path: Path) -> bytes:
    lines = [line for line in path.read_text().splitlines() if line.strip()]
    if len(lines) == 9600:
        payload = parse_gaysans_interleaved(lines)
    elif len(lines) == 28800:
        payload = parse_gaysans_planar(lines)
    else:
        raise SystemExit(
            f"{path}: expected 9600 (interleaved RGB) or 28800 (planar) "
            f"non-empty lines, got {len(lines)}"
        )
    if len(payload) != VGA_FRAME_BYTES:
        raise SystemExit(f"{path}: internal error: payload len {len(payload)}")
    return payload


def parse_bmp(path: Path) -> bytes:
    data = path.read_bytes()
    if len(data) < 54 or data[:2] != b"BM":
        raise SystemExit(f"{path}: not a BMP file")

    # BITMAPFILEHEADER: signature, file_size, reserved1, reserved2, pixel_offset
    _sig, _file_size, _res1, _res2, offset = struct.unpack_from("<2sIHHI", data, 0)
    dib_size = struct.unpack_from("<I", data, 14)[0]
    if dib_size < 40:
        raise SystemExit(f"{path}: unsupported DIB header size {dib_size}")

    width, height_raw, planes, bpp = struct.unpack_from("<iiHH", data, 18)
    if planes != 1:
        raise SystemExit(f"{path}: expected 1 color plane, got {planes}")
    if bpp != 24:
        raise SystemExit(f"{path}: expected 24 bpp, got {bpp}")
    if width != VGA_WIDTH or abs(height_raw) != VGA_HEIGHT:
        raise SystemExit(
            f"{path}: expected {VGA_WIDTH}x{VGA_HEIGHT}, got {width}x{abs(height_raw)}"
        )

    top_down = height_raw < 0
    height = abs(height_raw)
    row_stride = ((width * 3 + 3) // 4) * 4

    # Output row 0 = top scanline (matches gaysans .txt line order).
    rows: list[list[tuple[int, int, int]]] = []
    for screen_row in range(height):
        file_row = screen_row if top_down else (height - 1 - screen_row)
        row_off = offset + file_row * row_stride
        pixels: list[tuple[int, int, int]] = []
        for x in range(width):
            base = row_off + x * 3
            blue, green, red = data[base], data[base + 1], data[base + 2]
            pixels.append((red, green, blue))
        rows.append(pixels)

    out = bytearray()
    for row_pixels in rows:
        for x_byte in range(VGA_WIDTH_BYTES):
            even = row_pixels[x_byte * 2]
            odd = row_pixels[x_byte * 2 + 1]
            out.append(
                pack_nibbles(
                    rgb888_to_nibble(even[0]),
                    rgb888_to_nibble(odd[0]),
                )
            )
            out.append(
                pack_nibbles(
                    rgb888_to_nibble(even[1]),
                    rgb888_to_nibble(odd[1]),
                )
            )
            out.append(
                pack_nibbles(
                    rgb888_to_nibble(even[2]),
                    rgb888_to_nibble(odd[2]),
                )
            )

    if len(out) != VGA_FRAME_BYTES:
        raise SystemExit(f"{path}: internal error: payload len {len(out)}")
    return bytes(out)


def load_payload(path: Path) -> bytes:
    suffix = path.suffix.lower()
    if suffix == ".bmp":
        return parse_bmp(path)
    if suffix == ".txt":
        return parse_gaysans_text(path)
    raise SystemExit(f"{path}: unsupported source type {suffix!r} (use .bmp or .txt)")


def write_outputs(
    src: Path,
    dst_c: Path,
    dst_h: Path,
    array_name: str,
    payload: bytes,
    generator: str,
) -> None:
    if len(payload) != VGA_FRAME_BYTES:
        raise SystemExit(f"internal error: payload len {len(payload)}")

    chunks = [
        ", ".join(f"0x{b:02x}" for b in payload[i : i + 16])
        for i in range(0, len(payload), 16)
    ]
    body = ",\n    ".join(chunks)
    guard = f"{array_name.upper()}_H"

    dst_h.write_text(
        f"""/* Generated by {generator} from {src.name} — do not edit. */
#ifndef {guard}
#define {guard}

#include <stdint.h>

#define VGA_IMAGE_RGB_BYTES {VGA_FRAME_BYTES}u

extern const uint8_t {array_name}[VGA_IMAGE_RGB_BYTES];

#define VGA_IMAGE_ARRAY {array_name}

#endif
"""
    )

    dst_c.write_text(
        f"""/* Generated by {generator} from {src.name} — do not edit. */
#include "{dst_h.name}"

const uint8_t {array_name}[VGA_IMAGE_RGB_BYTES] = {{
    {body}
}};
"""
    )


def main() -> None:
    if len(sys.argv) != 5:
        print(
            "usage: embed_vga_image.py <source.bmp|source.txt> <out.c> <out.h> <array_name>",
            file=sys.stderr,
        )
        raise SystemExit(2)

    src = Path(sys.argv[1])
    dst_c = Path(sys.argv[2])
    dst_h = Path(sys.argv[3])
    array_name = sys.argv[4]

    if not array_name.isidentifier():
        raise SystemExit(f"array_name must be a C identifier, got {array_name!r}")

    payload = load_payload(src)
    write_outputs(src, dst_c, dst_h, array_name, payload, "scripts/embed_vga_image.py")
    print(
        f"wrote {dst_c} and {dst_h} ({len(payload)} bytes, symbol {array_name})",
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
