#include "emu.h"
#include <stdio.h>

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

