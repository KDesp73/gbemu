# Emulator Progress Assessment

## Overall Completeness: ~60-65%

The core CPU and timer are complete and well-tested, MBC1/MBC2/MBC5 bank
switching works, but the emulator cannot yet run most commercial games: no MBC3
support, no display output, no joypad, and no audio generation.

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
| MBC / cartridges | MBC1 (+ multicart), MBC2, MBC5 implemented; MBC3 and others not yet | ~55% |
| Joypad / input | Not implemented | 0% |
| APU / audio | "no wave generation" (`src/emu.h:315`) — length-counter state only; sweep/overflow/wave tests fail | ~30% |
| Serial / interrupts | Serial intercepted for test ROMs, interrupts work | ~70% |

## Recommended Next Steps

1. **MBC3 support** (highest value for game compatibility)
   - MBC1, MBC2, and MBC5 are implemented. MBC3 (types 0x0F-0x13) is the
     next most common mapper used by commercial games (e.g. Pokémon, Zelda).
   - Needs: ROM bank select ($2000-$3FFF), RAM/RTC enable ($0000-$1FFF),
     RAM bank select ($4000-$5FFF), clock counter ($6000-$7FFF), RTC registers.

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

If preferred over the above, investigate remaining mooneye MBC2 failures
(`bits_ramg`, `ram`, `bits_unused`) and MBC5 timeout issues. Also consider
the `mem_timing` suite (edge-rising timing for ops like `F0`/`FA` and the `CB`
register ops). MBC3 remains the bigger win for commercial game compatibility.
