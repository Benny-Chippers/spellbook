# Makefile for RISC-V bare-metal test programs
# Default ISA: rv32im. Use RV32I_ONLY=1 for rv32i (macOS Verilator / RTL without M).

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

# Select bare-metal application (basename of tests/$(PROGRAM).c)
PROGRAM ?= test_rv32i

# Verilator / local sim targets are rv32i-only (no M extension in RTL)
ifeq ($(PROGRAM),test_special_regs)
override RV32I_ONLY := 1
endif

# Default rv32im; RV32I_ONLY=1 strips M (Verilator sim on macOS when RTL is rv32i-only)
ISA_EXTENSIONS ?= m
ifeq ($(RV32I_ONLY),1)
override ISA_EXTENSIONS :=
endif

# Source layout overrides (must precede VGA image paths below)
TESTS_DIR ?= tests
DRIVERS_DIR ?= drivers
BUILD_DIR ?= build
PROGRAM_BUILD_DIR := $(BUILD_DIR)/$(PROGRAM)
OBJ_DIR := $(PROGRAM_BUILD_DIR)/obj
GEN_DIR := $(PROGRAM_BUILD_DIR)/generated

# --- Static VGA image embed (render_image / render_gaysans) ---
RENDER_IMAGE_PROGRAMS := render_image render_gaysans
VGA_IMAGE_SRC ?= images/plankton.bmp
VGA_IMAGE_ARRAY ?= vga_image_rgb
VGA_IMAGE_C ?= $(GEN_DIR)/vga_image_data.c
VGA_IMAGE_H ?= $(GEN_DIR)/vga_image_data.h
PROGRAM_SRC ?=

ifeq ($(PROGRAM),render_gaysans)
VGA_IMAGE_SRC := images/gaysans.txt
VGA_IMAGE_ARRAY := gaysans_rgb
VGA_IMAGE_C := $(GEN_DIR)/gaysans_bitmap.c
VGA_IMAGE_H := $(GEN_DIR)/gaysans_bitmap.h
endif

ifeq ($(filter $(PROGRAM),$(RENDER_IMAGE_PROGRAMS)),$(PROGRAM))
PROGRAM_SRC := render_image.c
PROGRAM_EXTRA_SRCS := $(VGA_IMAGE_C)
VGA_IMAGE_LINK_H := $(GEN_DIR)/vga_image_link.h
endif

# --- Pong (indexed sprites + software frame buffer) ---
PONG_DIR ?= pong
PONG_ASSETS_C := $(GEN_DIR)/pong_assets.c
PONG_ASSETS_H := $(GEN_DIR)/pong_assets.h
PROGRAM_CFLAGS ?=
ifeq ($(PROGRAM),pong)
PROGRAM_CFLAGS := -I$(PONG_DIR)
PROGRAM_EXTRA_SRCS := $(PONG_DIR)/pong_engine.c $(PONG_ASSETS_C)
endif

# Build full architecture string
ifneq ($(strip $(ISA_EXTENSIONS)),)
FULL_ARCH := $(ARCH)$(ISA_EXTENSIONS)
else
FULL_ARCH := $(ARCH)
endif

# Compiler flags
CFLAGS = -march=$(FULL_ARCH) \
         -mabi=$(ABI) \
         -I$(DRIVERS_DIR) \
         -I$(GEN_DIR) \
         -I$(dir $(VGA_IMAGE_H)) \
         $(PROGRAM_CFLAGS) \
         -O2 \
         -Wall \
         -Wextra \
         -Werror \
         -nostdlib \
         -nostartfiles \
         -ffreestanding \
         -fno-builtin

# Linker flags
MAP = $(PROGRAM_BUILD_DIR)/$(PROGRAM).map
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

