#include "emu.h"
#include <time.h>

uint64_t get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000L + (uint64_t)ts.tv_nsec;
}

void timer_init(Timer* timer)
{
    timer->internal_counter = 0xABCC; // Default post-boot value
    timer->tima = 0x00;
    timer->tma = 0x00;
    timer->tac = 0xF8; // High 5 bits always read 1
    timer->interrupt_requested = false;
}

// Map TAC bits 0-1 to the bit index of the internal 16-bit counter
static bool get_timer_bit(const Timer* timer)
{
    // TAC Bit 2: Timer Enable
    if (!(timer->tac & 0x04)) {
        return false;
    }

    // TAC Bits 0-1 select frequency:
    // 00: 4096 Hz   -> Bit 9  (1024 T-cycles)
    // 01: 262144 Hz -> Bit 3  (16 T-cycles)
    // 10: 65536 Hz  -> Bit 5  (64 T-cycles)
    // 11: 16384 Hz  -> Bit 7  (256 T-cycles)
    switch (timer->tac & 0x03) {
        case 0: return (timer->internal_counter & (1 << 9)) != 0;
        case 1: return (timer->internal_counter & (1 << 3)) != 0;
        case 2: return (timer->internal_counter & (1 << 5)) != 0;
        case 3: return (timer->internal_counter & (1 << 7)) != 0;
        default: return false;
    }
}

void timer_step(Timer* timer, int cycles)
{
    for (int i = 0; i < cycles; i++) {
        bool bit_before = get_timer_bit(timer);
        
        timer->internal_counter++;
        
        bool bit_after = get_timer_bit(timer);

        // Falling edge detector: when the clock bit goes from 1 to 0, TIMA increments!
        if (bit_before && !bit_after) {
            if (timer->tima == 0xFF) {
                timer->tima = timer->tma;             // Reload modulo
                timer->interrupt_requested = true;    // Request TIMER interrupt (bit 2 of IF)
            } else {
                timer->tima++;
            }
        }
    }
}

uint8_t timer_read(const Timer* timer, uint16_t addr)
{
    switch (addr) {
        case 0xFF04: // DIV
            return (uint8_t)(timer->internal_counter >> 8);
        case 0xFF05: // TIMA
            return timer->tima;
        case 0xFF06: // TMA
            return timer->tma;
        case 0xFF07: // TAC
            return timer->tac | 0xF8; // Unused bits read as 1
        default:
            return 0xFF;
    }
}

void timer_write(Timer* timer, uint16_t addr, uint8_t value)
{
    switch (addr) {
        case 0xFF04: // Writing ANY value to DIV resets internal counter to 0!
            timer->internal_counter = 0;
            break;
        case 0xFF05: // TIMA
            timer->tima = value;
            break;
        case 0xFF06: // TMA
            timer->tma = value;
            break;
        case 0xFF07: // TAC
            timer->tac = value & 0x07; // Only lower 3 bits are writable
            break;
    }
}
