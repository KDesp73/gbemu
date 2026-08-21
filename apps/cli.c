#include "emu.h"
#include <stdlib.h>

Frontend* frontend_sdl_create(APU* apu);
Frontend* frontend_headless_create(void);

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

    // Post-boot register state
    bus.io[0x0F] = 0x01; // IF: VBlank pending from last scanline of boot ROM
    bus.ie = 0x00;        // IE: no interrupt sources enabled after boot
    bus.joypad_buttons = 0x0F; // All face buttons released (active-low)
    bus.joypad_dpad = 0x0F;    // All D-pad buttons released (active-low)

    // Set CPU mode based on ROM header
    if (bus.rom[0x143] != 0x80 && bus.rom[0x143] != 0xC0)
        cpu.a = 0x01; // DMG mode

    Frontend* fe;
#ifdef EMU_HEADLESS
    fe = frontend_headless_create();
#else
    fe = frontend_sdl_create(&apu);
#endif

    if (!fe->init(fe, SCREEN_WIDTH, SCREEN_HEIGHT)) return 1;

    loop(&cpu, &bus, &timer, &ppu, &apu, fe);

    fe->destroy(fe);
    return 0;
}
