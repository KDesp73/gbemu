#ifndef GBEMU_H
#define GBEMU_H

//@module gbemu
//@author Konstantinos Despoinidis (KDesp73)
//@license MIT

//@const GB_VERSION_MAJOR
//@desc Major version number (incremented on breaking changes)
#define GB_VERSION_MAJOR 0

//@const GB_VERSION_MINOR
//@desc Minor version number (incremented on new features, backward-compatible)
#define GB_VERSION_MINOR 1

//@const GB_VERSION_PATCH
//@desc Patch version number (incremented on bug fixes)
#define GB_VERSION_PATCH 0

#define GB_STR(x) #x
#define GB_TOSTRING(x) GB_STR(x)

//@macro GB_VERSION_STRING
//@desc Human-readable version string in "MAJOR.MINOR.PATCH" format
#define GB_VERSION_STRING GB_TOSTRING(GB_VERSION_MAJOR) "." GB_TOSTRING(GB_VERSION_MINOR) "." GB_TOSTRING(GB_VERSION_PATCH)

//@macro GB_VERSION_HEX
//@desc Compact numeric version: MAJOR * 10000 + MINOR * 100 + PATCH
#define GB_VERSION_HEX ((GB_VERSION_MAJOR * 10000) + (GB_VERSION_MINOR * 100) + GB_VERSION_PATCH)

//@func gb_version
//@desc Get the current library version as individual components
//@param major Pointer to receive the major version (may be NULL)
//@param minor Pointer to receive the minor version (may be NULL)
//@param patch Pointer to receive the patch version (may be NULL)
static inline void gb_version(int* major, int* minor, int* patch) {
    if (major) *major = GB_VERSION_MAJOR;
    if (minor) *minor = GB_VERSION_MINOR;
    if (patch) *patch = GB_VERSION_PATCH;
}


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "frontend.h"

//@module cpu

//@type gb_cpu
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
} gb_cpu;

//@func gb_cpu_init
//@desc Initialize CPU state to boot values
//@param cpu CPU pointer to initialize
void gb_cpu_init(gb_cpu* cpu);

//@func gb_cpu_dump_fd
//@desc Dump CPU register state to a file descriptor
//@param cpu CPU state to dump
//@param fd Output file descriptor (e.g. stdout)
void gb_cpu_dump_fd(gb_cpu cpu, FILE* fd);

//@func gb_cpu_dump
//@desc Dump CPU register state to stdout
//@param cpu CPU state to dump
#define gb_cpu_dump(cpu) gb_cpu_dump_fd(cpu, stdout)

//@enum gb_flag
//@desc CPU flag bit positions in the F register
//@ref https://gbdev.io/pandocs/CPU_Registers_and_Flags.html
typedef enum {
    GB_FLAG_Z = (1 << 7),
    GB_FLAG_N = (1 << 6),
    GB_FLAG_H = (1 << 5),
    GB_FLAG_C = (1 << 4),
} gb_flag;

//@func gb_flag_set
//@desc Set or clear a specific CPU flag
//@param cpu CPU whose flag to modify
//@param flag The flag bit to set or clear
//@param value true to set, false to clear
void gb_flag_set(gb_cpu* cpu, gb_flag flag, bool value);

//@func gb_flag_get
//@desc Read the value of a specific CPU flag
//@param cpu CPU to read from
//@param flag The flag bit to read
//@returns true if the flag is set, false otherwise
bool gb_flag_get(const gb_cpu* cpu, gb_flag flag);

//@module memory

//@type gb_bus
//@desc Memory bus representation
//@ref https://gbdev.io/pandocs/Memory_Map.html
typedef struct gb_bus gb_bus;

struct gb_bus {
    uint8_t* rom;           // Dynamically allocated cartridge ROM (all banks)
    size_t rom_size;        // Total allocated ROM size in bytes
    uint16_t rom_banks;     // Number of 16KB ROM banks (power of two)

    uint8_t vram[0x2000];   // 8KB Video RAM
    uint8_t wram[0x2000];   // 8KB Work RAM
    uint8_t* sram;          // Dynamically allocated cartridge SRAM (0xA000-0xBFFF)
    size_t sram_size;       // Total allocated SRAM size in bytes
    uint16_t sram_banks;    // Number of 8KB SRAM banks
    bool sram_dirty;        // Set once the cart writes to SRAM/RTC (battery save hint)

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

