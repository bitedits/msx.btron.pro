/*
 * vdp.c — Yamaha V9958 / V9938 Video Display Processor Driver for BTRON.
 */

#include "vdp.h"
#include "core.h"

void vdp_write_reg(uint8_t reg, uint8_t val)
{
    msx_di();
    msx_out(PORT_VDP_REG, val);
    msx_out(PORT_VDP_REG, 0x80 | (reg & 0x3F));
    msx_ei();
}

uint8_t vdp_read_status(uint8_t reg)
{
    msx_di();
    msx_out(PORT_VDP_REG, reg);
    msx_out(PORT_VDP_REG, 0x80 | 15);
    uint8_t val = msx_in(PORT_VDP_REG);
    /* Reset status pointer to S#0 */
    msx_out(PORT_VDP_REG, 0x00);
    msx_out(PORT_VDP_REG, 0x80 | 15);
    msx_ei();
    return val;
}

void vdp_wait_cmd(void)
{
    uint16_t timeout = 60000;
    while (timeout--) {
        uint8_t s2 = vdp_read_status(2);
        if ((s2 & 0x01) == 0) {
            break;
        }
    }
}

void vdp_set_vram_write_addr(uint32_t addr)
{
    msx_di();
    vdp_write_reg(14, (uint8_t)((addr >> 14) & 0x07));
    msx_out(PORT_VDP_REG, (uint8_t)(addr & 0xFF));
    msx_out(PORT_VDP_REG, (uint8_t)(((addr >> 8) & 0x3F) | 0x40));
    msx_ei();
}

void vdp_set_vram_read_addr(uint32_t addr)
{
    msx_di();
    vdp_write_reg(14, (uint8_t)((addr >> 14) & 0x07));
    msx_out(PORT_VDP_REG, (uint8_t)(addr & 0xFF));
    msx_out(PORT_VDP_REG, (uint8_t)((addr >> 8) & 0x3F));
    msx_ei();
}

void vdp_set_palette_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    msx_di();
    vdp_write_reg(16, index & 0x0F);
    uint8_t rb = ((r & 0x07) << 4) | (b & 0x07);
    uint8_t g_val = (g & 0x07);
    msx_out(PORT_VDP_PALETTE, rb);
    msx_out(PORT_VDP_PALETTE, g_val);
    msx_ei();
}

void vdp_init_btron_palette(void)
{
    /* BTRON 16-color reference palette (R, G, B: each 0..7) */
    static const uint8_t palette_table[16][3] = {
        { 0, 0, 0 },  /* 0: Black */
        { 2, 2, 2 },  /* 1: Dark Gray */
        { 0, 2, 5 },  /* 2: BTRON Deep Title Blue */
        { 4, 1, 1 },  /* 3: Dark Red */
        { 3, 1, 3 },  /* 4: Dark Magenta */
        { 1, 3, 1 },  /* 5: Dark Green */
        { 1, 3, 4 },  /* 6: Dark Cyan */
        { 4, 4, 4 },  /* 7: Medium Gray (3D shadow) */
        { 6, 6, 6 },  /* 8: Light Gray (Window body & desktop) */
        { 7, 7, 7 },  /* 9: Pure White (3D hilite & paper) */
        { 7, 2, 2 },  /* 10: Bright Red */
        { 7, 7, 2 },  /* 11: Yellow */
        { 2, 7, 2 },  /* 12: Bright Green */
        { 2, 5, 7 },  /* 13: Hypermedia Link Cyan */
        { 3, 4, 7 },  /* 14: Sky Blue */
        { 1, 1, 1 }   /* 15: Charcoal */
    };

    for (uint8_t i = 0; i < 16; i++) {
        vdp_set_palette_entry(i, palette_table[i][0], palette_table[i][1], palette_table[i][2]);
    }
}

