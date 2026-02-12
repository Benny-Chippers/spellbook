# spellbook

RV32I test + small helper files for bringing up a RISC-V core (e.g. on FPGA / simulation).

## RV32I test program

Basic test suite for verifying a RISC-V RV32I ISA implementation.

### Key files

- `test_rv32i.c`: main test program covering core RV32I instructions
- `test_vga.c`: VGA frame-buffer write/swap test program
- `link.ld`: linker script (adjust memory addresses for your target)
- `vga_interface_properties.md`: interface-property notes and citations
- `<program>.bin`: program image produced from the ELF (load into CPU memory via `$fread`)

### Building

#### Prerequisites

Install a RISC-V GCC toolchain that provides:

- `riscv64-unknown-elf-gcc` (or `riscv32-unknown-elf-gcc`)
- `riscv64-unknown-elf-objcopy`
- `riscv64-unknown-elf-objdump`

#### Using the `Makefile` (recommended)

```bash
# Build default program (PROGRAM=test_rv32i)
make

# Build a specific test program source file
# (builds test_vga.c -> test_vga.elf/.bin/.dump)
make PROGRAM=test_vga

# Show available targets / current config
make help
make config

# Clean build artifacts
make clean

# Inspect
make asm
make size
```

##### Selecting among multiple test scripts

The `Makefile` uses:

- `PROGRAM ?= test_rv32i`

`PROGRAM` is the source basename (`.c` omitted). Artifacts are generated with matching names:

- `$(PROGRAM).elf`
- `$(PROGRAM).bin`
- `$(PROGRAM).dump`
- `$(PROGRAM).map`

Examples:

```bash
make PROGRAM=test_rv32i
make PROGRAM=test_vga
make PROGRAM=small
```

##### Toolchain selection

The `Makefile` tries to auto-detect a working RISC-V toolchain. If that fails, set `TOOLCHAIN_PREFIX` explicitly:

```bash
make TOOLCHAIN_PREFIX=riscv64-unknown-elf-
```

##### Architecture / ABI options

You can override these at build time:

```bash
# Base RV32I (default)
make ARCH=rv32i ABI=ilp32

# Add extensions (examples)
make ARCH=rv32i ISA_EXTENSIONS=m
make ARCH=rv32i ISA_EXTENSIONS=mc
```

#### Manual build commands (no Makefile)

```bash
# Build ELF
riscv64-unknown-elf-gcc -march=rv32i -mabi=ilp32 -nostdlib -ffreestanding \
  -T link.ld -Wl,-Map,test_rv32i.map -o test_rv32i.elf test_rv32i.c

# Produce raw binary (for $fread in Verilog)
riscv64-unknown-elf-objcopy -O binary test_rv32i.elf test_rv32i.bin

# Disassembly for inspection
riscv64-unknown-elf-objdump -d test_rv32i.elf > test_rv32i.dump
```

### Test coverage

The test program exercises the full RV32I base instruction set:

| Category  | Instructions |
|-----------|--------------|
| **Loads** | LW, LB, LH, LBU, LHU |
| **Stores**| SW, SB, SH |
| **Arithmetic** | ADD, ADDI, SUB, AND, ANDI, OR, ORI, XOR, XORI |
| **Shifts** | SLL, SLLI, SRL, SRLI, SRA, SRAI |
| **Compare** | SLT, SLTI, SLTU, SLTIU |
| **Upper** | LUI, AUIPC |
| **Control** | JAL, JALR, BEQ, BNE, BLT, BGE, BLTU, BGEU |

*(ECALL, EBREAK, FENCE are system-dependent and not exercised.)*

To verify instruction coverage after building, run:
```bash
make verify-instructions
```

Note: `verify-instructions` is intended for the RV32I ISA test (`PROGRAM=test_rv32i`), not graphics/demo tests like `test_vga`.

## VGA test timing guidance

For `test_vga` simulation:

- measured first frame swap: `348.47 ms`
- second frame draw takes a similar amount of time
- recommended simulation window for defined drawing-pattern checks: **900 ms to 1 s**

### Memory map

Edit `link.ld` to match your target memory map (the default is typically set up for a simple RAM-at-0 mapping).

## Other useful files in this repo

- `mem_memlog.sv`: simple memory module with logging (handy for bring-up)
- `boot.S`: minimal start-up / boot stub (if you’re doing bare-metal)
- `small.c`: tiny test program for quick sanity checks

## AI usage policy (Spellbook / Wizard Core)

- **Project codenames**:
  - **Spellbook**: test/code-generation utilities and RISC-V test programs
  - **Wizard Core**: the CPU RTL / Verilog implementation
- **AI-assisted work (allowed in Spellbook)**:
  - AI tools may be used to help **write or refactor code in Spellbook**, including scripts and test code that generate/validate RV32I test programs.
  - When AI is used, the expectation is that outputs are reviewed, tested, and adjusted by a human before merging.
- **No-AI guarantee (Wizard Core RTL)**:
  - **AI is not used to generate the Wizard Core CPU Verilog/RTL.**
  - The CPU implementation is written and maintained by humans to preserve authorship clarity and reduce the risk of subtle functional/corner-case errors from generated RTL.
- **Scope note**:
  - This policy is about *authorship of code*. Using AI for high-level brainstorming or documentation is fine, but **generated Verilog for Wizard Core is explicitly out of scope / not accepted**.
