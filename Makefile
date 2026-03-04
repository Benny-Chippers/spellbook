# Makefile for RISC-V 32I Test Program
# Easily configurable for different RISC-V architecture options

# Compiler settings
# Try to auto-detect RISC-V toolchain, or set manually
# Check PATH first, then check Homebrew installation directories
TOOLCHAIN_PREFIX ?= $(shell \
	if command -v riscv64-unknown-elf-gcc >/dev/null 2>&1; then \
		echo riscv64-unknown-elf-; \
	elif command -v riscv32-unknown-elf-gcc >/dev/null 2>&1; then \
		echo riscv32-unknown-elf-; \
	elif command -v riscv64-linux-gnu-gcc >/dev/null 2>&1; then \
		echo riscv64-linux-gnu-; \
	elif command -v riscv32-linux-gnu-gcc >/dev/null 2>&1; then \
		echo riscv32-linux-gnu-; \
	elif [ -f /opt/homebrew/bin/riscv64-unknown-elf-gcc ]; then \
		echo riscv64-unknown-elf-; \
	elif [ -f /usr/local/bin/riscv64-unknown-elf-gcc ]; then \
		echo riscv64-unknown-elf-; \
	else \
		echo riscv64-unknown-elf-; \
	fi)

CC = $(TOOLCHAIN_PREFIX)gcc
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP = $(TOOLCHAIN_PREFIX)objdump
SIZE = $(TOOLCHAIN_PREFIX)size

# Architecture options (easily tweakable)
ARCH = rv32i
ABI = ilp32

# Additional ISA extensions (add as needed)
# Examples: m (multiply/divide), a (atomic), c (compressed), f (float), d (double)
ISA_EXTENSIONS = 

# Build full architecture string
ifneq ($(ISA_EXTENSIONS),)
    FULL_ARCH = $(ARCH)$(ISA_EXTENSIONS)
else
    FULL_ARCH = $(ARCH)
endif

# Compiler flags
CFLAGS = -march=$(FULL_ARCH) \
         -mabi=$(ABI) \
         -O2 \
         -Wall \
         -Wextra \
         -Werror \
         -nostdlib \
         -nostartfiles \
         -ffreestanding \
         -fno-builtin

# Select which test source to build (without .c)
PROGRAM ?= test_rv32i

# Linker flags
MAP = $(PROGRAM).map
LDFLAGS = -T link.ld \
          -Wl,--gc-sections \
          -Wl,-Map=$(MAP)

# Optional flags (uncomment to use)
# CFLAGS += -mstrict-align          # Force strict alignment
# CFLAGS += -mno-relax              # Disable linker relaxations
# CFLAGS += -mcmodel=medlow         # Medium-low code model (default)
# CFLAGS += -mcmodel=medany         # Medium-any code model
# CFLAGS += -msave-restore          # Use library calls for prologue/epilogue
# CFLAGS += -mno-div                # Don't use hardware division (if M extension not available)

# Source files
SRCS = $(PROGRAM).c
ASMS = boot.S
OBJS = $(SRCS:.c=.o) $(ASMS:.S=.o)
TARGET = $(PROGRAM).elf
BIN = $(PROGRAM).bin
DUMP = $(PROGRAM).dump
MEM = $(PROGRAM).mem

# Check if compiler is available
CHECK_TOOLCHAIN = @if ! command -v $(CC) >/dev/null 2>&1 && \
	! [ -f /opt/homebrew/bin/$(CC) ] && \
	! [ -f /usr/local/bin/$(CC) ]; then \
	echo "ERROR: $(CC) not found in PATH!"; \
	echo "Please install RISC-V toolchain or set TOOLCHAIN_PREFIX manually."; \
	echo "Example: make TOOLCHAIN_PREFIX=riscv64-unknown-elf-"; \
	echo ""; \
	echo "To install RISC-V toolchain:"; \
	echo "  macOS: brew tap riscv-software-src/riscv && brew install riscv-gnu-toolchain"; \
	echo "  Linux: Use your distribution's package manager"; \
	echo ""; \
	echo "If installed but not found, ensure /opt/homebrew/bin is in your PATH:"; \
	echo "  echo 'export PATH=\"/opt/homebrew/bin:\$$PATH\"' >> ~/.zshrc"; \
	echo "  source ~/.zshrc"; \
	exit 1; \
	fi

# Default target
all: check-toolchain $(TARGET) $(BIN) $(MEM) $(DUMP)

# Check toolchain before building
check-toolchain:
	$(CHECK_TOOLCHAIN)

