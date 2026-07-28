#include "emu.h"
#include "opcodes.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BOOL(x) ((x) ? "true" : "false")

void cpu_dump_fd(CPU cpu, FILE* fd)
{
    fprintf(fd, "AF: %d\ta: %d\tf: %d\n", cpu.af, cpu.a, cpu.f);
    fprintf(fd, "AF: %d\tb: %d\tc: %d\n", cpu.bc, cpu.b, cpu.c);
    fprintf(fd, "AF: %d\td: %d\te: %d\n", cpu.de, cpu.d, cpu.e);
    fprintf(fd, "HL: %d\th: %d\tl: %d\n", cpu.hl, cpu.h, cpu.l);

    fprintf(fd, "SP: %d\tPC: %d\n", cpu.sp, cpu.pc);
    fprintf(fd, "IME: %s\tHALTED: %s\n", BOOL(cpu.ime), BOOL(cpu.halted));
}


int cpu_step(CPU* cpu, Bus* bus)
{
    // 1. Fetch
    uint8_t opcode = bus_read(bus, cpu->pc++);

    // 2. Decode & Execute
    switch (opcode) {
        case OP_NOP:
            return 4;
        default:
            fprintf(stderr, "Unhandled opcode: 0x%02X at PC: 0x%04X\n", opcode, cpu->pc - 1);
            exit(1);
    }
}
