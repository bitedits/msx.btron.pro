/*
 * app_editor.c — BTRON Document Viewer / Editor Application implementation.
 */

#include "app_editor.h"
#include "../kernel/bmsx_body.h"
#include "../tad/tad_render.h"
#include "../msxgl/vdp.h"
#include "../font/font_latin.h"
#include <stddef.h>

static ID s_current_doc_id = 1; /* Default: Heart Sutra */

void app_editor_init(void)
{
    s_current_doc_id = 1;
}

void app_editor_load_body(ID body_id)
{
    if (body_id == 3 || body_id == 4) {
        ID target_wid = (body_id == 3) ? 2 : 4;
        Window *w = wm_get_window(target_wid);
        if (w) {
            if (w->state != WIN_STATE_NORMAL) {
                w->state = WIN_STATE_NORMAL;
                wm_set_active_window(target_wid);
                wm_mark_window_dirty(target_wid);
            } else {
                wm_focus_window(target_wid);
            }
        }
        return;
    }

    s_current_doc_id = body_id;
    Window *win = wm_get_window(1);
    if (win) {
        win->state = WIN_STATE_NORMAL;
        win->scroll_y = 0;
        const BMSX_RealBody *body = bmsx_get_real_body(body_id);
        if (body) {
            UH j = 0;
            while (body->title[j] != '\0' && j < 23) {
                win->title[j] = body->title[j];
                j++;
            }
            win->title[j] = '\0';
        }
        wm_set_active_window(1);
        wm_mark_window_dirty(1);
    }
}

void app_editor_paint(Window *win)
{
    if (!win) return;

    uint16_t content_x = win->x + 2;
    uint16_t content_y = win->y + 14;
    uint16_t content_w = win->w - 16; /* Leave room for scroll bar */
    uint16_t content_h = win->h - 16;

    const BMSX_RealBody *body = bmsx_get_real_body(s_current_doc_id);
    if (body && body->data && body->size > 0) {
        tad_render_document(content_x, content_y, content_w, content_h,
                            win->scroll_y, body->data, body->size, 0, 0, false);
    } else {
        font_draw_string(content_x + 8, content_y + 8, "Empty Document.", BTRON_COL_DARK_GRAY, BTRON_COL_WHITE);
    }

    /* Vertical Scroll Bar on right */
    uint16_t scroll_x = win->x + win->w - 14;
    uint16_t scroll_y = win->y + 14;
    uint16_t scroll_w = 12;
    uint16_t scroll_h = win->h - 16;

    vdp_draw_bevel_box(scroll_x, scroll_y, scroll_w, scroll_h,
                       BTRON_COL_DARK_GRAY, BTRON_COL_WHITE, BTRON_COL_MED_GRAY);

    /* Scroll Up Button [^] */
    vdp_draw_bevel_box(scroll_x, scroll_y, scroll_w, 12,
                       BTRON_COL_WHITE, BTRON_COL_DARK_GRAY, BTRON_COL_LIGHT_GRAY);
    font_draw_string(scroll_x + 2, scroll_y + 2, "^", BTRON_COL_BLACK, BTRON_COL_LIGHT_GRAY);

    /* Scroll Down Button [v] */
    vdp_draw_bevel_box(scroll_x, scroll_y + scroll_h - 12, scroll_w, 12,
                       BTRON_COL_WHITE, BTRON_COL_DARK_GRAY, BTRON_COL_LIGHT_GRAY);
    font_draw_string(scroll_x + 2, scroll_y + scroll_h - 10, "v", BTRON_COL_BLACK, BTRON_COL_LIGHT_GRAY);
}

void app_editor_on_click(Window *win, uint16_t local_x, uint16_t local_y)
{
    if (!win) return;

    /* Check scroll bar buttons */
    if (local_x >= win->w - 16) {
        if (local_y < 14) {
            /* Scroll Up */
            if (win->scroll_y >= 20) win->scroll_y -= 20;
            else win->scroll_y = 0;
            wm_mark_window_dirty(win->win_id);
            return;
        } else if (local_y >= win->h - 30) {
            /* Scroll Down */
            win->scroll_y += 20;
            wm_mark_window_dirty(win->win_id);
            return;
        }
    }

    /* Check click on TAD link */
    const BMSX_RealBody *body = bmsx_get_real_body(s_current_doc_id);
    if (body && body->data && body->size > 0) {
        uint16_t click_abs_x = win->x + 2 + local_x;
        uint16_t click_abs_y = win->y + 14 + local_y;

        ID clicked_id = tad_render_document(win->x + 2, win->y + 14, win->w - 16, win->h - 16,
                                            win->scroll_y, body->data, body->size,
                                            click_abs_x, click_abs_y, true);
        if (clicked_id > 0) {
            app_editor_load_body(clicked_id);
        }
    }
}

