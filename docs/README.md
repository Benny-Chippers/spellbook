# Spellbook documentation

Test programs and helpers for bringing up the Wizard Core RV32 CPU (FPGA / simulation).

| Document | Description |
|----------|-------------|
| [VGA overview](vga/overview.md) | Indexed framebuffer, palette memory map, `vga_driver` API |
| [VGA interface properties](vga/interface_properties.md) | CPU↔GPU contract, timing, and design evidence |
| [External memory integration](plans/ext_mem_integration.md) | Planned PSRAM / northbridge SPI work |

RTL memory-map source of truth: `wizardCore/docs/memory_Map.md`.

## Quick links (repo root)

- **Build:** `make PROGRAM=test_isa_vga` (add `RV32I_ONLY=1` for Verilator)
- **Drivers:** `drivers/vga_driver.h`, `drivers/vga_text.h`
- **Tests:** `tests/` — set `PROGRAM` to the basename of any `tests/<name>.c`