# Build ELF executable (libgcc after OBJS so __mulsi3 etc. are pulled in)
$(TARGET): $(OBJS) link.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -lgcc -o $@

# Build binary file (for loading into FPGA)
$(BIN): $(TARGET)
	$(OBJCOPY) -O binary $< $@
	@echo "Binary size: $$(stat -f%z $@ 2>/dev/null || stat -c%s $@ 2>/dev/null) bytes"

# Build packed 32-bit memory init file (for $readmemh into logic [31:0] mem_array [...])
$(MEM): $(TARGET)
	@python3 -c 'from pathlib import Path; import sys; data=Path(sys.argv[1]).read_bytes(); data+=b"\x00"*((4-len(data)%4)%4); Path(sys.argv[2]).write_text("".join(format(int.from_bytes(data[i:i+4], "little"), "08x")+"\n" for i in range(0, len(data), 4)))' "$(BIN)" "$@"
	@echo "Generated packed memory init file: $@"

# Generate disassembly dump
$(DUMP): $(TARGET)
	$(OBJDUMP) -d -S $< > $@

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f *.o *.elf *.bin *.mem *.dump *.map

# Show current configuration
config:
	@echo "Current Configuration:"
	@echo "  Toolchain prefix: $(TOOLCHAIN_PREFIX)"
	@echo "  Architecture: $(FULL_ARCH)"
	@echo "  ABI: $(ABI)"
	@echo "  Compiler: $(CC)"
	@echo "  CFLAGS: $(CFLAGS)"
	@echo ""
	@if ! command -v $(CC) >/dev/null 2>&1 && \
		! [ -f /opt/homebrew/bin/$(CC) ] && \
		! [ -f /usr/local/bin/$(CC) ]; then \
		echo "WARNING: $(CC) not found in PATH!"; \
		echo "Please install RISC-V toolchain or set TOOLCHAIN_PREFIX manually."; \
		echo "Example: make TOOLCHAIN_PREFIX=riscv64-unknown-elf-"; \
		echo ""; \
		echo "If installed via Homebrew, ensure /opt/homebrew/bin is in your PATH."; \
	fi

# Run objdump to see generated assembly
asm: $(TARGET)
	$(OBJDUMP) -d $(TARGET) | less

# Show file sizes
size: $(TARGET)
	$(SIZE) $(TARGET)

# Verify RV32I instruction coverage in the dump
verify-instructions: $(DUMP)
	@echo "Checking RV32I instruction coverage..."
	@missing=0; \
	for insn in lb lh lw lbu lhu sb sh sw add addi sub and andi or ori xor xori \
	             sll slli srl srli sra srai slt slti sltu sltiu lui auipc jal jalr \
	             beq bne blt bge bltu bgeu; do \
	  if ! grep -qE "\b$$insn\b" $(DUMP); then \
	    echo "  MISSING: $$insn"; missing=$$((missing+1)); \
	  fi; \
	done; \
	if [ $$missing -eq 0 ]; then \
	  echo "  All RV32I instructions present in dump."; \
	else \
	  echo "  $$missing instruction(s) not found."; exit 1; \
	fi

# Help target
help:
	@echo "RISC-V 32I Test Program Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build all output files (ELF, BIN, MEM, DUMP)"
	@echo "  verify-instructions - Check dump for RV32I coverage"
	@echo "  clean    - Remove all build artifacts"
	@echo "  config   - Show current build configuration"
	@echo "  asm      - View disassembly of the compiled program"
	@echo "  size     - Show size information"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Configuration:"
	@echo "  Edit ARCH, ABI, and ISA_EXTENSIONS variables to change architecture"
	@echo "  Example: make ARCH=rv32i ISA_EXTENSIONS=m ABI=ilp32"
	@echo ""
	@echo "  Set TOOLCHAIN_PREFIX if auto-detection fails:"
	@echo "  Example: make TOOLCHAIN_PREFIX=riscv64-unknown-elf-"
	@echo "  Select program source: make PROGRAM=test_vga"
	@echo ""
	@echo "Current settings:"
	@echo "  TOOLCHAIN_PREFIX=$(TOOLCHAIN_PREFIX)"
	@echo "  PROGRAM=$(PROGRAM)"
	@echo "  ARCH=$(ARCH)"
	@echo "  ISA_EXTENSIONS=$(ISA_EXTENSIONS)"
	@echo "  ABI=$(ABI)"

.PHONY: all clean config asm size verify-instructions help
