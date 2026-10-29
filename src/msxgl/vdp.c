/*
 * vdp.c — Yamaha V9958 / V9938 Video Display Processor Driver for BTRON.
 */

#include "vdp.h"
#include "core.h"

static uint8_t  s_draw_page = 0;
static uint8_t  s_disp_page = 0;
static uint16_t s_draw_y_offset = 0;

void vdp_set_draw_page(uint8_t page)
{
    s_draw_page = page & 1;
    s_draw_y_offset = (s_draw_page == 0) ? 0 : 256;
}

void vdp_set_display_page(uint8_t page)
{
    s_disp_page = page & 1;
    /* In Screen 7, R#2 bit 5 selects Page 0 (0x1F) or Page 1 (0x3F) */
    vdp_write_reg(2, (s_disp_page == 0) ? 0x1F : 0x3F);
}

uint8_t vdp_get_draw_page(void)
{
    return s_draw_page;
}

uint8_t vdp_get_display_page(void)
{
    return s_disp_page;
}

uint16_t vdp_get_draw_y_offset(void)
{
    return s_draw_y_offset;
}

void vdp_present_backbuffer(void)
{
    vdp_wait_cmd();
    /* Atomic hardware VRAM-to-VRAM BitBlt from Page 1 (offscreen backbuffer) to Page 0 (display) */
    vdp_cmd_hmmm(0, 256, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00);
    vdp_wait_cmd();
}

static void vdp_xfer_underlay(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool save)
{
    if (w == 0 || h == 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;
    if (x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    uint16_t ax = x & (uint16_t)~1u;
    uint16_t aw = (uint16_t)(((x + w + 1u) & (uint16_t)~1u) - ax);
    if (ax + aw > SCREEN_WIDTH) aw = SCREEN_WIDTH - ax;
    if (aw == 0) return;
    uint16_t sy = save ? y : (uint16_t)(256u + y);
    uint16_t dy = save ? (uint16_t)(256u + y) : y;
    vdp_cmd_hmmm(ax, sy, ax, dy, aw, h, 0x00);
}

void vdp_restore_underlay_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    vdp_xfer_underlay(x, y, w, h, false);
}

void vdp_save_underlay_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    vdp_xfer_underlay(x, y, w, h, true);
}

void vdp_swap_pages_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    if (w == 0 || h == 0) return;
    uint16_t ax = x & (uint16_t)~1u;
    uint16_t aw = (uint16_t)(((x + w + 1u) & (uint16_t)~1u) - ax);
    if (aw == 0) return;
    uint16_t cur_y = y;
    uint16_t rem_h = h;
    while (rem_h > 0) {
        uint16_t sh = (rem_h > 44u) ? 44u : rem_h;
        /* Swap via offscreen scanlines Y = 212..255 */
        vdp_cmd_hmmm(ax, cur_y, ax, 212u, aw, sh, 0x00);
        vdp_cmd_hmmm(ax, (uint16_t)(256u + cur_y), ax, cur_y, aw, sh, 0x00);
        vdp_cmd_hmmm(ax, 212u, ax, (uint16_t)(256u + cur_y), aw, sh, 0x00);
        cur_y += sh;
        rem_h -= sh;
    }
}

void vdp_write_reg(uint8_t reg, uint8_t val)
{
    msx_di();
    msx_out(PORT_VDP_REG, val);
    msx_out(PORT_VDP_REG, 0x80 | (reg & 0x3F));
    msx_ei();
}

void vdp_wait_cmd(void)
{
    vdp_write_reg(15, 2);
    uint16_t timeout = 60000;
    while (timeout--) {
        uint8_t s2 = msx_in(PORT_VDP_REG);
        if ((s2 & 0x01) == 0) {
            break;
        }
    }
    vdp_write_reg(15, 0);
}

static uint8_t s_last_r14 = 0xFF;

void vdp_set_vram_write_addr(uint32_t addr)
{
    uint8_t r14 = (uint8_t)((addr >> 14) & 0x07);
    msx_di();
    if (r14 != s_last_r14) {
        s_last_r14 = r14;
        msx_out(PORT_VDP_REG, r14);
        msx_out(PORT_VDP_REG, 0x80 | 14);
    }
    msx_out(PORT_VDP_REG, (uint8_t)(addr & 0xFF));
    msx_out(PORT_VDP_REG, (uint8_t)(((addr >> 8) & 0x3F) | 0x40));
    msx_ei();
}

