# VGA Interface Properties and Evidence

This document captures the requested interface properties for the Wizard Core CPU <-> VGA path and explains why each value is used, plus how the current design meets it.

## CPU_GPU_comm

| Interface property | Value | Why this value? | Why design should meet/exceed it |
|---|---:|---|---|
| Memory access from CPU | Stores (`SB/SH/SW`) to frame buffer | CPU must write display memory to change pixels. Memory map reserves `0x1000_0000`-`0x1002_774F` for VGA color planes and `0x1003_0000` as frame swap trigger [2]. | `vga_color` has explicit byte/half/word write logic gated by `memWrite`, and `id_control` maps S-type `FUNCT3` into size controls; this directly supports store-based updates [3], [4]. |
| Memory size | 2 x 28.8 KB frame buffers | One frame = 160 x 120 x 12 bits = 230,400 bits = 28,800 bytes. Double buffering requires 57,600 bytes total. | `vga_memory` instantiates two `vga_frame` modules (double buffer behavior) [6]. `vga_color` stores 120 x 80 bytes per color plane (9,600 bytes), and each frame has 3 color planes = 28,800 bytes [3], [6]. Available BRAM on XC7S50 is 2,700 Kb (337.5 KB), so required storage is below available on-chip BRAM [1]. |
| Store granularity | 8/16/32-bit writes | RV32I software commonly uses byte, halfword, and word stores for efficient updates and alignment flexibility [7]. | `mem_ctrl_t.size` defines byte/half/word encodings and `vga_color` handles all three cases in the write path [3], [4]. |

## GPU_VGA_sig

| Interface property | Value | Why this value? | Why design should meet/exceed it |
|---|---:|---|---|
| Color resolution | 4 bits per channel (RGB444) | Design output bus is 12-bit color, split 4 bits each for R/G/B. | `vga_out_t` and `vga_color_t` define `[3:0]` for each channel; this enforces RGB444 in RTL [4]. |
| Frame rate | 60 Hz target | 640x480 @ ~60 Hz is standard VGA timing used by most monitors [5]. | `vga_top` uses 800 horizontal counts and a vertical counter derived from the horizontal sync stream, targeting VGA-style timing generation [6]. |
| Sync cycles | 800 horizontal, 521 vertical (current implementation) | 800 horizontal aligns with VGA-style 640x480 timing. The current RTL sets vertical reset to 521 as an implementation choice. | `vga_compCount` parameters in `vga_top` are `RESET=800` (horizontal) and `RESET=521` (vertical) [6]. This is what silicon will do today. If strict 640x480@59.94 compatibility is required, compare against the common 525-line reference and adjust if needed [5]. |

## Note on non-contiguous color addressing

Color address space is intentionally non-contiguous for pixel RGB tuples:
- Red/green/blue are in separate base ranges (`0x1000_0000`, `0x1001_0000`, `0x1002_0000`) [2].
- Within each color plane, bytes index two horizontal pixels via nibbles (even X = low nibble, odd X = high nibble) [2], [3].

## References (IEEE)

[1] AMD, "Spartan-7 FPGA Family Product Selection Guide," 2023. [Online]. Available: https://www.avnet.com/wcm/connect/99ba9d49-930d-41d8-ae05-55e49cdcb962/7-series-product-selection-guide.pdf. [Accessed: Feb. 12, 2026].

[2] D. Donovan, "Memory Mapping," `Capstone/docs/memory_Map.md`, 2026.

[3] D. Donovan, "`vga_color.sv`," `Capstone/src/VGA/vga_color.sv`, 2026.

[4] D. Donovan, "`macro.sv` and `id_control.sv`," `Capstone/src/COM/macro.sv` and `Capstone/src/2_ID/id_control.sv`, 2026.

[5] M. Hinner, "VGA timing information," [Online]. Available: https://martin.hinner.info/vga/timing.html. [Accessed: Feb. 12, 2026].

[6] D. Donovan, "`vga_top.sv` and `vga_memory.sv`," `Capstone/src/VGA/vga_top.sv` and `Capstone/src/VGA/vga_memory.sv`, 2026.

[7] RISC-V International, "The RISC-V Instruction Set Manual, Volume I: Unprivileged ISA," 2023. [Online]. Available: https://riscv-specs.timhutt.co.uk/spec/riscv-isa-release-056b6ff-2023-10-02/unpriv-isa-asciidoc.html. [Accessed: Feb. 12, 2026].
