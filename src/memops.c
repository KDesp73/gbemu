#include "gbemu.h"

uint8_t gb_fetch8(gb_cpu* cpu, gb_bus* bus)
{
    gb_machine_tick(bus, 4); // operand fetch occupies one M-cycle
    return gb_bus_read(bus, cpu->pc++);
}

uint16_t gb_fetch16(gb_cpu* cpu, gb_bus* bus)
{
    gb_machine_tick(bus, 4);
    uint8_t low = gb_bus_read(bus, cpu->pc++);
    gb_machine_tick(bus, 4);
    uint8_t high = gb_bus_read(bus, cpu->pc++);
    return (high << 8) | low;
}
