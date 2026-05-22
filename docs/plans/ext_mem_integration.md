# External Memory Integration Plan — Wizard Core

Branch: `core/WC-07-Add-External-Memory`

RTL and memory-map source of truth: `wizardCore/` (`docs/memory_Map.md`).

---

## Current State

| File | Status |
|---|---|
| `src/3_EX/ex_top.sv` | Routing stub exists (else branch silently passes ctrlMEM through) |
| `src/4_MEM/xmem_top.sv` | Empty placeholder |
| `src/4_MEM/xmem_spi.sv` | Empty placeholder |
| `src/4_MEM/xmem_fms.sv` | Empty placeholder — FSM driving the SPI transaction |
| `src/TOP/top.sv` | stall_MEM initialized to 0, no ctrlXMEM path wired |
| `wizardCore/docs/memory_Map.md` | Address routing + metadata encoding spec complete |

---

## SPI Protocol Design

### No Separate Command Byte

The SPI frame does **not** use a separate command opcode (like a flash chip's `0x02`/`0x03`). The only channel to the MCU is the data line, so all control information is encoded into the address itself before dispatch.

Per `memory_Map.md`, on northbridge dispatch, bits `[29:26]` of the address are **overwritten** with metadata:

```
addr[31:30]  Device routing (preserved from software):
               00 = PSRAM
               01 = Flash
               10 = SD card
               11 = Southbridge

addr[29:27]  Byte width (written by xmem_top from mem_ctrl.size/sign):
               000 = Word (32-bit)
               001 = First halfword
               010 = Second halfword
               011 = First byte
               100 = Second byte
               101 = Third byte
               110 = Fourth byte

addr[26]     Direction (written by xmem_top from mem_ctrl.memWrite):
               0 = Read
               1 = Write

addr[25:0]   Actual device address
```

The full 4-byte modified address is sent over SPI first. The MCU reads **byte 3 first** (addr[31:24]), which contains: device routing `[31:30]`, byte width `[29:27]`, R/W `[26]`, and the top 2 address bits `[25:24]`. This byte is the effective command. The MCU fully decodes the operation from this single byte before the lower address bytes arrive.

### SPI Frame

```
FPGA → MCU:  [addr[31:24]] [addr[23:16]] [addr[15:8]] [addr[7:0]] ([D3] [D2] [D1] [D0])
                 ^cmd byte      addr         addr         addr       ^write data only

MCU → FPGA:  (ignored)     (ignored)     (ignored)    (ignored)   [D3] [D2] [D1] [D0]
                                                                    ^read data only
```

Total: 8 bytes per transaction. Read and write responses are handled separately (see open questions on full-duplex vs half-duplex).

---

## Phase 1 — Address Routing in `ex_top.sv`

**What:** Replace the fallthrough `else` branch (lines 84–87) with an explicit `ctrlXMEM` output path.

Currently:
```verilog
end else begin
    oB_ctrlMEM = i_ctrlMEM;   // ← wrong: passes NB traffic to on-chip RAM
    oB_ctrlVGA = '0;
end
```

Change to:
```verilog
end else begin
    oB_ctrlMEM  = '0;
    oB_ctrlVGA  = '0;
    oB_ctrlXMEM = i_ctrlMEM;  // ← new output
end
```

**New port on `ex_top`:**
```verilog
output mem_ctrl_t o_ctrlXMEM;
```

Add corresponding staging register `oB_ctrlXMEM` and latch it in the `always_ff` block alongside the existing outputs.

---

## Phase 2 — `xmem_top.sv` (Routing Layer)

Routes northbridge traffic to the correct sub-controller based on `addr[31:30]`, builds the modified SPI address by overwriting `addr[29:26]` with metadata, and presents a unified read-data / stall interface to the rest of the pipeline.

**Ports:**
```
Inputs:
  i_clk, i_reset_n
  i_addr       [31:0]     // resultALU from EX (original CPU address)
  i_writeData  [31:0]
  i_ctrlXMEM   mem_ctrl_t
  en_MEM

Outputs:
  o_readData   [31:0]
  o_stall                 // hold MEM stage until transaction completes
```

**Address rewrite** (done here, not in EX):
```verilog
logic [31:0] spi_addr;
assign spi_addr = {
    i_addr[31:30],                           // device select (preserved)
    encode_width(i_ctrlXMEM.size),           // [29:27] byte width
    i_ctrlXMEM.memWrite,                     // [26] R/W
    i_addr[25:0]                             // actual device address
};
```

**Sub-controller routing** on `i_addr[31:30]`:
- `2'b00` → PSRAM (`xmem_fms` instance)
- `2'b01` → Flash (stub: stall=0, readData=`32'hDEAD_BEEF`)
- `2'b10` → SD card (stub)
- `2'b11` → Southbridge (stub)

Stubs assert `o_stall = 0` and return `32'hDEAD_BEEF` so unimplemented devices fail visibly in simulation.

---

## Phase 3 — `xmem_spi.sv` (SPI Master)

Parametrizable byte-serial SPI master. All external device controllers drive this.

**Parameters:**
- `CLK_DIV` — system clock cycles per SPI half-period (testbench has SPI at 2 MHz, i.e., `CLK_DIV = sys_clk_hz / (2 * 2_000_000)`)
- `CPOL`, `CPHA` — SPI mode (match MCU slave mode)

**Interface:**
```
Inputs:
  i_clk, i_reset_n
  i_start          // pulse to begin one byte transfer
  i_tx_byte [7:0]

Outputs:
  o_rx_byte [7:0]
  o_busy           // high during transfer
  o_done           // 1-cycle pulse on completion

SPI pins:
  o_sclk, o_mosi, i_miso, o_cs_n
```

Keep this strictly byte-serial — the FSM in `xmem_fms.sv` handles sequencing multiple bytes per transaction.

---

## Phase 4 — `xmem_fms.sv` (PSRAM Transaction FSM)

State machine that translates a CPU load/store (address + data + ctrl) into an 8-byte SPI sequence using `xmem_spi`.

**FSM states:**
```
IDLE → SEND_A3 → SEND_A2 → SEND_A1 → SEND_A0 →
    [write: SEND_D3 → SEND_D2 → SEND_D1 → SEND_D0]
    [read:  RECV_D3 → RECV_D2 → RECV_D1 → RECV_D0]
→ DONE → IDLE
```

- Assert `o_stall = 1` leaving IDLE, deassert entering DONE
- Send `spi_addr[31:24]` first (the effective command byte), then lower address bytes
- For writes: transmit `i_writeData` bytes after address
- For reads: clock in 4 bytes, latch into `o_readData` before DONE
- CS asserts at start, deasserts after last byte (resolve overlap vs sequential with MCU)

**Latency estimate:** At 2 MHz SPI with 100 MHz sysclk, CLK_DIV=25. Each byte = 8 × 2 × 25 = 400 cycles. 8 bytes = 3,200 sys_clk cycles stalled per access (plus MCU processing time). Factor into timing analysis.

---

## Phase 5 — FPGA↔MCU Protocol Definition

This must be resolved before writing RTL — it determines byte ordering, CS behavior, and response timing.

**Agreed frame structure (proposed):**
```
Write (FPGA → MCU, 8 bytes):
  [addr[31:24]] [addr[23:16]] [addr[15:8]] [addr[7:0]] [D3] [D2] [D1] [D0]
   ^cmd (R/W, device, width)

Read (FPGA → MCU, 4 bytes sent, then receive):
  [addr[31:24]] [addr[23:16]] [addr[15:8]] [addr[7:0]]
  then MCU responds with: [D3] [D2] [D1] [D0]
```

**Open questions (resolve before coding `xmem_fms`):**

| Question | Options |
|---|---|
| Read response timing | Full-duplex overlapped during addr bytes, or CS deassert/reassert? |
| MCU "ready" signal | Fixed SPI clock timing, or does MCU need a separate GPIO ready line? |
| Sub-word stores | Does FPGA always send 4 data bytes and MCU applies width mask, or does FPGA only send the relevant bytes? Recommend: FPGA always sends 4, MCU masks — simpler FSM. |
| CS behavior | CS stays asserted for entire 8-byte frame, or deasserts between address and data? |

---

## Phase 6 — Stall Wiring in `top.sv`

**What:** Wire `ctrlXMEM` from EX through to `xmem_top`, and connect the real stall.

Add to `top.sv`:
```verilog
mem_ctrl_t ctrlXMEM;
logic xmem_stall;
logic [31:0] xmem_readData;
```

Replace `initial begin stall_MEM = 0; end` with:
```verilog
assign stall_MEM = xmem_stall;
```

Add `xmem_top` instantiation after `mem_top`:
```verilog
xmem_top XMEM (
    .i_clk       (clk),
    .i_reset_n   (reset_n),
    .i_addr      (resultALU),
    .i_writeData (regData2),
    .i_ctrlXMEM  (ctrlXMEM),
    .en_MEM      (en_MEM),
    .o_readData  (xmem_readData),
    .o_stall     (xmem_stall)
);
```

Wire `ctrlXMEM` from `EX.o_ctrlXMEM` in the `ex_top` instantiation block.

The WB mux in `wb_top` may also need updating to select `xmem_readData` for northbridge loads — or `xmem_top` can present its read data on the same `readData` bus as on-chip RAM (gated by `ctrlXMEM.memRead`).

---

## Phase 7 — Simulation Support

### 7a. SPI Slave Behavioral Model (non-synthesizable)

`spi_slave_model.sv` — simulates MCU + external memory with configurable response latency:
- Decodes the 8-byte frame protocol from Phase 5
- Maintains `logic [31:0] mem_model[0:N]`
- Parametrizable `RESPONSE_DELAY` in cycles
- Guard with `` `ifdef SIMULATION ``
- Log each transaction with `$display`: address, data, direction, cycle count

### 7b. Testbench Integration

`testBench.sv` already declares a 2 MHz SPI clock. Wire the SPI pins between `top` and `spi_slave_model` when simulation is enabled.

---

## Phase 8 — Spellbook Test Program (`test_ext_mem.c`)

Write/read-back test sequence:
1. Write known patterns (word, halfword, byte granularity) to several external addresses
2. Read back and verify
3. Test first and last address in range
4. Write a pattern large enough to exercise any MCU batching behavior
5. Report via `test_passed`/`test_failed` globals (consistent with existing tests)

External addresses accessed via raw pointer casts to `0x2000_xxxx`. No linker changes needed — define base as `#define PSRAM_BASE 0x20000000u`.

---

## Suggested Implementation Order

```
1. Phase 5: Define MCU protocol              ← unblocks everything
2. Phase 3: xmem_spi.sv                      ← standalone, test in isolation with spi_slave_model
3. Phase 7a: spi_slave_model.sv              ← needed to test xmem_spi
4. Phase 4: xmem_fms.sv                      ← depends on xmem_spi + protocol
5. Phase 2: xmem_top.sv                      ← wires fms into routable interface
6. Phase 1: ex_top.sv ctrlXMEM output        ← routes NB traffic through pipeline
7. Phase 6: top.sv wiring                    ← connects ctrlXMEM + stall end-to-end
8. Phase 7b: testBench.sv integration        ← full simulation
9. Phase 8: test_ext_mem.c                   ← validate end-to-end
10. MCU firmware                             ← parallelize with steps 2–8
```
