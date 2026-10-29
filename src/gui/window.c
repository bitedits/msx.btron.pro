/*
 * window.c — BTRON 3D Bevel Window Manager implementation.
 */

#include "window.h"
#include "../msxgl/vdp.h"
#include "../font/font_latin.h"
#include <stddef.h>

static Window s_windows[MAX_WINDOWS];
static UH s_window_count = 0;
static ID s_active_window_id = 0;

void wm_init(void)
{
    s_window_count = 0;
    s_active_window_id = 0;
    for (UH i = 0; i < MAX_WINDOWS; i++) {
        s_windows[i].win_id = 0;
        s_windows[i].state = WIN_STATE_CLOSED;
        s_windows[i].is_active = false;
        s_windows[i].scroll_y = 0;
        s_windows[i].paint = NULL;
        s_windows[i].on_click = NULL;
        s_windows[i].title[0] = '\0';
    }
}

Window *wm_create_window(ID id, const char *title, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                         void (*paint)(Window *win),
                         void (*on_click)(Window *win, uint16_t local_x, uint16_t local_y))
{
    if (s_window_count >= MAX_WINDOWS || id <= 0) {
        return NULL;
    }

    Window *win = &s_windows[s_window_count];
    win->win_id = id;
    win->x = win->orig_x = x;
    win->y = win->orig_y = y;
    win->w = win->orig_w = w;
    win->h = win->orig_h = h;
    win->scroll_y = 0;
    win->state = WIN_STATE_NORMAL;
    win->is_active = false;
    win->paint = paint;
    win->on_click = on_click;

    UH j = 0;
    if (title) {
        while (title[j] != '\0' && j < 23) {
            win->title[j] = title[j];
            j++;
        }
    }
    win->title[j] = '\0';
    s_window_count++;

    wm_set_active_window(id);
    return win;
}

Window *wm_get_window(ID id)
{
    for (UH i = 0; i < s_window_count; i++) {
        if (s_windows[i].win_id == id) {
            return &s_windows[i];
        }
    }
    return NULL;
}

Window *wm_get_active_window(void)
{
    return wm_get_window(s_active_window_id);
}

void wm_set_active_window(ID id)
{
    s_active_window_id = id;
    for (UH i = 0; i < s_window_count; i++) {
        s_windows[i].is_active = (s_windows[i].win_id == id);
    }
}

void wm_draw_window(Window *win)
{
    if (!win || win->state == WIN_STATE_CLOSED || win->state == WIN_STATE_MINIMIZED) {
        return;
    }

    /* Outer 3D Bevel Frame */
    vdp_draw_bevel_box(win->x, win->y, win->w, win->h,
                       BTRON_COL_WHITE, BTRON_COL_DARK_GRAY, BTRON_COL_LIGHT_GRAY);

    /* Title Bar */
    uint8_t title_bg = win->is_active ? BTRON_COL_TITLE_BLUE : BTRON_COL_MED_GRAY;
    uint8_t title_fg = win->is_active ? BTRON_COL_WHITE : BTRON_COL_CHARCOAL;
    vdp_fill_rect(win->x + 2, win->y + 2, win->w - 4, 11, title_bg);

    /* Title text */
    font_draw_string(win->x + 6, win->y + 3, win->title, title_fg, title_bg);

    /* Window Controls: Zoom [+] and Close [X] buttons */
    uint16_t btn_close_x = win->x + win->w - 14;
    uint16_t btn_zoom_x = win->x + win->w - 26;

    vdp_draw_bevel_box(btn_zoom_x, win->y + 2, 10, 10,
                       BTRON_COL_WHITE, BTRON_COL_DARK_GRAY, BTRON_COL_LIGHT_GRAY);
    font_draw_string(btn_zoom_x + 2, win->y + 3, "+", BTRON_COL_BLACK, BTRON_COL_LIGHT_GRAY);

    vdp_draw_bevel_box(btn_close_x, win->y + 2, 10, 10,
                       BTRON_COL_WHITE, BTRON_COL_DARK_GRAY, BTRON_COL_LIGHT_GRAY);
    font_draw_string(btn_close_x + 2, win->y + 3, "x", BTRON_COL_BLACK, BTRON_COL_LIGHT_GRAY);

    /* Content Area Background (Inset Bevel) */
    uint16_t content_x = win->x + 2;
    uint16_t content_y = win->y + 14;
    uint16_t content_w = win->w - 4;
    uint16_t content_h = win->h - 16;

    vdp_draw_bevel_box(content_x, content_y, content_w, content_h,
                       BTRON_COL_DARK_GRAY, BTRON_COL_WHITE, BTRON_COL_WHITE);

    /* Paint window contents */
    if (win->paint) {
        win->paint(win);
    }
}

void wm_draw_all(void)
{
    /* Clear desktop background (BTRON Light Gray = 8) */
    vdp_fill_rect(0, 13, SCREEN_WIDTH, SCREEN_HEIGHT - 13, BTRON_COL_LIGHT_GRAY);

    /* Draw all inactive windows first, then active on top */
    for (UH i = 0; i < s_window_count; i++) {
        if (!s_windows[i].is_active) {
            wm_draw_window(&s_windows[i]);
        }
    }
    for (UH i = 0; i < s_window_count; i++) {
        if (s_windows[i].is_active) {
            wm_draw_window(&s_windows[i]);
        }
    }
}

bool wm_handle_click(uint16_t x, uint16_t y)
{
    /* Check from topmost (active) down to bottommost */
    for (int16_t i = (int16_t)s_window_count - 1; i >= 0; i--) {
        Window *win = &s_windows[i];
        if (win->state == WIN_STATE_CLOSED || win->state == WIN_STATE_MINIMIZED) {
            continue;
        }

        if (x >= win->x && x < win->x + win->w &&
            y >= win->y && y < win->y + win->h) {

            wm_set_active_window(win->win_id);

            /* Check Close button [X] */
            uint16_t btn_close_x = win->x + win->w - 14;
            if (x >= btn_close_x && x <= btn_close_x + 10 && y >= win->y + 2 && y <= win->y + 12) {
                win->state = WIN_STATE_CLOSED;
                wm_draw_all();
                return true;
            }

            /* Check Zoom button [+] */
            uint16_t btn_zoom_x = win->x + win->w - 26;
            if (x >= btn_zoom_x && x <= btn_zoom_x + 10 && y >= win->y + 2 && y <= win->y + 12) {
                if (win->state == WIN_STATE_NORMAL) {
                    win->state = WIN_STATE_ZOOMED;
                    win->x = 2; win->y = 14; win->w = 508; win->h = 194;
                } else {
                    win->state = WIN_STATE_NORMAL;
                    win->x = win->orig_x; win->y = win->orig_y;
                    win->w = win->orig_w; win->h = win->orig_h;
                }
                wm_draw_all();
                return true;
            }

            /* Content area click */
            if (y >= win->y + 14 && win->on_click) {
                win->on_click(win, x - (win->x + 2), y - (win->y + 14));
            }

            wm_draw_all();
            return true;
        }
    }

    return false;
}

