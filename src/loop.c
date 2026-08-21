#include "emu.h"
#include <stdlib.h>
#include <time.h>

void machine_tick(Bus* bus, int t_cycles)
{
    // The timer is CPU-clock derived: it runs at the same rate in
    // T-cycles regardless of speed mode (so DIV/TIMA frequencies
    // double in real time in double-speed mode). The PPU and APU
    // are dot-clock derived: they advance at the fixed 8.39 MHz
    // CGB dot rate, so one T-cycle = two dots at normal speed and
    // one dot in double-speed mode.
    int scale = bus->double_speed ? 1 : 2;
    bus_tick(bus); // OAM DMA advances one byte per M-cycle
    if (bus->timer) timer_step(bus->timer, t_cycles);
    if (bus->ppu) ppu_step(bus->ppu, bus, t_cycles * scale);
    if (bus->apu) apu_step(bus->apu, t_cycles * scale);
}

void loop(CPU* cpu, Bus* bus, Timer* timer, PPU* ppu, APU* apu, Frontend* fe)
{
    bool running = true;

    // Main Execution Loop
    while (running) {
        uint64_t frame_start_time = get_time_ns();
        int frame_cycles = 0;

        while (frame_cycles < CYCLES_PER_FRAME) {
            // cpu_step advances the timer/PPU/APU itself at each M-cycle
            // boundary (via machine_tick), so bus accesses made by an
            // instruction land on their exact hardware cycle slots.
            int cycles = cpu_step(cpu, bus);

            int scale = bus->double_speed ? 1 : 2;
            int sys_cycles = cycles * scale;

            // Interrupt dispatch advances the machine for its own M-cycles.
            int int_cycles = handle_interrupts(cpu, bus, ppu, timer);
            if (int_cycles > 0) {
                sys_cycles += int_cycles * scale;
            }

            frame_cycles += sys_cycles;
        }

        if (ppu->frame_ready) {
            fe->render(fe, &ppu->frame_buffer[0][0], SCREEN_WIDTH, SCREEN_HEIGHT);
            ppu->frame_ready = false;
        }

        fe->poll_events(fe, bus, &running);

        uint64_t frame_duration = get_time_ns() - frame_start_time;
        if (!getenv("EMU_NOSLEEP") && frame_duration < FRAME_TIME_NS) {
            struct timespec sleep_time;
            sleep_time.tv_sec = 0;
            sleep_time.tv_nsec = (long)(FRAME_TIME_NS - frame_duration);
            nanosleep(&sleep_time, NULL);
        }
    }
}
