#ifndef EMU_H
#define EMU_H

//@module emu
//@author Konstantinos Despoinidis (KDesp73)
//@license MIT

//@const VERSION_MAJOR
//@desc Major version number (incremented on breaking changes)
#define VERSION_MAJOR 0

//@const VERSION_MINOR
//@desc Minor version number (incremented on new features, backward-compatible)
#define VERSION_MINOR 1

//@const VERSION_PATCH
//@desc Patch version number (incremented on bug fixes)
#define VERSION_PATCH 0

#define STR(x) #x
#define TOSTRING(x) STR(x)

//@macro VERSION_STRING
//@desc Human-readable version string in "MAJOR.MINOR.PATCH" format
#define VERSION_STRING TOSTRING(VERSION_MAJOR) "." TOSTRING(VERSION_MINOR) "." TOSTRING(VERSION_PATCH)

//@macro VERSION_HEX
//@desc Compact numeric version: MAJOR * 10000 + MINOR * 100 + PATCH
#define VERSION_HEX ((VERSION_MAJOR * 10000) + (VERSION_MINOR * 100) + VERSION_PATCH)

//@func version
//@desc Get the current library version as individual components
//@param major Pointer to receive the major version (may be NULL)
//@param minor Pointer to receive the minor version (may be NULL)
//@param patch Pointer to receive the patch version (may be NULL)
static inline void version(int* major, int* minor, int* patch) {
    if (major) *major = VERSION_MAJOR;
    if (minor) *minor = VERSION_MINOR;
    if (patch) *patch = VERSION_PATCH;
}


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "frontend.h"

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
    uint8_t* rom;           // Dynamically allocated cartridge ROM (all banks)
    size_t rom_size;        // Total allocated ROM size in bytes
    uint16_t rom_banks;     // Number of 16KB ROM banks (power of two)

    uint8_t vram[0x2000];   // 8KB Video RAM
    uint8_t wram[0x2000];   // 8KB Work RAM
    uint8_t* sram;          // Dynamically allocated cartridge SRAM (0xA000-0xBFFF)
    size_t sram_size;       // Total allocated SRAM size in bytes
    uint16_t sram_banks;    // Number of 8KB SRAM banks

    uint8_t oam[0xA0];      // Sprite Attribute Table
    uint8_t io[0x80];       // Input/Output Registers
    uint8_t hram[0x7F];     // High RAM
    uint8_t ie;             // Interrupt Enable Register

    // OAM DMA state (transfer of 160 bytes to OAM, one byte per M-cycle)
    bool dma_active;        // DMA transfer in progress (OAM is inaccessible)
    int dma_start_delay;    // M-cycles remaining before the first byte copies
    uint8_t dma_src_high;   // Source page (value written to 0xFF46)
    uint16_t dma_offset;    // Bytes transferred so far (0-160)
    bool dma_pending;       // Restart queued while a transfer is running
    int dma_pend_delay;     // M-cycles until the queued restart takes over
    uint8_t dma_pend_src;   // Source page of the queued restart

    struct Timer* timer;    // For routing timer register reads/writes
    struct PPU* ppu;        // For routing PPU register reads/writes
    struct APU* apu;        // For routing sound register reads/writes

    bool double_speed;      // CGB double-speed mode (CPU T-cycle = 0.5 system cycle)

    // Joypad state (active-low: 0 = pressed)
    // Bit layout: bit 7 = Start, 6 = Select, 5 = B, 4 = A
    //             bit 3 = Down, 2 = Up, 1 = Left, 0 = Right
    uint8_t joypad_buttons; // Face buttons + Start/Select (active-low)
    uint8_t joypad_dpad;    // D-pad buttons (active-low)

    // MBC bank-switching state (active when mbc_type selects an MBC cart)
    uint8_t mbc_type;          // Cartridge type from header 0x0147 (0 = no MBC)
    uint8_t mbc1_rom_bank;     // MBC1: ROM bank register (0x2000-0x3FFF), low 5 bits
    uint8_t mbc1_ram_bank;     // MBC1: RAM bank / upper ROM bits (0x4000-0x5FFF), low 2 bits
    bool    mbc1_mode;         // MBC1: banking mode (0x6000-0x7FFF): 0 = ROM, 1 = RAM
    bool    mbc1_ram_enable;   // MBC1: external RAM enable (0x0000-0x1FFF)
    bool    mbc1_multicart;    // MBC1M multicart mode (1MB carts with multiple games)

    uint8_t mbc2_rom_bank;     // MBC2: 4-bit ROM bank register (A8-set writes in 0x0000-0x3FFF)
    bool    mbc2_ram_enable;   // MBC2: built-in RAM enable (A8-clear write of 0x0A)
    uint8_t mbc2_ram[0x200];   // MBC2: built-in 512x4-bit RAM (mirrored over 0xA000-0xBFFF)

    uint8_t mbc3_rom_bank;     // MBC3: 7-bit ROM bank register ($2000-$3FFF); 0 -> 1 at write
    uint8_t mbc3_ram_bank;     // MBC3: RAM bank (0-3) / RTC register select ($08-$0C)
    bool    mbc3_ram_enable;   // MBC3: external RAM / RTC enable ($0000-$1FFF)
    uint8_t mbc3_latch_state;  // MBC3: latch state machine ($6000-$7FFF)
    uint8_t mbc3_seconds;      // MBC3 RTC: seconds (0-59)
    uint8_t mbc3_minutes;      // MBC3 RTC: minutes (0-59)
    uint8_t mbc3_hours;        // MBC3 RTC: hours (0-23)
    uint16_t mbc3_day_counter; // MBC3 RTC: day counter (9-bit, 0-511)
    bool    mbc3_day_carry;    // MBC3 RTC: day counter carry flag
    bool    mbc3_timer_halt;   // MBC3 RTC: halt flag

    uint8_t mbc5_rom_bank_l;   // MBC5: ROM bank low 8 bits (0x2000-0x2FFF)
    uint8_t mbc5_rom_bank_h;   // MBC5: ROM bank bit 8 (0x3000-0x3FFF), bit 0 only
    uint8_t mbc5_ram_bank;     // MBC5: RAM bank select (0x4000-0x5FFF), low 4 bits
    bool    mbc5_ram_enable;   // MBC5: external RAM enable (0x0000-0x1FFF)
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

