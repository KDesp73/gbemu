#include "gbemu.h"

// Service hardware interrupts and return cycles consumed (0 if none serviced)
int gb_handle_interrupts(gb_cpu* cpu, gb_bus* bus, gb_ppu* ppu, gb_timer* timer)
{
    // 1. Sync PPU/Timer interrupt requests into IF register (0xFF0F)
    uint8_t if_reg = gb_bus_read(bus, 0xFF0F);

    if (ppu->vblank_interrupt) {
        if_reg |= (1 << 0);
        ppu->vblank_interrupt = false;
    }
    if (ppu->stat_interrupt) {
        if_reg |= (1 << 1);
        ppu->stat_interrupt = false;
    }
    if (timer->interrupt_requested) {
        if_reg |= (1 << 2);
        timer->interrupt_requested = false;
    }
    if (bus->joypad_interrupt) {
        if_reg |= (1 << 4);
        bus->joypad_interrupt = false;
    }

    gb_bus_write(bus, 0xFF0F, if_reg);

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
                gb_bus_write(bus, 0xFF0F, if_reg);

                // Dispatch takes 5 M-cycles: two internal cycles, then the
                // PC push (high byte first), then one final internal cycle.
                gb_machine_tick(bus, 4);
                gb_machine_tick(bus, 4);

                // Push PC to stack (high byte first, like hardware)
                cpu->sp -= 2;
                gb_machine_tick(bus, 4);
                gb_bus_write(bus, cpu->sp + 1, (cpu->pc >> 8) & 0xFF); // High byte
                gb_machine_tick(bus, 4);
                gb_bus_write(bus, cpu->sp, cpu->pc & 0xFF);            // Low byte

                // Jump to interrupt vector table (0x0040, 0x0048, 0x0050, 0x0058, 0x0060)
                cpu->pc = 0x0040 + (bit * 8);

                gb_machine_tick(bus, 4);
                return 20; // Servicing an interrupt vector consumes 20 T-cycles (5 M-cycles)
            }
        }
    }

    return 0;
}