void vdp_set_vram_write_line(uint8_t byte_x, uint16_t eff_y)
{
    uint8_t r14 = (uint8_t)(eff_y >> 6);
    msx_di();
    if (r14 != s_last_r14) {
        s_last_r14 = r14;
        msx_out(PORT_VDP_REG, r14);
        msx_out(PORT_VDP_REG, 0x80 | 14);
    }
    msx_out(PORT_VDP_REG, byte_x);
    msx_out(PORT_VDP_REG, (uint8_t)((eff_y & 0x3F) | 0x40));
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
    /* V9958 / V9938 Screen 7 Setup: 512x212, 16 colors per pixel (Mode 0x14) */
    vdp_write_reg(0, 0x0A);  /* M5=1, M4=0, M3=1 (Screen 7 mode bits: (0x0A & 0x0E) << 1 = 0x14) */
    vdp_write_reg(1, 0x62);  /* 4K/16K=1, BL=1 (display on), SI=1 (16x16 sprites) */
    vdp_write_reg(2, 0x1F);  /* Pattern layout base address 0x00000 */
    vdp_write_reg(7, 0x08);  /* Backdrop color: Light Gray (BTRON background) */
    vdp_write_reg(8, 0x08);  /* VR=1 (64K/128K VRAM), SPD=0 (sprites enabled) */
    vdp_write_reg(9, 0x80);  /* LN=1 (212 display lines), IL=0 (non-interlace) */

    /* Setup Sprite tables: Pattern at 0x1F000, Attributes at 0x1FA00, Colors at 0x1F800 */
    vdp_write_reg(5, 0xF4);  /* Sprite attribute table low: (0xF4 << 7) = 0x7A00 */
    vdp_write_reg(11, 0x03); /* Sprite attribute table high: (0x03 << 15) = 0x18000 -> 0x1FA00 */
    vdp_write_reg(6, 0x3E);  /* Sprite pattern generator: (0x3E << 11) = 0x1F000 */

    vdp_init_btron_palette();

    s_draw_page = 0;
    s_disp_page = 0;
    s_draw_y_offset = 0;

    /* Clear both Page 0 and Page 1 to desktop background color (BTRON Light Gray = 8) */
    uint8_t gray_pair = COLOR_PAIR(BTRON_COL_LIGHT_GRAY, BTRON_COL_LIGHT_GRAY);
    vdp_cmd_hmmv(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, gray_pair);
    vdp_cmd_hmmv(0, 256, SCREEN_WIDTH, SCREEN_HEIGHT, gray_pair);
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

void vdp_cmd_hmmm(uint16_t sx, uint16_t sy, uint16_t dx, uint16_t dy, uint16_t nx, uint16_t ny, uint8_t arg)
{
    vdp_wait_cmd();

    msx_di();
    vdp_write_reg(17, 32); /* Start at R#32 (SX) */
    msx_out(PORT_VDP_INDIR, (uint8_t)(sx & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((sx >> 8) & 0x01));
    msx_out(PORT_VDP_INDIR, (uint8_t)(sy & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((sy >> 8) & 0x03));
    msx_out(PORT_VDP_INDIR, (uint8_t)(dx & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((dx >> 8) & 0x01));
    msx_out(PORT_VDP_INDIR, (uint8_t)(dy & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((dy >> 8) & 0x03));
    msx_out(PORT_VDP_INDIR, (uint8_t)(nx & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((nx >> 8) & 0x03));
    msx_out(PORT_VDP_INDIR, (uint8_t)(ny & 0xFF));
    msx_out(PORT_VDP_INDIR, (uint8_t)((ny >> 8) & 0x03));
    msx_out(PORT_VDP_INDIR, 0x00);
    msx_out(PORT_VDP_INDIR, arg);
    msx_out(PORT_VDP_INDIR, 0xD0);         /* CMD: HMMM (High-speed VRAM to VRAM BitBlt) */
    msx_ei();
}

void vdp_copy_rect(uint16_t sx, uint16_t sy, uint16_t dx, uint16_t dy, uint16_t w, uint16_t h)
{
    if (w == 0 || h == 0) return;
    uint8_t arg = 0;
    if (sx < dx && (sx + w) > dx) {
        arg |= 0x04; /* DIX = 1: right to left */
        sx += w - 1;
        dx += w - 1;
    }
    if (sy < dy && (sy + h) > dy) {
        arg |= 0x08; /* DIY = 1: bottom to top */
        sy += h - 1;
        dy += h - 1;
    }
    vdp_cmd_hmmm(sx, sy + s_draw_y_offset, dx, dy + s_draw_y_offset, w, h, arg);
}

void vdp_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color)
{
    if (w == 0 || h == 0) return;
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;
    if (x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;

    uint8_t color_pair = COLOR_PAIR(color, color);
    vdp_cmd_hmmv(x, y + s_draw_y_offset, w, h, color_pair);
}

void vdp_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint8_t color)
{
    if (len == 0 || y >= SCREEN_HEIGHT || x >= SCREEN_WIDTH) return;
    if (x + len > SCREEN_WIDTH) len = SCREEN_WIDTH - x;
    vdp_cmd_hmmv(x, y + s_draw_y_offset, len, 1, COLOR_PAIR(color, color));
}

void vdp_draw_vline(uint16_t x, uint16_t y, uint16_t len, uint8_t color)
{
    if (len == 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;
    if (y + len > SCREEN_HEIGHT) len = SCREEN_HEIGHT - y;
    vdp_cmd_hmmv(x, y + s_draw_y_offset, 1, len, COLOR_PAIR(color, color));
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
    vdp_wait_cmd();
}

void vdp_init_sprites(void)
{
    /* BTRON Mouse Arrow Cursor Pattern (16x16: 16 rows * 2 bytes = 32 bytes) */
    /* Standard V9938 16x16 Sprite Layout:
     * Bytes 0..7:   Top-Left (rows 0..7)
     * Bytes 8..15:  Bottom-Left (rows 8..15)
     * Bytes 16..23: Top-Right (rows 0..7)
     * Bytes 24..31: Bottom-Right (rows 8..15)
     */
    static const uint8_t arrow_cursor_pat[32] = {
        /* Top-Left (8x8) */
        0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF, 0xF8,
        /* Bottom-Left (8x8) */
        0xD8, 0x8C, 0x0C, 0x06, 0x06, 0x00, 0x00, 0x00,
        /* Top-Right (8x8) */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* Bottom-Right (8x8) */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    vdp_set_sprite_pattern(0, arrow_cursor_pat);

    /* Terminate Sprite Attribute Table at Plane 1 (Y = 216 stops Sprite Mode 2 scan) */
    vdp_set_vram_write_addr(0x1FA04UL);
    msx_out(PORT_VDP_DATA, 216);
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
    static uint8_t s_last_col = 0xFF;
    uint8_t col_attr = (color & 0x0F);

    /* 1. Sprite Color Table (Mode 2): 0x1F800 + plane * 16 */
    if (plane != 0 || col_attr != s_last_col) {
        uint32_t sct_addr = 0x1F800UL + ((uint32_t)plane * 16);
        vdp_set_vram_write_addr(sct_addr);
        for (uint8_t line = 0; line < 16; line++) {
            msx_out(PORT_VDP_DATA, col_attr);
        }
        if (plane == 0) s_last_col = col_attr;
    }

    /* 2. Sprite Attribute Table (Mode 2): 0x1FA00 + plane * 4
     * In Screen 7 (512x212), sprite horizontal coordinate is 0..255 (x / 2). */
    uint32_t sat_addr = 0x1FA00UL + ((uint32_t)plane * 4);
    vdp_set_vram_write_addr(sat_addr);
    msx_out(PORT_VDP_DATA, y);
    msx_out(PORT_VDP_DATA, (uint8_t)((x >> 1) & 0xFF));
    msx_out(PORT_VDP_DATA, pat_idx);
    msx_out(PORT_VDP_DATA, 0x00);
    if (plane == 0) {
        msx_out(PORT_VDP_DATA, 216); /* Plane 1 Y = 216 terminator */
    }
}
