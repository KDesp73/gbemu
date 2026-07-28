#include "emu.h"

uint8_t fetch8(CPU* cpu, Bus* bus)
{
    return bus_read(bus, cpu->pc++);
}

uint16_t fetch16(CPU* cpu, Bus* bus)
{
    uint8_t low = bus_read(bus, cpu->pc++);
    uint8_t high = bus_read(bus, cpu->pc++);
    return (high << 8) | low;
}