void vdp_init_screen7(void)
{
    /* V9958 / V9938 Screen 7 Setup: 512x212, 16 colors per pixel */
    vdp_write_reg(0, 0x0E);  /* M5=1, M4=1, M3=1 (Screen 7 mode bits) */
    vdp_write_reg(1, 0x62);  /* 4K/16K=1, BL=1 (display on), SI=1 (16x16 sprites) */
    vdp_write_reg(2, 0x1F);  /* Pattern layout base address 0x00000 */
    vdp_write_reg(7, 0x08);  /* Backdrop color: Light Gray (BTRON background) */
    vdp_write_reg(8, 0x08);  /* VR=1 (64K/128K VRAM), SPD=0 (sprites enabled) */
    vdp_write_reg(9, 0x80);  /* LN=1 (212 display lines), IL=0 (non-interlace) */

    /* Setup Sprite tables: Pattern at 0x1F000, Attributes at 0x1FA00 */
    vdp_write_reg(5, 0x3F);  /* Sprite attribute table low */
    vdp_write_reg(11, 0x03); /* Sprite attribute table high */
    vdp_write_reg(6, 0x3E);  /* Sprite pattern generator */

    vdp_init_btron_palette();

    /* Clear screen to desktop background color (BTRON Light Gray = 8) */
    vdp_fill_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BTRON_COL_LIGHT_GRAY);
}

