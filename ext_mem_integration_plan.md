# External Memory Integration Plan — Wizard Core / Capstone

## System Topology

```
CPU (Wizard Core)
  └─► Northbridge (RTL, new)
        ├─► PSRAM controller (RTL, new)  ──SPI──► MCU ──SPI──► PSRAM chip
        ├─► Flash controller             (future)
        └─► SD card controller           (future)
```

The memory map doc already reserves `addr[29:28] = 2'b10` for Northbridge traffic, and `addr[31:30]` for sub-device routing within it. This plan implements the first concrete device (PSRAM) within that planned architecture.

---

## Phase 1 — Address Routing in `ex_top.sv`

**What:** Add the `2'b10` branch to the existing address mux in `ex_top.sv` (lines ~75–92).

Currently the `else` case silently routes unrecognized addresses back to main SRAM. Replace it with explicit handling:

```
addr[29:28] == 2'b00  →  ctrlMEM  (on-chip SRAM)
addr[29:28] == 2'b01  →  ctrlVGA  (VGA frame buffer)
addr[29:28] == 2'b10  →  ctrlNB   (Northbridge, new signal)
addr[29:28] == 2'b11  →  ctrlCFG  (config registers, future)
```

**What to add to `top.sv`:**
- New wire `ctrlNB` of type `mem_ctrl_t` propagated from EX → MEM → Northbridge
- New wire `nb_readData` (32-bit) returned from Northbridge to WB mux
- New stall signal `stall_NB` that feeds into `stall_MEM`

**Design decision to resolve:** The memory map encodes `addr[26]` as read/write and `addr[29:27]` as byte width. This duplicates what `ctrlMEM.memRead/Write` and `ctrlMEM.size` already carry. Decide whether Northbridge reads these from the control struct (simpler, consistent with existing design) or reconstructs them from the address bits (matches the memory map spec but adds redundancy). Recommend using the control struct.

---

## Phase 2 — Northbridge Top Module (`nb_top.sv`)

**What:** A routing layer that sits between the CPU pipeline and device controllers. Accepts traffic from `ex_top.sv`, dispatches to the correct sub-controller based on `addr[31:30]`, and returns read data + a stall signal.

**Ports:**
```
Inputs:
  i_clk, i_reset_n
  i_addr       [31:0]     // From resultALU
  i_writeData  [31:0]     // From regData2
  i_ctrlNB     mem_ctrl_t
  en_MEM

Outputs:
  o_readData   [31:0]
  o_stall                 // Hold MEM stage until transaction completes
```

**Internal routing:** `addr[31:30]` selects sub-controller:
- `2'b00` → PSRAM controller (first to implement)
- `2'b01` → Flash controller (stub/unimplemented)
- `2'b10` → SD card controller (stub/unimplemented)
- `2'b11` → Southbridge (stub/unimplemented)

Stubs should assert `o_stall = 0` and return `o_readData = 32'hDEAD_BEEF` so unimplemented devices fail visibly in simulation rather than silently.

---

## Phase 3 — SPI Master (`spi_master.sv`)

**What:** A reusable, parametrizable SPI master. This is the lowest-level RTL block — all external device controllers use it.

