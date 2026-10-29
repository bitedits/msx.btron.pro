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

/* Color packing helper (two 4-bit nibbles for Screen 7) */
#define COLOR_PAIR(left, right) (((left) << 4) | ((right) & 0x0F))

/* Core VDP control */
void vdp_write_reg(uint8_t reg, uint8_t val);
uint8_t vdp_read_status(uint8_t reg);
void vdp_wait_cmd(void);
void vdp_set_vram_write_addr(uint32_t addr);
void vdp_set_vram_read_addr(uint32_t addr);

/* Screen 7 & Palette setup */
void vdp_init_screen7(void);
void vdp_set_palette_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void vdp_init_btron_palette(void);

/* V9958 Hardware Accelerated Commands */
void vdp_cmd_hmmv(uint16_t dx, uint16_t dy, uint16_t nx, uint16_t ny, uint8_t color_byte);
void vdp_cmd_lmmv(uint16_t dx, uint16_t dy, uint16_t nx, uint16_t ny, uint8_t color, uint8_t op);
void vdp_cmd_line(uint16_t dx, uint16_t dy, uint16_t maj_len, uint16_t min_len, uint8_t color, uint8_t arg, uint8_t op);
void vdp_cmd_pset(uint16_t x, uint16_t y, uint8_t color, uint8_t op);

/* High-level BTRON Drawing Primitives */
void vdp_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color);
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

