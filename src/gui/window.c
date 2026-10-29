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
static ID s_z_stack[MAX_WINDOWS];
static bool s_wm_dirty = false;
static ID s_underlay_win_id = 0;

static bool windows_overlap(const Window *a, const Window *b)
{
    if (!a || !b) return false;
    if (a->state == WIN_STATE_CLOSED || a->state == WIN_STATE_MINIMIZED) return false;
    if (b->state == WIN_STATE_CLOSED || b->state == WIN_STATE_MINIMIZED) return false;
    if (a->x + a->w <= b->x || b->x + b->w <= a->x) return false;
    if (a->y + a->h <= b->y || b->y + b->h <= a->y) return false;
    return true;
}

void wm_mark_dirty(void)
{
    s_wm_dirty = true;
}

bool wm_consume_dirty(void)
{
    bool d = s_wm_dirty;
    s_wm_dirty = false;
    return d;
}

void wm_init(void)
{
    s_window_count = 0;
    s_active_window_id = 0;
    s_wm_dirty = false;
    s_underlay_win_id = 0;
    for (UH i = 0; i < MAX_WINDOWS; i++) {
        s_windows[i].win_id = 0;
        s_windows[i].state = WIN_STATE_CLOSED;
        s_windows[i].is_active = false;
        s_windows[i].scroll_y = 0;
        s_windows[i].paint = NULL;
        s_windows[i].on_click = NULL;
        s_windows[i].title[0] = '\0';
        s_z_stack[i] = 0;
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

    s_z_stack[s_window_count] = id;
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

    /* Move active window id to top of z-stack */
    UH found = 0;
    bool has = false;
    for (UH i = 0; i < s_window_count; i++) {
        if (s_z_stack[i] == id) {
            found = i;
            has = true;
            break;
        }
    }
    if (has) {
        for (UH i = found; i + 1 < s_window_count; i++) {
            s_z_stack[i] = s_z_stack[i + 1];
        }
        s_z_stack[s_window_count - 1] = id;
    }
}

void wm_draw_titlebar(Window *win)
{
    if (!win || win->state == WIN_STATE_CLOSED || win->state == WIN_STATE_MINIMIZED) {
        return;
    }

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
    wm_draw_titlebar(win);

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
    s_underlay_win_id = 0;

    /* Clear desktop background (BTRON Light Gray = 8) */
    vdp_fill_rect(0, 13, SCREEN_WIDTH, SCREEN_HEIGHT - 13, BTRON_COL_LIGHT_GRAY);

    /* Draw windows in z-stack order: bottom to top */
    for (UH i = 0; i < s_window_count; i++) {
        Window *w = wm_get_window(s_z_stack[i]);
        if (w) {
            wm_draw_window(w);
        }
    }
}

void wm_draw_underlay(ID exclude_win_id)
{
    /* Clear desktop background (BTRON Light Gray = 8) */
    vdp_fill_rect(0, 13, SCREEN_WIDTH, SCREEN_HEIGHT - 13, BTRON_COL_LIGHT_GRAY);

    /* Draw all visible windows except the captured/dragged window */
    for (UH i = 0; i < s_window_count; i++) {
        ID wid = s_z_stack[i];
        if (wid == exclude_win_id) continue;
        Window *w = wm_get_window(wid);
        if (w) {
            wm_draw_window(w);
        }
    }
}

static void sync_underlay_if_needed(void)
{
    if (s_underlay_win_id > 0) {
        Window *uw = wm_get_window(s_underlay_win_id);
        if (uw && uw->state == WIN_STATE_NORMAL) {
            vdp_save_underlay_rect(uw->x, uw->y, uw->w, uw->h);
            vdp_wait_cmd();
        }
        s_underlay_win_id = 0;
    }
}

void wm_focus_window(ID id)
{
    if (id == s_active_window_id) return;
    Window *new_win = wm_get_window(id);
    if (!new_win || new_win->state == WIN_STATE_CLOSED || new_win->state == WIN_STATE_MINIMIZED) return;

    sync_underlay_if_needed();

    Window *old_win = wm_get_window(s_active_window_id);

    /* Check if new_win was overlapped by any window above it in s_z_stack */
    bool was_occluded = false;
    bool seen_new = false;
    for (UH i = 0; i < s_window_count; i++) {
        if (s_z_stack[i] == id) {
            seen_new = true;
        } else if (seen_new) {
            Window *above = wm_get_window(s_z_stack[i]);
            if (windows_overlap(new_win, above)) {
                was_occluded = true;
                break;
            }
        }
    }

    wm_set_active_window(id);

    uint8_t prev_page = vdp_get_draw_page();
    vdp_set_draw_page(1);

    if (old_win && old_win->state != WIN_STATE_CLOSED && old_win->state != WIN_STATE_MINIMIZED) {
        wm_draw_titlebar(old_win);
        if (!windows_overlap(old_win, new_win)) {
            vdp_restore_underlay_rect(old_win->x + 2, old_win->y + 2, old_win->w - 4, 11);
        }
    }

    if (!was_occluded) {
        wm_draw_titlebar(new_win);
        vdp_restore_underlay_rect(new_win->x + 2, new_win->y + 2, new_win->w - 4, 11);
        if (old_win && windows_overlap(old_win, new_win)) {
            vdp_restore_underlay_rect(old_win->x + 2, old_win->y + 2, old_win->w - 4, 11);
            vdp_restore_underlay_rect(new_win->x, new_win->y, new_win->w, new_win->h);
        }
    } else {
        wm_draw_window(new_win);
        if (old_win && windows_overlap(old_win, new_win)) {
            vdp_restore_underlay_rect(old_win->x + 2, old_win->y + 2, old_win->w - 4, 11);
        }
        vdp_restore_underlay_rect(new_win->x, new_win->y, new_win->w, new_win->h);
    }

    vdp_wait_cmd();
    vdp_set_draw_page(prev_page);
}

void wm_refresh_window(ID win_id)
{
    Window *win = wm_get_window(win_id);
    if (!win || win->state == WIN_STATE_CLOSED || win->state == WIN_STATE_MINIMIZED) return;

    sync_underlay_if_needed();

    uint8_t prev_page = vdp_get_draw_page();
    vdp_set_draw_page(1);
    wm_draw_window(win);

    /* Redraw any window above win in z-stack that overlaps win */
    bool seen = false;
    for (UH i = 0; i < s_window_count; i++) {
        if (s_z_stack[i] == win_id) {
            seen = true;
        } else if (seen) {
            Window *above = wm_get_window(s_z_stack[i]);
            if (windows_overlap(win, above)) {
                wm_draw_window(above);
            }
        }
    }

    vdp_restore_underlay_rect(win->x, win->y, win->w, win->h);
    vdp_wait_cmd();
    vdp_set_draw_page(prev_page);
}

void wm_begin_drag(ID win_id)
{
    Window *win = wm_get_window(win_id);
    if (!win) return;

    /* If Page 1 already holds the underlay for this exact window, reuse it in 0 T-states! */
    if (s_underlay_win_id == win_id) {
        return;
    }

    sync_underlay_if_needed();

    uint8_t prev_page = vdp_get_draw_page();
    vdp_set_draw_page(1);

    bool any_overlap = false;
    for (UH i = 0; i < s_window_count; i++) {
        ID wid = s_z_stack[i];
        if (wid == win_id) continue;
        Window *other = wm_get_window(wid);
        if (windows_overlap(win, other)) {
            any_overlap = true;
            break;
        }
    }

    if (!any_overlap) {
        /* Fast path (< 0.02 frames): clear only win's rectangle on Page 1 to desktop gray */
        vdp_fill_rect(win->x, win->y, win->w, win->h, BTRON_COL_LIGHT_GRAY);
    } else {
        wm_draw_underlay(win_id);
    }

    vdp_wait_cmd();
    vdp_set_draw_page(prev_page);
    s_underlay_win_id = win_id;
}

void wm_end_drag(ID win_id)
{
    (void)win_id;
    /* Keep s_underlay_win_id cached on Page 1 so subsequent drags of the same
     * window require zero redrawing; sync_underlay_if_needed() commits it on focus switch. */
}

bool wm_move_window_bitblt(ID win_id, uint16_t new_x, uint16_t new_y)
{
    Window *win = wm_get_window(win_id);
    if (!win || win->state != WIN_STATE_NORMAL) return false;

    /* Align X to even pixel boundary for Screen 7 4bpp HMMM */
    new_x &= (uint16_t)~1u;
    if (new_x < 2u) new_x = 2u;
    if (new_x + win->w > 510u) new_x = (uint16_t)((510u - win->w) & (uint16_t)~1u);
    if (new_y < 14u) new_y = 14u;
    if (new_y + win->h > 210u) new_y = (uint16_t)(210u - win->h);

    uint16_t old_x = win->x;
    uint16_t old_y = win->y;
    uint16_t w = win->w;
    uint16_t h = win->h;

    if (new_x == old_x && new_y == old_y) return false;

    uint8_t prev_page = vdp_get_draw_page();
    vdp_set_draw_page(0);

    /* 1. Hardware BitBlt window bitmap from (old_x, old_y) to (new_x, new_y) */
    vdp_copy_rect(old_x, old_y, new_x, new_y, w, h);

    /* 2. Restore exposed horizontal strip from Page 1 Underlay */
    if (new_y > old_y) {
        uint16_t dh = new_y - old_y;
        if (dh > h) dh = h;
        vdp_restore_underlay_rect(old_x, old_y, w, dh);
    } else if (new_y < old_y) {
        uint16_t dh = old_y - new_y;
        if (dh > h) dh = h;
        vdp_restore_underlay_rect(old_x, (uint16_t)(old_y + h - dh), w, dh);
    }

    /* 3. Restore exposed vertical strip from Page 1 Underlay */
    if (new_x > old_x) {
        uint16_t dw = new_x - old_x;
        if (dw > w) dw = w;
        vdp_restore_underlay_rect(old_x, old_y, dw, h);
    } else if (new_x < old_x) {
        uint16_t dw = old_x - new_x;
        if (dw > w) dw = w;
        vdp_restore_underlay_rect((uint16_t)(old_x + w - dw), old_y, dw, h);
    }

    vdp_wait_cmd();
    vdp_set_draw_page(prev_page);

    win->x = new_x;
    win->y = new_y;
    return true;
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
}

bool wm_handle_click(uint16_t x, uint16_t y, ID *out_drag_win_id)
{
    if (out_drag_win_id) *out_drag_win_id = 0;

    /* Check from topmost (active) down to bottommost */
    for (int16_t i = (int16_t)s_window_count - 1; i >= 0; i--) {
        Window *win = wm_get_window(s_z_stack[i]);
        if (!win || win->state == WIN_STATE_CLOSED || win->state == WIN_STATE_MINIMIZED) {
            continue;
        }

        if (x >= win->x && x < win->x + win->w &&
            y >= win->y && y < win->y + win->h) {

            /* Check Close button [X] */
            uint16_t btn_close_x = win->x + win->w - 14;
            if (x >= btn_close_x && x <= btn_close_x + 10 && y >= win->y + 2 && y <= win->y + 12) {
                wm_set_active_window(win->win_id);
                win->state = WIN_STATE_CLOSED;
                s_wm_dirty = true;
                return true;
            }

            /* Check Zoom button [+] */
            uint16_t btn_zoom_x = win->x + win->w - 26;
            if (x >= btn_zoom_x && x <= btn_zoom_x + 10 && y >= win->y + 2 && y <= win->y + 12) {
                wm_set_active_window(win->win_id);
                if (win->state == WIN_STATE_NORMAL) {
                    win->state = WIN_STATE_ZOOMED;
                    win->x = 2; win->y = 14; win->w = 508; win->h = 194;
                } else {
                    win->state = WIN_STATE_NORMAL;
                    win->x = win->orig_x; win->y = win->orig_y;
                    win->w = win->orig_w; win->h = win->orig_h;
                }
                s_wm_dirty = true;
                return true;
            }

            if (!win->is_active) {
                wm_focus_window(win->win_id);
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

