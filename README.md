# spellbook

RV32I test programs and helper code for bringing up the **Wizard Core** RISC-V CPU (FPGA / simulation).

**Documentation:** [docs/README.md](docs/README.md) — VGA memory map, driver API, interface properties, integration plans.

## Layout

| Path | Purpose |
|------|---------|
| `tests/` | Bare-metal programs (`make PROGRAM=<name>` builds `tests/<name>.c`) |
| `drivers/` | MMIO helpers plus VGA, text, GPIO, and delay drivers (`-Idrivers` in Makefile) |
| `images/` | Source bitmap/text assets used by image embedding scripts |
| `docs/` | VGA and integration documentation |
| `scripts/` | Image embed tools |
| `sim/` | Optional simulation support files |
| `build/` | Ignored build output directory |
| `boot.S`, `link.ld` | Boot stub and linker script |

## Building

### Prerequisites

RISC-V GCC toolchain: `riscv64-unknown-elf-gcc`, `objcopy`, `objdump`.

### Makefile

```bash
make                              # default: test_rv32i (rv32im)
make RV32I_ONLY=1 PROGRAM=test_isa_vga_rv32i   # rv32i Verilator bring-up
make PROGRAM=test_isa_vga                 # rv32im + M tests + blue anim
make PROGRAM=render_image         # BMP → embedded frame data
make PROGRAM=pong                 # Pong demo
make verify-instructions          # RV32I opcode coverage in dump
make clean
make help
```

`PROGRAM` is the basename of `tests/$(PROGRAM).c`. Outputs go under `build/$(PROGRAM)/`: `.elf`, `.bin`, `.mem`, `.dump`, `.map`, and object files.

Toolchain override: `make TOOLCHAIN_PREFIX=riscv64-unknown-elf-`

## Loading into simulation

```bash
make RV32I_ONLY=1 PROGRAM=test_isa_vga_rv32i
cp build/test_isa_vga_rv32i/test_isa_vga_rv32i.mem ../wizardCore/scripts/
cd ../wizardCore && make simview
```

Capstone loads the hex file via `$readmemh` in `mem_memory.sv` (default `INIT_FILENAME = test_isa_vga.mem`).

## Test programs

| Program | Purpose |
|---------|---------|
| `test_rv32i` | Full RV32I ISA coverage |
| `test_rv32m` | RV32M multiply/divide smoke tests |
| `test_isa_vga_rv32i` | RV32I + static VGA gradient (Verilator bring-up) |
| `test_isa_vga` | RV32I+M + VGA gradient with 1 s blue ramp |
| `test_special_regs` | GPIO and timing counter special-register tests |
| `test_mem_hammer` | RAM integrity patterns |
| `render_image` | Blit embedded 160×120 BMP |
| `pong` | Pong demo using indexed sprites |

## Memory map (on-chip RAM)

- Base `0x00000000`, 32768 × 32-bit words (`0x20000` bytes / 128 KiB)
- Word index: `addr[16:2]`

VGA map (indexed FB + palette): see [docs/vga/overview.md](docs/vga/overview.md).

## Static VGA image render

```bash
make PROGRAM=render_image
make PROGRAM=render_gaysans    # images/gaysans.txt alias
```

`scripts/embed_vga_image.py` accepts 160×120 24-bit BMP or gaysans `.txt`. Generated image C/header files are written under `build/$(PROGRAM)/generated/`; `tests/render_image.c` blits via the palette driver.

## Other files

- `sim/mem_memlog.sv` — optional memory transaction logger for simulation
- `boot.S` — sets `sp`, jumps to `main`

## AI usage policy

- **Spellbook** (this repo): AI-assisted code is allowed; review before merge.
- **Wizard Core RTL** (`wizardCore/`): CPU Verilog is human-authored, not AI-generated.
