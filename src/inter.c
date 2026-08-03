#include "emu.h"

// Service hardware interrupts and return cycles consumed (0 if none serviced)
int handle_interrupts(CPU* cpu, Bus* bus, PPU* ppu, Timer* timer)
{
    // 1. Sync PPU/Timer interrupt requests into IF register (0xFF0F)
    uint8_t if_reg = bus_read(bus, 0xFF0F);
    
    if (ppu->vblank_interrupt) {
        if_reg |= (1 << 0); // VBlank Interrupt (Bit 0)
        ppu->vblank_interrupt = false;
    }
    if (ppu->stat_interrupt) {
        if_reg |= (1 << 1); // STAT Interrupt (Bit 1)
        ppu->stat_interrupt = false;
    }
    if (timer->interrupt_requested) {
        if_reg |= (1 << 2); // Timer Interrupt (Bit 2)
        timer->interrupt_requested = false;
    }
    
    bus_write(bus, 0xFF0F, if_reg);

    // 2. Unhalt CPU if an interrupt is pending (even if IME is false)
    uint8_t pending = if_reg & bus->ie & 0x1F;
    if (pending != 0) {
        cpu->halted = false;
    }

    // 3. Service interrupt vector if IME is enabled
    if (cpu->ime && pending != 0) {
        cpu->ime = false; // Disable further interrupts

        // Determine highest priority interrupt bit (0 to 4)
        for (int bit = 0; bit < 5; bit++) {
            if (pending & (1 << bit)) {
                // Clear the serviced bit in IF
                if_reg &= ~(1 << bit);
                bus_write(bus, 0xFF0F, if_reg);

                // Push PC to stack
                cpu->sp -= 2;
                bus_write(bus, cpu->sp, cpu->pc & 0xFF);         // Low byte
                bus_write(bus, cpu->sp + 1, (cpu->pc >> 8) & 0xFF); // High byte

                // Jump to interrupt vector table (0x0040, 0x0048, 0x0050, 0x0058, 0x0060)
                cpu->pc = 0x0040 + (bit * 8);

                return 20; // Servicing an interrupt vector consumes 20 T-cycles (5 M-cycles)
            }
        }
    }

    return 0;
}
