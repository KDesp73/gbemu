#include "emu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <emscripten.h>

static CPU cpu;
static Bus bus;
static Timer timer;
static PPU ppu;
static APU apu;
static Frontend* fe;
static bool emu_running = false;

EMSCRIPTEN_KEEPALIVE
void wasm_set_key(int key, int pressed)
{
    bool p = pressed != 0;
    switch (key) {
    case 37: bus.joypad_dpad    = p ? (bus.joypad_dpad    & ~0x02) : (bus.joypad_dpad    | 0x02); break;
    case 38: bus.joypad_dpad    = p ? (bus.joypad_dpad    & ~0x04) : (bus.joypad_dpad    | 0x04); break;
    case 39: bus.joypad_dpad    = p ? (bus.joypad_dpad    & ~0x01) : (bus.joypad_dpad    | 0x01); break;
    case 40: bus.joypad_dpad    = p ? (bus.joypad_dpad    & ~0x08) : (bus.joypad_dpad    | 0x08); break;
    case 90: bus.joypad_buttons = p ? (bus.joypad_buttons & ~0x01) : (bus.joypad_buttons | 0x01); break;
    case 88: bus.joypad_buttons = p ? (bus.joypad_buttons & ~0x02) : (bus.joypad_buttons | 0x02); break;
    case  8: bus.joypad_buttons = p ? (bus.joypad_buttons & ~0x04) : (bus.joypad_buttons | 0x04); break;
    case 13: bus.joypad_buttons = p ? (bus.joypad_buttons & ~0x08) : (bus.joypad_buttons | 0x08); break;
    }
}

static void init_emu(void)
{
    memset(&cpu, 0, sizeof(cpu));
    memset(&bus, 0, sizeof(bus));
    memset(&timer, 0, sizeof(timer));
    memset(&ppu, 0, sizeof(ppu));
    memset(&apu, 0, sizeof(apu));

    bus.timer = &timer;
    bus.ppu = &ppu;
    bus.apu = &apu;

    cpu_init(&cpu);
    timer_init(&timer);
    ppu_init(&ppu);
    apu_init(&apu);
}

EMSCRIPTEN_KEEPALIVE
void wasm_load_rom(void)
{
    init_emu();

    if (!bus_load_rom(&bus, "/rom.gb")) {
        fprintf(stderr, "Failed to load ROM\n");
        emu_running = false;
        return;
    }

    bus.io[0x0F] = 0x01;
    bus.ie = 0x01;
    bus.joypad_buttons = 0x0F;
    bus.joypad_dpad = 0x0F;

    if (bus.rom[0x143] != 0x80 && bus.rom[0x143] != 0xC0)
        cpu.a = 0x01;

    emu_running = true;
}

EMSCRIPTEN_KEEPALIVE
int wasm_is_running(void)
{
    return emu_running ? 1 : 0;
}

static void main_loop(void)
{
    if (!emu_running) return;

    int frame_cycles = 0;
    while (frame_cycles < CYCLES_PER_FRAME) {
        int cycles = cpu_step(&cpu, &bus);
        int scale = bus.double_speed ? 1 : 2;
        int sys_cycles = cycles * scale;

        timer_step(&timer, cycles);
        ppu_step(&ppu, &bus, sys_cycles);
        apu_step(&apu, sys_cycles);

        int int_cycles = handle_interrupts(&cpu, &bus, &ppu, &timer);
        if (int_cycles > 0) {
            timer_step(&timer, int_cycles);
            ppu_step(&ppu, &bus, int_cycles * scale);
            sys_cycles += int_cycles * scale;
        }

        frame_cycles += sys_cycles;
    }

    if (ppu.frame_ready) {
        fe->render(fe, &ppu.frame_buffer[0][0], SCREEN_WIDTH, SCREEN_HEIGHT);
        ppu.frame_ready = false;
    }
}

int main(int argc, char** argv)
{
    fe = frontend_wasm_create();
    if (!fe->init(fe, SCREEN_WIDTH, SCREEN_HEIGHT)) {
        fprintf(stderr, "Failed to initialize frontend\n");
        return 1;
    }

    FILE* f = fopen("/rom.gb", "rb");
    if (f) {
        fclose(f);
        wasm_load_rom();
    }

    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;
}
