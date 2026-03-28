# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Spellbook** is a test and helper-file repository for bringing up a RISC-V RV32I CPU (the "Wizard Core") on FPGA or in simulation. It is part of a larger project called **Capstone**. It provides bare-metal C test programs, a bootloader, linker script, and a memory logger utility.

**AI policy**: Spellbook code may be AI-generated. The Wizard Core RTL (in Capstone) is explicitly NOT AI-generated to preserve authorship clarity.

## Build Commands

The Makefile auto-detects the RISC-V toolchain (`riscv64-unknown-elf-gcc`, `riscv32-unknown-elf-gcc`, or Homebrew variants).

```bash
make                          # Build default (test_rv32i): .elf, .bin, .dump, .map, .mem
make PROGRAM=test_vga         # Build a different test program
make PROGRAM=test_mem_hammer
make PROGRAM=test_isa_vga
make verify-instructions      # Check RV32I opcode coverage in .dump
make asm                      # View disassembly in pager
make size                     # Show binary size
make config                   # Show current toolchain/arch config
make clean
```

ISA extensions can be appended: `make ISA_EXTENSIONS=m` (adds `rv32im`).

## Architecture

### Memory Map

- `0x00000000`: Execution start (unified 32 KB RAM, 8192 × 32-bit words)
- `0x1000_0000–0x1000_774F`: VGA red channel
- `0x1001_0000–0x1001_774F`: VGA green channel
- `0x1002_0000–0x1002_774F`: VGA blue channel
- `0x1003_0000`: Frame buffer swap trigger (write any value)

Stack: 1 KB at top of RAM, grows downward. Heap: remainder below stack.

### VGA Frame Buffer

- Resolution: 160×120, RGB444 (4 bits/channel), double-buffered
- 2 pixels packed per byte: low nibble = even X pixel, high nibble = odd X pixel
- Color planes are non-contiguous (separate address ranges per channel)

### Loading Binaries into Capstone

After building, copy the `.mem` file into the Capstone scripts directory and simulate:

```bash
make PROGRAM=test_isa_vga
cp test_isa_vga.mem ../Capstone/scripts/
cd ../Capstone && make simview
```

Capstone loads the binary at init time via `$readmemh({"../scripts/", INIT_FILENAME}, mem_array)` in `src/4_MEM/mem_memory.sv`. The default `INIT_FILENAME` is `test_isa_vga.mem`. The path is resolved relative to the `output/` directory where `make sim` runs.

The `.bin` file (raw binary) is an older mechanism that is no longer the primary loading path — use `.mem` (Verilog hex) for Capstone.

### Test Programs

| Program | Purpose |
|---|---|
| `test_rv32i.c` | Full RV32I ISA coverage (loads, stores, arithmetic, shifts, branches, jumps, recursion) |
| `test_vga.c` | VGA frame-buffer memory-mapped interface |
| `test_mem_hammer.c` | Memory stress/integrity test (MARCH-like patterns, randomized hammering) |
| `test_isa_vga.c` | Combined regression: ISA tests first, then VGA animation; solid red on ISA failure |
| `small.c` | Minimal Fibonacci sanity check |

All tests use `test_passed`/`test_failed`/`test_result` globals for pass/fail tracking.

### Boot Sequence

`boot.S` → loads `_stack_end` into `sp`, then jumps to `main()` via `jalr`. `link.ld` provides the memory layout and exports symbols (`__ram_base`, `__ram_size_words`, `_stack_end`, etc.).

### mem_memlog.sv

Non-synthesizable SystemVerilog module that logs memory transactions to a CSV (`mem.log`). Wire it into a testbench alongside the RAM module for transaction tracing.

### Inline Assembly Usage

Tests use inline `asm volatile(...)` blocks to prevent the compiler from optimizing away instruction sequences under test. This is intentional — do not "simplify" these into plain C expressions.

---

## Capstone (Wizard Core) — CPU Being Tested

The CPU under test lives at `/Users/donovan/Code/Verilog/Capstone`. Key details relevant to writing correct test programs:

### Pipeline Behavior

Capstone is a **5-stage single-flow CPU** (not a true pipeline). The `top_en.sv` FSM advances one stage per cycle in strict sequence: IF → ID → EX → MEM → WB. This means **5 cycles per instruction**. There is no instruction-level parallelism, no forwarding, and no hazard detection wired up (stall signals are declared but tied to 0).

### Memory Interface Quirks

- **Word index from address:** `mem_array[addr[14:2]]` — bits [1:0] are the byte lane selector, not part of the word index. This matches the spellbook linker layout.
- **Sub-word loads/stores** are handled in `mem_memory.sv` by masking based on `addr[1:0]` (byte) or `addr[1]` (halfword). Sign extension is controlled by the `sign` control bit (`LB`/`LH` are signed; `LBU`/`LHU` are unsigned).
- **Dual-port access:** IF and MEM stages both read the same memory array each cycle with no stall for contention. In practice this is safe because the FSM only activates one stage at a time.

### VGA Routing

Address bits `[29:28] == 2'b01` routes a store to the VGA subsystem instead of main memory (handled in `ex_top.sv`). Main memory access is suppressed for VGA-addressed writes. The frame buffer swap at `0x1003_0000` toggles `buffer_select` in `vga_memory.sv`.

### Clock Domain

Both system clock and VGA clock are 50 MHz in simulation (`testBench.sv`). The spellbook VGA timing calibration constant (`DELAY_LOOP_CPI_EST=4`, `5 MHz` effective CPU rate) does not reflect the simulation clock — it reflects the FPGA target clock. Adjust delay loops if running at a different effective speed.

### Simulation

```bash
cd /Users/donovan/Code/Verilog/Capstone
make simview   # build + simulate 600 ms + open GTKWave
make sim       # build + simulate only
make view      # open existing dump.vcd in GTKWave
```

VCD output: `output/dump.vcd`. Simulation duration: hardcoded 600 ms in `testBench.sv`.

### Known Limitations / Gotchas

- **No forwarding:** RAW hazards are possible in principle, but the single-flow FSM prevents true hazards since only one stage is active per cycle.
- **Branch resolution is late:** Branch/jump target is computed in EX, but PC is only corrected when the decision is used in the next IF stage cycle. The FSM serialization masks this issue currently.
- **Hardcoded init path in `mem_memory.sv`:** Contains a Windows fallback path (`C:/Users/Donov/...`) for non-simulation synthesis. Only the `../scripts/` relative path matters for `make sim`.
- **VGA test timing:** `test_vga.c` produces its first frame swap at ~348 ms simulated time. Use at least 900 ms simulation window to see both frames — the default 600 ms window may cut off frame B.
