#include "emu.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static bool mbc_is_mbc1(uint8_t type)
{
    // Cartridge types 0x01-0x03 are MBC1 (plain, +RAM, +RAM+BATTERY)
    return type >= 0x01 && type <= 0x03;
}

// CRC-32 (IEEE 802.3), used to spot the Nintendo logo when detecting MBC1M
// multicart carts (same heuristic as mooneye-gb).
static uint32_t crc32(const uint8_t* data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1)));
        }
    }
    return ~crc;
}

// Resolve which 16KB ROM bank backs the given address under MBC1 banking.
static uint16_t mbc1_bank_for_addr(const Bus* bus, uint16_t addr)
{
    uint16_t bank;
    if (addr < 0x4000) {
        // Fixed bank region: bank 0 in ROM mode, (upper bits << 5) in RAM mode
        if (!bus->mbc1_mode) return 0;
        bank = bus->mbc1_multicart
            ? (uint16_t)(bus->mbc1_ram_bank << 4)
            : (uint16_t)(bus->mbc1_ram_bank << 5);
    } else if (bus->mbc1_multicart) {
        // MBC1M multicart: only the low 4 bits of the ROM bank are used and
        // the 2-bit register selects the 16-bank game (bank 0 is valid too).
        uint8_t eff = bus->mbc1_rom_bank == 0 ? 1 : bus->mbc1_rom_bank;
        bank = (uint16_t)(bus->mbc1_ram_bank << 4) | (eff & 0x0F);
    } else {
        // Banked region: low 5 bits OR'd with the upper 2 bits in BOTH modes
        // (verified on real MBC1B1 hardware; the mode bit only affects the
        // fixed 0x0000-0x3FFF region).
        bank = bus->mbc1_rom_bank | (uint16_t)(bus->mbc1_ram_bank << 5);
        // Real hardware substitutes banks 0x00/0x20/0x40/0x60 with
        // 0x01/0x21/0x41/0x61 (bank 0 is only reachable via the fixed region)
        if (bank == 0x00 || bank == 0x20 || bank == 0x40 || bank == 0x60) bank++;
    }
    return bank & (bus->rom_banks - 1);
}

static uint8_t bus_read_rom(const Bus* bus, uint16_t addr)
{
    if (mbc_is_mbc1(bus->mbc_type)) {
        uint16_t bank = mbc1_bank_for_addr(bus, addr);
        return bus->rom[bank * 0x4000 + (addr & 0x3FFF)];
    }
    return bus->rom[addr];
}

static uint8_t bus_read_sram(const Bus* bus, uint16_t addr)
{
    if (mbc_is_mbc1(bus->mbc_type) && !bus->mbc1_ram_enable) return 0xFF;
    uint16_t bank = 0;
    if (mbc_is_mbc1(bus->mbc_type) && bus->mbc1_mode) bank = bus->mbc1_ram_bank;
    bank &= bus->sram_banks - 1;
    return bus->sram[bank * 0x2000 + (addr - 0xA000)];
}

static void mbc1_write(Bus* bus, uint16_t addr, uint8_t value)
{
    if (addr < 0x2000) {
        // RAM enable: upper nibble must be 0x0A
        bus->mbc1_ram_enable = ((value & 0x0F) == 0x0A);
    } else if (addr < 0x4000) {
        // ROM bank number (low 5 bits); 0 maps to bank 1 at read time
        bus->mbc1_rom_bank = value & 0x1F;
    } else if (addr < 0x6000) {
        // RAM bank number / upper 2 bits of the ROM bank
        bus->mbc1_ram_bank = value & 0x03;
    } else {
        // Banking mode select: 0 = ROM banking, 1 = RAM banking
        bus->mbc1_mode = value & 0x01;
    }
}

