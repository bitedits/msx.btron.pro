/*
 * vdp.h — Yamaha V9958 / V9938 Video Display Processor Driver for BTRON.
 * Target: Screen 7 (512x212, 16 colors per pixel from 512 palette).
 */

#ifndef _MSXGL_VDP_H_
#define _MSXGL_VDP_H_

#include <stdint.h>
#include <stdbool.h>
#include "msxgl_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BTRON Standard Palette Indices */
#define BTRON_COL_BLACK         0   /* #000000 */
#define BTRON_COL_DARK_GRAY     1   /* #444444 */
#define BTRON_COL_TITLE_BLUE    2   /* #003366 Active window title */
#define BTRON_COL_DARK_RED      3   /* #882222 */
#define BTRON_COL_DARK_MAGENTA  4   /* #662266 */
#define BTRON_COL_DARK_GREEN    5   /* #226622 */
#define BTRON_COL_DARK_CYAN     6   /* #226688 */
#define BTRON_COL_MED_GRAY      7   /* #888888 3D Bevel Shadow */
#define BTRON_COL_LIGHT_GRAY    8   /* #C0C0C0 3D Window Body & Desk */
#define BTRON_COL_WHITE         9   /* #FFFFFF 3D Bevel Hilite / Paper */
#define BTRON_COL_BRIGHT_RED    10  /* #FF4444 */
#define BTRON_COL_YELLOW        11  /* #FFFF44 */
#define BTRON_COL_BRIGHT_GREEN  12  /* #44FF44 */
#define BTRON_COL_LINK_CYAN     13  /* #44AAFF Hypermedia Link Tag */
#define BTRON_COL_SKY_BLUE      14  /* #6688FF */
#define BTRON_COL_CHARCOAL      15  /* #222222 */

/* VDP Logical Operations */
#define VDP_OP_IMP              0x00    /* PSET replace */
#define VDP_OP_AND              0x01
#define VDP_OP_OR               0x02
#define VDP_OP_XOR              0x03
#define VDP_OP_NOT              0x04
#define VDP_OP_TIMP             0x08    /* Transparent PSET (skip color 0) */
#define VDP_OP_TAND             0x09
#define VDP_OP_TOR              0x0A
#define VDP_OP_TXOR             0x0B
#define VDP_OP_TNOT             0x0C

/* Color packing helper (two 4-bit nibbles for Screen 7) */
#define COLOR_PAIR(left, right) ((uint8_t)(((uint8_t)(left) << 4) | ((uint8_t)(right) & 0x0F)))

/* Core VDP control */
void vdp_write_reg(uint8_t reg, uint8_t val);
uint8_t vdp_read_status(uint8_t reg);
void vdp_wait_cmd(void);
void vdp_set_vram_write_addr(uint32_t addr);
void vdp_set_vram_write_line(uint8_t byte_x, uint16_t eff_y);
void vdp_set_vram_read_addr(uint32_t addr);

/* Double Buffering & Page Flipping */
void vdp_set_draw_page(uint8_t page);
void vdp_set_display_page(uint8_t page);
uint8_t vdp_get_draw_page(void);
uint8_t vdp_get_display_page(void);
uint16_t vdp_get_draw_y_offset(void);
void vdp_flip_page(void);
void vdp_sync_pages(void);
void vdp_present_backbuffer(void);
void vdp_restore_underlay_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void vdp_save_underlay_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/* Screen 7 & Palette setup */
void vdp_init_screen7(void);
void vdp_set_palette_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void vdp_init_btron_palette(void);

/* V9958 Hardware Accelerated BitBlt & Drawing Commands */
void vdp_cmd_hmmv(uint16_t dx, uint16_t dy, uint16_t nx, uint16_t ny, uint8_t color_byte);
void vdp_cmd_hmmm(uint16_t sx, uint16_t sy, uint16_t dx, uint16_t dy, uint16_t nx, uint16_t ny, uint8_t arg);
void vdp_cmd_lmmm(uint16_t sx, uint16_t sy, uint16_t dx, uint16_t dy, uint16_t nx, uint16_t ny, uint8_t arg, uint8_t op);
void vdp_cmd_hmmc(uint16_t dx, uint16_t dy, uint16_t nx, uint16_t ny, const uint8_t *data, uint16_t len);
void vdp_cmd_ymmm(uint16_t sy, uint16_t dx, uint16_t dy, uint16_t ny, uint8_t arg);
void vdp_cmd_lmmv(uint16_t dx, uint16_t dy, uint16_t nx, uint16_t ny, uint8_t color, uint8_t op);
void vdp_cmd_line(uint16_t dx, uint16_t dy, uint16_t maj_len, uint16_t min_len, uint8_t color, uint8_t arg, uint8_t op);
void vdp_cmd_pset(uint16_t x, uint16_t y, uint8_t color, uint8_t op);

/* High-level BTRON BitBlt & Drawing Primitives */
void vdp_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color);
void vdp_copy_rect(uint16_t sx, uint16_t sy, uint16_t dx, uint16_t dy, uint16_t w, uint16_t h);
void vdp_scroll_rect_up(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t delta_y, uint8_t fill_color);
void vdp_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint8_t color);
void vdp_draw_vline(uint16_t x, uint16_t y, uint16_t len, uint8_t color);
void vdp_draw_bevel_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        uint8_t light_col, uint8_t dark_col, uint8_t fill_col);
void vdp_draw_rect_outline(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color);

/* Hardware Sprites (for BTRON pointer/mouse cursor) */
void vdp_init_sprites(void);
void vdp_set_sprite_pattern(uint8_t pat_idx, const uint8_t *pattern16);
void vdp_set_sprite_pos(uint8_t plane, uint16_t x, uint8_t y, uint8_t pat_idx, uint8_t color);

#ifdef __cplusplus
}
#endif

#endif /* _MSXGL_VDP_H_ */

