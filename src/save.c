#include "emu.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//@module saves

#define STATE_MAGIC "EMU1"
#define PATH_MAX_LEN 1024

// Versioned save-state header. rom_hash guards against restoring a state
// captured from a different cartridge; sram_size must match the allocation
// bus_load_rom made for the current ROM.
typedef struct {
    char     magic[4];   // STATE_MAGIC
    uint32_t version;    // VERSION_HEX of the emulator that wrote the file
    uint32_t rom_hash;   // FNV-1a over the full (padded) ROM image
    uint32_t sram_size;  // Cartridge RAM size at capture time
} StateHeader;

// FNV-1a 32-bit hash: cheap, dependency-free content fingerprint
static uint32_t fnv1a(const uint8_t* data, size_t len)
{
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

// Cartridge types whose header declares a battery-backed RAM (or RTC)
static bool cart_has_battery(uint8_t type)
{
    switch (type) {
        case 0x03: // MBC1 + RAM + Battery
        case 0x06: // MBC2 (built-in battery)
        case 0x09: // ROM + RAM + Battery
        case 0x0F: // MBC3 + Timer + Battery
        case 0x10: // MBC3 + Timer + RAM + Battery
        case 0x13: // MBC3 + RAM + Battery
        case 0x1B: // MBC5 + RAM + Battery
        case 0x1E: // MBC5 + Rumble + RAM + Battery
            return true;
        default:
            return false;
    }
}

// MBC3 timer carts (RTC registers mapped at $08-$0C)
static bool cart_has_rtc(uint8_t type)
{
    return type == 0x0F || type == 0x10;
}

// MBC2 carts keep their 512x4-bit save RAM inside the controller
static bool cart_is_mbc2(uint8_t type)
{
    return type == 0x05 || type == 0x06;
}

// Pack the MBC3 RTC registers into the 5-byte layout other emulators use:
// sec, min, hr, day-low, day-high (bit0 = day MSB, bit6 = halt, bit7 = carry)
static void rtc_pack(const Bus* bus, uint8_t out[5])
{
    out[0] = bus->mbc3_seconds;
    out[1] = bus->mbc3_minutes;
    out[2] = bus->mbc3_hours;
    out[3] = (uint8_t)(bus->mbc3_day_counter & 0xFF);
    out[4] = (uint8_t)(((bus->mbc3_day_counter >> 8) & 0x01)
                       | (bus->mbc3_timer_halt ? 0x40 : 0x00)
                       | (bus->mbc3_day_carry ? 0x80 : 0x00));
}

static void rtc_unpack(Bus* bus, const uint8_t in[5])
{
    bus->mbc3_seconds     = in[0];
    bus->mbc3_minutes     = in[1];
    bus->mbc3_hours       = in[2];
    bus->mbc3_day_counter = (uint16_t)(in[3] | ((in[4] & 0x01) << 8));
    bus->mbc3_timer_halt  = (in[4] & 0x40) != 0;
    bus->mbc3_day_carry   = (in[4] & 0x80) != 0;
}

bool battery_load(Bus* bus, const char* rom_path)
{
    if (!bus->sram || bus->sram_size == 0) return false;

    char path[PATH_MAX_LEN];
    snprintf(path, sizeof(path), "%s.sav", rom_path);

    FILE* f = fopen(path, "rb");
    if (!f) return false;

    bool ok;
    if (cart_is_mbc2(bus->mbc_type)) {
        ok = fread(bus->mbc2_ram, 1, sizeof(bus->mbc2_ram), f) == sizeof(bus->mbc2_ram);
    } else {
        ok = fread(bus->sram, 1, bus->sram_size, f) == bus->sram_size;
    }

    if (ok && cart_has_rtc(bus->mbc_type)) {
        uint8_t rtc[5];
        if (fread(rtc, 1, sizeof(rtc), f) == sizeof(rtc)) rtc_unpack(bus, rtc);
    }

    fclose(f);

    if (!ok) {
        fprintf(stderr, "[ERR] Battery save is truncated: %s\n", path);
        return false;
    }

    printf("[INFO] Battery save loaded: %s\n", path);
    return true;
}

bool battery_save(const Bus* bus, const char* rom_path)
{
    // Only persist carts that declare a battery; this keeps test ROMs and
    // battery-less games from littering .sav files next to them.
    if (!cart_has_battery(bus->mbc_type)) return false;

    char path[PATH_MAX_LEN];
    snprintf(path, sizeof(path), "%s.sav", rom_path);

    FILE* f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "[ERR] Cannot write battery save: %s\n", path);
        return false;
    }

    size_t written;
    if (cart_is_mbc2(bus->mbc_type)) {
        written = fwrite(bus->mbc2_ram, 1, sizeof(bus->mbc2_ram), f);
    } else {
        written = fwrite(bus->sram, 1, bus->sram_size, f) ? bus->sram_size : 0;
    }

    if (cart_has_rtc(bus->mbc_type)) {
        uint8_t rtc[5];
        rtc_pack(bus, rtc);
        written += fwrite(rtc, 1, sizeof(rtc), f);
    }

    fclose(f);
    printf("[INFO] Battery save written: %s (%zu bytes)\n", path, written);
    return true;
}

