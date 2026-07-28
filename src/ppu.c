#include "emu.h"

// 1. Clock Management & Mode Transitions
static void ppu_change_mode(PPU* ppu, PPUMode new_mode);
static void check_lyc_stat_interrupt(PPU* ppu);

// 2. Scanline Rendering Helpers (Executed during Mode 3 -> Mode 0 transition)
static void ppu_render_scanline(PPU* ppu);
static void ppu_render_bg(PPU* ppu);
static void ppu_render_window(PPU* ppu);
static void ppu_render_sprites(PPU* ppu);

// 3. Palette Conversion
// Converts a 2-bit Game Boy color index (0-3) via BGP/OBP registers to RGBA8888
static uint32_t ppu_get_color(uint8_t palette_reg, uint8_t color_index);

void ppu_init(PPU* ppu);

void ppu_step(PPU* ppu, int cycles)
{
    // If LCD is disabled in LCDC (Bit 7 == 0), clear LCD and return
    if (!(ppu->lcdc & 0x80)) {
        ppu->ly = 0;
        ppu->dots = 0;
        ppu_change_mode(ppu, PPU_MODE_HBLANK);
        return;
    }

    ppu->dots += cycles;

    switch ((PPUMode)(ppu->stat & 0x03)) {
        case PPU_MODE_OAM:
            if (ppu->dots >= 80) {
                ppu_change_mode(ppu, PPU_MODE_XFER);
            }
            break;

        case PPU_MODE_XFER:
            if (ppu->dots >= 252) { // 80 + 172
                ppu_render_scanline(ppu); // Draw current line LY to frame_buffer
                ppu_change_mode(ppu, PPU_MODE_HBLANK);
            }
            break;

        case PPU_MODE_HBLANK:
            if (ppu->dots >= 456) {
                ppu->dots -= 456;
                ppu->ly++;

                if (ppu->ly == 144) {
                    ppu_change_mode(ppu, PPU_MODE_VBLANK);
                    ppu->frame_ready = true;
                    ppu->vblank_interrupt = true; // Request INT 0x40
                } else {
                    ppu_change_mode(ppu, PPU_MODE_OAM);
                }
                check_lyc_stat_interrupt(ppu);
            }
            break;

        case PPU_MODE_VBLANK:
            if (ppu->dots >= 456) {
                ppu->dots -= 456;
                ppu->ly++;

                if (ppu->ly > 153) { // End of VBlank, restart frame
                    ppu->ly = 0;
                    ppu_change_mode(ppu, PPU_MODE_OAM);
                }
                check_lyc_stat_interrupt(ppu);
            }
            break;
    }
}

uint8_t ppu_read(const PPU* ppu, uint16_t addr);
void ppu_write(PPU* ppu, uint16_t addr, uint8_t value);