    struct gb_timer* timer;    // For routing timer register reads/writes
    struct gb_ppu* ppu;        // For routing PPU register reads/writes
    struct gb_apu* apu;        // For routing sound register reads/writes

    bool double_speed;      // CGB double-speed mode (CPU T-cycle = 0.5 system cycle)

    // Joypad state (active-low: 0 = pressed)
    // Bit layout: bit 7 = Start, 6 = Select, 5 = B, 4 = A
    //             bit 3 = Down, 2 = Up, 1 = Left, 0 = Right
    uint8_t joypad_buttons; // Face buttons + Start/Select (active-low)
    uint8_t joypad_dpad;    // D-pad buttons (active-low)
    bool joypad_interrupt;  // Joypad interrupt pending (IF bit 4)

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

//@func gb_bus_read
//@desc Read a byte from the memory-mapped bus
//@param bus Memory bus to read from
//@param addr 16-bit address to read
//@returns Byte value at the given address
uint8_t gb_bus_read(gb_bus* bus, uint16_t addr);

//@func gb_bus_write
//@desc Write a byte to the memory-mapped bus
//@param bus Memory bus to write to
//@param addr 16-bit address to write
//@param value Byte value to write
void gb_bus_write(gb_bus* bus, uint16_t addr, uint8_t value);

//@func gb_bus_tick
//@desc Advance the bus by one M-cycle (drives the OAM DMA transfer)
//@param bus Memory bus to advance
void gb_bus_tick(gb_bus* bus);

//@func gb_bus_load_rom
//@desc Load a cartridge ROM file into the bus (allocating all ROM banks), set up MBC1 bank switching and SRAM from the cartridge header, and set CGB mode
//@param bus Memory bus to load into
//@param filepath Path to the ROM file to load
//@returns Number of bytes read, or 0 on failure
size_t gb_bus_load_rom(gb_bus* bus, const char* filepath);

//@module misc

//@func gb_get_reg_by_index
//@desc Get a CPU register value by its 3-bit index (0-7)
//@param cpu CPU to read from
//@param bus Memory bus for HL indirect reads
//@param index Register index (0=B,1=C,2=D,3=E,4=H,5=L,6=(HL),7=A)
//@returns Byte value of the register
uint8_t gb_get_reg_by_index(gb_cpu* cpu, gb_bus* bus, uint8_t index);

//@func gb_set_reg_by_index
//@desc Set a CPU register value by its 3-bit index (0-7)
//@param cpu CPU to write to
//@param bus Memory bus for HL indirect writes
//@param index Register index (0=B,1=C,2=D,3=E,4=H,5=L,6=(HL),7=A)
//@param value Byte value to write
void gb_set_reg_by_index(gb_cpu* cpu, gb_bus* bus, uint8_t index, uint8_t value);

//@func gb_fetch8
//@desc Fetch the next byte from PC and advance PC by 1
//@param cpu CPU to fetch from
//@param bus Memory bus to read from
//@returns The fetched byte
uint8_t gb_fetch8(gb_cpu* cpu, gb_bus* bus);

//@func gb_fetch16
//@desc Fetch the next 16-bit value from PC (little-endian) and advance PC by 2
//@param cpu CPU to fetch from
//@param bus Memory bus to read from
//@returns The fetched 16-bit value
uint16_t gb_fetch16(gb_cpu* cpu, gb_bus* bus);

//@module timer

//@type gb_timer
//@desc Game Boy timer hardware (DIV, TIMA, TMA, TAC registers)
//@ref https://gbdev.io/pandocs/Timer_and_Divider_Registers.html
typedef struct gb_timer {
    uint16_t internal_counter; // 16-bit internal clock (DIV is the upper byte)
    
    uint8_t tima; // 0xFF05
    uint8_t tma;  // 0xFF06
    uint8_t tac;  // 0xFF07

    bool interrupt_requested; // Set to true when TIMA overflows
    bool tima_reload_pending; // TIMA overflow reload is delayed by 1 cycle
} gb_timer;

//@func gb_timer_init
//@desc Initialize timer state to default values
//@param timer Timer pointer to initialize
void gb_timer_init(gb_timer* timer);

//@func gb_timer_step
//@desc Advance the timer by the given number of T-cycles
//@param timer Timer state to update
//@param cycles Number of T-cycles elapsed
void gb_timer_step(gb_timer* timer, int cycles);

//@func gb_timer_read
//@desc Read a timer register value
//@param timer Timer state to read from
//@param addr Register address (0xFF04-0xFF07)
//@returns Register byte value
uint8_t gb_timer_read(const gb_timer* timer, uint16_t addr);

//@func gb_timer_write
//@desc Write a value to a timer register
//@param timer Timer state to write to
//@param addr Register address (0xFF04-0xFF07)
//@param value Byte value to write
void gb_timer_write(gb_timer* timer, uint16_t addr, uint8_t value);

//@func gb_get_time_ns
//@desc Get the current monotonic clock time in nanoseconds
//@returns Monotonic time in nanoseconds since an arbitrary epoch
uint64_t gb_get_time_ns(void);

//@func gb_sleep_ns
//@desc Suspend execution for at least the given duration. On RTOS targets (ESP-IDF) this yields to the scheduler; on POSIX hosts it maps to nanosleep(2)
//@param ns Sleep duration in nanoseconds
void gb_sleep_ns(uint64_t ns);

//@module ppu

//@macro GB_SCREEN_WIDTH
//@desc Game Boy native screen width in pixels
#define GB_SCREEN_WIDTH 160

//@macro GB_SCREEN_HEIGHT
//@desc Game Boy native screen height in pixels
#define GB_SCREEN_HEIGHT 144

//@enum gb_ppu_mode
//@desc PPU mode states stored in the lower 2 bits of STAT (0xFF41)
//@ref https://gbdev.io/pandocs/STAT_Register.html
typedef enum {
    GB_PPU_MODE_HBLANK = 0, // Mode 0: Horizontal Blank (204 M-cycles)
    GB_PPU_MODE_VBLANK = 1, // Mode 1: Vertical Blank (4560 M-cycles / Scanlines 144-153)
    GB_PPU_MODE_OAM    = 2, // Mode 2: Searching OAM for sprites (80 M-cycles)
    GB_PPU_MODE_XFER   = 3  // Mode 3: Transferring pixel data to LCD (172 M-cycles)
} gb_ppu_mode;

//@type gb_ppu
//@desc Pixel Processing Unit - renders scanlines to the frame buffer
//@ref https://gbdev.io/pandocs/PPU.html
typedef struct gb_ppu {
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
    bool frame_ready;        // Set to true at VBlank, signals SDL to render
    bool vblank_interrupt;   // Set to true to request INT 0x40
    bool stat_interrupt;     // Set to true to request INT 0x48

    // Pixel buffer for SDL. Kept last so save states can serialize the whole
    // register/state prefix with offsetof(PPU, frame_buffer); the pixels are
    // redrawn within one frame after a restore.
    uint32_t frame_buffer[GB_SCREEN_HEIGHT][GB_SCREEN_WIDTH];
} gb_ppu;

//@func gb_ppu_init
//@desc Initialize PPU registers and frame buffer to default values
//@param ppu PPU pointer to initialize
void gb_ppu_init(gb_ppu* ppu);

//@func gb_ppu_step
//@desc Advance the PPU state machine by the given T-cycles
//@param ppu PPU state to update
//@param bus Memory bus for VRAM and OAM reads
//@param cycles Number of T-cycles elapsed
void gb_ppu_step(gb_ppu* ppu, gb_bus* bus, int cycles);

//@func gb_ppu_read
//@desc Read a PPU register value
//@param ppu PPU state to read from
//@param addr Register address (0xFF40-0xFF4B)
//@returns Register byte value
uint8_t gb_ppu_read(const gb_ppu* ppu, uint16_t addr);

//@func gb_ppu_write
//@desc Write a value to a PPU register
//@param ppu PPU state to write to
//@param addr Register address (0xFF40-0xFF4B)
//@param value Byte value to write
void gb_ppu_write(gb_ppu* ppu, uint16_t addr, uint8_t value);

//@func gb_handle_interrupts
//@desc Service pending hardware interrupts: sync PPU/Timer requests into IF (0xFF0F), unhalt the CPU if a pending interrupt is enabled, and jump to the interrupt vector if IME is set
//@param cpu CPU state to mutate
//@param bus Memory bus for IF register reads/writes
//@param ppu PPU providing vblank/stat interrupt requests
//@param timer Timer providing TIMA overflow interrupt requests
//@returns Number of T-cycles consumed by interrupt servicing (0 if none serviced)
int gb_handle_interrupts(gb_cpu* cpu, gb_bus* bus, gb_ppu* ppu, gb_timer* timer);

//@module apu

// Audio ring buffer capacity (must be power of two)
#define GB_APU_BUF_SIZE 4096
#define GB_APU_BUF_MASK (GB_APU_BUF_SIZE - 1)

// Sample rate for audio output
#define GB_APU_SAMPLE_RATE 44100

//@type gb_apu
//@desc Game Boy APU with full 4-channel waveform synthesis
//@ref https://gbdev.io/pandocs/Audio.html
typedef struct gb_apu {
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

    // Downsampling accumulator (dot clock -> sample rate)
    uint32_t sample_accum;   // Dots accumulated since last sample

    // Audio output ring buffer (SPSC, float mono). Not serialized: the
    // indices are reset on save-state load.
    float audio_buf[GB_APU_BUF_SIZE];
    volatile uint32_t buf_write; // Write index (producer: apu_step)
    volatile uint32_t buf_read;  // Read index (consumer: SDL callback)
} gb_apu;

//@func gb_apu_init
//@desc Initialize APU state to default values
//@param apu APU pointer to initialize
void gb_apu_init(gb_apu* apu);

//@func gb_apu_step
//@desc Advance the APU frame sequencer by the given number of dots (system cycles)
//@param apu APU state to update
//@param dots Number of dot clock cycles elapsed (fixed 4.194304 MHz)
void gb_apu_step(gb_apu* apu, int dots);

//@func gb_apu_read
//@desc Read a sound register value
//@param apu APU state to read from
//@param addr Register address (0xFF10-0xFF3F)
//@returns Register byte value
uint8_t gb_apu_read(const gb_apu* apu, uint16_t addr);

//@func gb_apu_write
//@desc Write a value to a sound register
//@param apu APU state to write to
//@param addr Register address (0xFF10-0xFF3F)
//@param value Byte value to write
void gb_apu_write(gb_apu* apu, uint16_t addr, uint8_t value);

//@func gb_apu_buf_pop
//@desc Pop a single float sample from the APU audio ring buffer (called from audio callback)
//@param apu APU state to read from
//@returns Audio sample in range [-1.0, 1.0], or 0.0 if buffer empty
float gb_apu_buf_pop(gb_apu* apu);

//@module exec

//@macro GB_CPU_FREQ
//@desc CGB dot clock frequency in Hz (8.388608 MHz), the emulator's base time unit
#define GB_CPU_FREQ 8388608

//@macro GB_CYCLES_PER_FRAME
//@desc System cycles per frame at ~60 Hz (~139,810 dots); constant across both CPU speed modes
#define GB_CYCLES_PER_FRAME (GB_CPU_FREQ / 60)

//@macro GB_FRAME_TIME_NS
//@desc Frame duration in nanoseconds at 60 Hz (~16.66 ms)
#define GB_FRAME_TIME_NS (1000000000L / 60)

//@func gb_instr
//@desc Execute a single CPU instruction by opcode
//@param cpu CPU state to mutate
//@param bus Memory bus for reads/writes
//@param opcode The opcode byte to execute
//@returns Number of T-cycles the instruction consumed
int gb_instr(gb_cpu* cpu, gb_bus* bus, uint8_t opcode);

//@func gb_machine_tick
//@desc Advance every component except the CPU (timer, PPU, APU) by the given number of T-cycles. Called at each M-cycle boundary during instruction execution so bus accesses land on their exact hardware cycle slots.
//@param bus Memory bus providing access to the timer, PPU and APU
//@param t_cycles Number of T-cycles to advance the machine by
void gb_machine_tick(gb_bus* bus, int t_cycles);

//@func gb_cpu_step
//@desc Execute one full CPU step (fetch + decode + execute)
//@param cpu CPU state to mutate
//@param bus Memory bus for reads/writes
//@returns Number of T-cycles the step consumed
int gb_cpu_step(gb_cpu* cpu, gb_bus* bus);

//@func gb_loop
//@desc Main emulation loop: executes CPU steps until a frame's worth of cycles elapse, advances the timer/PPU/APU, services interrupts, and paces the frame to ~60 Hz
//@param cpu CPU state to run
//@param bus Memory bus for reads/writes
//@param timer Timer state to advance
//@param ppu PPU state to advance
//@param apu APU state to advance
//@param fe Frontend for display and input
void gb_loop(gb_cpu* cpu, gb_bus* bus, gb_timer* timer, gb_ppu* ppu, gb_apu* apu, gb_frontend* fe);

//@module saves

//@func gb_battery_load
//@desc Restore cartridge RAM from the battery save file next to the ROM (<rom>.sav). MBC2 carts restore the built-in 512x4-bit RAM; MBC3 timer carts also restore the RTC registers appended after the SRAM block
//@param bus Memory bus whose SRAM is restored (bus_load_rom must have run)
//@param rom_path Path to the ROM file (the .sav path is derived from it)
//@returns true if a save file was found and loaded, false otherwise
bool gb_battery_load(gb_bus* bus, const char* rom_path);

//@func gb_battery_save
//@desc Write cartridge RAM to the battery save file next to the ROM (<rom>.sav), but only for cartridges whose header declares a battery. MBC3 timer carts append the 5 RTC register bytes after the SRAM block
//@param bus Memory bus whose SRAM is written out
//@param rom_path Path to the ROM file (the .sav path is derived from it)
//@returns true if the save file was written, false otherwise
bool gb_battery_save(const gb_bus* bus, const char* rom_path);

//@func gb_save_state
//@desc Snapshot the full machine state (CPU, Bus, Timer, PPU, APU) into <rom>.state with a versioned header containing an FNV-1a hash of the loaded ROM
//@param cpu CPU state to serialize
//@param bus Memory bus to serialize (ROM/SRAM buffers by content, not pointer)
//@param timer Timer state to serialize
//@param ppu PPU state to serialize (frame buffer excluded)
//@param apu APU state to serialize (audio ring buffer excluded)
//@param rom_path Path to the ROM file (the .state path is derived from it)
//@returns true on success, false if the file could not be written
bool gb_save_state(const gb_cpu* cpu, const gb_bus* bus, const gb_timer* timer,
                const gb_ppu* ppu, const gb_apu* apu, const char* rom_path);

//@func gb_load_state
//@desc Restore a machine snapshot previously written by save_state. The same ROM must be loaded (hash-checked) and bus_load_rom must have run so the SRAM buffer exists; the APU ring buffer is reset to empty
//@param cpu CPU state to restore into
//@param bus Memory bus to restore into
//@param timer Timer state to restore into
//@param ppu PPU state to restore into
//@param apu APU state to restore into
//@param rom_path Path to the ROM file (the .state path is derived from it)
//@returns true on success, false if the file is missing or incompatible
bool gb_load_state(gb_cpu* cpu, gb_bus* bus, gb_timer* timer,
                gb_ppu* ppu, gb_apu* apu, const char* rom_path);

//@module debugger

//@macro GB_DEBUGGER_MAX_BREAKPOINTS
//@desc Maximum number of PC breakpoints a gb_debugger can store
#define GB_DEBUGGER_MAX_BREAKPOINTS 64

//@type gb_debugger
//@desc Debugger control state for one emulated machine: pause/resume/step control, a PC breakpoint list and a one-shot step-over target. The emulation loop consults it before every instruction; attach it to a frontend through gb_frontend::debug before calling gb_loop. The UI half lives in the frontend (frontend/debugger.c).
struct gb_debugger {
    gb_cpu* cpu;                                       // CPU whose execution is controlled
    gb_bus* bus;                                       // Bus used for breakpoint/disassembly reads
    bool paused;                                       // Execution frozen: gb_loop skips CPU steps
    int  steps_remaining;                              // Instructions left to run before auto-pausing
    bool skip_check_once;                              // Skip the next breakpoint check (continue/step while stopped on a breakpoint)
    bool has_temp_bp;                                  // A one-shot step-over breakpoint is pending
    uint16_t temp_bp;                                  // Address the pending step-over breakpoint fires at
    uint16_t breakpoints[GB_DEBUGGER_MAX_BREAKPOINTS]; // PC breakpoints (linear scan, order unspecified)
    int num_breakpoints;                               // Valid entries in breakpoints[]
};

//@func gb_debugger_init
//@desc Initialize a debugger and bind it to the CPU/bus it controls
//@param dbg Debugger to initialize
//@param cpu CPU whose execution the debugger controls
//@param bus Bus used for breakpoint and disassembly reads
void gb_debugger_init(gb_debugger* dbg, gb_cpu* cpu, gb_bus* bus);

//@func gb_debugger_has_breakpoint
//@desc Check whether a PC breakpoint is set
//@param dbg Debugger to query
//@param addr Address to look up
//@returns true if a breakpoint exists at addr
bool gb_debugger_has_breakpoint(const gb_debugger* dbg, uint16_t addr);

//@func gb_debugger_add_breakpoint
//@desc Add a PC breakpoint (ignored when already present or the list is full)
//@param dbg Debugger to modify
//@param addr Address that pauses execution when reached
//@returns true if the breakpoint was added
bool gb_debugger_add_breakpoint(gb_debugger* dbg, uint16_t addr);

//@func gb_debugger_remove_breakpoint
//@desc Remove a PC breakpoint if present
//@param dbg Debugger to modify
//@param addr Address to remove
//@returns true if a breakpoint was removed
bool gb_debugger_remove_breakpoint(gb_debugger* dbg, uint16_t addr);

//@func gb_debugger_toggle_breakpoint
//@desc Add the breakpoint if missing, remove it if present
//@param dbg Debugger to modify
//@param addr Address to toggle
//@returns true if a breakpoint is now set at addr, false otherwise
bool gb_debugger_toggle_breakpoint(gb_debugger* dbg, uint16_t addr);

//@func gb_debugger_pause
//@desc Freeze execution immediately and cancel any pending step request
//@param dbg Debugger to pause
void gb_debugger_pause(gb_debugger* dbg);

//@func gb_debugger_continue
//@desc Resume execution. The next instruction is exempt from breakpoint checks, so continuing while stopped on a breakpoint executes it instead of re-triggering immediately
//@param dbg Debugger to resume
void gb_debugger_continue(gb_debugger* dbg);

//@func gb_debugger_toggle_pause
//@desc Pause when running, resume when paused
//@param dbg Debugger to toggle
void gb_debugger_toggle_pause(gb_debugger* dbg);

//@func gb_debugger_step
//@desc Run exactly n instructions (they complete within the current frame), then pause again
//@param dbg Debugger to step
//@param n Instruction count (values below 1 are treated as 1)
void gb_debugger_step(gb_debugger* dbg, int n);

//@func gb_debugger_step_over
//@desc Step one instruction, but when it is a CALL/RST, set a one-shot breakpoint on the return address and continue so the callee runs to completion
//@param dbg Debugger to step
void gb_debugger_step_over(gb_debugger* dbg);

//@func gb_debugger_hit
//@desc Breakpoint predicate for the emulation loop: when a breakpoint (or the pending step-over target) matches pc, pause the debugger, cancel any step request and return true. The step-over breakpoint is consumed on hit
//@param dbg Debugger to check
//@param pc Program counter about to be executed
//@returns true when execution must stop at pc
bool gb_debugger_hit(gb_debugger* dbg, uint16_t pc);

//@module disasm

//@func gb_disasm
//@desc Decode one instruction at addr into a human-readable string (e.g. "LD A,$01", "JR $C123"). CB-prefixed instructions are folded into a single line; unknown and illegal opcodes render as "DB $XX"
//@param bus Bus to read the instruction bytes from
//@param addr Address of the first instruction byte
//@param out Buffer receiving the NUL-terminated text
//@param out_size Buffer size in bytes
//@returns Length of the instruction in bytes (1-3), or 0 if the buffer is too small
int gb_disasm(gb_bus* bus, uint16_t addr, char* out, size_t out_size);

//@func gb_instruction_length
//@desc Length in bytes of the instruction at addr (used by step-over)
//@param bus Bus to read the opcode from
//@param addr Address of the first instruction byte
//@returns Instruction length in bytes (1-3)
int gb_instruction_length(gb_bus* bus, uint16_t addr);

//@module prefix
//@desc GBEMU_STRIP_PREFIX lives at the end of the header so that every symbol
//@desc above is declared in its real, prefixed form before the aliases are
//@desc introduced. Each alias maps the unprefixed spelling to the prefixed
//@desc one, so `cpu_init` expands to `gb_cpu_init` and the ABI never changes.

//@macro GBEMU_STRIP_PREFIX
//@desc Define this before including gbemu.h (or frontend.h) to alias every prefixed symbol to its unprefixed spelling, so a program that links nothing else can keep writing `CPU`, `ppu_init` or `SCREEN_WIDTH` while the library itself still exports the `gb_`/`GB_` names. This deliberately gives up namespacing: it redefines very common words (`loop`, `version`, `STR`) process-wide, so never enable it alongside another library or code of your own that uses those names. Prefer passing it on the command line (-DGBEMU_STRIP_PREFIX) over defining it in a source file, so it cannot leak into unrelated translation units.
#ifdef GBEMU_STRIP_PREFIX
    //@type CPU
    #define CPU gb_cpu
    //@type Bus
    #define Bus gb_bus
    //@type Timer
    #define Timer gb_timer
    //@type PPU
    #define PPU gb_ppu
    //@type APU
    #define APU gb_apu
    //@type Frontend
    #define Frontend gb_frontend
    //@type Flag
    #define Flag gb_flag
    //@type PPUMode
    #define PPUMode gb_ppu_mode
    //@type Hotkey
    #define Hotkey gb_hotkey
    //@type Debugger
    #define Debugger gb_debugger

