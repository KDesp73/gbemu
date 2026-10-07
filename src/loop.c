#include "gbemu.h"
#include <stdlib.h>
#include <time.h>

void gb_loop(gb_cpu* cpu, gb_bus* bus, gb_timer* timer, gb_ppu* ppu, gb_apu* apu, gb_frontend* fe)
{
    bool running = true;
    gb_debugger* dbg = fe ? fe->debug : NULL; // may be NULL: no debugger attached

    // Main Execution Loop
    while (running) {
        uint64_t frame_start_time = gb_get_time_ns();
        int frame_cycles = 0;

        while (frame_cycles < GB_CYCLES_PER_FRAME) {
            // Debugger: freeze before executing anything while paused.
            if (dbg && dbg->paused) break;

            // Breakpoint check. Skipped once after continue/step so execution
            // can leave the PC that triggered the breakpoint, and skipped while
            // halted (PC does not advance, which would re-trigger instantly).
            if (dbg && !cpu->halted) {
                if (dbg->skip_check_once) dbg->skip_check_once = false;
                else if (gb_debugger_hit(dbg, cpu->pc)) break;
            }

            // cpu_step advances the timer/PPU/APU itself at each M-cycle
            // boundary (via machine_tick), so bus accesses made by an
            // instruction land on their exact hardware cycle slots.
            int cycles = gb_cpu_step(cpu, bus);

            int scale = bus->double_speed ? 1 : 2;
            int sys_cycles = cycles * scale;

            // Interrupt dispatch advances the machine for its own M-cycles.
            int int_cycles = gb_handle_interrupts(cpu, bus, ppu, timer);
            if (int_cycles > 0) {
                sys_cycles += int_cycles * scale;
            }

            frame_cycles += sys_cycles;

            // A step request (Step/Step Over) runs its instructions within
            // this frame and re-pauses as soon as the count reaches zero.
            if (dbg && dbg->steps_remaining > 0 && --dbg->steps_remaining == 0) {
                dbg->paused = true;
            }
            if (dbg && dbg->paused) break;
        }

        // Keep presenting the last frame while paused so the game window
        // stays fresh (expose events, resize, etc).
        if (ppu->frame_ready || (dbg && dbg->paused)) {
            fe->render(fe, &ppu->frame_buffer[0][0], GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT);
            ppu->frame_ready = false;
        }

        fe->poll_events(fe, bus, &running);

        // Optional debugger UI: presented every iteration so it keeps
        // updating while the CPU is frozen.
        if (fe->render_debug) fe->render_debug(fe);

        uint64_t frame_duration = gb_get_time_ns() - frame_start_time;
#ifdef ESP_PLATFORM
        bool nosleep = false; // no host environment on embedded targets
#else
        bool nosleep = getenv("EMU_NOSLEEP") != NULL;
#endif
        if (!nosleep && frame_duration < GB_FRAME_TIME_NS) {
            gb_sleep_ns(GB_FRAME_TIME_NS - frame_duration);
        }
    }
}
