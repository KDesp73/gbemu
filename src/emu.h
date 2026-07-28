#ifndef EMU_H
#define EMU_H

//@author Konstantinos Despoinidis (KDesp73)
//@license MIT


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


//@type Bus
//@desc Memory bus representation
//@ref https://gbdev.io/pandocs/Memory_Map.html
typedef struct {
    uint8_t rom[0x8000];    // 32KB Cartridge
    uint8_t vram[0x2000];   // 8KB Video RAM
    uint8_t wram[0x2000];   // 8KB Work RAM
    uint8_t oam[0xA0];      // Sprite Attribute Table
    uint8_t io[0x80];       // Input/Output Registers
    uint8_t hram[0x7F];     // High RAM
    uint8_t ie;             // Interrupt Enable Register
} Bus;

uint8_t bus_read(Bus* bus, uint16_t addr);
void bus_write(Bus* bus, uint16_t addr, uint8_t value);

int cpu_step(CPU* cpu, Bus* bus);

#endif // EMU_H
