#include "emu.h"

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
    APU apu = {0};

    bus.timer = &timer;
    bus.ppu = &ppu;
    bus.apu = &apu;

    cpu_init(&cpu);
    timer_init(&timer);
    ppu_init(&ppu);
    apu_init(&apu);

    if (!bus_load_rom(&bus, rom_path)) return 1;

    loop(&cpu, &bus, &timer, &ppu, &apu);

    return 0;
}