**Parameters:**
- `CLK_DIV` — system clock cycles per SPI half-period (e.g., `4` → SPI runs at `sys_clk / 8`)
- `CPOL`, `CPHA` — SPI mode (match your MCU's expected mode)

**Interface:**
```
Inputs:
  i_clk, i_reset_n
  i_start          // Pulse to begin transfer
  i_tx_byte [7:0]  // Byte to send

Outputs:
  o_rx_byte [7:0]  // Byte received
  o_busy           // High while transfer in progress
  o_done           // 1-cycle pulse on completion

SPI pins:
  o_sclk, o_mosi, o_cs_n, i_miso
```

The MCU intermediary means you don't need to match the raw memory chip's SPI protocol here — only the FPGA↔MCU protocol. The MCU owns the chip-level detail.

---

## Phase 4 — PSRAM Controller (`psram_ctrl.sv`)

**What:** The state machine that translates a CPU load/store into a multi-byte SPI transaction sequence and drives the SPI master.

**Transaction sequence (example — exact bytes depend on MCU protocol, see Phase 5):**

| Phase | Bytes sent | Content |
|---|---|---|
| CMD | 1 | Read (`0x03`) or Write (`0x02`) |
| ADDR | 3–4 | Target address in external memory |
| DATA | 4 | Write data (store) or dummy bytes (load) |
| RESP | 4 | Read data returned by MCU (load only) |

**FSM states:**
```
IDLE → SEND_CMD → SEND_ADDR → SEND_DATA → RECV_DATA → DONE → IDLE
```

**Stall behavior:**
- Assert `o_stall = 1` when leaving IDLE
- Deassert `o_stall = 0` on entering DONE
- Read data latched into `o_readData` before DONE

**Latency math to do early:** Calculate worst-case stall depth.
Example: SPI at `sys_clk/8`, 9-byte transaction → `9 × 8 × 8 = 576 sys_clk cycles` stalled per access. Factor in MCU processing time on top. This drives decisions about whether to add a cache.

---

## Phase 5 — FPGA↔MCU Protocol Definition

**What:** A byte-level protocol that the FPGA SPI master and MCU SPI slave both implement. This needs to be defined before writing RTL or firmware so both sides agree.

**Proposed minimal protocol:**

```
Write transaction (FPGA → MCU):
  [0x02] [A2] [A1] [A0] [D3] [D2] [D1] [D0]
   cmd    addr[23:16] [15:8] [7:0]   data[31:0]

Read transaction (FPGA → MCU):
  [0x03] [A2] [A1] [A0] [0x00] [0x00] [0x00] [0x00]
   cmd    addr bytes          dummy bytes

MCU → FPGA response (read only, overlapped or sequential):
  [D3] [D2] [D1] [D0]
```

**Questions to resolve:**
- Is the response overlapped with the dummy bytes (full-duplex) or does CS deassert and reassert?
- Does the MCU need a "ready" signal back to the FPGA, or is timing handled by fixed delay?
- Sub-word access: does the FPGA always transact 32 bits and extract sub-words itself, or does the MCU handle byte/halfword granularity?
- What is the MCU's memory address space relative to the CPU's `addr[31:30]` device selection?

**MCU firmware deliverable:** An SPI slave interrupt handler or polling loop that decodes the above protocol and drives the memory chip.

---

## Phase 6 — Stall Wiring in `top.sv`

**What:** Connect the real stall signal from the Northbridge into the FSM.

Current state in `top.sv` (lines 68–72) initializes all stalls to 0 in an `initial` block. Replace the `stall_MEM` constant with a continuous assignment:

```verilog
assign stall_MEM = nb_stall;  // from nb_top
```

`top_en.sv` already handles stall correctly — the MEM stage holds and does not advance to WB until `stall_MEM` deasserts. No FSM changes needed.

**Important edge case:** If instruction fetch ever needs to come from external memory (executing code in PSRAM), `stall_IF` will also need to be driven. The IF stage currently has no stall path to the instruction memory. That would require a separate fetch controller or a unified memory arbiter — defer unless needed.

---

## Phase 7 — Simulation Support

### 7a. SPI Slave Behavioral Model (`spi_slave_model.sv`, non-synthesizable)

A testbench-only module that simulates the MCU + memory with configurable response latency:
- Decodes the agreed protocol from Phase 5
- Maintains an internal `logic [31:0] mem_model [0:N]` array
- Responds to reads with stored values, processes writes
- Parametrizable `RESPONSE_DELAY` in cycles to simulate MCU processing time
- Guard with `` `ifdef SIMULATION ``

### 7b. Testbench Integration

Wire `spi_slave_model` into `testBench.sv` alongside the existing CPU. Connect MOSI/MISO/SCLK/CS between the CPU's SPI pins and the model.

### 7c. Waveform Annotations

Add `` `ifdef SIMULATION `` `$display` statements in `psram_ctrl.sv` to log each transaction: address, data, direction, cycle count. Helps correlate VCD signals with logical transactions in GTKWave.

---

## Phase 8 — Spellbook Test Program (`test_ext_mem.c`)

**Test sequence:**
1. Write known patterns to several external memory addresses (word, halfword, byte granularity)
2. Read them back and compare
3. Test address boundary behavior (first address, last address)
4. Write a pattern large enough to exercise any MCU batching behavior
5. Report pass/fail via `test_passed` / `test_failed` globals (consistent with existing tests)

**Linker note:** External memory addresses are not covered by `link.ld`. The test accesses them via raw pointer casts — no linker changes needed. Define the base address as a `#define`.

---

## Open Design Questions

Resolve these before writing RTL — they have significant architectural impact.

| Question | Options | Impact |
|---|---|---|
| **Cache?** | None / Direct-mapped / Fully associative | Without cache, every external access stalls for hundreds of cycles. A small 16–64 line cache cuts this dramatically for sequential code. |
| **Instruction fetch from external memory?** | No (code runs from SRAM only) / Yes | If yes, IF stage needs its own stall signal and a memory arbiter. |
| **SPI clock domain** | Synchronous (divided from sys_clk) / Asynchronous (separate clock) | Synchronous is simpler; async requires CDC FIFOs at the SPI master boundary. |
| **MCU response: overlapped or sequential?** | Full-duplex overlapped / CS reassert | Overlapped halves transaction byte count; sequential is simpler to implement. |
| **Sub-word granularity** | FPGA sends 32-bit always, extracts sub-word itself / MCU handles width | Simpler MCU firmware if FPGA handles it; narrower transactions if MCU does. |
| **Read-modify-write for sub-word stores** | Required if external memory is not byte-addressable | Adds complexity to `psram_ctrl.sv` FSM. |

---

## Suggested Implementation Order

```
1. Define MCU protocol (Phase 5)         ← blocks everything else
2. spi_master.sv (Phase 3)               ← standalone, testable in isolation
3. spi_slave_model.sv (Phase 7a)         ← needed to test spi_master
4. psram_ctrl.sv (Phase 4)               ← depends on spi_master + protocol
5. nb_top.sv (Phase 2)                   ← wires psram_ctrl into routable interface
6. ex_top.sv routing (Phase 1)           ← adds nb signal path through pipeline
7. stall wiring in top.sv (Phase 6)      ← activates the stall path
8. testBench.sv integration (Phase 7b)   ← full end-to-end simulation
9. test_ext_mem.c (Phase 8)              ← validate through simulation
10. MCU firmware                         ← parallelize with steps 2–8
```
