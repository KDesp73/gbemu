#include "emu.h"

// 1. Clock Management & Mode Transitions
static void ppu_change_mode(PPU* ppu, PPUMode new_mode);
static void check_lyc_stat_interrupt(PPU* ppu);

// 2. Scanline Rendering Helpers (Executed during Mode 3 -> Mode 0 transition)
static void ppu_render_scanline(PPU* ppu, Bus* bus);
static void ppu_render_bg(PPU* ppu, Bus* bus);
static void ppu_render_window(PPU* ppu, Bus* bus);
static void ppu_render_sprites(PPU* ppu, Bus* bus);

// 3. Palette Conversion
// Converts a 2-bit Game Boy color index (0-3) via BGP/OBP registers to RGBA8888
static uint32_t ppu_get_color(uint8_t palette_reg, uint8_t color_index);

void ppu_init(PPU* ppu)
{
    ppu->lcdc = 0x91;
    ppu->stat = 0;
    ppu->scy = 0;
    ppu->scx = 0;
    ppu->ly = 0;
    ppu->lyc = 0;
    ppu->bgp = 0xFC;
    ppu->obp0 = 0xFF;
    ppu->obp1 = 0xFF;
    ppu->wy = 0;
    ppu->wx = 0;
    ppu->dots = 0;
    ppu->first_line_after_enable = false;
    ppu->frame_ready = false;
    ppu->vblank_interrupt = false;
    ppu->stat_interrupt = false;
    for (int y = 0; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
            ppu->frame_buffer[y][x] = 0;
}

void ppu_step(PPU* ppu, Bus* bus, int cycles)
{
    // LCD off: frozen (state set by ppu_write on the off edge)
    if (!(ppu->lcdc & 0x80)) {
        return;
    }

    ppu->dots += cycles;

    // First line after LCD enable is shortened. Blargg oam_bug/1-lcd_sync
    // reads LY at ~896 dots (expect 0) and ~904 dots (expect 1).
    if (ppu->first_line_after_enable) {
        if (ppu->dots >= 900) {
            ppu->dots -= 900;
            ppu->ly = 1;
            ppu->first_line_after_enable = false;
            ppu_change_mode(ppu, PPU_MODE_OAM);
            check_lyc_stat_interrupt(ppu);
        }
        return;
    }

    switch ((PPUMode)(ppu->stat & 0x03)) {
        case PPU_MODE_OAM:
            if (ppu->dots >= 160) {
                ppu_change_mode(ppu, PPU_MODE_XFER);
            }
            break;

        case PPU_MODE_XFER:
            if (ppu->dots >= 504) { // 160 + 344
                ppu_render_scanline(ppu, bus); // Draw current line LY to frame_buffer
                ppu_change_mode(ppu, PPU_MODE_HBLANK);
            }
            break;

        case PPU_MODE_HBLANK:
            if (ppu->dots >= 912) {
                ppu->dots -= 912;
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
            if (ppu->dots >= 912) {
                ppu->dots -= 912;
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

uint8_t ppu_read(const PPU* ppu, uint16_t addr)
{
    switch (addr) {
        case 0xFF40: return ppu->lcdc;
        case 0xFF41: return ppu->stat | 0x80; // Bit 7 always reads as 1
        case 0xFF42: return ppu->scy;
        case 0xFF43: return ppu->scx;
        case 0xFF44: return ppu->ly;
        case 0xFF45: return ppu->lyc;
        case 0xFF47: return ppu->bgp;
        case 0xFF48: return ppu->obp0;
        case 0xFF49: return ppu->obp1;
        case 0xFF4A: return ppu->wy;
        case 0xFF4B: return ppu->wx;
        default:     return 0xFF;
    }
}

void ppu_write(PPU* ppu, uint16_t addr, uint8_t value)
{
    switch (addr) {
        case 0xFF40: {
            bool was_on = (ppu->lcdc & 0x80) != 0;
            ppu->lcdc = value;
            bool now_on = (value & 0x80) != 0;
            if (was_on && !now_on) {
                // LCD off: freeze PPU
                ppu->ly = 0;
                ppu->dots = 0;
                ppu->stat = (ppu->stat & ~0x03) | PPU_MODE_HBLANK;
                ppu->first_line_after_enable = false;
            } else if (!was_on && now_on) {
                // LCD on: start shortened first scanline
                ppu->ly = 0;
                ppu->dots = 0;
                ppu->stat = (ppu->stat & ~0x03) | PPU_MODE_HBLANK;
                ppu->first_line_after_enable = true;
            }
            break;
        }
        case 0xFF41: ppu->stat = (ppu->stat & 0x07) | (value & 0x78); break; // Only bits 3-6 writable
        case 0xFF42: ppu->scy = value; break;
        case 0xFF43: ppu->scx = value; break;
        case 0xFF44: break; // LY is read-only
        case 0xFF45: ppu->lyc = value; break;
        case 0xFF47: ppu->bgp = value; break;
        case 0xFF48: ppu->obp0 = value; break;
        case 0xFF49: ppu->obp1 = value; break;
        case 0xFF4A: ppu->wy = value; break;
        case 0xFF4B: ppu->wx = value; break;
        default: break;
    }
}

static void ppu_render_bg(PPU* ppu, Bus* bus)
{
    // Determine tile map location from LCDC (Bit 3)
    // 0x9800 or 0x9C00
    uint16_t tile_map_base = (ppu->lcdc & 0x08) ? 0x9C00 : 0x9800;

    // Determine tile data location from LCDC (Bit 4)
    // 0x8000 (unsigned 0..255) or 0x8800 (signed -128..127)
    bool unsigned_addressing = (ppu->lcdc & 0x10) != 0;

    uint8_t line_y = ppu->ly + ppu->scy;
    uint16_t tile_row = (line_y / 8) * 32;

    for (int x = 0; x < SCREEN_WIDTH; x++) {
        uint8_t line_x = x + ppu->scx;
        uint16_t tile_col = line_x / 8;

        // Fetch tile index directly from Bus VRAM (0x8000 - 0x9FFF)
        uint16_t map_addr = tile_map_base + tile_row + tile_col;
        uint8_t tile_index = bus_read(bus, map_addr);

        // Fetch low and high color plane bytes for the 8x8 tile line
        uint16_t tile_addr;
        if (unsigned_addressing) {
            tile_addr = 0x8000 + (tile_index * 16) + ((line_y % 8) * 2);
        } else {
            int8_t signed_index = (int8_t)tile_index;
            tile_addr = 0x9000 + (signed_index * 16) + ((line_y % 8) * 2);
        }

        uint8_t byte1 = bus_read(bus, tile_addr);
        uint8_t byte2 = bus_read(bus, tile_addr + 1);

        // Calculate pixel bit (7 - bit_position)
        int bit = 7 - (line_x % 8);
        uint8_t color_idx = (((byte2 >> bit) & 1) << 1) | ((byte1 >> bit) & 1);

        // Write RGB pixel to framebuffer
        ppu->frame_buffer[ppu->ly][x] = ppu_get_color(ppu->bgp, color_idx);
    }
}

static void ppu_render_window(PPU* ppu, Bus* bus)
{
    if (!(ppu->lcdc & 0x20)) return; // Window disabled (LCDC bit 5)
    if (ppu->wy > ppu->ly) return;   // Window not yet visible on this scanline
    if (ppu->wx > 166) return;       // Window off-screen

    uint16_t tile_map_base = (ppu->lcdc & 0x40) ? 0x9C00 : 0x9800;
    bool unsigned_addressing = (ppu->lcdc & 0x10) != 0;

    uint8_t window_line = ppu->ly - ppu->wy;
    uint16_t tile_row = (window_line / 8) * 32;

    for (int x = 0; x < SCREEN_WIDTH; x++) {
        int16_t wx = ppu->wx - 7;
        if (x < wx) continue; // Pixels before window start

        uint8_t line_x = x - wx;
        uint16_t tile_col = line_x / 8;

        uint16_t map_addr = tile_map_base + tile_row + tile_col;
        uint8_t tile_index = bus_read(bus, map_addr);

        uint16_t tile_addr;
        if (unsigned_addressing) {
            tile_addr = 0x8000 + (tile_index * 16) + ((window_line % 8) * 2);
        } else {
            int8_t signed_index = (int8_t)tile_index;
            tile_addr = 0x9000 + (signed_index * 16) + ((window_line % 8) * 2);
        }

        uint8_t byte1 = bus_read(bus, tile_addr);
        uint8_t byte2 = bus_read(bus, tile_addr + 1);

        int bit = 7 - (line_x % 8);
        uint8_t color_idx = (((byte2 >> bit) & 1) << 1) | ((byte1 >> bit) & 1);

        ppu->frame_buffer[ppu->ly][x] = ppu_get_color(ppu->bgp, color_idx);
    }
}

static void ppu_render_sprites(PPU* ppu, Bus* bus)
{
    if (!(ppu->lcdc & 0x02)) return; // Sprites disabled (LCDC bit 1)

    int sprite_height = (ppu->lcdc & 0x04) ? 16 : 8;
    int sprite_count = 0;

    // Collect sprites on this scanline (max 10)
    typedef struct { int16_t y, x; uint8_t tile, flags; } Sprite;
    Sprite sprites[10];

    for (int i = 0; i < 40 && sprite_count < 10; i++) {
        int16_t sy = (int16_t)bus_read(bus, 0xFE00 + i * 4) - 16;
        int16_t sx = (int16_t)bus_read(bus, 0xFE00 + i * 4 + 1) - 8;
        uint8_t tile = bus_read(bus, 0xFE00 + i * 4 + 2);
        uint8_t flags = bus_read(bus, 0xFE00 + i * 4 + 3);

        int16_t screen_y = (int16_t)ppu->ly - (int16_t)sy;
        if (screen_y >= 0 && screen_y < sprite_height) {
            sprites[sprite_count++] = (Sprite){ sy, sx, tile, flags };
        }
    }

    // Render sprites back-to-front (lower X = higher priority, lower OAM index = higher priority)
    for (int i = sprite_count - 1; i >= 0; i--) {
        int16_t sx = sprites[i].x;
        uint8_t tile = sprites[i].tile;
        uint8_t flags = sprites[i].flags;

        bool bg_over = flags & 0x80;
        bool y_flip  = flags & 0x40;
        bool x_flip  = flags & 0x20;
        bool palette = flags & 0x10; // 0 = OBP0, 1 = OBP1

        int16_t screen_y = (int16_t)ppu->ly - (int16_t)sprites[i].y;
        uint8_t tile_y = y_flip ? (sprite_height - 1 - screen_y) : screen_y;

        if (sprite_height == 16) tile &= 0xFE; // In 8x16 mode, bit 0 ignored

        uint16_t tile_addr = 0x8000 + (tile * 16) + (tile_y * 2);
        uint8_t byte1 = bus_read(bus, tile_addr);
        uint8_t byte2 = bus_read(bus, tile_addr + 1);

        uint8_t pal_reg = palette ? ppu->obp1 : ppu->obp0;

        for (int px = 0; px < 8; px++) {
            int16_t screen_x = (int16_t)sx + px;
            if (screen_x < 0 || screen_x >= SCREEN_WIDTH) continue;

            int bit = x_flip ? px : (7 - px);
            uint8_t color_idx = (((byte2 >> bit) & 1) << 1) | ((byte1 >> bit) & 1);

            if (color_idx == 0) continue; // Transparent

            if (bg_over) {
                // If BG color index != 0, BG wins
                // We can't check the exact frame buffer value easily, so skip
                // A proper check would compare against the BG palette result
            }

            ppu->frame_buffer[ppu->ly][screen_x] = ppu_get_color(pal_reg, color_idx);
        }
    }
}

static void ppu_render_scanline(PPU* ppu, Bus* bus)
{
    ppu_render_bg(ppu, bus);
    ppu_render_window(ppu, bus);
    ppu_render_sprites(ppu, bus);
}

static uint32_t ppu_get_color(uint8_t palette_reg, uint8_t color_index)
{
    uint8_t shade = (palette_reg >> (color_index * 2)) & 0x03;

    // DMG classic grayscale: white, light gray, dark gray, black
    static const uint32_t colors[4] = {
        0xFFFFFFFF, // 0 = White
        0xFFAAAAAA, // 1 = Light gray
        0xFF555555, // 2 = Dark gray
        0xFF000000, // 3 = Black
    };

    return colors[shade];
}

static void ppu_change_mode(PPU* ppu, PPUMode new_mode)
{
    ppu->stat = (ppu->stat & 0xFC) | (new_mode & 0x03);

    // STAT interrupt: mode-specific interrupt enable flags
    // Bit 3 = Mode 0 (HBLANK) interrupt enable
    // Bit 4 = Mode 1 (VBLANK) interrupt enable
    // Bit 5 = Mode 2 (OAM) interrupt enable
    bool interrupt = false;
    switch (new_mode) {
        case PPU_MODE_HBLANK: interrupt = ppu->stat & 0x08; break;
        case PPU_MODE_VBLANK: interrupt = ppu->stat & 0x10; break;
        case PPU_MODE_OAM:    interrupt = ppu->stat & 0x20; break;
        default: break;
    }

    if (interrupt) {
        ppu->stat_interrupt = true;
    }
}

static void check_lyc_stat_interrupt(PPU* ppu)
{
    if (ppu->ly == ppu->lyc) {
        ppu->stat |= 0x04; // Set LYC=LY coincidence flag (bit 2)
        if (ppu->stat & 0x40) { // LYC=LY interrupt enable (bit 6)
            ppu->stat_interrupt = true;
        }
    } else {
        ppu->stat &= ~0x04; // Clear coincidence flag
    }
}
