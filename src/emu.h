#ifndef EMU_H
#define EMU_H

//@module emu
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
    bool ime_scheduled; // EI has a 1-instruction delay; set to true, copies to ime after next instr
    
    bool halted;
    bool halt_bug; // HALT executed with IME=0 + pending interrupt: PC not incremented on next fetch
} CPU;

//@func cpu_init
//@desc Initialize CPU state to boot values
//@param cpu CPU pointer to initialize
void cpu_init(CPU* cpu);

//@func cpu_dump_fd
//@desc Dump CPU register state to a file descriptor
//@param cpu CPU state to dump
//@param fd Output file descriptor (e.g. stdout)
void cpu_dump_fd(CPU cpu, FILE* fd);

//@func cpu_dump
//@desc Dump CPU register state to stdout
//@param cpu CPU state to dump
#define cpu_dump(cpu) cpu_dump_fd(cpu, stdout)

//@enum Flag
//@desc CPU flag bit positions in the F register
//@ref https://gbdev.io/pandocs/CPU_Registers_and_Flags.html
typedef enum {
    FLAG_Z = (1 << 7),
    FLAG_N = (1 << 6),
    FLAG_H = (1 << 5),
    FLAG_C = (1 << 4),
} Flag;

//@func flag_set
//@desc Set or clear a specific CPU flag
//@param cpu CPU whose flag to modify
//@param flag The flag bit to set or clear
//@param value true to set, false to clear
void flag_set(CPU* cpu, Flag flag, bool value);

//@func flag_get
//@desc Read the value of a specific CPU flag
//@param cpu CPU to read from
//@param flag The flag bit to read
//@returns true if the flag is set, false otherwise
bool flag_get(const CPU* cpu, Flag flag);

//@module memory

//@type Bus
//@desc Memory bus representation
//@ref https://gbdev.io/pandocs/Memory_Map.html
typedef struct Bus Bus;

struct Bus {
    uint8_t rom[0x8000];    // 32KB Cartridge
    uint8_t vram[0x2000];   // 8KB Video RAM
    uint8_t wram[0x2000];   // 8KB Work RAM
    uint8_t sram[0x2000];   // 8KB Cartridge SRAM (MBC1 external RAM at 0xA000-0xBFFF)
    uint8_t oam[0xA0];      // Sprite Attribute Table
    uint8_t io[0x80];       // Input/Output Registers
    uint8_t hram[0x7F];     // High RAM
    uint8_t ie;             // Interrupt Enable Register

    struct Timer* timer;    // For routing timer register reads/writes
    struct PPU* ppu;        // For routing PPU register reads/writes
    struct APU* apu;        // For routing sound register reads/writes

    bool double_speed;      // CGB double-speed mode (CPU T-cycle = 0.5 system cycle)
};

//@func bus_read
//@desc Read a byte from the memory-mapped bus
//@param bus Memory bus to read from
//@param addr 16-bit address to read
//@returns Byte value at the given address
uint8_t bus_read(Bus* bus, uint16_t addr);

//@func bus_write
//@desc Write a byte to the memory-mapped bus
//@param bus Memory bus to write to
//@param addr 16-bit address to write
//@param value Byte value to write
void bus_write(Bus* bus, uint16_t addr, uint8_t value);

size_t bus_load_rom(Bus* bus, const char* filepath);

//@module misc

//@func get_reg_by_index
//@desc Get a CPU register value by its 3-bit index (0-7)
//@param cpu CPU to read from
//@param bus Memory bus for HL indirect reads
//@param index Register index (0=B,1=C,2=D,3=E,4=H,5=L,6=(HL),7=A)
//@returns Byte value of the register
uint8_t get_reg_by_index(CPU* cpu, Bus* bus, uint8_t index);

