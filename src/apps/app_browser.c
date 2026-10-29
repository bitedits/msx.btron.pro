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
                     "BTRON Hypermedia Object Network:", BTRON_COL_TITLE_BLUE, BTRON_COL_WHITE);
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

        /* Icon: "[doc]" or "[exe]" */
        const char *icon = (b->type == BODY_TYPE_TAD) ? "[TAD]" : "[APP]";
        font_draw_string(content_x + 12, item_y + 4, icon, BTRON_COL_TITLE_BLUE, BTRON_COL_LIGHT_GRAY);

        /* Title */
        font_draw_string(content_x + 48, item_y + 4, b->title, BTRON_COL_BLACK, BTRON_COL_LIGHT_GRAY);

        /* Size / ID indicator on right */
        char info[16];
        info[0] = '#';
        info[1] = '0' + (char)(b->body_id);
        info[2] = '\0';
        font_draw_string(content_x + content_w - 40, item_y + 4, info, BTRON_COL_DARK_GRAY, BTRON_COL_LIGHT_GRAY);
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
            if (b->body_id == 1 || b->body_id == 2) {
                /* Open in Editor */
                app_editor_load_body(b->body_id);
                Window *editor_win = wm_get_window(1);
                if (editor_win) {
                    editor_win->state = WIN_STATE_NORMAL;
                    wm_set_active_window(1);
                }
            } else if (b->body_id == 3) {
                Window *term = wm_get_window(2);
                if (term) {
                    term->state = WIN_STATE_NORMAL;
                    wm_set_active_window(2);
                }
            } else if (b->body_id == 4) {
                Window *cab = wm_get_window(4);
                if (cab) {
                    cab->state = WIN_STATE_NORMAL;
                    wm_set_active_window(4);
                }
            }
            wm_draw_all();
        }
    }
}

