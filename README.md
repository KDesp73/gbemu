# gbemu

A Game Boy / Game Boy Color emulator written in C.

![recording](./assets/supermarioland-record.gif)

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
| `gbemu-cli` | Executable |
| `libgbemu.a` | Static library (frontend-agnostic) |
| `libgbemu.so` | Shared library |

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

## License

[MIT](./LICENSE)
