#include "emu.h"
#include <stddef.h>
#include <stdio.h>

uint8_t bus_read(Bus *bus, uint16_t addr)
{
    if (addr < 0x8000) return bus->rom[addr];
    if (addr >= 0x8000 && addr < 0xA000) return bus->vram[addr - 0x8000];
    if (addr >= 0xA000 && addr < 0xC000) return bus->sram[addr - 0xA000];
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
        return bus->io[addr - 0xFF00];
    }
    if (addr >= 0xFF80 && addr < 0xFFFF) return bus->hram[addr - 0xFF80];
    if (addr == 0xFFFF) return bus->ie;
    return 0xFF; // Unmapped reads return 0xFF
}

void bus_write(Bus *bus, uint16_t addr, uint8_t value)
{
    if (addr < 0x8000) {
        // ROM / MBC region: Read-only for base cartridge (MBC routing goes here later)
        return;
    } 
    else if (addr >= 0x8000 && addr < 0xA000) {
        bus->vram[addr - 0x8000] = value;
    } 
    else if (addr >= 0xA000 && addr < 0xC000) {
        bus->sram[addr - 0xA000] = value;
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

    // Read up to 32KB into the fixed cartridge ROM array
    size_t bytes_read = fread(bus->rom, 1, sizeof(bus->rom), file);
    fclose(file);

    if (bytes_read == 0) {
        fprintf(stderr, "ROM file is empty or invalid.\n");
        return 0;
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
