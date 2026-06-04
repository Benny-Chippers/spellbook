# VGA interface properties and evidence

Interface contract between the Wizard Core CPU and the VGA subsystem. RTL lives in `wizardCore/src/VGA/`.

## CPU ↔ GPU (memory)

| Property | Value | Rationale | Design evidence |
|----------|------:|-----------|-----------------|
| CPU access | Stores (`SB`/`SH`/`SW`) to frame buffer and palette | CPU updates display by store-only MMIO | `vga_color` / `vga_palette` write paths gated on `memWrite`; `id_control` maps `FUNCT3` to size [3][4] |
| Frame buffer size | 2 × 19,200 B (160×120×8 index bits) | One index byte per pixel; double-buffered | Two `vga_frame` instances in `vga_memory` [6]; `vga_color` BRAM = 160×120 bytes [3] |
| Palette size | 768 B shared (256 × RGB444) | 256 colors, 4 bits per channel | `vga_palette` COLORS=256, contiguous from `0x1000_8000` [3] |
| Store granularity | 8 / 16 / 32-bit | RV32I software flexibility [7] | `mem_ctrl_t.size` + sub-word write masks in `vga_color` / `vga_palette` [3][4] |

## GPU ↔ VGA (timing / color)

| Property | Value | Rationale | Design evidence |
|----------|------:|-----------|-----------------|
| Color resolution | RGB444 (4 bits per channel) | 12-bit `vga_color_t` output | `macro.sv` channel widths [4] |
| Active image | 160 × 120 | On-chip BRAM budget | `vga_color` WIDTH/HEIGHT parameters [3] |
| Frame rate target | ~60 Hz | Standard VGA monitor expectation [5] | `vga_top` horizontal/vertical counters [6] |
| Sync counts | 800 horizontal, 521 vertical (current RTL) | Implementation choice for 640×480-class timing [5] | `vga_compCount` RESET values in `vga_top` [6] |

## Addressing notes

- **Frame buffer:** one palette index per pixel; row stride `0x100`; `addr = 0x1000_0000 + (y << 8) + x`.
- **Palette:** byte-linear from `0x1000_8000`; separate R/G/B 256-byte slices in the CPU map; RTL packs nibbles into 32-bit BRAM words for scan-out [3].
- **No RGB frame planes:** software writes palette indices to the framebuffer and RGB444 nibbles to the palette.

See [overview.md](overview.md) for software usage.

## References

[1] AMD, *Spartan-7 FPGA Family Product Selection Guide*, 2023.  
[2] D. Donovan, *Memory Mapping*, `wizardCore/docs/memory_Map.md`, 2026.  
[3] D. Donovan, `vga_color.sv` / `vga_palette.sv`, `wizardCore/src/VGA/`, 2026.  
[4] D. Donovan, `macro.sv`, `id_control.sv`, `wizardCore/src/`, 2026.  
[5] M. Hinner, [VGA timing information](https://martin.hinner.info/vga/timing.html).  
[6] D. Donovan, `vga_top.sv`, `vga_memory.sv`, `wizardCore/src/VGA/`, 2026.  
[7] RISC-V International, *The RISC-V Instruction Set Manual, Volume I: Unprivileged ISA*, 2023.
