#include "emu.h"
#include <stdlib.h>
#include <time.h>

void loop(CPU* cpu, Bus* bus, Timer* timer, PPU* ppu, APU* apu)
{
    bool running = true;

    // Main Execution Loop
    while (running) {
        uint64_t frame_start_time = get_time_ns();
        int frame_cycles = 0;

        while (frame_cycles < CYCLES_PER_FRAME) {
            // if (cpu.pc == 0xC106) fprintf(stderr, "[CRC] byte=0x%02X\n", cpu.a);
            // if (cpu.pc == 0xC293) fprintf(stderr, "[CHECK] crc=%02X%02X%02X%02X\n", bus_read(&bus,0xFF80), bus_read(&bus,0xFF81), bus_read(&bus,0xFF82), bus_read(&bus,0xFF83));
            int cycles = cpu_step(cpu, bus);

            // On CGB the dot clock is fixed at 8.39 MHz: at normal speed the
            // CPU runs at half that (2 dots per T-cycle), at double speed the
            // CPU runs at the dot clock (1 dot per T-cycle).
            int scale = bus->double_speed ? 1 : 2;
            int sys_cycles = cycles * scale;

            // The timer is CPU-clock derived: it runs at the same rate in
            // T-cycles regardless of speed mode (so DIV/TIMA frequencies
            // double in real time in double-speed mode). The PPU and APU
            // are dot-clock derived: they advance at the fixed 8.39 MHz
            // CGB dot rate, so one T-cycle = two dots at normal speed and
            // one dot in double-speed mode.
            timer_step(timer, cycles);
            ppu_step(ppu, bus, sys_cycles);
            apu_step(apu, sys_cycles);

            int int_cycles = handle_interrupts(cpu, bus, ppu, timer);
            if (int_cycles > 0) {
                timer_step(timer, int_cycles);
                ppu_step(ppu, bus, int_cycles * scale);
                sys_cycles += int_cycles * scale;
            }

            frame_cycles += sys_cycles;
        }

        if (ppu->frame_ready) {
            // host_render_frame(ppu.frame_buffer);
            ppu->frame_ready = false;
        }

        uint64_t frame_duration = get_time_ns() - frame_start_time;
        if (!getenv("EMU_NOSLEEP") && frame_duration < FRAME_TIME_NS) {
            struct timespec sleep_time;
            sleep_time.tv_sec = 0;
            sleep_time.tv_nsec = (long)(FRAME_TIME_NS - frame_duration);
            nanosleep(&sleep_time, NULL);
        }
    }
}
