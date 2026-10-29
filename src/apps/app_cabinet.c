/*
 * app_cabinet.c — BTRON Storage Cabinet / Volume Viewer implementation.
 */

#include "app_cabinet.h"
#include "../msxgl/vdp.h"
#include "../font/font_latin.h"
#include <stddef.h>

void app_cabinet_init(void)
{
    /* Ready */
}

void app_cabinet_paint(Window *win)
{
    if (!win) return;

    uint16_t content_x = win->x + 2;
    uint16_t content_y = win->y + 14;
    uint16_t content_w = win->w - 4;
    uint16_t content_h = win->h - 16;

    vdp_fill_rect(content_x, content_y, content_w, content_h, BTRON_COL_LIGHT_GRAY);

    /* Volume Header */
    font_draw_string(content_x + 8, content_y + 6,
                     "Cabinet: B-MSX Storage Volumes", BTRON_COL_TITLE_BLUE, BTRON_COL_LIGHT_GRAY);
    vdp_draw_hline(content_x + 8, content_y + 18, content_w - 16, BTRON_COL_DARK_GRAY);

    /* Volume Box */
    uint16_t box_x = content_x + 8;
    uint16_t box_y = content_y + 24;
    uint16_t box_w = content_w - 16;
    uint16_t box_h = 70;

    vdp_draw_bevel_box(box_x, box_y, box_w, box_h,
                       BTRON_COL_WHITE, BTRON_COL_DARK_GRAY, BTRON_COL_WHITE);

    font_draw_string(box_x + 8, box_y + 6,
                     "[VOL:0] ASCII16 MegaROM Cartridge", BTRON_COL_BLACK, BTRON_COL_WHITE);
    font_draw_string(box_x + 8, box_y + 18,
                     "Total Size: 1,024 KiB (64 Banks x 16 KB)", BTRON_COL_CHARCOAL, BTRON_COL_WHITE);
    font_draw_string(box_x + 8, box_y + 30,
                     "Used: 80 KiB (5 System Banks)", BTRON_COL_DARK_RED, BTRON_COL_WHITE);
    font_draw_string(box_x + 8, box_y + 42,
                     "Free: 944 KiB (59 User Real Body Banks)", BTRON_COL_DARK_GREEN, BTRON_COL_WHITE);

    /* Memory Bar Graph */
    uint16_t bar_x = box_x + 8;
    uint16_t bar_y = box_y + 54;
    uint16_t bar_w = box_w - 16;
    uint16_t bar_h = 8;
    vdp_draw_rect_outline(bar_x, bar_y, bar_w, bar_h, BTRON_COL_DARK_GRAY);
    /* 5 / 64 ~ 8% filled */
    uint16_t used_w = (bar_w * 5) / 64;
    if (used_w < 4) used_w = 4;
    vdp_fill_rect(bar_x + 1, bar_y + 1, used_w, bar_h - 2, BTRON_COL_TITLE_BLUE);
    vdp_fill_rect(bar_x + 1 + used_w, bar_y + 1, bar_w - 2 - used_w, bar_h - 2, BTRON_COL_BRIGHT_GREEN);
}

void app_cabinet_on_click(Window *win, uint16_t local_x, uint16_t local_y)
{
    (void)win; (void)local_x; (void)local_y;
}