# Source files (bare-metal apps under tests/, VGA helpers under drivers/)
COMMON_SRCS = $(DRIVERS_DIR)/vga_driver.c
ifeq ($(PROGRAM),test_special_regs)
COMMON_SRCS :=
endif
PROGRAM_EXTRA_SRCS ?=
SRCS = $(TESTS_DIR)/$(if $(PROGRAM_SRC),$(PROGRAM_SRC),$(PROGRAM).c) $(COMMON_SRCS) $(PROGRAM_EXTRA_SRCS)
ASMS = boot.S
OBJS = $(addprefix $(OBJ_DIR)/,$(SRCS:.c=.o)) $(addprefix $(OBJ_DIR)/,$(ASMS:.S=.o))
TARGET = $(PROGRAM_BUILD_DIR)/$(PROGRAM).elf
BIN = $(PROGRAM_BUILD_DIR)/$(PROGRAM).bin
DUMP = $(PROGRAM_BUILD_DIR)/$(PROGRAM).dump
MEM = $(PROGRAM_BUILD_DIR)/$(PROGRAM).mem

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


# Default goal
all: check-toolchain $(TARGET) $(BIN) $(MEM) $(DUMP)

# Check toolchain before building
check-toolchain:
	$(CHECK_TOOLCHAIN)

# Build ELF executable (libgcc after OBJS so __mulsi3 etc. are pulled in)
$(TARGET): $(OBJS) link.ld
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -lgcc -o $@

# Build binary file (for loading into FPGA)
$(BIN): $(TARGET)
	@mkdir -p $(dir $@)
	$(OBJCOPY) -O binary $< $@
	@echo "Binary size: $$(stat -f%z $@ 2>/dev/null || stat -c%s $@ 2>/dev/null) bytes"

# Build packed 32-bit memory init file (for $readmemh into logic [31:0] mem_array [...])
$(MEM): $(BIN)
	@mkdir -p $(dir $@)
	@python3 -c 'from pathlib import Path; import sys; data=Path(sys.argv[1]).read_bytes(); data+=b"\x00"*((4-len(data)%4)%4); Path(sys.argv[2]).write_text("".join(format(int.from_bytes(data[i:i+4], "little"), "08x")+"\n" for i in range(0, len(data), 4)))' "$(BIN)" "$@"
	@echo "Generated packed memory init file: $@"

# Generate disassembly dump
$(DUMP): $(TARGET)
	@mkdir -p $(dir $@)
	$(OBJDUMP) -d -S $< > $@

# Compile source files
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# render_image.o must recompile when the selected embedded header changes
ifneq ($(filter $(PROGRAM),$(RENDER_IMAGE_PROGRAMS)),)
$(OBJ_DIR)/$(TESTS_DIR)/render_image.o: force
endif
$(OBJ_DIR)/$(TESTS_DIR)/render_image.o: $(TESTS_DIR)/render_image.c $(VGA_IMAGE_C) $(VGA_IMAGE_H) $(VGA_IMAGE_LINK_H) scripts/embed_vga_image.py
	@mkdir -p $(dir $@) $(GEN_DIR)
	@echo '#include "$(notdir $(VGA_IMAGE_H))"' > $(VGA_IMAGE_LINK_H)
	$(CC) $(CFLAGS) -c $(TESTS_DIR)/render_image.c -o $@

# Regenerate embedded VGA frame from BMP or gaysans .txt
$(VGA_IMAGE_C): $(VGA_IMAGE_SRC) scripts/embed_vga_image.py
	@mkdir -p $(dir $(VGA_IMAGE_C)) $(dir $(VGA_IMAGE_H))
	python3 scripts/embed_vga_image.py "$(VGA_IMAGE_SRC)" "$(VGA_IMAGE_C)" "$(VGA_IMAGE_H)" "$(VGA_IMAGE_ARRAY)"
	@echo '#include "$(notdir $(VGA_IMAGE_H))"' > $(VGA_IMAGE_LINK_H)

$(VGA_IMAGE_H) $(VGA_IMAGE_LINK_H): $(VGA_IMAGE_C)

$(OBJ_DIR)/$(TESTS_DIR)/pong.o $(OBJ_DIR)/$(PONG_DIR)/pong_engine.o: $(PONG_ASSETS_H)

$(PONG_ASSETS_C): $(PONG_DIR)/include/sprites/background.bmp \
                  $(PONG_DIR)/include/sprites/paddle.bmp \
                  $(PONG_DIR)/include/sprites/ball.bmp \
                  scripts/embed_pong_assets.py
	@mkdir -p $(GEN_DIR)
	python3 scripts/embed_pong_assets.py "$(GEN_DIR)"

