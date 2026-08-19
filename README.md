# emu

A Game Boy / Game Boy Color emulator written in C.

## Building

### Prerequisites

- **gcc** (or any C99 compiler)
- **SDL3** (`brew install sdl3`)
- **pkg-config** (for SDL3 detection)

### Build

```bash
make all type=RELEASE
```

This produces:

| Artifact | Description |
|---|---|
| `emu-cli` | Executable |
| `libemu.a` | Static library (frontend-agnostic) |
| `libemu.so` | Shared library |

A debug build with sanitizers:

```bash
make all
```

## Usage

```bash
./emu-cli path/to/rom.gb
```

### Environment Variables

| Variable | Default | Description |
|---|---|---|
| `EMU_SCALE` | `4` | Window scale factor (e.g. `EMU_SCALE=3` for 480x432) |
| `EMU_NOSLEEP` | unset | When set, disables frame pacing for max speed (testing/benchmarking) |

## Controls

| Key | Game Boy Button |
|---|---|
| Arrow keys | D-pad (Up/Down/Left/Right) |
| Z | A |
| X | B |
| Backspace | Select |
| Enter | Start |

Close the window or press the window close button to quit.

## Features

### CPU

- Full SM83 (Game Boy CPU) instruction set
- All 256 base opcodes + 256 CB-prefixed opcodes
- Interrupt handling (VBlank, Timer, Serial, LCD STAT, Joypad)
- HALT bug emulation

### PPU (Pixel Processing Unit)

- Background, window, and sprite rendering
- Scanline-accurate mode transitions (OAM, Transfer, HBlank, VBlank)
- DMG palette (4 shades of gray)
- LCDC, STAT, SCY/SCX, LY/LYC, BGP, OBP0/OBP1, WY/WX registers

### Memory / MBC

| MBC | Status | Notes |
|---|---|---|
| ROM-only | Working | No bank switching |
| MBC1 | Working | 11/11 mooneye tests pass |
| MBC2 | Working | 7/7 mooneye tests pass |
| MBC3 | Working | ROM/RAM banking, RTC register stub (no clock source yet) |
| MBC5 | Working | 9/9 mooneye tests pass |

### Timer

- DIV, TIMA, TMA, TAC registers
- Falling-edge counter with correct frequency selection

### Joypad

- P1 register (0xFF00) with proper select lines and active-low button state

### Serial

- Serial output intercepted for test ROM diagnostics (Blargg, SameSuite, mooneye)

## Test Results

### mooneye-test-suite (~85% pass rate)

| Suite | Passed | Total |
|---|---|---|
| CPU instruction tests | 9 | 9 |
| MBC1 emulator-only | 11 | 11 |
| MBC2 emulator-only | 7 | 7 |
| MBC5 emulator-only | 9 | 9 |
| Other acceptance | 8 | 16 |
| **Total** | **44** | **52** |

### Blargg cpu_instrs

All 10 individual ROMs pass (01-special through 10-bit ops).

Run the full test suite:

```bash
./scripts/test           # all suites
./scripts/test mooneye   # mooneye only
./scripts/test gb        # blargg only
```

## What's Missing

- **APU / Audio** — Length counters only; no sweep, envelope, or wave generation
- **CGB support** — Color palettes, WRAM banking, double-speed mode registers
- **M-cycle timing** — Instructions execute atomically; memory access timing within instructions is not dot-accurate
- **OAM DMA / GDMA / HDMA** — No DMA transfer support
- **OAM scan corruption** — Mode 2 OAM buffer not modeled
- **MBC3 RTC clock** — Latch mechanism works but doesn't read system time
- **Debugger** — No step-by-step or breakpoint support yet

## Project Structure

```
src/
  emu.h          All types and declarations (single header)
  main.c         Entry point
  loop.c         Main emulation loop (60 Hz frame pacing)
  cpu.c          CPU initialization and step
  instr.c        Full opcode implementation
  bus.c          Memory bus, ROM loading, MBC1/2/3/5
  ppu.c          PPU state machine and scanline renderer
  timer.c        Timer hardware
  apu.c          Audio (minimal)
  inter.c        Interrupt handler
  memops.c       fetch8/fetch16 helpers
  frontend.h     Frontend interface (vtable)
  frontend_sdl.c SDL3 display and input
  opcodes.h      Auto-generated opcode tables
```

## License

MIT