    //@func version
    #define version gb_version
    //@func cpu_init
    #define cpu_init gb_cpu_init
    //@func cpu_dump_fd
    #define cpu_dump_fd gb_cpu_dump_fd
    // gb_cpu_dump is a function-like macro, so it has to be undefined before
    // the object-like alias below can take the name over.
    #undef cpu_dump
    //@func cpu_dump
    #define cpu_dump gb_cpu_dump
    //@func cpu_step
    #define cpu_step gb_cpu_step
    //@func flag_set
    #define flag_set gb_flag_set
    //@func flag_get
    #define flag_get gb_flag_get
    //@func bus_read
    #define bus_read gb_bus_read
    //@func bus_write
    #define bus_write gb_bus_write
    //@func bus_tick
    #define bus_tick gb_bus_tick
    //@func bus_load_rom
    #define bus_load_rom gb_bus_load_rom
    //@func get_reg_by_index
    #define get_reg_by_index gb_get_reg_by_index
    //@func set_reg_by_index
    #define set_reg_by_index gb_set_reg_by_index
    //@func fetch8
    #define fetch8 gb_fetch8
    //@func fetch16
    #define fetch16 gb_fetch16
    //@func timer_init
    #define timer_init gb_timer_init
    //@func timer_step
    #define timer_step gb_timer_step
    //@func timer_read
    #define timer_read gb_timer_read
    //@func timer_write
    #define timer_write gb_timer_write
    //@func get_time_ns
    #define get_time_ns gb_get_time_ns
    //@func sleep_ns
    #define sleep_ns gb_sleep_ns
    //@func ppu_init
    #define ppu_init gb_ppu_init
    //@func ppu_step
    #define ppu_step gb_ppu_step
    //@func ppu_read
    #define ppu_read gb_ppu_read
    //@func ppu_write
    #define ppu_write gb_ppu_write
    //@func handle_interrupts
    #define handle_interrupts gb_handle_interrupts
    //@func apu_init
    #define apu_init gb_apu_init
    //@func apu_step
    #define apu_step gb_apu_step
    //@func apu_read
    #define apu_read gb_apu_read
    //@func apu_write
    #define apu_write gb_apu_write
    //@func apu_buf_pop
    #define apu_buf_pop gb_apu_buf_pop
    //@func instr
    #define instr gb_instr
    //@func machine_tick
    #define machine_tick gb_machine_tick
    //@func loop
    #define loop gb_loop
    //@func battery_load
    #define battery_load gb_battery_load
    //@func battery_save
    #define battery_save gb_battery_save
    //@func save_state
    #define save_state gb_save_state
    //@func load_state
    #define load_state gb_load_state
    //@func debugger_init
    #define debugger_init gb_debugger_init
    //@func debugger_has_breakpoint
    #define debugger_has_breakpoint gb_debugger_has_breakpoint
    //@func debugger_add_breakpoint
    #define debugger_add_breakpoint gb_debugger_add_breakpoint
    //@func debugger_remove_breakpoint
    #define debugger_remove_breakpoint gb_debugger_remove_breakpoint
    //@func debugger_toggle_breakpoint
    #define debugger_toggle_breakpoint gb_debugger_toggle_breakpoint
    //@func debugger_pause
    #define debugger_pause gb_debugger_pause
    //@func debugger_continue
    #define debugger_continue gb_debugger_continue
    //@func debugger_toggle_pause
    #define debugger_toggle_pause gb_debugger_toggle_pause
    //@func debugger_step
    #define debugger_step gb_debugger_step
    //@func debugger_step_over
    #define debugger_step_over gb_debugger_step_over
    //@func debugger_hit
    #define debugger_hit gb_debugger_hit
    //@func disasm
    #define disasm gb_disasm
    //@func instruction_length
    #define instruction_length gb_instruction_length

