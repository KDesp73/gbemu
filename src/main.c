#include "emu.h"
#include <time.h>

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Please provide a rom\n");
        fprintf(stderr, "Usage: %s <ROM>\n", argv[0]);
        return 1;
    }
    const char* rom_path = argv[1];

    CPU cpu = {0};
    Bus bus = {0};
    Timer timer = {0};
    PPU ppu = {0};

    bus.timer = &timer;
    bus.ppu = &ppu;

    cpu_init(&cpu);
    timer_init(&timer);
    ppu_init(&ppu);

    if (!bus_load_rom(&bus, rom_path)) return 1;

    bool running = true;

    // Main Execution Loop
    while (running) {
        uint64_t frame_start_time = get_time_ns();
        int frame_cycles = 0;

        int step = 0;
        static int measured_count = 0;
        while (frame_cycles < CYCLES_PER_FRAME) {
            
            int cycles = cpu_step(&cpu, &bus);
            g_cycles += cycles;

            timer_step(&timer, cycles);
            ppu_step(&ppu, &bus, cycles);

            int int_cycles = handle_interrupts(&cpu, &bus, &ppu, &timer);
            if (int_cycles > 0) {
                g_cycles += int_cycles;
                timer_step(&timer, int_cycles);
                ppu_step(&ppu, &bus, int_cycles);
                cycles += int_cycles;
            }

            frame_cycles += cycles;
            step++;

            if (step > 100000 && step < 100400) {
                fprintf(stderr, "[STEP] %d PC=0x%04X A=0x%02X IME=%d IF=0x%02X IE=0x%02X TIMA=%d DIV=%d\n",
                    step, cpu.pc, cpu.a, cpu.ime, bus_read(&bus, 0xFF0F), bus.ie, timer.tima, (uint8_t)(timer.internal_counter >> 8));
            }
            if (cpu.pc == 0xC3C1) measured_count++;
            if (measured_count > 2 && measured_count < 4) {
                fprintf(stderr, "[MEASURE] %d PC=0x%04X A=0x%02X BC=0x%04X DE=0x%04X HL=0x%04X IME=%d IF=0x%02X IE=0x%02X TIMA=%d\n",
                    step, cpu.pc, cpu.a, cpu.bc, cpu.de, cpu.hl, cpu.ime, bus_read(&bus, 0xFF0F), bus.ie, timer.tima);
            }
        }

        if (ppu.frame_ready) {
            // host_render_frame(ppu.frame_buffer);
            ppu.frame_ready = false;
        }

        uint64_t frame_duration = get_time_ns() - frame_start_time;
        if (frame_duration < FRAME_TIME_NS) {
            struct timespec sleep_time;
            sleep_time.tv_sec = 0;
            sleep_time.tv_nsec = (long)(FRAME_TIME_NS - frame_duration);
            nanosleep(&sleep_time, NULL);
        }
    }

    return 0;
}
