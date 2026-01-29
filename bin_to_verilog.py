#!/usr/bin/env python3
"""
Convert a flat RISC-V binary file (with bootloader already linked in)
to a Verilog instruction-memory initialization for your CPU/FPGA.

Outputs lines like:
    32'h00000000:  o_instr <= 32'hXXXXXXXX;
    32'h00000004:  o_instr <= 32'hYYYYYYYY;
"""

import sys
import os


def bin_to_verilog(bin_file, output_file=None, start_addr=0x0):
    """
    Convert binary file to Verilog instruction format.
    
    Args:
        bin_file: Path to input binary file
        output_file: Path to output Verilog file (default: bin_file + '_code.v')
        start_addr: Starting address in memory for the first instruction
                    (for your current setup this should be 0x0).
    """
    # Read binary file
    try:
        with open(bin_file, 'rb') as f:
            data = f.read()
    except FileNotFoundError:
        print(f"Error: File '{bin_file}' not found.", file=sys.stderr)
        sys.exit(1)
    
    # Determine output file
    if output_file is None:
        output_file = os.path.splitext(bin_file)[0] + '_code.v'
    
    # Open output file
    with open(output_file, 'w') as f:
        # Write header comment
        f.write(f"// Verilog instruction memory initialization\n")
        f.write(f"// Generated from: {os.path.basename(bin_file)}\n")
        f.write(f"// Total size: {len(data)} bytes ({len(data) // 4} instructions)\n")
        f.write(f"// Starting address: 0x{start_addr:08x}\n\n")

        # Process 4-byte chunks (32-bit instructions)
        instruction_count = 0

        for i in range(0, len(data), 4):
            # Map binary file offset to memory address
            addr = start_addr + i
            # Read 4 bytes (little-endian for RISC-V)
            if i + 4 <= len(data):
                instruction_bytes = data[i:i+4]
            else:
                # Pad with zeros if not aligned
                instruction_bytes = data[i:] + bytes(4 - (len(data) - i))
            
            # Convert to 32-bit integer (little-endian)
            instruction = int.from_bytes(instruction_bytes, byteorder='little')
            
            # Format as Verilog assignment
            f.write(f"    32'h{addr:x}:  o_instr <= 32'h{instruction:08x};\n")
            
            addr += 4
            instruction_count += 1
        
        f.write(f"\n// Total instructions: {instruction_count}\n")
    
    print(f"Successfully converted {bin_file}")
    print(f"  Output: {output_file}")
    print(f"  Instructions: {instruction_count}")
    print(f"  Address range: 0x{start_addr:08x} - 0x{addr-4:08x}")

if __name__ == "__main__":
    # Default input file
    default_bin = "test_rv32i.bin"

    if len(sys.argv) > 1:
        bin_file = sys.argv[1]
    else:
        bin_file = default_bin
    
    # Optional output file
    output_file = sys.argv[2] if len(sys.argv) > 2 else None

    # Optional start address (hex)
    start_addr = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x0

    bin_to_verilog(bin_file, output_file, start_addr)
