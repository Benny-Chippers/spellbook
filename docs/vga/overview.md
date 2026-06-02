# VGA software overview

Spellbook talks to the Wizard Core VGA block through memory-mapped stores. The CPU never writes RGB tuples directly into scan-out memory; it writes **palette indices** into a frame buffer and **RGB444 nibbles** into a shared palette.

Authoritative RTL map: `wizardCore/docs/memory_Map.md`.

Physical addresses are exported from `link.ld` and consumed in C via `drivers/mmio_map.h` (included by `vga_driver.h`). **VGA is write-only from the CPU** — stores (`SB`/`SH`/`SW`) only; loads are undefined. Use `MMIO_STORE8/16/32()`:

```c
#include "mmio_map.h"

MMIO_STORE8(__vga_fb_base, (y * 0x100u) + x, palette_index);
MMIO_STORE32(__vga_swap_addr, 0, 1u);   /* frame swap */
```

Address constants (`VGA_FB_BASE`, etc.) remain available for offset math in tests. Only program sections live in on-chip RAM (`0x00000000`, 128 KiB).

## Memory map

| Address range | CPU access | Description |
|---------------|------------|-------------|
| `0x1000_0000` – `0x1000_779F` | **Store only** | Indexed frame buffer (double-buffered in hardware) |
| `0x1000_8000` – `0x1000_80FF` | **Store only** | Palette red bytes (256 entries; **low nibble = value**, high nibble padding) |
| `0x1000_8100` – `0x1000_81FF` | **Store only** | Palette green bytes (same layout) |
| `0x1000_8200` – `0x1000_82FF` | **Store only** | Palette blue bytes (same layout) |
| `0x1003_0000` | **Store only** | Frame swap (write any value to toggle active buffer) |

Routing in RTL: `i_pxlAddr[15] == 0` → frame buffer; `i_pxlAddr[15] == 1` → palette.

## Frame buffer layout

- Resolution: **160 × 120** pixels.
- **One byte per pixel** — an 8-bit palette index (0–255).
- Row stride: **`0x100`** bytes (256-byte row page; columns 160–255 are unused).
- Pixel address: `VGA_FB_BASE + (y * 0x100) + x` with `x ∈ [0, 159]`, `y ∈ [0, 119]`.
- Last valid pixel `(159, 119)` → offset `0x779F`.

Stores `SB` / `SH` / `SW` are supported on the frame buffer (see `vga_color.sv`).

## Palette layout

256 colors × 3 channels × 4 bits, exposed to software as **768 contiguous bytes** starting at `0x1000_8000`.

### One byte per index, per channel (byte-addressable `SB`)

Each palette **channel byte** holds a single 4-bit color nibble in the **low bits**; the **high nibble is unused padding** so RV32I byte stores (`SB`) can target exactly one palette index at a time without touching neighbors:

| Byte address | Stores |
|--------------|--------|
| `VGA_PAL_RED   + index` | Red nibble for palette entry `index` (bits `[3:0]`) |
| `VGA_PAL_GREEN + index` | Green nibble for entry `index` |
| `VGA_PAL_BLUE  + index` | Blue nibble for entry `index` |

You can update red, green, or blue for a single entry independently — three separate `SB` instructions to three addresses. Scan-out reads only the low nibble of each stored byte (`vga_palette.sv` muxes `[3:0]`, `[11:8]`, … from internal 32-bit BRAM words).

Software should still mask to 4 bits when writing; the driver does this for you:

```c
*(volatile uint8_t *)(VGA_PAL_RED   + index) = r & 0x0F;  /* 0xF0 is ignored padding */
*(volatile uint8_t *)(VGA_PAL_GREEN + index) = g & 0x0F;
*(volatile uint8_t *)(VGA_PAL_BLUE  + index) = b & 0x0F;
```

Halfword/word stores (`SH`/`SW`) are also supported on palette addresses if you need them; byte stores are the natural fit for “touch one index, one channel.”

Scan-out reads index `I` from the frame buffer, looks up `{R,G,B}` in the palette, and drives RGB444 to the VGA timing block.

The palette is **shared** across both frame buffers (not duplicated per buffer).

## Drawing model

1. **Program palette entries** (once, or incrementally via `vga_palette_alloc_color`).
2. **Write indices** to the frame buffer (`vga_write_index_fast`, row fills, etc.).
3. **Swap** with `swap_frame()` when the back buffer is ready.

Solid fills (including the ISA fail screen) must set a palette entry first, then fill every pixel with that index — there is no direct “red plane” write path anymore.

### Fail screen convention

`test_isa_vga.c` uses palette index **`VGA_PAL_INDEX_FAIL_RED` (254)** for a full-screen solid red on ISA failure:

```c
vga_fill_screen_fail_red_fast();  /* writes {15,0,0} @ index 254, fills FB */
swap_frame();
```

### Gradient test (`test_isa_vga`)

After RV32I checks pass, the success path initializes a 16×16 R×G table (blue fixed at 15) and draws:

| Pixel | RGB444 |
|-------|--------|
| `(0, 0)` | `0xFFF` white |
| `(159, 119)` | `0x00F` blue |

Red ramps with **X**, green with **Y**, blue stays at full brightness — exercises the full R/G range and clean horizontal/vertical color boundaries for sync inspection.

## `vga_driver` API (`drivers/vga_driver.h`)

| Function | Purpose |
|----------|---------|
| `vga_fb_addr_fast(x, y)` | Frame-buffer byte address |
| `vga_palette_set_fast(idx, r, g, b)` | Write one palette entry |
| `vga_write_index_fast(x, y, idx)` | One pixel |
| `vga_fill_row_index_fast(y, idx, count)` | Horizontal run |
| `vga_fill_screen_index_fast(idx)` | Full frame with existing palette entry |
| `vga_fill_screen_rgb_fast(r, g, b, idx)` | Set palette entry + full frame |
| `vga_fill_screen_fail_red_fast()` | ISA fail screen helper |
| `vga_init_palette_rg_blue15_fast()` | Preload 256-entry R×G gradient table (B=15) |
| `vga_palette_alloc_color(r, g, b)` | Dynamic palette slot (images/text) |
| `swap_frame()` | Toggle double buffer |

Header-only fast paths use the `_fast` suffix; `drivers/vga_driver.c` provides alloc/fill helpers that need `.bss` state.

## Embedded images

`scripts/embed_vga_image.py` still emits **legacy packed RGB444** row bytes (80 column-bytes × 3 channels per row). The Makefile writes generated image C/header files under `build/$(PROGRAM)/generated/`. `tests/render_image.c` unpacks nibbles, allocates palette colors, and writes per-pixel indices.

## Simulation notes

- Default Capstone load: copy `test_isa_vga.mem` to `wizardCore/scripts/`, run `make simview`.
- `test_isa_vga` swaps every ~0.5 s (calibrated for ~5 MHz effective CPU rate in the delay loop).
- For older `test_vga` timing notes, allow **≥ 900 ms** simulation window to see multiple frame swaps.