// Field-wise writer/reader macros: keeps the Bus serialization explicit
// (pointers and heap buffers are deliberately skipped) while staying terse.
#define W(field) fwrite(&(field), sizeof(field), 1, f)
#define R(field) (fread(&(field), sizeof(field), 1, f) == 1)

bool save_state(const CPU* cpu, const Bus* bus, const Timer* timer,
                const PPU* ppu, const APU* apu, const char* rom_path)
{
    char path[PATH_MAX_LEN];
    snprintf(path, sizeof(path), "%s.state", rom_path);

    FILE* f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "[ERR] Cannot write save state: %s\n", path);
        return false;
    }

    StateHeader hdr = {0};
    memcpy(hdr.magic, STATE_MAGIC, sizeof(hdr.magic));
    hdr.version   = VERSION_HEX;
    hdr.rom_hash  = fnv1a(bus->rom, bus->rom_size);
    hdr.sram_size = (uint32_t)bus->sram_size;

    bool ok = W(hdr)
           && W(*cpu)
           && W(*timer)
           && fwrite(ppu, offsetof(PPU, frame_buffer), 1, f) == 1 // registers + state, pixels excluded
           && fwrite(apu, offsetof(APU, audio_buf), 1, f) == 1    // synthesis state, ring buffer excluded
        // Bus: everything except rom/sram buffers, component pointers and sram_dirty
           && W(bus->vram) && W(bus->wram) && W(bus->oam) && W(bus->io) && W(bus->hram) && W(bus->ie)
           && W(bus->dma_active) && W(bus->dma_start_delay) && W(bus->dma_src_high) && W(bus->dma_offset)
           && W(bus->dma_pending) && W(bus->dma_pend_delay) && W(bus->dma_pend_src)
           && W(bus->double_speed)
           && W(bus->joypad_buttons) && W(bus->joypad_dpad)
           && W(bus->mbc_type)
           && W(bus->mbc1_rom_bank) && W(bus->mbc1_ram_bank) && W(bus->mbc1_mode)
           && W(bus->mbc1_ram_enable) && W(bus->mbc1_multicart)
           && W(bus->mbc2_rom_bank) && W(bus->mbc2_ram_enable) && W(bus->mbc2_ram)
           && W(bus->mbc3_rom_bank) && W(bus->mbc3_ram_bank) && W(bus->mbc3_ram_enable)
           && W(bus->mbc3_latch_state) && W(bus->mbc3_seconds) && W(bus->mbc3_minutes)
           && W(bus->mbc3_hours) && W(bus->mbc3_day_counter) && W(bus->mbc3_day_carry)
           && W(bus->mbc3_timer_halt)
           && W(bus->mbc5_rom_bank_l) && W(bus->mbc5_rom_bank_h) && W(bus->mbc5_ram_bank)
           && W(bus->mbc5_ram_enable)
           && fwrite(bus->sram, 1, bus->sram_size, f) == bus->sram_size;

    if (fclose(f) != 0) ok = false;

    if (!ok) {
        fprintf(stderr, "[ERR] Save state write failed: %s\n", path);
        return false;
    }

    printf("[INFO] State saved: %s\n", path);
    return true;
}

