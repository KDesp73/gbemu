# Emulator Progress Assessment

## Overall Completeness: ~70%

The core CPU, timer, and PPU scanline renderer are complete and well-tested.
MBC1/MBC2/MBC3/MBC5 bank switching is implemented. SDL3 display output,
joypad input, and OAM DMA are working. Commercial games boot and are
playable (Super Mario Land, Pokemon Blue), though timing edge cases remain.
No audio output yet.

### Test Suite Results

**mooneye-test-suite (97 tests)**

| Suite | Passed | Total | Rate |
|---|---|---|---|
| MBC1 emulator-only | 8 | 13 | 62% |
| MBC2 emulator-only | 7 | 7 | 100% |
| MBC5 emulator-only | 9 | 9 | 100% |
| Acceptance | 22 | 68 | 32% |
| **Total** | **46** | **97** | **~47%** |

**Blargg cpu_instrs: 9/9 pass**

## Subsystem Breakdown

| Subsystem | Status | Completeness |
|---|---|---|
| CPU + instructions | All cpu_instrs pass (9/9); halt bug implemented | ~98% |
| Timer | Core TIMA/TMA/TAC/DIV pass basic tests; div_trigger, reload edge cases fail | ~80% |
| PPU / video | BG, window, sprites rendered to scanline buffer; SDL3 display via frontend_vtable; DMG palette only; no CGB support | ~65% |
| MBC / cartridges | MBC1 (8/13), MBC2 (7/7), MBC3 (implemented, RTC stub), MBC5 (9/9) | ~80% |
| Joypad / input | P1 register (0xFF00) with select lines; SDL frontend maps arrows/Z/X/Backspace/Enter | 100% |
| OAM DMA | Basic DMA transfer (0xFF46) implemented; timing and source validation not tested | ~40% |
| APU / audio | Length-counter state only; no wave generation, no sweep, no envelope | ~30% |
| Serial / interrupts | Serial intercepted for test ROMs; all interrupt sources working | ~80% |

## Implemented Features

- **Full SM83 instruction set** — 256 base + 256 CB-prefixed opcodes
- **SDL3 display** — `frontend.h` vtable abstraction, `frontend_sdl.c` implementation
- **Joypad** — P1 register with active-low buttons, SDL key mapping
- **MBC1** — ROM/RAM banking, mode select, multicart detection
- **MBC2** — ROM banking, internal 4-bit RAM
- **MBC3** — ROM/RAM banking, RTC register stub (no clock source)
- **MBC5** — 9-bit ROM bank, 4-bit RAM bank
- **OAM DMA** — Transfer on write to 0xFF46
- **Boot state** — IF=0x01, IE=0x01, CPU A=0x01 for DMG, timer TAC internal 0x00

## MBC1 Failures

5 of 13 MBC1 mooneye tests still fail: `bits_bank2`, `bits_mode`, `bits_ramg`,
`ram_256kb`, `ram_64kb`. These test advanced MBC1 banking mode edge cases.

## Recommended Next Steps

1. **Fix EI delay + HALT interaction** (1 failing SameSuite test; could break
   interrupt-heavy commercial games)

2. **Fix MBC1 edge cases** (`bits_bank2`, `bits_mode`, `bits_ramg`, RAM tests)
   — 5 passable tests remaining

3. **PPU timing accuracy** — Many acceptance timing tests fail
   (`intr_2_mode0_timing`, `lcdon_timing`, `stat_irq_blocking`, etc.).
   The scanline renderer works for display but is not dot-accurate.

4. **Timer edge cases** — `div_trigger`, `tima_reload`, `tma_write_reloading`
   tests fail; the falling-edge counter has subtle timing bugs.

5. **Full APU implementation** — 70 SameSuite APU tests fail. Needed for
   sound output.

6. **CGB support** — Color palettes (BGPI/BGPD), GDMA/HDMA, double-speed mode.