//@func bus_tick
//@desc Advance the bus by one M-cycle (drives the OAM DMA transfer)
//@param bus Memory bus to advance
void bus_tick(Bus* bus);

//@func bus_load_rom
//@desc Load a cartridge ROM file into the bus (allocating all ROM banks), set up MBC1 bank switching and SRAM from the cartridge header, and set CGB mode
//@param bus Memory bus to load into
//@param filepath Path to the ROM file to load
//@returns Number of bytes read, or 0 on failure
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
    bool tima_reload_pending; // TIMA overflow reload is delayed by 1 cycle
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

//@func get_time_ns
//@desc Get the current monotonic clock time in nanoseconds
//@returns Monotonic time in nanoseconds since an arbitrary epoch
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

//@func handle_interrupts
//@desc Service pending hardware interrupts: sync PPU/Timer requests into IF (0xFF0F), unhalt the CPU if a pending interrupt is enabled, and jump to the interrupt vector if IME is set
//@param cpu CPU state to mutate
//@param bus Memory bus for IF register reads/writes
//@param ppu PPU providing vblank/stat interrupt requests
//@param timer Timer providing TIMA overflow interrupt requests
//@returns Number of T-cycles consumed by interrupt servicing (0 if none serviced)
int handle_interrupts(CPU* cpu, Bus* bus, PPU* ppu, Timer* timer);

//@module apu

// Audio ring buffer capacity (must be power of two)
#define APU_BUF_SIZE 4096
#define APU_BUF_MASK (APU_BUF_SIZE - 1)

// Sample rate for audio output
#define APU_SAMPLE_RATE 44100