void vdp_cmd_hmmv(uint16_t dx, uint16_t dy, uint16_t nx, uint16_t ny, uint8_t color_byte)
{
    vdp_wait_cmd();

    msx_di();
    /* Set R#17 to 36 (first register of HMMV parameters) with auto-increment (bit 7 = 0) */
    vdp_write_reg(17, 36);

    /* Stream parameters through PORT_VDP_INDIR (0x9B) */
    msx_out(PORT_VDP_INDIR, (uint8_t)(dx & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((dx >> 8) & 0x01));
    msx_out(PORT_VDP_INDIR, (uint8_t)(dy & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((dy >> 8) & 0x03));
    msx_out(PORT_VDP_INDIR, (uint8_t)(nx & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((nx >> 8) & 0x03));
    msx_out(PORT_VDP_INDIR, (uint8_t)(ny & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((ny >> 8) & 0x03));
    msx_out(PORT_VDP_INDIR, color_byte);
    msx_out(PORT_VDP_INDIR, 0x00);         /* ARG: 0 */
    msx_out(PORT_VDP_INDIR, 0xC0);         /* CMD: HMMV */
    msx_ei();
}

void vdp_cmd_lmmv(uint16_t dx, uint16_t dy, uint16_t nx, uint16_t ny, uint8_t color, uint8_t op)
{
    vdp_wait_cmd();

    msx_di();
    vdp_write_reg(17, 36);
    msx_out(PORT_VDP_INDIR, (uint8_t)(dx & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((dx >> 8) & 0x01));
    msx_out(PORT_VDP_INDIR, (uint8_t)(dy & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((dy >> 8) & 0x03));
    msx_out(PORT_VDP_INDIR, (uint8_t)(nx & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((nx >> 8) & 0x03));
    msx_out(PORT_VDP_INDIR, (uint8_t)(ny & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((ny >> 8) & 0x03));
    msx_out(PORT_VDP_INDIR, color);
    msx_out(PORT_VDP_INDIR, 0x00);
    msx_out(PORT_VDP_INDIR, (uint8_t)(0x90 | (op & 0x0F)));  /* LMMV */
    msx_ei();
}

void vdp_cmd_line(uint16_t dx, uint16_t dy, uint16_t maj_len, uint16_t min_len, uint8_t color, uint8_t arg, uint8_t op)
{
    vdp_wait_cmd();

    msx_di();
    vdp_write_reg(17, 36);
    msx_out(PORT_VDP_INDIR, (uint8_t)(dx & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((dx >> 8) & 0x01));
    msx_out(PORT_VDP_INDIR, (uint8_t)(dy & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((dy >> 8) & 0x03));
    msx_out(PORT_VDP_INDIR, (uint8_t)(maj_len & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((maj_len >> 8) & 0x03));
    msx_out(PORT_VDP_INDIR, (uint8_t)(min_len & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((min_len >> 8) & 0x03));
    msx_out(PORT_VDP_INDIR, color);
    msx_out(PORT_VDP_INDIR, arg);
    msx_out(PORT_VDP_INDIR, (uint8_t)(0x70 | (op & 0x0F)));  /* LINE */
    msx_ei();
}

void vdp_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color)
{
    if (w == 0 || h == 0) return;
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;
    if (x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;

    uint8_t color_pair = COLOR_PAIR(color, color);
    vdp_cmd_hmmv(x, y, w, h, color_pair);
}

void vdp_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint8_t color)
{
    if (len == 0 || y >= SCREEN_HEIGHT || x >= SCREEN_WIDTH) return;
    if (x + len > SCREEN_WIDTH) len = SCREEN_WIDTH - x;
    vdp_cmd_hmmv(x, y, len, 1, COLOR_PAIR(color, color));
}

void vdp_draw_vline(uint16_t x, uint16_t y, uint16_t len, uint8_t color)
{
    if (len == 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;
    if (y + len > SCREEN_HEIGHT) len = SCREEN_HEIGHT - y;
    vdp_cmd_hmmv(x, y, 1, len, COLOR_PAIR(color, color));
}

void vdp_draw_rect_outline(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color)
{
    if (w < 2 || h < 2) return;
    vdp_draw_hline(x, y, w, color);
    vdp_draw_hline(x, y + h - 1, w, color);
    vdp_draw_vline(x, y, h, color);
    vdp_draw_vline(x + w - 1, y, h, color);
}

void vdp_draw_bevel_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        uint8_t light_col, uint8_t dark_col, uint8_t fill_col)
{
    if (w < 4 || h < 4) return;

    /* Fill interior body */
    vdp_fill_rect(x + 1, y + 1, w - 2, h - 2, fill_col);

    /* Top and Left borders (hilite) */
    vdp_draw_hline(x, y, w, light_col);
    vdp_draw_vline(x, y, h, light_col);

    /* Bottom and Right borders (shadow) */
    vdp_draw_hline(x, y + h - 1, w, dark_col);
    vdp_draw_vline(x + w - 1, y, h, dark_col);
}

void vdp_init_sprites(void)
{
    /* BTRON Mouse Arrow Cursor Pattern (16x16: 16 rows * 2 bytes = 32 bytes) */
    static const uint8_t arrow_cursor_pat[32] = {
        0xC0, 0x00, /* 11000000 00000000 */
        0xE0, 0x00, /* 11100000 00000000 */
        0xF0, 0x00, /* 11110000 00000000 */
        0xF8, 0x00, /* 11111000 00000000 */
        0xFC, 0x00, /* 11111100 00000000 */
        0xFE, 0x00, /* 11111110 00000000 */
        0xFF, 0x00, /* 11111111 00000000 */
        0xF8, 0x00, /* 11111000 00000000 */
        0xD8, 0x00, /* 11011000 00000000 */
        0x8C, 0x00, /* 10001100 00000000 */
        0x0C, 0x00, /* 00001100 00000000 */
        0x06, 0x00, /* 00000110 00000000 */
        0x06, 0x00, /* 00000110 00000000 */
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00
    };

    vdp_set_sprite_pattern(0, arrow_cursor_pat);
}

void vdp_set_sprite_pattern(uint8_t pat_idx, const uint8_t *pattern16)
{
    /* VRAM base for sprite patterns: 0x1F000 + pat_idx * 32 */
    uint32_t addr = 0x1F000UL + ((uint32_t)pat_idx * 32);
    vdp_set_vram_write_addr(addr);

    for (uint8_t i = 0; i < 32; i++) {
        msx_out(PORT_VDP_DATA, pattern16[i]);
    }
}

void vdp_set_sprite_pos(uint8_t plane, uint16_t x, uint8_t y, uint8_t pat_idx, uint8_t color)
{
    /* VRAM base for sprite attribute table: 0x1FA00 + plane * 4 */
    uint32_t addr = 0x1FA00UL + ((uint32_t)plane * 4);
    vdp_set_vram_write_addr(addr);

    /* Sprite attributes: Y, X, Pattern, Color & Flags */
    msx_out(PORT_VDP_DATA, y);
    msx_out(PORT_VDP_DATA, (uint8_t)(x & 0xFF));
    msx_out(PORT_VDP_DATA, pat_idx);
    msx_out(PORT_VDP_DATA, (uint8_t)(color & 0x0F));
}
