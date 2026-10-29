/*
 * app_browser.c — BTRON Hypermedia Real & Virtual Body Navigator implementation.
 */

#include "app_browser.h"
#include "app_editor.h"
#include "../kernel/bmsx_body.h"
#include "../msxgl/vdp.h"
#include "../font/font_latin.h"
#include <stddef.h>

void app_browser_init(void)
{
    /* Ready */
}

void app_browser_paint(Window *win)
{
    if (!win) return;

    uint16_t content_x = win->x + 2;
    uint16_t content_y = win->y + 14;
    uint16_t content_w = win->w - 4;
    uint16_t content_h = win->h - 16;

    vdp_fill_rect(content_x, content_y, content_w, content_h, BTRON_COL_WHITE);

    font_draw_string(content_x + 8, content_y + 6,
                     "Hypermedia Network:", BTRON_COL_TITLE_BLUE, BTRON_COL_WHITE);
    vdp_draw_hline(content_x + 8, content_y + 18, content_w - 16, BTRON_COL_MED_GRAY);

    UH count = bmsx_get_real_body_count();
    for (UH i = 0; i < count; i++) {
        const BMSX_RealBody *b = bmsx_get_body_by_index(i);
        if (!b) continue;

        uint16_t item_y = content_y + 24 + (i * 20);
        if (item_y + 16 > content_y + content_h) break;

        /* Draw Virtual Body Icon badge */
        vdp_draw_bevel_box(content_x + 8, item_y, content_w - 16, 16,
                           BTRON_COL_WHITE, BTRON_COL_DARK_GRAY, BTRON_COL_LIGHT_GRAY);

        /* Icon: "[T]" for TAD document, "[A]" for Application */
        const char *badge = (b->type == BODY_TYPE_TAD) ? "[T]" : "[A]";
        font_draw_string(content_x + 12, item_y + 4, badge, BTRON_COL_TITLE_BLUE, BTRON_COL_LIGHT_GRAY);
        font_draw_string(content_x + 36, item_y + 4, b->title, BTRON_COL_BLACK, BTRON_COL_LIGHT_GRAY);
    }
}

void app_browser_on_click(Window *win, uint16_t local_x, uint16_t local_y)
{
    (void)local_x;
    if (!win) return;

    if (local_y >= 24) {
        uint8_t item_idx = (uint8_t)((local_y - 24) / 20);
        const BMSX_RealBody *b = bmsx_get_body_by_index(item_idx);
        if (b) {
            app_editor_load_body(b->body_id);
        }
    }
}