    //@macro VERSION_MAJOR
    #define VERSION_MAJOR GB_VERSION_MAJOR
    //@macro VERSION_MINOR
    #define VERSION_MINOR GB_VERSION_MINOR
    //@macro VERSION_PATCH
    #define VERSION_PATCH GB_VERSION_PATCH
    //@macro VERSION_STRING
    #define VERSION_STRING GB_VERSION_STRING
    //@macro VERSION_HEX
    #define VERSION_HEX GB_VERSION_HEX
    //@macro STR
    #define STR GB_STR
    //@macro TOSTRING
    #define TOSTRING GB_TOSTRING
    //@macro SCREEN_WIDTH
    #define SCREEN_WIDTH GB_SCREEN_WIDTH
    //@macro SCREEN_HEIGHT
    #define SCREEN_HEIGHT GB_SCREEN_HEIGHT
    //@macro APU_BUF_SIZE
    #define APU_BUF_SIZE GB_APU_BUF_SIZE
    //@macro APU_BUF_MASK
    #define APU_BUF_MASK GB_APU_BUF_MASK
    //@macro APU_SAMPLE_RATE
    #define APU_SAMPLE_RATE GB_APU_SAMPLE_RATE
    //@macro CPU_FREQ
    #define CPU_FREQ GB_CPU_FREQ
    //@macro CYCLES_PER_FRAME
    #define CYCLES_PER_FRAME GB_CYCLES_PER_FRAME
    //@macro FRAME_TIME_NS
    #define FRAME_TIME_NS GB_FRAME_TIME_NS

