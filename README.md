# spellbook

RV32I test + small helper files for bringing up a RISC-V core (e.g. on FPGA / simulation).

## RV32I test program

Basic test suite for verifying a RISC-V RV32I ISA implementation.

### Key files

- `test_rv32i.c`: main test program covering core RV32I instructions
- `link.ld`: linker script (adjust memory addresses for your target)
- `test_rv32i.hex` / `test_rv32i.bin`: example images produced from the ELF (optional to commit; regenerate any time)
- `test_rv32i_code.v`: a Verilog-friendly representation of the program image (if you’re loading via `$readmemh`/ROM init)

### Building

#### Prerequisites

Install a RISC-V GCC toolchain that provides:

- `riscv64-unknown-elf-gcc` (or `riscv32-unknown-elf-gcc`)
- `riscv64-unknown-elf-objcopy`
- `riscv64-unknown-elf-objdump`

#### Using the `Makefile` (recommended)

```bash
# Build all outputs (ELF, BIN, HEX, DUMP)
make

# Show available targets / current config
make help
make config

# Clean build artifacts
make clean

# Inspect
make asm
make size
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

# Produce raw binary and Intel HEX
riscv64-unknown-elf-objcopy -O binary test_rv32i.elf test_rv32i.bin
riscv64-unknown-elf-objcopy -O ihex   test_rv32i.elf test_rv32i.hex

# Disassembly for inspection
riscv64-unknown-elf-objdump -d test_rv32i.elf > test_rv32i.dump
```

### Test coverage

The test program covers:

1. **Arithmetic operations**: ADD, SUB, AND, OR, XOR, shifts (SLL, SRL, SRA), comparisons (SLT, SLTU)
2. **Memory operations**: loads/stores (LW/SW), byte operations (LB/LBU), halfword (LH/LHU)
3. **Control flow**: branches (BEQ, BNE, BLT, BGE, BLTU, BGEU)
4. **Loops**: for/while loops
5. **Function calls**: regular and recursive functions (exercises JAL/JALR)
6. **Immediate operations**: ADDI, ANDI, ORI, XORI, SLTI, SLTIU
7. **Upper immediates**: LUI, AUIPC patterns

### Memory map

Edit `link.ld` to match your target memory map (the default is typically set up for a simple RAM-at-0 mapping).

## Other useful files in this repo

- `bin_to_verilog.py`: helper to convert binaries into Verilog-friendly formats
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
