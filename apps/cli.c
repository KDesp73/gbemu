#include "gbemu.h"
#include <stdlib.h>

gb_frontend* frontend_sdl_create(gb_apu* apu);
gb_frontend* frontend_headless_create(void);
gb_frontend* frontend_terminal_create(void);

// State the hotkey handler needs access to
typedef struct {
    gb_cpu* cpu;
    gb_bus* bus;
    gb_timer* timer;
    gb_ppu* ppu;
    gb_apu* apu;
    const char* rom_path;
} EmuContext;

static void on_hotkey(void* userdata, gb_hotkey key)
{
    EmuContext* ctx = userdata;
    switch (key) {
    case GB_HOTKEY_SAVE_STATE:
        gb_save_state(ctx->cpu, ctx->bus, ctx->timer, ctx->ppu, ctx->apu, ctx->rom_path);
        break;
    case GB_HOTKEY_LOAD_STATE:
        gb_load_state(ctx->cpu, ctx->bus, ctx->timer, ctx->ppu, ctx->apu, ctx->rom_path);
        break;
    }
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Please provide a rom\n");
        fprintf(stderr, "Usage: %s <ROM>\n", argv[0]);
        return 1;
    }
    const char* rom_path = argv[1];

    gb_cpu cpu = {0};
    gb_bus bus = {0};
    gb_timer timer = {0};
    gb_ppu ppu = {0};
    gb_apu apu = {0};

    bus.timer = &timer;
    bus.ppu = &ppu;
    bus.apu = &apu;

    gb_cpu_init(&cpu);
    gb_timer_init(&timer);
    gb_ppu_init(&ppu);
    gb_apu_init(&apu);

    if (!gb_bus_load_rom(&bus, rom_path)) return 1;

    // Restore cartridge RAM from a previous session (<rom>.sav)
    gb_battery_load(&bus, rom_path);

    // Post-boot register state
    bus.io[0x0F] = 0x01; // IF: VBlank pending from last scanline of boot ROM
    bus.ie = 0x00;        // IE: no interrupt sources enabled after boot
    bus.joypad_buttons = 0x0F; // All face buttons released (active-low)
    bus.joypad_dpad = 0x0F;    // All D-pad buttons released (active-low)

    // Set CPU mode based on ROM header
    if (bus.rom[0x143] != 0x80 && bus.rom[0x143] != 0xC0)
        cpu.a = 0x01; // DMG mode

    gb_frontend* fe;
#ifdef EMU_TERM
    fe = frontend_terminal_create();
#elif defined(EMU_HEADLESS)
    fe = frontend_headless_create();
#else
    fe = frontend_sdl_create(&apu);
#endif

    if (!fe->init(fe, GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT)) return 1;

    EmuContext ctx = {
        .cpu = &cpu,
        .bus = &bus,
        .timer = &timer,
        .ppu = &ppu,
        .apu = &apu,
        .rom_path = rom_path,
    };
    fe->hotkey_ctx = &ctx;
    fe->on_hotkey = on_hotkey;

    gb_loop(&cpu, &bus, &timer, &ppu, &apu, fe);

    // Persist cartridge RAM for the next session (<rom>.sav)
    gb_battery_save(&bus, rom_path);

    fe->destroy(fe);
    return 0;
}