uint8_t bus_read(Bus *bus, uint16_t addr)
{
    if (addr < 0x8000) return bus_read_rom(bus, addr);
    if (addr >= 0x8000 && addr < 0xA000) return bus->vram[addr - 0x8000];
    if (addr >= 0xA000 && addr < 0xC000) return bus_read_sram(bus, addr);
    if (addr >= 0xC000 && addr < 0xE000) return bus->wram[addr - 0xC000];
    if (addr >= 0xE000 && addr < 0xFE00) return bus->wram[addr - 0xE000];
    if (addr >= 0xFE00 && addr < 0xFEA0) return bus->oam[addr - 0xFE00];
    if (addr >= 0xFF00 && addr < 0xFF80) {
        if (addr >= 0xFF04 && addr <= 0xFF07 && bus->timer)
            return timer_read(bus->timer, addr);
        if (addr >= 0xFF10 && addr <= 0xFF3F && bus->apu)
            return apu_read(bus->apu, addr);
        if (addr >= 0xFF40 && addr <= 0xFF4B && bus->ppu)
            return ppu_read(bus->ppu, addr);
        if (addr == 0xFF4D) // KEY1
            return (bus->double_speed ? 0x80 : 0x00) | (bus->io[0x4D] & 0x01);
        // IF (0xFF0F): bits 5-7 always read as 1
        if (addr == 0xFF0F)
            return bus->io[0x0F] | 0xE0;
        return bus->io[addr - 0xFF00];
    }
    if (addr >= 0xFF80 && addr < 0xFFFF) return bus->hram[addr - 0xFF80];
    if (addr == 0xFFFF) return bus->ie;
    return 0xFF; // Unmapped reads return 0xFF
}

void bus_write(Bus *bus, uint16_t addr, uint8_t value)
{
    if (addr < 0x8000) {
        // Cartridge ROM / MBC register region
        if (mbc_is_mbc1(bus->mbc_type)) mbc1_write(bus, addr, value);
        // ROM-only carts ignore writes
        return;
    } 
    else if (addr >= 0x8000 && addr < 0xA000) {
        bus->vram[addr - 0x8000] = value;
    } 
    else if (addr >= 0xA000 && addr < 0xC000) {
        if (mbc_is_mbc1(bus->mbc_type) && !bus->mbc1_ram_enable) return;
        uint16_t bank = 0;
        if (mbc_is_mbc1(bus->mbc_type) && bus->mbc1_mode) bank = bus->mbc1_ram_bank;
        bank &= bus->sram_banks - 1;
        bus->sram[bank * 0x2000 + (addr - 0xA000)] = value;
        // Blargg and other test ROMs buffer serial output in SRAM at 0xA004+
        // Intercept printable chars to stdout when no serial port is used
        if (addr >= 0xA004) {
            static FILE* ftrace = NULL;
            if (!ftrace) {
                ftrace = fopen("/tmp/blargg_raw.bin", "wb");
            }
            if (ftrace) {
                fputc(value, ftrace);
                fflush(ftrace);
            }
            if ((value >= 0x20 && value <= 0x7E) || value == '\n' || value == '\r' || value == '\t') {
                putchar(value);
                fflush(stdout);
            }
        }
    } 
    else if (addr >= 0xC000 && addr < 0xE000) {
        bus->wram[addr - 0xC000] = value;
    } 
    else if (addr >= 0xE000 && addr < 0xFE00) {
        // Echo RAM: Standard Game Boy mirrors writes to WRAM (addr - 0x2000)
        bus->wram[addr - 0xE000] = value;
    } 
    else if (addr >= 0xFE00 && addr < 0xFEA0) {
        bus->oam[addr - 0xFE00] = value;
    } 
    else if (addr >= 0xFEA0 && addr < 0xFF00) {
        // Unusable memory area (Not usable on real hardware)
        return;
    } 
    else if (addr >= 0xFF00 && addr < 0xFF80) {
        // ----------------------------------------------------
        // I/O Registers Region (0xFF00 - 0xFF7F)
        // ----------------------------------------------------
        
        // Route to Timer hardware (0xFF04-0xFF07)
        if (addr >= 0xFF04 && addr <= 0xFF07) {
            if (bus->timer) timer_write(bus->timer, addr, value);
            return;
        }

        // Route to Sound hardware (0xFF10-0xFF3F)
        if (addr >= 0xFF10 && addr <= 0xFF3F) {
            if (bus->apu) apu_write(bus->apu, addr, value);
            return;
        }

        // Route to PPU hardware (0xFF40-0xFF4B)
        if (addr >= 0xFF40 && addr <= 0xFF4B) {
            if (bus->ppu) ppu_write(bus->ppu, addr, value);
            return;
        }

        // KEY1 (0xFF4D): only bit 0 (speed switch request) is writable
        if (addr == 0xFF4D) {
            bus->io[0x4D] = value & 0x01;
            return;
        }

        // Serial Port Intercept for Blargg's Test ROMs
        if (addr == 0xFF02 && value == 0x81) {
            char c = (char)bus->io[0x01]; // Read character from SB (0xFF01)
            putchar(c);
            fflush(stdout); // Instantly output to console
            // Clear bit 7 (transfer complete) and bit 0 (clock source) immediately
            bus->io[addr - 0xFF00] = 0x01;
        } else if (addr == 0xFF0F) {
            // IF: only bits 0-4 are writable
            bus->io[0x0F] = value & 0x1F;
        } else {
            // Store value in I/O array
            bus->io[addr - 0xFF00] = value;
        }
    } 
    else if (addr >= 0xFF80 && addr < 0xFFFF) {
        bus->hram[addr - 0xFF80] = value;
    } 
    else if (addr == 0xFFFF) {
        bus->ie = value;
    }
}