//@func set_reg_by_index
//@desc Set a CPU register value by its 3-bit index (0-7)
//@param cpu CPU to write to
//@param bus Memory bus for HL indirect writes
//@param index Register index (0=B,1=C,2=D,3=E,4=H,5=L,6=(HL),7=A)
//@param value Byte value to write
void set_reg_by_index(CPU* cpu, Bus* bus, uint8_t index, uint8_t value);

//@func fetch8
//@desc Fetch the next byte from PC and advance PC by 1
//@param cpu CPU to fetch from
//@param bus Memory bus to read from
//@returns The fetched byte
uint8_t fetch8(CPU* cpu, Bus* bus);

//@func fetch16
//@desc Fetch the next 16-bit value from PC (little-endian) and advance PC by 2
//@param cpu CPU to fetch from
//@param bus Memory bus to read from
//@returns The fetched 16-bit value
uint16_t fetch16(CPU* cpu, Bus* bus);

//@module exec

//@macro CPU_FREQ
//@desc CGB dot clock frequency in Hz (8.388608 MHz), the emulator's base time unit
#define CPU_FREQ 8388608

//@macro CYCLES_PER_FRAME
//@desc System cycles per frame at ~60 Hz (~139,810 dots); constant across both CPU speed modes
#define CYCLES_PER_FRAME (CPU_FREQ / 60)

//@macro FRAME_TIME_NS
//@desc Frame duration in nanoseconds at 60 Hz (~16.66 ms)
#define FRAME_TIME_NS (1000000000L / 60)

//@func instr
//@desc Execute a single CPU instruction by opcode
//@param cpu CPU state to mutate
//@param bus Memory bus for reads/writes
//@param opcode The opcode byte to execute
//@returns Number of T-cycles the instruction consumed
int instr(CPU* cpu, Bus* bus, uint8_t opcode);

//@func cpu_step
//@desc Execute one full CPU step (fetch + decode + execute)
//@param cpu CPU state to mutate
//@param bus Memory bus for reads/writes
//@returns Number of T-cycles the step consumed
int cpu_step(CPU* cpu, Bus* bus);

//@module timer

//@type Timer
//@desc Game Boy timer hardware (DIV, TIMA, TMA, TAC registers)
//@ref https://gbdev.io/pandocs/Timer_and_Divider_Registers.html
typedef struct Timer {
    uint16_t internal_counter; // 16-bit internal clock (DIV is the upper byte)
    
    uint8_t tima; // 0xFF05
    uint8_t tma;  // 0xFF06
    uint8_t tac;  // 0xFF07

    bool interrupt_requested; // Set to true when TIMA overflows
} Timer;

//@func timer_init
//@desc Initialize timer state to default values
//@param timer Timer pointer to initialize
void timer_init(Timer* timer);

//@func timer_step
//@desc Advance the timer by the given number of T-cycles
//@param timer Timer state to update
//@param cycles Number of T-cycles elapsed
void timer_step(Timer* timer, int cycles);

//@func timer_read
//@desc Read a timer register value
//@param timer Timer state to read from
//@param addr Register address (0xFF04-0xFF07)
//@returns Register byte value
uint8_t timer_read(const Timer* timer, uint16_t addr);

//@func timer_write
//@desc Write a value to a timer register
//@param timer Timer state to write to
//@param addr Register address (0xFF04-0xFF07)
//@param value Byte value to write
void timer_write(Timer* timer, uint16_t addr, uint8_t value);

uint64_t get_time_ns(void);

//@module ppu

//@macro SCREEN_WIDTH
//@desc Game Boy native screen width in pixels
#define SCREEN_WIDTH 160

//@macro SCREEN_HEIGHT
//@desc Game Boy native screen height in pixels
#define SCREEN_HEIGHT 144

