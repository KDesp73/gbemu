#include "emu.h"
#include <string.h>

// Game Boy APU: full 4-channel waveform synthesis
// ref: https://gbdev.io/pandocs/Audio.html

// ---------- Lookup tables ----------

// Duty cycle bit patterns for square channels (8-bit shift patterns).
//   duty 0: 12.5% high -> 00000001
//   duty 1: 25%   high -> 10000001
//   duty 2: 50%   high -> 10000111
//   duty 3: 75%   high -> 01111110
static const uint8_t DUTY_TABLE[4] = { 0x01, 0x81, 0x87, 0x7E };

// Noise LFSR divisor lookup (index from NR43 low 3 bits)
static const int NOISE_DIVISOR[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };

// Unused bits in register reads (OR mask). Indexed by (addr - 0xFF10).
static const uint8_t NR_READ_MASK[0x30] = {
    /* FF10-FF14 */ 0x80, 0x3F, 0x00, 0xFF, 0xBF,
    /* FF15-FF19 */ 0xFF, 0x3F, 0x00, 0xFF, 0xBF,
    /* FF1A-FF1E */ 0x7F, 0xFF, 0x9F, 0xFF, 0xBF,
    /* FF1F-FF23 */ 0xFF, 0xFF, 0x00, 0x00, 0xBF,
    /* FF24-FF25 */ 0x00, 0x00,
    /* FF26-FF2F */ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* FF30-FF3F */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// ---------- Helpers ----------

static bool is_wave_ram(uint16_t addr)
{
    return addr >= 0xFF30 && addr <= 0xFF3F;
}

// DAC enabled for square/noise when (NRx2 & 0xF8) != 0; wave uses NR30 bit 7.
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

// Frequency timer reload value from 11-bit frequency register (NRx3|NRx4 high bits)
// For noise channel (ch==3), uses NOISE_DIVISOR instead.
static int freq_timer_reload(const APU* apu, int ch)
{
    uint16_t freq;
    switch (ch) {
        case 0: freq = apu->regs[0x03] | ((uint16_t)(apu->regs[0x04] & 0x07) << 8); break;
        case 1: freq = apu->regs[0x08] | ((uint16_t)(apu->regs[0x09] & 0x07) << 8); break;
        case 2: freq = apu->regs[0x0D] | ((uint16_t)(apu->regs[0x0E] & 0x07) << 8); break;
        case 3: return NOISE_DIVISOR[apu->regs[0x1F] & 0x07] * 4; // NR43 low 3 bits
        default: freq = 0; break;
    }
    return (2048 - freq) * 4;
}

// Square channel output: returns 0 or current volume (0-15)
static int square_output(const APU* apu, int ch)
{
    if (!apu->ch_on[ch]) return 0;
    int duty_idx = (apu->regs[ch == 0 ? 0x01 : 0x06] >> 6) & 0x03;
    bool high = (DUTY_TABLE[duty_idx] >> apu->duty_pos[ch]) & 1;
    return high ? apu->vol[ch] : 0;
}

// Wave channel output: reads from wave RAM, returns 0-15 attenuated by volume shift
static int wave_output(const APU* apu)
{
    if (!apu->ch_on[2]) return 0;
    // Wave RAM byte: each byte holds two 4-bit samples (high nibble first)
    uint8_t byte = apu->regs[0x20 + (apu->wave_pos / 2)];
    int sample = (apu->wave_pos & 1) ? (byte & 0x0F) : (byte >> 4);
    // Volume shift: NR30 bits 5-4 (0=mute, 1=100%, 2=50%, 3=25%)
    int shift = (apu->regs[0x0B] >> 5) & 0x03;
    return shift ? (sample >> (shift - 1)) : 0;
}

// Noise channel output: LFSR bit 0 determines output
static int noise_output(const APU* apu)
{
    if (!apu->ch_on[3]) return 0;
    return (~apu->lfsr & 1) ? apu->vol[2] : 0;
}

// ---------- Frame sequencer ----------

// Steps at 512 Hz (every 2048 T-cycles from the 2 MHz frame counter):
//   0: length
//   1: (nothing)
//   2: length + sweep
//   3: (nothing)
//   4: length
//   5: (nothing)
//   6: length + sweep
//   7: envelope
static void fs_step_length(APU* apu)
{
    for (int i = 0; i < 4; i++) {
        if (apu->length_enable[i] && apu->length[i] > 0) {
            apu->length[i]--;
            if (apu->length[i] == 0)
                apu->ch_on[i] = false;
        }
    }
}

static void fs_step_sweep(APU* apu)
{
    if (apu->sweep_counter > 0)
        apu->sweep_counter--;
    else {
        apu->sweep_counter = apu->sweep_period;
        if (apu->sweep_enabled && apu->sweep_period != 0) {
            uint16_t new_freq = apu->sweep_freq + (apu->sweep_negate
                ? -(apu->sweep_freq >> apu->regs[0x10])
                : (apu->sweep_freq >> apu->regs[0x10]));
            if (new_freq > 2047) {
                apu->ch_on[0] = false;
                apu->sweep_enabled = false;
            } else if ((apu->regs[0x10] & 0x07) != 0 && apu->sweep_enabled) {
                // Write new frequency back to NR13/NR14 shadow
                apu->regs[0x03] = new_freq & 0xFF;
                apu->regs[0x04] = (apu->regs[0x04] & 0xF8) | ((new_freq >> 8) & 0x07);
                apu->sweep_freq = new_freq;
                // Second overflow check after rewrite
                new_freq = apu->sweep_freq + (apu->sweep_negate
                    ? -(apu->sweep_freq >> apu->regs[0x10])
                    : (apu->sweep_freq >> apu->regs[0x10]));
                if (new_freq > 2047)
                    apu->ch_on[0] = false;
            }
            // Recalculate shadow for subsequent sub-steps
            apu->sweep_freq = apu->regs[0x03] | ((uint16_t)(apu->regs[0x04] & 0x07) << 8);
        }
    }
}

static void fs_step_envelope(APU* apu)
{
    for (int i = 0; i < 3; i++) {
        if (apu->env_period[i] == 0) continue;
        if (apu->env_counter[i] > 0) {
            apu->env_counter[i]--;
            continue;
        }
        apu->env_counter[i] = apu->env_period[i];
        if (apu->env_dir[i]) {
            if (apu->vol[i] < 15) apu->vol[i]++;
        } else {
            if (apu->vol[i] > 0) apu->vol[i]--;
        }
    }
}

// ---------- Ring buffer (SPSC) ----------

static inline void buf_push(APU* apu, float sample)
{
    uint32_t next = (apu->buf_write + 1) & APU_BUF_MASK;
    if (next != apu->buf_read) {
        apu->audio_buf[apu->buf_write] = sample;
        apu->buf_write = next;
    }
}

// Public: called from SDL audio callback
float apu_buf_pop(APU* apu)
{
    if (apu->buf_read == apu->buf_write)
        return 0.0f;
    float s = apu->audio_buf[apu->buf_read];
    apu->buf_read = (apu->buf_read + 1) & APU_BUF_MASK;
    return s;
}

// ---------- Core ----------

void apu_init(APU* apu)
{
    memset(apu, 0, sizeof(*apu));
    apu->power = true;
    apu->lfsr = 0x7FFF;
    // Default sweep period/shift = 0; wave RAM stays zeroed
    memset(apu->regs, 0, sizeof(apu->regs));
    // NR52 power-on default: 0xF1 (all bits 4-6 high, bit 0 high on DMG)
    apu->regs[0x16] = 0xF1; // NR52 at offset 0x16
    apu->regs[0x20] = 0xFF; // NR11 at offset 0x01 default (duty 50%, full length)
    apu->regs[0x25] = 0xFF; // NR51 pan default: all channels to both L/R
}

void apu_step(APU* apu, int dots)
{
    if (!apu->power) return;

    // --- Per-dot: frequency timers ---
    for (int i = 0; i < 4; i++) {
        if (!apu->ch_on[i]) continue;
        apu->freq_timer[i] -= dots;
        while (apu->freq_timer[i] <= 0) {
            int reload = freq_timer_reload(apu, i);
            apu->freq_timer[i] += reload > 0 ? reload : 1;

            if (i <= 1) {
                // Square: advance duty position
                apu->duty_pos[i] = (apu->duty_pos[i] + 1) & 7;
            } else if (i == 2) {
                // Wave: advance position
                apu->wave_pos = (apu->wave_pos + 1) & 31;
            } else {
                // Noise: clock LFSR
                uint8_t xor_result = (apu->lfsr & 1) ^ ((apu->lfsr >> 1) & 1);
                apu->lfsr = (apu->lfsr >> 1) | (xor_result << 14);
                bool width_mode = (apu->regs[0x1F] & 0x08) != 0; // NR43 bit 3
                if (width_mode) {
                    apu->lfsr &= ~(1 << 6);
                    apu->lfsr |= (xor_result << 6);
                }
            }
        }
    }

    // --- Frame sequencer ---
    apu->frame_div += (uint16_t)dots;
    while (apu->frame_div >= 2048) {
        apu->frame_div -= 2048;
        switch (apu->frame_step) {
            case 0: case 2: case 4: case 6: fs_step_length(apu); break;
            default: break;
        }
        switch (apu->frame_step) {
            case 2: case 6: fs_step_sweep(apu); break;
            default: break;
        }
        if (apu->frame_step == 7) fs_step_envelope(apu);
        apu->frame_step = (apu->frame_step + 1) & 7;
    }

    // --- Downsampling: produce one sample every CPU_FREQ/SAMPLE_RATE dots ---
    apu->sample_accum += (uint32_t)dots;
    uint32_t dots_per_sample = CPU_FREQ / APU_SAMPLE_RATE;
    while (apu->sample_accum >= dots_per_sample) {
        apu->sample_accum -= dots_per_sample;

        // Mix all channels
        float ch[4];
        ch[0] = (float)square_output(apu, 0) / 15.0f;
        ch[1] = (float)square_output(apu, 1) / 15.0f;
        ch[2] = (float)wave_output(apu)    / 15.0f;
        ch[3] = (float)noise_output(apu)   / 15.0f;

        // NR51 panning (byte at 0xFF25 = regs[0x15])
        uint8_t pan = apu->regs[0x15];
        float left = 0.0f, right = 0.0f;
        for (int i = 0; i < 4; i++) {
            if (pan & (1 << i))       right += ch[i];
            if (pan & (1 << (i + 4))) left  += ch[i];
        }

        // Master volume (NR50 bits 6-4 = left, bits 2-0 = right, 0 = mute)
        uint8_t nr50 = apu->regs[0x14];
        float vol_l = (float)((nr50 >> 4) & 0x07) / 7.0f;
        float vol_r = (float)(nr50 & 0x07) / 7.0f;

        float sample = ((left * vol_l) + (right * vol_r)) / 8.0f;

        // Clamp to [-1.0, 1.0] and push to ring buffer
        if (sample > 1.0f)  sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        buf_push(apu, sample);
    }
}

// ---------- Register read ----------

uint8_t apu_read(const APU* apu, uint16_t addr)
{
    if (addr == 0xFF26) { // NR52
        if (!apu->power) return 0x70; // bits 4-6 always high
        return 0xF0
            | (apu->ch_on[0] ? 0x01 : 0x00)
            | (apu->ch_on[1] ? 0x02 : 0x00)
            | (apu->ch_on[2] ? 0x04 : 0x00)
            | (apu->ch_on[3] ? 0x08 : 0x00);
    }
    if (addr >= 0xFF30 && addr <= 0xFF3F) return apu->regs[addr - 0xFF10];
    return apu->regs[addr - 0xFF10] | NR_READ_MASK[addr - 0xFF10];
}

// ---------- Register write ----------

static void apu_power_off(APU* apu)
{
    apu->power = false;
    memset(apu->regs, 0, 0x16); // NR10-NR51
    for (int i = 0; i < 4; i++) {
        apu->ch_on[i] = false;
        apu->length_enable[i] = false;
        if (apu->cgb) {
            apu->length[i] = 0;
            apu->length_load[i] = 0;
        }
        apu->freq_timer[i] = 0;
        apu->duty_pos[i] = 0;
    }
    apu->wave_pos = 0;
    apu->lfsr = 0x7FFF;
    apu->frame_step = 0;
    apu->frame_div = 0;
}

// Trigger event for a channel (bit 7 of NRx4)
static void trigger_channel(APU* apu, int ch)
{
    apu->ch_on[ch] = true;

    // Reload frequency timer
    apu->freq_timer[ch] = freq_timer_reload(apu, ch);

    // Reload envelope volume
    apu->vol[ch] = (apu->regs[ch == 0 ? 0x02 : ch == 1 ? 0x07 : 0x11] >> 4) & 0x0F;

    // Reset envelope counter
    apu->env_counter[ch == 2 ? 0 : (ch == 3 ? 2 : ch)] = apu->env_period[ch == 2 ? 0 : (ch == 3 ? 2 : ch)];

    switch (ch) {
    case 0: // Square 1: reset sweep state
        apu->duty_pos[0] = 0;
        apu->sweep_freq = apu->regs[0x03] | ((uint16_t)(apu->regs[0x04] & 0x07) << 8);
        apu->sweep_period = (apu->regs[0x10] >> 4) & 0x07;
        apu->sweep_counter = apu->sweep_period;
        apu->sweep_negate = (apu->regs[0x10] & 0x08) != 0;
        apu->sweep_negate_used = false;
        apu->sweep_enabled = apu->sweep_period != 0 || (apu->regs[0x10] & 0x07) != 0;
        // Frequency shadow reload -> immediate overflow check
        if (apu->sweep_enabled && (apu->regs[0x10] & 0x07) != 0) {
            uint16_t new_freq = apu->sweep_freq + (apu->sweep_negate
                ? -(apu->sweep_freq >> (apu->regs[0x10] & 0x07))
                : (apu->sweep_freq >> (apu->regs[0x10] & 0x07)));
            if (new_freq > 2047)
                apu->ch_on[0] = false;
        }
        break;
    case 1: // Square 2
        apu->duty_pos[1] = 0;
        break;
    case 2: // Wave: reset position
        apu->wave_pos = 0;
        break;
    case 3: // Noise: reset LFSR
        apu->lfsr = 0x7FFF;
        break;
    }

    // DAC disabled -> channel off
    if (!dac_enabled(apu, ch))
        apu->ch_on[ch] = false;

    // If length counter is zero, unfreeze it to max (and apply extra length clock)
    if (apu->length[ch] == 0) {
        apu->length[ch] = (ch == 2) ? 256 : 64;
        // Extra length clock on trigger when length_enable + first half of frame
        bool first_half = apu->frame_div < 1024;
        if (apu->length_enable[ch] && apu->power && first_half) {
            if (--apu->length[ch] == 0)
                apu->ch_on[ch] = false;
        }
    }
}

void apu_write(APU* apu, uint16_t addr, uint8_t value)
{
    // NR52 master control
    if (addr == 0xFF26) {
        bool on = (value & 0x80) != 0;
        if (!on && apu->power)
            apu_power_off(apu);
        else if (on && !apu->power) {
            apu->power = true;
            apu->frame_step = 0;
            apu->frame_div = 1024; // Start at second half of length period
        }
        return;
    }

    // While powered off: NR10-NR51 are read-only; wave RAM stays writable
    if (!apu->power && !is_wave_ram(addr)) {
        if (apu->cgb) return;
        // DMG exception: NRx1 updates internal length counter only
        int ch = -1;
        switch (addr) {
            case 0xFF11: ch = 0; break;
            case 0xFF16: ch = 1; break;
            case 0xFF1B: ch = 2; break;
            case 0xFF20: ch = 3; break;
            default: return;
        }
        apu->length_load[ch] = (ch == 2) ? (256 - value) : (64 - (value & 0x3F));
        apu->length[ch] = apu->length_load[ch];
        return;
    }

    // Store value
    apu->regs[addr - 0xFF10] = value;

    // --- Length counters (NRx1) ---
    switch (addr) {
        case 0xFF11: // NR11
            apu->length_load[0] = 64 - (value & 0x3F);
            break;
        case 0xFF16: // NR21
            apu->length_load[1] = 64 - (value & 0x3F);
            break;
        case 0xFF1B: // NR31
            apu->length_load[2] = 256 - value;
            break;
        case 0xFF20: // NR41
            apu->length_load[3] = 64 - (value & 0x3F);
            break;
    }

    // --- DAC disable clears channel ---
    switch (addr) {
        case 0xFF12: if (!dac_enabled(apu, 0)) apu->ch_on[0] = false; break;
        case 0xFF17: if (!dac_enabled(apu, 1)) apu->ch_on[1] = false; break;
        case 0xFF1A: if (!dac_enabled(apu, 2)) apu->ch_on[2] = false; break;
        case 0xFF21: if (!dac_enabled(apu, 3)) apu->ch_on[3] = false; break;
    }

    // --- Control registers (NRx4) with length enable + trigger ---
    switch (addr) {
    case 0xFF14: { // NR14
        bool was_enabled = apu->length_enable[0];
        bool now_enabled = (value & 0x40) != 0;
        bool first_half = apu->frame_div < 1024;
        apu->length_enable[0] = now_enabled;
        // Extra length clock: enabling 0->1 in first half of frame
        if (!was_enabled && now_enabled && apu->power && first_half) {
            if (apu->length[0] > 0 && --apu->length[0] == 0)
                apu->ch_on[0] = false;
        }
        if (value & 0x80) trigger_channel(apu, 0);
        break;
    }
    case 0xFF19: { // NR24
        bool was_enabled = apu->length_enable[1];
        bool now_enabled = (value & 0x40) != 0;
        bool first_half = apu->frame_div < 1024;
        apu->length_enable[1] = now_enabled;
        if (!was_enabled && now_enabled && apu->power && first_half) {
            if (apu->length[1] > 0 && --apu->length[1] == 0)
                apu->ch_on[1] = false;
        }
        if (value & 0x80) trigger_channel(apu, 1);
        break;
    }
    case 0xFF1E: { // NR34
        bool was_enabled = apu->length_enable[2];
        bool now_enabled = (value & 0x40) != 0;
        bool first_half = apu->frame_div < 1024;
        apu->length_enable[2] = now_enabled;
        if (!was_enabled && now_enabled && apu->power && first_half) {
            if (apu->length[2] > 0 && --apu->length[2] == 0)
                apu->ch_on[2] = false;
        }
        if (value & 0x80) trigger_channel(apu, 2);
        break;
    }
    case 0xFF23: { // NR44
        bool was_enabled = apu->length_enable[3];
        bool now_enabled = (value & 0x40) != 0;
        bool first_half = apu->frame_div < 1024;
        apu->length_enable[3] = now_enabled;
        if (!was_enabled && now_enabled && apu->power && first_half) {
            if (apu->length[3] > 0 && --apu->length[3] == 0)
                apu->ch_on[3] = false;
        }
        if (value & 0x80) trigger_channel(apu, 3);
        break;
    }

    // --- Sweep register (NR10) ---
    case 0xFF10: {
        bool negate_now = (value & 0x08) != 0;
        // If negate was used and is now off, disable channel
        if (apu->sweep_negate_used && !negate_now)
            apu->ch_on[0] = false;
        apu->sweep_negate_used = apu->sweep_negate_used || negate_now;
        apu->sweep_negate = negate_now;
        apu->sweep_period = (value >> 4) & 0x07;
        break;
    }

    default: break;
    }
}
