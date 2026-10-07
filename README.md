# gbemu

A Game Boy / Game Boy Color emulator written in C.

![recording](./assets/supermarioland-record.gif)

## Building

### Prerequisites

- **gcc** (or any C99 compiler)
- **g++** (only needed for the debugger's C++ sources)
- **SDL3** (`brew install sdl3`)
- **pkg-config** (for SDL3 detection)
- **git submodules** (ImGui + cimgui, for the debugger)

### Build

```bash
git submodule update --init   # once, for the debugger
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

#### Debugger

The SDL frontend builds with an ImGui debugger in a separate window when the
`third_party/imgui` and `third_party/cimgui` submodules are present. The
debugger is auto-enabled in that case; control it with:

```bash
make all DEBUGGER=0    # force off (no ImGui/C++ objects)
make all DEBUGGER=1    # force on (hard error if submodules are missing)
```

TERM (`TERM=1`) and HEADLESS (`HEADLESS=1`) builds never enable it. If the
submodules are missing, the build prints a warning and continues without the
debugger.

## Usage

```bash
./gbemu-cli path/to/rom.gb
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
| F5 | Save state |
| F9 | Load state |
| F1 | Toggle the debugger window |
| F6 | Pause / resume the debugger |

Close the window or press the window close button to quit.

The debugger window takes input only while it has focus, so game controls keep
working with the game window focused.

## Saves

| Type | File | Behavior |
|---|---|---|
| Battery save | `<rom>.sav` | Cartridge RAM persisted automatically on exit and restored on launch, for carts whose header declares a battery. MBC3 timer carts also persist the RTC registers; MBC2 carts persist the built-in 512x4-bit RAM |
| Save state | `<rom>.state` | Full machine snapshot (CPU, bus, timer, PPU, APU) via F5/F9. Versioned and ROM-hash-checked; states are invalidated when the emulator version or ROM changes |

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

### Debugger

- Separate ImGui window (SDL frontend only), toggled with F1
- Register + flag view (AF/BC/DE/HL/SP/PC, Z/N/H/C, IME, halted)
- Disassembly around PC with the current instruction highlighted
- Breakpoints: toggle from the disassembly view or add by hex address
- Step / Step Over / Continue / Pause (F6), step-over steps across CALLs
- Memory hex viewer with region selector and address goto

### Saves

- Battery-backed cartridge RAM persistence (`.sav`) for MBC1/MBC2/MBC3/MBC5 carts
- MBC3 RTC register persistence (standard 5-byte trailer)
- Full-machine save states (`.state`) with version + ROM hash validation (F5/F9)

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
