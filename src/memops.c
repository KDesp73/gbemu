#include "emu.h"

uint8_t fetch8(CPU* cpu, Bus* bus)
{
    machine_tick(bus, 4); // operand fetch occupies one M-cycle
    return bus_read(bus, cpu->pc++);
}

uint16_t fetch16(CPU* cpu, Bus* bus)
{
    machine_tick(bus, 4);
    uint8_t low = bus_read(bus, cpu->pc++);
    machine_tick(bus, 4);
    uint8_t high = bus_read(bus, cpu->pc++);
    return (high << 8) | low;
}
