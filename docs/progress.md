# Emulator Progress Assessment

## Overall Completeness: ~60-65%

The core CPU and timer are complete and well-tested, but the emulator cannot yet
run a single commercial game: no MBC support, no display output, no joypad, and
no audio generation.

### Test Suite Results (52 tests)

- Passed: 28
- Failed: 18
- Partial/Timeout: 6
- **Pass rate: ~54%**

## Subsystem Breakdown

| Subsystem | Status | Completeness |
|---|---|---|
| CPU + instructions | All cpu_instrs, instr_timing, interrupt_time, halt_bug pass | ~98% |
| Timer | instr_timing / interrupt_time tests pass | ~95% |
| PPU / video | BG, window, sprites rendered to a buffer; **no display output** (`host_render_frame` commented out in `src/loop.c:46`); DMG palette only; no CGB VRAM banking (only 8KB) | ~60% |
| MBC / cartridges | **Not implemented** (`src/bus.c:35` — first 32KB only, no bank switching) | ~10% |
| Joypad / input | Not implemented | 0% |
| APU / audio | "no wave generation" (`src/emu.h:315`) — length-counter state only; sweep/overflow/wave tests fail | ~30% |
| Serial / interrupts | Serial intercepted for test ROMs, interrupts work | ~70% |

## Recommended Next Steps

1. **MBC1 ROM banking** (highest value)
   - Every commercial game exceeds the fixed 32KB array (`src/bus.c:7`, `:135`)
     and requires bank switching, so no real game can run until this lands.
   - Self-contained and well-documented: 2 ROM bank registers + 1 RAM
     enable/bank register.
   - Writes to ROM currently dead-end at `src/bus.c:34`.
   - Verify against MBC1 test ROMs (e.g. mooneye-gb `emulator-only/mbc1/*`).

2. **SDL display**
   - Hook `ppu.frame_buffer` to a window (`src/loop.c:46` is commented out)
     to finally see output and watch games boot.

3. **Joypad**
   - Read 0xFF00 (`P1`) so games become interactive.

4. **PPU timing fixes**
   - The failing `mem_timing` / `oam_bug` suites are PPU/DMA timing edge
     cases. Important for frame-accurate behavior, but only pay off once real
     software runs.

### Alternative: Continue Fixing Test Failures

If preferred over the above, the next failing suite is `mem_timing`
(edge-rising timing for ops like `F0`/`FA` and the `CB` register ops).
MBC1 remains the bigger win: it is the difference between "test ROMs only"
and "actually boots a game".