//@type APU
//@desc Game Boy APU with full 4-channel waveform synthesis
//@ref https://gbdev.io/pandocs/Audio.html
typedef struct APU {
    uint8_t regs[0x30];     // Sound registers FF10-FF3F
    bool power;             // NR52 bit 7 (master power)
    bool cgb;               // CGB mode (from cartridge header); affects length on power-off
    bool ch_on[4];          // Channel active status (NR52 bits 0-3)
    uint16_t length[4];     // Length counter (max 64 for square/noise, 256 for wave)
    uint16_t length_load[4]; // Length counter reload value
    bool length_enable[4];  // NRx4 bit 6 (length counter enable)

    // Per-channel synthesis state
    int freq_timer[4];       // Frequency period countdown (T-cycles)
    uint8_t duty_pos[2];     // Duty cycle position (0-7) for CH1/CH2
    uint8_t wave_pos;        // Wave table position (0-31) for CH3
    uint16_t lfsr;           // 15-bit LFSR for CH4 (noise)
    uint8_t vol[3];          // Current volume for CH1, CH2, CH4 (0-15)
    uint8_t env_period[3];   // Envelope period (from NRx2 low 3 bits) for CH1/CH2/CH4
    uint8_t env_counter[3];  // Envelope step counter for CH1/CH2/CH4
    bool env_dir[3];         // Envelope direction: false=down, true=up for CH1/CH2/CH4
    bool env_loop[3];        // Envelope loop flag for CH1/CH2/CH4

    // CH1 sweep state
    uint16_t sweep_freq;     // Shadow frequency register
    uint8_t sweep_period;    // Sweep counter (reload from NR10 bits 6-4)
    uint8_t sweep_counter;   // Sweep step countdown
    bool sweep_enabled;      // Sweep active (turned on by trigger)
    bool sweep_negate;       // Sweep negate from NR10 bit 3
    bool sweep_negate_used;  // Whether negate has been used (blocks re-negate)

    // Frame sequencer (512 Hz, 8 steps, 2048 T-cycles/step)
    uint8_t frame_step;      // Current step (0-7)
    uint16_t frame_div;      // T-cycle divider for frame sequencer

    // Audio output ring buffer (SPSC, float mono)
    float audio_buf[APU_BUF_SIZE];
    volatile uint32_t buf_write; // Write index (producer: apu_step)
    volatile uint32_t buf_read;  // Read index (consumer: SDL callback)

    // Downsampling accumulator (dot clock -> sample rate)
    uint32_t sample_accum;   // Dots accumulated since last sample
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

//@func apu_buf_pop
//@desc Pop a single float sample from the APU audio ring buffer (called from audio callback)
//@param apu APU state to read from
//@returns Audio sample in range [-1.0, 1.0], or 0.0 if buffer empty
float apu_buf_pop(APU* apu);

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

//@func machine_tick
//@desc Advance every component except the CPU (timer, PPU, APU) by the given number of T-cycles. Called at each M-cycle boundary during instruction execution so bus accesses land on their exact hardware cycle slots.
//@param bus Memory bus providing access to the timer, PPU and APU
//@param t_cycles Number of T-cycles to advance the machine by
void machine_tick(Bus* bus, int t_cycles);

//@func cpu_step
//@desc Execute one full CPU step (fetch + decode + execute)
//@param cpu CPU state to mutate
//@param bus Memory bus for reads/writes
//@returns Number of T-cycles the step consumed
int cpu_step(CPU* cpu, Bus* bus);

//@func loop
//@desc Main emulation loop: executes CPU steps until a frame's worth of cycles elapse, advances the timer/PPU/APU, services interrupts, and paces the frame to ~60 Hz
//@param cpu CPU state to run
//@param bus Memory bus for reads/writes
//@param timer Timer state to advance
//@param ppu PPU state to advance
//@param apu APU state to advance
//@param fe Frontend for display and input
void loop(CPU* cpu, Bus* bus, Timer* timer, PPU* ppu, APU* apu, Frontend* fe);

#endif // EMU_H
