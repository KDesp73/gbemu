#ifndef EMU_H
#define EMU_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

//@type CPU
//@desc Software representation of the Central Processing Unit
//@ref https://gbdev.io/pandocs/CPU_Registers_and_Flags.html
typedef struct {
    union {
        struct {
            uint8_t f; uint8_t a;
        };
        uint16_t af;
    };
    union {
        struct {
            uint8_t c; uint8_t b;
        };
        uint16_t bc;
    };
    union {
        struct {
            uint8_t e; uint8_t d;
        };
        uint16_t de;
    };
    union {
        struct {
            uint8_t l; uint8_t h;
        };
        uint16_t hl;
    };

    uint16_t sp;
    uint16_t pc;

    bool ime; // Interrupt Master Enable
    bool halted;
} CPU;

void cpu_dump_fd(CPU cpu, FILE* fd);
#define cpu_dump(cpu) cpu_dump_fd(cpu, stdout)

#endif // EMU_H