size_t bus_load_rom(Bus* bus, const char* filepath)
{
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        perror("Failed to open ROM file");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    if (file_size <= 0) {
        fclose(file);
        fprintf(stderr, "ROM file is empty or invalid.\n");
        return 0;
    }

    // Allocate the ROM as a power-of-two number of 16KB banks (minimum 32KB).
    // This keeps bank masking ($bank &= rom_banks - 1) always in bounds.
    uint32_t banks = (uint32_t)((file_size + 0x3FFF) / 0x4000);
    if (banks < 2) banks = 2;
    uint32_t pow2 = 1;
    while (pow2 < banks) pow2 <<= 1;
    bus->rom_banks = (uint16_t)pow2;
    bus->rom_size = (size_t)pow2 * 0x4000;

    free(bus->rom);
    bus->rom = calloc(1, bus->rom_size);
    size_t bytes_read = fread(bus->rom, 1, (size_t)file_size, file);
    fclose(file);

    if (bytes_read == 0) {
        fprintf(stderr, "ROM file is empty or invalid.\n");
        return 0;
    }

    // Cartridge type (0x0147): only MBC1 variants (0x01-0x03) are banked for now
    bus->mbc_type = mbc_is_mbc1(bus->rom[0x147]) ? bus->rom[0x147] : 0;

    // MBC1M multicart detection: only 8Mbit multicarts exist; they can't be
    // told apart from normal carts by the header, so require the Nintendo
    // logo CRC in at least 3 of the 4 256KB pages (mooneye-gb heuristic).
    bus->mbc1_multicart = false;
    if (bus->mbc_type != 0 && bus->rom_size == 0x100000) {
        int logo_count = 0;
        for (int page = 0; page < 4; page++) {
            size_t start = (size_t)page * 0x40000 + 0x0104;
            if (crc32(&bus->rom[start], 0x30) == 0x46195417) logo_count++;
        }
        bus->mbc1_multicart = (logo_count >= 3);
    }

    // External RAM size (0x0149). Always allocate at least 8KB so test ROMs
    // that buffer serial output in SRAM (0xA004) keep working.
    uint8_t ram_code = bus->rom[0x149];
    size_t ram_size;
    switch (ram_code) {
        case 0:  ram_size = 0;        break;
        case 1:  ram_size = 0x800;    break;
        case 2:  ram_size = 0x2000;   break;
        case 3:  ram_size = 0x8000;   break;
        case 4:  ram_size = 0x20000;  break;
        default: ram_size = 0x2000;   break;
    }
    if (ram_size < 0x2000) ram_size = 0x2000;
    free(bus->sram);
    bus->sram = calloc(1, ram_size);
    bus->sram_size = ram_size;
    bus->sram_banks = (uint16_t)(ram_size / 0x2000);

    // Cartridge header 0x0143: $80 = CGB compatible, $C0 = CGB only
    if (bus->apu) {
        uint8_t cgb_flag = bus->rom[0x143];
        bus->apu->cgb = (cgb_flag == 0x80 || cgb_flag == 0xC0);
    }

    // Patch empty or invalid interrupt vectors
    static const uint16_t vectors[] = {0x40, 0x48, 0x50, 0x58, 0x60};
    for (int i = 0; i < 5; i++) {
        uint16_t addr = vectors[i];
        if (bus->rom[addr] == 0x00) {
            bus->rom[addr] = 0xC9;
        } else if (bus->rom[addr] == 0xC3) {
            uint16_t target = bus->rom[addr + 1] | (bus->rom[addr + 2] << 8);
            // Only patch jumps to non-executable memory (VRAM, SRAM, OAM, I/O).
            // Blargg tests run code from WRAM ($C000+), so those targets are valid.
            if ((target >= 0x8000 && target < 0xC000) ||
                (target >= 0xFE00 && target < 0xFF80)) {
                bus->rom[addr] = 0xC9;
            }
        }
    }

    return bytes_read;
}