$(PONG_ASSETS_H): $(PONG_ASSETS_C)

$(OBJ_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TESTS_DIR)/*.o $(DRIVERS_DIR)/*.o $(PONG_DIR)/*.o *.o *.elf *.bin *.mem *.dump *.map
	rm -f $(TESTS_DIR)/vga_image_link.h

# Show current configuration
config:
	@echo "Current Configuration:"
	@echo "  Toolchain prefix: $(TOOLCHAIN_PREFIX)"
	@echo "  Architecture: $(FULL_ARCH)"
	@echo "  ABI: $(ABI)"
	@echo "  RV32I_ONLY: $(RV32I_ONLY)"
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

# Verify RV32M presence in the dump
verify-rv32m-instructions:
	@$(MAKE) check-toolchain PROGRAM=test_rv32m $(BUILD_DIR)/test_rv32m/test_rv32m.dump
	@echo "Checking RV32M instruction coverage..."
	@missing=0; \
	for insn in mul mulh mulhu mulhsu div divu rem remu; do \
	  if ! grep -qE "\b$$insn\b" $(BUILD_DIR)/test_rv32m/test_rv32m.dump; then \
	    echo "  MISSING: $$insn"; missing=$$((missing+1)); \
	  fi; \
	done; \
	if [ $$missing -eq 0 ]; then \
	  echo "  All RV32M instructions present in $(BUILD_DIR)/test_rv32m/test_rv32m.dump."; \
	else \
	  echo "  $$missing instruction(s) not found."; exit 1; \
	fi

# Help target
help:
	@echo "RISC-V bare-metal test program Makefile (default: rv32im)"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build all output files (ELF, BIN, MEM, DUMP)"
	@echo "  verify-instructions - Check dump for RV32I coverage"
	@echo "  verify-rv32m-instructions - Build test_rv32m.dump and check M-extension coverage"
	@echo "  clean    - Remove all build artifacts"
	@echo "  config   - Show current build configuration"
	@echo "  asm      - View disassembly of the compiled program"
	@echo "  size     - Show size information"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Configuration:"
	@echo "  Default march is rv32im (ISA_EXTENSIONS=m). Override extensions: make ISA_EXTENSIONS=mc"
	@echo "  Verilator / rv32i-only RTL: make RV32I_ONLY=1 ..."
	@echo "  Example: make RV32I_ONLY=1 PROGRAM=test_isa_vga"
	@echo ""
	@echo "  Set TOOLCHAIN_PREFIX if auto-detection fails:"
	@echo "  Example: make TOOLCHAIN_PREFIX=riscv64-unknown-elf-"
	@echo "  Select program source: make PROGRAM=test_vga"
	@echo "  Static bitmap: make PROGRAM=render_image (default: images/plankton.bmp)"
	@echo "  Gaysans text:  make PROGRAM=render_gaysans"
	@echo "  Pong demo:     make PROGRAM=pong"
	@echo "  Special regs:  make PROGRAM=test_special_regs   (auto rv32i for Verilator)"
	@echo "  Custom image:  make PROGRAM=render_image VGA_IMAGE_SRC=path/to/file.bmp \\"
	@echo "                   VGA_IMAGE_C=tests/my_image.c VGA_IMAGE_H=tests/my_image.h VGA_IMAGE_ARRAY=my_rgb"
	@echo ""
	@echo "Current settings:"
	@echo "  TOOLCHAIN_PREFIX=$(TOOLCHAIN_PREFIX)"
	@echo "  PROGRAM=$(PROGRAM)"
	@echo "  ARCH=$(ARCH)"
	@echo "  ISA_EXTENSIONS=$(ISA_EXTENSIONS)"
	@echo "  FULL_ARCH=$(FULL_ARCH)"
	@echo "  RV32I_ONLY=$(RV32I_ONLY)"
	@echo "  ABI=$(ABI)"

.PHONY: all clean config asm size verify-instructions verify-rv32m-instructions help force
