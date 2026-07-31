#include "emu.h"

// Minimal audio hardware: master power (NR52), channel on/off status,
// and length counters clocked by the length clock (256 Hz).
// No wave generation is performed.

#define LENGTH_CLOCK_PERIOD 32768 // Dots per 256 Hz length clock (CGB dot clock 8.39 MHz)

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

void apu_init(APU* apu)
{
    apu->power = true;
}

void apu_step(APU* apu, int dots)
{
    // The length clock runs off the fixed 8.388608 MHz CGB dot clock, so
    // the length-counter tick period (32768 dots) is constant in real time.
    apu->frame_accum += (uint16_t)dots;

    while (apu->frame_accum >= LENGTH_CLOCK_PERIOD) {
        apu->frame_accum -= LENGTH_CLOCK_PERIOD;

        // Length counter: one decrement per length clock tick
        for (int i = 0; i < 4; i++) {
            if (apu->ch_on[i] && apu->length_enable[i]) {
                if (apu->length[i] > 0) apu->length[i]--;
                if (apu->length[i] == 0) {
                    apu->ch_on[i] = false;
                }
            }
        }
    }
}

uint8_t apu_read(const APU* apu, uint16_t addr)
{
    if (addr == 0xFF26) { // NR52
        if (!apu->power) return 0x00;
        return 0x80
            | (apu->ch_on[0] ? 0x01 : 0x00)
            | (apu->ch_on[1] ? 0x02 : 0x00)
            | (apu->ch_on[2] ? 0x04 : 0x00)
            | (apu->ch_on[3] ? 0x08 : 0x00);
    }
    return apu->regs[addr - 0xFF10];
}

void apu_write(APU* apu, uint16_t addr, uint8_t value)
{
    if (addr == 0xFF26) { // NR52
        if (!(value & 0x80)) {
            apu->power = false;
            for (int i = 0; i < 4; i++) apu->ch_on[i] = false;
        } else {
            apu->power = true;
        }
        return;
    }

    apu->regs[addr - 0xFF10] = value;

    int ch = channel_from_reg(addr);
    if (ch >= 0) {
        // NRx1: load length counter (64 - low 6 bits)
        apu->length_load[ch] = 64 - (value & 0x3F);
        apu->length[ch] = apu->length_load[ch];
        return;
    }

    ch = control_from_reg(addr);
    if (ch >= 0) {
        // NRx4: length enable (bit 6) and trigger (bit 7)
        apu->length_enable[ch] = (value & 0x40) != 0;
        if (value & 0x80) {
            apu->ch_on[ch] = true;
            apu->length[ch] = apu->length_load[ch];
            if (apu->length_enable[ch] && apu->length[ch] == 0) {
                apu->ch_on[ch] = false;
            }
        }
        return;
    }
}
