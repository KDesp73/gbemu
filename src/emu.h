#ifndef EMU_H
#define EMU_H

//@author Konstantinos Despoinidis (KDesp73)
//@license MIT


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

//@module cpu

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

void cpu_init(CPU* cpu);
void cpu_dump_fd(CPU cpu, FILE* fd);
#define cpu_dump(cpu) cpu_dump_fd(cpu, stdout)

typedef enum {
    FLAG_Z = (1 << 7),
    FLAG_N = (1 << 6),
    FLAG_H = (1 << 5),
    FLAG_C = (1 << 4),
} Flag;

void flag_set(CPU* cpu, Flag flag, bool value);
bool flag_get(const CPU* cpu, Flag flag);

//@module memory

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

//@module misc

uint8_t get_reg_by_index(CPU* cpu, Bus* bus, uint8_t index);
void set_reg_by_index(CPU* cpu, Bus* bus, uint8_t index, uint8_t value);

uint8_t fetch8(CPU* cpu, Bus* bus);
uint16_t fetch16(CPU* cpu, Bus* bus);

//@module exec

#define CPU_FREQ 4194304
#define CYCLES_PER_FRAME (CPU_FREQ / 60) // ~69,905 T-cycles per frame
#define FRAME_TIME_NS (1000000000L / 60) // ~16.66 ms in nanoseconds

int instr(CPU* cpu, Bus* bus, uint8_t opcode);
int cpu_step(CPU* cpu, Bus* bus);

//@module timer

typedef struct {
    uint16_t internal_counter; // 16-bit internal clock (DIV is the upper byte)
    
    uint8_t tima; // 0xFF05
    uint8_t tma;  // 0xFF06
    uint8_t tac;  // 0xFF07

    bool interrupt_requested; // Set to true when TIMA overflows
} Timer;

void timer_init(Timer* timer);
void timer_step(Timer* timer, int cycles);

uint8_t timer_read(const Timer* timer, uint16_t addr);
void timer_write(Timer* timer, uint16_t addr, uint8_t value);

//@module ppu

// Game Boy native resolution: 160 pixels wide by 144 pixels high
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144

// PPU Modes (stored in lower 2 bits of STAT register 0xFF41)
typedef enum {
    PPU_MODE_HBLANK = 0, // Mode 0: Horizontal Blank (204 M-cycles)
    PPU_MODE_VBLANK = 1, // Mode 1: Vertical Blank (4560 M-cycles / Scanlines 144-153)
    PPU_MODE_OAM    = 2, // Mode 2: Searching OAM for sprites (80 M-cycles)
    PPU_MODE_XFER   = 3  // Mode 3: Transferring pixel data to LCD (172 M-cycles)
} PPUMode;

typedef struct {
    uint8_t lcdc; // 0xFF40 - LCD Control
    uint8_t stat; // 0xFF41 - LCD Status
    uint8_t scy;  // 0xFF42 - Scroll Y
    uint8_t scx;  // 0xFF43 - Scroll X
    uint8_t ly;   // 0xFF44 - LCD Y-Coordinate (Current Scanline 0-153)
    uint8_t lyc;  // 0xFF45 - LY Compare
    uint8_t bgp;  // 0xFF47 - BG Palette Data
    uint8_t obp0; // 0xFF48 - Object Palette 0 Data
    uint8_t obp1; // 0xFF49 - Object Palette 1 Data
    uint8_t wy;   // 0xFF4A - Window Y Position
    uint8_t wx;   // 0xFF4B - Window X Position + 7

    // Internal State
    uint32_t dots; // Dot/T-cycle counter within the current scanline (0-455)
    
    // Output Interfaces
    uint32_t frame_buffer[SCREEN_HEIGHT][SCREEN_WIDTH]; // Pixel buffer for SDL
    bool frame_ready;        // Set to true at VBlank, signals SDL to render
    bool vblank_interrupt;   // Set to true to request INT 0x40
    bool stat_interrupt;     // Set to true to request INT 0x48
} PPU;

void ppu_init(PPU* ppu);
void ppu_step(PPU* ppu, int cycles);

uint8_t ppu_read(const PPU* ppu, uint16_t addr);
void ppu_write(PPU* ppu, uint16_t addr, uint8_t value);

#endif // EMU_H
