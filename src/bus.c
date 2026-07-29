#include "emu.h"
#include <stddef.h>

uint8_t bus_read(Bus *bus, uint16_t addr)
{
    if (addr < 0x8000) return bus->rom[addr];
    if (addr >= 0x8000 && addr < 0xA000) return bus->vram[addr - 0x8000];
    if (addr >= 0xC000 && addr < 0xE000) return bus->wram[addr - 0xC000];
    if (addr >= 0xFE00 && addr < 0xFEA0) return bus->oam[addr - 0xFE00];
    if (addr >= 0xFF00 && addr < 0xFF80) return bus->io[addr - 0xFF00];
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
        // External RAM / Cartridge RAM (if supported by MBC)
        return;
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

    return bytes_read;
}
