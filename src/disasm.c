#include "gbemu.h"
#include "disasm_tables.h"
#include <stdio.h>

int gb_instruction_length(gb_bus* bus, uint16_t addr)
{
    uint8_t op = gb_bus_read(bus, addr);
    if (op == 0xCB) {
        uint8_t cb = gb_bus_read(bus, (uint16_t)(addr + 1));
        return disasm_cb_table[cb].bytes;
    }
    return disasm_table[op].bytes;
}

int gb_disasm(gb_bus* bus, uint16_t addr, char* out, size_t out_size)
{
    if (!out || out_size == 0) return 0;

    uint8_t op = gb_bus_read(bus, addr);
    const disasm_op* entry;
    uint16_t cursor;

    if (op == 0xCB) {
        uint8_t cb = gb_bus_read(bus, (uint16_t)(addr + 1));
        entry = &disasm_cb_table[cb];
        cursor = (uint16_t)(addr + 2);
    } else {
        entry = &disasm_table[op];
        cursor = (uint16_t)(addr + 1);
    }

    int args[2] = {0, 0};
    for (int i = 0; i < entry->arg_count && i < 2; i++) {
        switch (entry->arg_kinds[i]) {
        case DISASM_ARG_IMM8:
            args[i] = gb_bus_read(bus, cursor);
            cursor = (uint16_t)(cursor + 1);
            break;
        case DISASM_ARG_IMM16:
        case DISASM_ARG_ADDR16:
            args[i] = gb_bus_read(bus, cursor) | (gb_bus_read(bus, (uint16_t)(cursor + 1)) << 8);
            cursor = (uint16_t)(cursor + 2);
            break;
        case DISASM_ARG_ADDR8:
            args[i] = 0xFF00 | gb_bus_read(bus, cursor);
            cursor = (uint16_t)(cursor + 1);
            break;
        case DISASM_ARG_REL8:
            args[i] = (uint16_t)(addr + entry->bytes + (int8_t)gb_bus_read(bus, cursor));
            cursor = (uint16_t)(cursor + 1);
            break;
        case DISASM_ARG_SIGNED:
            args[i] = (int8_t)gb_bus_read(bus, cursor);
            cursor = (uint16_t)(cursor + 1);
            break;
        default:
            break;
        }
    }

    int len;
    switch (entry->arg_count) {
    case 0:
        len = snprintf(out, out_size, "%s", entry->fmt);
        break;
    case 1:
        len = snprintf(out, out_size, entry->fmt, args[0]);
        break;
    default:
        len = snprintf(out, out_size, entry->fmt, args[0], args[1]);
        break;
    }

    if (len < 0 || (size_t)len >= out_size) return 0;
    return entry->bytes;
}
