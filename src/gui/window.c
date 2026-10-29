/*
 * window.c — BTRON 3D Bevel Window Manager implementation.
 */

#include "window.h"
#include "compositor.h"
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

UH wm_get_window_count(void)
{
    return s_window_count;
}

Window *wm_get_window_by_index(UH index)
{
    if (index < s_window_count) return &s_windows[index];
    return NULL;
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

void wm_cascade_windows(void)
{
    Window *w1 = wm_get_window(1);
    Window *w2 = wm_get_window(2);
    Window *w3 = wm_get_window(3);
    Window *w4 = wm_get_window(4);

    if (w1) { w1->state = WIN_STATE_NORMAL; w1->x = 10; w1->y = 16; w1->w = 330; w1->h = 170; }
    if (w2) { w2->state = WIN_STATE_NORMAL; w2->x = 40; w2->y = 28; w2->w = 330; w2->h = 160; }
    if (w3) { w3->state = WIN_STATE_NORMAL; w3->x = 70; w3->y = 40; w3->w = 330; w3->h = 150; }
    if (w4) { w4->state = WIN_STATE_NORMAL; w4->x = 100; w4->y = 52; w4->w = 330; w4->h = 140; }

    wm_set_active_window(1);
    wm_draw_all();
}

void wm_tile_windows(void)
{
    Window *w1 = wm_get_window(1);
    Window *w2 = wm_get_window(2);
    Window *w3 = wm_get_window(3);
    Window *w4 = wm_get_window(4);

    if (w1) { w1->state = WIN_STATE_NORMAL; w1->x = 2;   w1->y = 14;  w1->w = 252; w1->h = 96; }
    if (w2) { w2->state = WIN_STATE_NORMAL; w2->x = 258; w2->y = 14;  w2->w = 252; w2->h = 96; }
    if (w3) { w3->state = WIN_STATE_NORMAL; w3->x = 2;   w3->y = 113; w3->w = 252; w3->h = 96; }
    if (w4) { w4->state = WIN_STATE_NORMAL; w4->x = 258; w4->y = 113; w4->w = 252; w4->h = 96; }

    wm_set_active_window(1);
    wm_draw_all();
}

void wm_restore_default(void)
{
    Window *w1 = wm_get_window(1);
    Window *w2 = wm_get_window(2);
    Window *w3 = wm_get_window(3);
    Window *w4 = wm_get_window(4);

    if (w1) { w1->state = WIN_STATE_NORMAL; w1->x = w1->orig_x; w1->y = w1->orig_y; w1->w = w1->orig_w; w1->h = w1->orig_h; }
    if (w2) { w2->state = WIN_STATE_NORMAL; w2->x = w2->orig_x; w2->y = w2->orig_y; w2->w = w2->orig_w; w2->h = w2->orig_h; }
    if (w3) { w3->state = WIN_STATE_NORMAL; w3->x = w3->orig_x; w3->y = w3->orig_y; w3->w = w3->orig_w; w3->h = w3->orig_h; }
    if (w4) { w4->state = WIN_STATE_CLOSED; w4->x = w4->orig_x; w4->y = w4->orig_y; w4->w = w4->orig_w; w4->h = w4->orig_h; }

    wm_set_active_window(2);
    wm_draw_all();
}

bool wm_handle_click(uint16_t x, uint16_t y, ID *out_drag_win_id)
{
    if (out_drag_win_id) *out_drag_win_id = 0;

    /* Check from topmost (active) down to bottommost */
    for (int16_t i = (int16_t)s_window_count - 1; i >= 0; i--) {
        Window *win = &s_windows[i];
        if (win->state == WIN_STATE_CLOSED || win->state == WIN_STATE_MINIMIZED) {
            continue;
        }

        if (x >= win->x && x < win->x + win->w &&
            y >= win->y && y < win->y + win->h) {

            bool was_active = win->is_active;
            if (!was_active) {
                comp_activate_window(win);
            }

            /* Check Close button [X] */
            uint16_t btn_close_x = win->x + win->w - 14;
            if (x >= btn_close_x && x <= btn_close_x + 10 && y >= win->y + 2 && y <= win->y + 12) {
                win->state = WIN_STATE_CLOSED;
                comp_repair_rect(win->x, win->y, win->w, win->h);
                return true;
            }

            /* Check Zoom button [+] */
            uint16_t btn_zoom_x = win->x + win->w - 26;
            if (x >= btn_zoom_x && x <= btn_zoom_x + 10 && y >= win->y + 2 && y <= win->y + 12) {
                uint16_t old_x = win->x, old_y = win->y, old_w = win->w, old_h = win->h;
                if (win->state == WIN_STATE_NORMAL) {
                    win->state = WIN_STATE_ZOOMED;
                    win->x = 2; win->y = 14; win->w = 508; win->h = 194;
                } else {
                    win->state = WIN_STATE_NORMAL;
                    win->x = win->orig_x; win->y = win->orig_y;
                    win->w = win->orig_w; win->h = win->orig_h;
                }
                comp_repair_rect(old_x, old_y, old_w, old_h);
                wm_draw_window(win);
                return true;
            }

            /* Title bar click (outside buttons): start dragging! */
            if (y >= win->y && y < win->y + 14) {
                if (out_drag_win_id && win->state == WIN_STATE_NORMAL) {
                    *out_drag_win_id = win->win_id;
                }
                return true;
            }

            /* Content area click */
            if (y >= win->y + 14 && win->on_click) {
                win->on_click(win, x - (win->x + 2), y - (win->y + 14));
            }

            return true;
        }
    }

    return false;
}