//@enum PPUMode
//@desc PPU mode states stored in the lower 2 bits of STAT (0xFF41)
//@ref https://gbdev.io/pandocs/STAT_Register.html
typedef enum {
    PPU_MODE_HBLANK = 0, // Mode 0: Horizontal Blank (204 M-cycles)
    PPU_MODE_VBLANK = 1, // Mode 1: Vertical Blank (4560 M-cycles / Scanlines 144-153)
    PPU_MODE_OAM    = 2, // Mode 2: Searching OAM for sprites (80 M-cycles)
    PPU_MODE_XFER   = 3  // Mode 3: Transferring pixel data to LCD (172 M-cycles)
} PPUMode;

//@type PPU
//@desc Pixel Processing Unit - renders scanlines to the frame buffer
//@ref https://gbdev.io/pandocs/PPU.html
typedef struct PPU {
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
    bool first_line_after_enable; // Shortened first scanline after LCD on

    // Output Interfaces
    uint32_t frame_buffer[SCREEN_HEIGHT][SCREEN_WIDTH]; // Pixel buffer for SDL
    bool frame_ready;        // Set to true at VBlank, signals SDL to render
    bool vblank_interrupt;   // Set to true to request INT 0x40
    bool stat_interrupt;     // Set to true to request INT 0x48
} PPU;

//@func ppu_init
//@desc Initialize PPU registers and frame buffer to default values
//@param ppu PPU pointer to initialize
void ppu_init(PPU* ppu);

//@func ppu_step
//@desc Advance the PPU state machine by the given T-cycles
//@param ppu PPU state to update
//@param bus Memory bus for VRAM and OAM reads
//@param cycles Number of T-cycles elapsed
void ppu_step(PPU* ppu, Bus* bus, int cycles);

//@func ppu_read
//@desc Read a PPU register value
//@param ppu PPU state to read from
//@param addr Register address (0xFF40-0xFF4B)
//@returns Register byte value
uint8_t ppu_read(const PPU* ppu, uint16_t addr);

//@func ppu_write
//@desc Write a value to a PPU register
//@param ppu PPU state to write to
//@param addr Register address (0xFF40-0xFF4B)
//@param value Byte value to write
void ppu_write(PPU* ppu, uint16_t addr, uint8_t value);

int handle_interrupts(CPU* cpu, Bus* bus, PPU* ppu, Timer* timer);

//@module apu

//@type APU
//@desc Minimal audio unit: channel power/length-counter state for NR52 (no wave generation)
//@ref https://gbdev.io/pandocs/Audio.html
typedef struct APU {
    uint8_t regs[0x30];     // Sound registers FF10-FF3F
    bool power;             // NR52 bit 7 (master power)
    bool cgb;               // CGB mode (from cartridge header); affects length on power-off
    bool ch_on[4];          // Channel active status (NR52 bits 0-3)
    uint16_t length[4];     // Length counter (max 64 for square/noise, 256 for wave)
    uint16_t length_load[4]; // Length counter reload value
    bool length_enable[4];  // NRx4 bit 6 (length counter enable)
    uint16_t frame_accum;   // System-cycle accumulator (length clock ticks every 16384)
} APU;

//@func apu_init
//@desc Initialize APU state to default values
//@param apu APU pointer to initialize
void apu_init(APU* apu);

//@func apu_step
//@desc Advance the APU frame sequencer by the given number of dots (system cycles)
//@param apu APU state to update
//@param dots Number of dot clock cycles elapsed (fixed 4.194304 MHz)
void apu_step(APU* apu, int dots);

//@func apu_read
//@desc Read a sound register value
//@param apu APU state to read from
//@param addr Register address (0xFF10-0xFF3F)
//@returns Register byte value
uint8_t apu_read(const APU* apu, uint16_t addr);

//@func apu_write
//@desc Write a value to a sound register
//@param apu APU state to write to
//@param addr Register address (0xFF10-0xFF3F)
//@param value Byte value to write
void apu_write(APU* apu, uint16_t addr, uint8_t value);


void loop(CPU* cpu, Bus* bus, Timer* timer, PPU* ppu, APU* apu);

#endif // EMU_H
