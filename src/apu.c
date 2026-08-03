#include "emu.h"
#include <string.h>

// Minimal audio hardware: master power (NR52), channel on/off status,
// and length counters clocked by the length clock (256 Hz).
// No wave generation is performed.

#define LENGTH_CLOCK_PERIOD 32768 // Dots per 256 Hz length clock (CGB dot clock 8.39 MHz)

// NR10-NR51 occupy regs[0x00..0x15]; wave RAM is regs[0x20..0x2F] (FF30-FF3F).
#define NR_REG_COUNT 0x16 // FF10 through FF25 inclusive

// Unused bits read as 1 (OR'd with stored value). Indexed by (addr - 0xFF10).
static const uint8_t NR_READ_MASK[0x30] = {
    /* FF10-FF14 */ 0x80, 0x3F, 0x00, 0xFF, 0xBF,
    /* FF15-FF19 */ 0xFF, 0x3F, 0x00, 0xFF, 0xBF,
    /* FF1A-FF1E */ 0x7F, 0xFF, 0x9F, 0xFF, 0xBF,
    /* FF1F-FF23 */ 0xFF, 0xFF, 0x00, 0x00, 0xBF,
    /* FF24-FF25 */ 0x00, 0x00,
    /* FF26-FF2F */ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* FF30-FF3F wave RAM */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static bool is_wave_ram(uint16_t addr)
{
    return addr >= 0xFF30 && addr <= 0xFF3F;
}

// NRx1 length-load registers (writable on DMG even while APU is powered off).
static bool is_length_reg(uint16_t addr)
{
    return addr == 0xFF11 || addr == 0xFF16 || addr == 0xFF1B || addr == 0xFF20;
}

static int channel_from_reg(uint16_t addr)
{
    switch (addr) {
        case 0xFF11: return 0; // NR11
        case 0xFF16: return 1; // NR21
        case 0xFF1B: return 2; // NR31
        case 0xFF20: return 3; // NR41
        default:     return -1;
    }
}

static int volume_from_reg(uint16_t addr)
{
    switch (addr) {
        case 0xFF12: return 0; // NR12
        case 0xFF17: return 1; // NR22
        case 0xFF1A: return 2; // NR30 (DAC power for wave)
        case 0xFF21: return 3; // NR42
        default:     return -1;
    }
}

static int control_from_reg(uint16_t addr)
{
    switch (addr) {
        case 0xFF14: return 0; // NR14
        case 0xFF19: return 1; // NR24
        case 0xFF1E: return 2; // NR34
        case 0xFF23: return 3; // NR44
        default:     return -1;
    }
}

// Square/noise DAC is off when (NRx2 & 0xF8) == 0; wave DAC uses NR30 bit 7.
static bool dac_enabled(const APU* apu, int ch)
{
    switch (ch) {
        case 0: return (apu->regs[0x02] & 0xF8) != 0; // NR12
        case 1: return (apu->regs[0x07] & 0xF8) != 0; // NR22
        case 2: return (apu->regs[0x0A] & 0x80) != 0; // NR30
        case 3: return (apu->regs[0x11] & 0xF8) != 0; // NR42
        default: return false;
    }
}

static void apu_clock_length(APU* apu, int ch)
{
    if (!apu->length_enable[ch] || apu->length[ch] == 0) return;
    if (--apu->length[ch] == 0) {
        apu->ch_on[ch] = false;
    }
}

static void apu_power_off(APU* apu)
{
    apu->power = false;

    // Powering off instantly clears NR10-NR51. Wave RAM is unaffected.
    // DMG: internal length counters survive, but the readable register
    // bytes still clear (length bits are write-only on read-back).
    memset(apu->regs, 0, NR_REG_COUNT);

    for (int i = 0; i < 4; i++) {
        apu->ch_on[i] = false;
        apu->length_enable[i] = false;
        if (apu->cgb) {
            apu->length[i] = 0;
            apu->length_load[i] = 0;
        }
        // DMG: leave length[] / length_load[] as-is
    }
}

void apu_init(APU* apu)
{
    apu->power = true;
}

void apu_step(APU* apu, int dots)
{
    if (!apu->power) return;

    // The length clock runs off the fixed 8.388608 MHz CGB dot clock, so
    // the length-counter tick period (32768 dots) is constant in real time.
    apu->frame_accum += (uint16_t)dots;

    while (apu->frame_accum >= LENGTH_CLOCK_PERIOD) {
        apu->frame_accum -= LENGTH_CLOCK_PERIOD;

        // Length clocks whenever enabled, even if the channel is already off.
        for (int i = 0; i < 4; i++) {
            apu_clock_length(apu, i);
        }
    }
}

uint8_t apu_read(const APU* apu, uint16_t addr)
{
    if (addr == 0xFF26) { // NR52
        // Bits 4-6 always read as 1
        if (!apu->power) return 0x70;
        return 0xF0
            | (apu->ch_on[0] ? 0x01 : 0x00)
            | (apu->ch_on[1] ? 0x02 : 0x00)
            | (apu->ch_on[2] ? 0x04 : 0x00)
            | (apu->ch_on[3] ? 0x08 : 0x00);
    }
    return apu->regs[addr - 0xFF10] | NR_READ_MASK[addr - 0xFF10];
}

void apu_write(APU* apu, uint16_t addr, uint8_t value)
{
    if (addr == 0xFF26) { // NR52
        bool on = (value & 0x80) != 0;
        if (!on && apu->power) {
            apu_power_off(apu);
        } else if (on && !apu->power) {
            apu->power = true;
            // Powering on resets the frame sequencer so the next step is 0.
            apu->frame_accum = 0;
        }
        return;
    }

    // While powered off, NR10-NR51 are read-only (wave RAM stays writable).
    // DMG exception: NRx1 updates the internal length counter only.
    if (!apu->power && !is_wave_ram(addr)) {
        if (apu->cgb || !is_length_reg(addr)) {
            return;
        }
        // DMG: apply length load without storing to the readable register.
        int ch = channel_from_reg(addr);
        if (ch == 2) {
            apu->length_load[ch] = 256 - value;
        } else {
            apu->length_load[ch] = 64 - (value & 0x3F);
        }
        apu->length[ch] = apu->length_load[ch];
        return;
    }

    apu->regs[addr - 0xFF10] = value;

    int ch = volume_from_reg(addr);
    if (ch >= 0) {
        // Disabling a channel's DAC immediately clears its NR52 active bit.
        if (!dac_enabled(apu, ch)) {
            apu->ch_on[ch] = false;
        }
        return;
    }

    ch = channel_from_reg(addr);
    if (ch >= 0) {
        // NRx1: load length counter (64 - low 6 bits); wave uses 256 - n8
        if (ch == 2) {
            apu->length_load[ch] = 256 - value;
        } else {
            apu->length_load[ch] = 64 - (value & 0x3F);
        }
        apu->length[ch] = apu->length_load[ch];
        return;
    }

    ch = control_from_reg(addr);
    if (ch >= 0) {
        // NRx4: length enable (bit 6) and trigger (bit 7)
        bool was_enabled = apu->length_enable[ch];
        bool now_enabled = (value & 0x40) != 0;
        bool first_half = apu->frame_accum < (LENGTH_CLOCK_PERIOD / 2);
        apu->length_enable[ch] = now_enabled;

        // Extra clock: enabling length (0→1) in the first half of the period
        if (!was_enabled && now_enabled && apu->power && first_half) {
            apu_clock_length(apu, ch);
        }

        if (value & 0x80) {
            // Trigger only enables the channel if its DAC is on.
            bool unfroze = (apu->length[ch] == 0);
            if (dac_enabled(apu, ch)) {
                apu->ch_on[ch] = true;
            } else {
                apu->ch_on[ch] = false;
            }
            // Trigger unfreezes a zeroed length counter to maximum
            if (unfroze) {
                apu->length[ch] = (ch == 2) ? 256 : 64;
            }
            // Unfreezing with length enabled in the first half clocks once
            if (unfroze && apu->length_enable[ch] && apu->power && first_half) {
                apu_clock_length(apu, ch);
            }
            if (apu->length_enable[ch] && apu->length[ch] == 0) {
                apu->ch_on[ch] = false;
            }
        }
        return;
    }
}