    //@const FLAG_Z
    #define FLAG_Z GB_FLAG_Z
    //@const FLAG_N
    #define FLAG_N GB_FLAG_N
    //@const FLAG_H
    #define FLAG_H GB_FLAG_H
    //@const FLAG_C
    #define FLAG_C GB_FLAG_C
    //@const PPU_MODE_HBLANK
    #define PPU_MODE_HBLANK GB_PPU_MODE_HBLANK
    //@const PPU_MODE_VBLANK
    #define PPU_MODE_VBLANK GB_PPU_MODE_VBLANK
    //@const PPU_MODE_OAM
    #define PPU_MODE_OAM GB_PPU_MODE_OAM
    //@const PPU_MODE_XFER
    #define PPU_MODE_XFER GB_PPU_MODE_XFER
    //@const HOTKEY_SAVE_STATE
    #define HOTKEY_SAVE_STATE GB_HOTKEY_SAVE_STATE
    //@const HOTKEY_LOAD_STATE
    #define HOTKEY_LOAD_STATE GB_HOTKEY_LOAD_STATE
    //@const HOTKEY_DEBUG_PAUSE
    #define HOTKEY_DEBUG_PAUSE GB_HOTKEY_DEBUG_PAUSE
    //@macro DEBUGGER_MAX_BREAKPOINTS
    #define DEBUGGER_MAX_BREAKPOINTS GB_DEBUGGER_MAX_BREAKPOINTS
#endif // GBEMU_STRIP_PREFIX

#endif // GBEMU_H
