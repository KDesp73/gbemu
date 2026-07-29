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
    if (addr >= 0x8000 && addr < 0xA000) bus->vram[addr - 0x8000] = value;
    else if (addr >= 0xC000 && addr < 0xE000) bus->wram[addr - 0xC000] = value;
    else if (addr >= 0xFE00 && addr < 0xFEA0) bus->oam[addr - 0xFE00] = value;
    else if (addr >= 0xFF00 && addr < 0xFF80) bus->io[addr - 0xFF00] = value;
    else if (addr >= 0xFF80 && addr < 0xFFFF) bus->hram[addr - 0xFF80] = value;
    else if (addr == 0xFFFF) bus->ie = value;
    // NOTE: Writing to 0x0000-0x7FFF is trapped by Memory Bank Controllers (MBCs)
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