bool load_state(CPU* cpu, Bus* bus, Timer* timer,
                PPU* ppu, APU* apu, const char* rom_path)
{
    char path[PATH_MAX_LEN];
    snprintf(path, sizeof(path), "%s.state", rom_path);

    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[ERR] No save state found: %s\n", path);
        return false;
    }

    StateHeader hdr;
    if (!R(hdr) || memcmp(hdr.magic, STATE_MAGIC, sizeof(hdr.magic)) != 0) {
        fprintf(stderr, "[ERR] Not a save state: %s\n", path);
        fclose(f);
        return false;
    }
    if (hdr.version != VERSION_HEX) {
        fprintf(stderr, "[ERR] Save state version mismatch: file %u, emulator %u\n",
                hdr.version, (uint32_t)VERSION_HEX);
        fclose(f);
        return false;
    }
    if (hdr.rom_hash != fnv1a(bus->rom, bus->rom_size)) {
        fprintf(stderr, "[ERR] Save state was captured from a different ROM\n");
        fclose(f);
        return false;
    }
    if (hdr.sram_size != bus->sram_size) {
        fprintf(stderr, "[ERR] Save state SRAM size mismatch: file %u, emulator %zu\n",
                hdr.sram_size, bus->sram_size);
        fclose(f);
        return false;
    }

    bool ok = R(*cpu)
           && R(*timer)
           && fread(ppu, offsetof(PPU, frame_buffer), 1, f) == 1
           && fread(apu, offsetof(APU, audio_buf), 1, f) == 1
           && R(bus->vram) && R(bus->wram) && R(bus->oam) && R(bus->io) && R(bus->hram) && R(bus->ie)
           && R(bus->dma_active) && R(bus->dma_start_delay) && R(bus->dma_src_high) && R(bus->dma_offset)
           && R(bus->dma_pending) && R(bus->dma_pend_delay) && R(bus->dma_pend_src)
           && R(bus->double_speed)
           && R(bus->joypad_buttons) && R(bus->joypad_dpad)
           && R(bus->mbc_type)
           && R(bus->mbc1_rom_bank) && R(bus->mbc1_ram_bank) && R(bus->mbc1_mode)
           && R(bus->mbc1_ram_enable) && R(bus->mbc1_multicart)
           && R(bus->mbc2_rom_bank) && R(bus->mbc2_ram_enable) && R(bus->mbc2_ram)
           && R(bus->mbc3_rom_bank) && R(bus->mbc3_ram_bank) && R(bus->mbc3_ram_enable)
           && R(bus->mbc3_latch_state) && R(bus->mbc3_seconds) && R(bus->mbc3_minutes)
           && R(bus->mbc3_hours) && R(bus->mbc3_day_counter) && R(bus->mbc3_day_carry)
           && R(bus->mbc3_timer_halt)
           && R(bus->mbc5_rom_bank_l) && R(bus->mbc5_rom_bank_h) && R(bus->mbc5_ram_bank)
           && R(bus->mbc5_ram_enable)
           && fread(bus->sram, 1, bus->sram_size, f) == bus->sram_size;

    fclose(f);

    if (!ok) {
        fprintf(stderr, "[ERR] Save state is truncated: %s\n", path);
        return false;
    }

    // The audio callback may have consumed samples while the state was being
    // restored; start the ring buffer empty so playback resumes cleanly.
    apu->buf_write = 0;
    apu->buf_read = 0;

    printf("[INFO] State loaded: %s\n", path);
    return true;
}

#undef W
#undef R
