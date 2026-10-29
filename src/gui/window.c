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

typedef enum {
    ASYNC_IDLE = 0,
    ASYNC_SWAP_FINISH,
    ASYNC_REFRESH_WIN,
    ASYNC_BAKE_INIT,
    ASYNC_BAKE_WIN,
    ASYNC_SCENE_INIT,
    ASYNC_SCENE_WIN,
    ASYNC_SCENE_PRESENT
} AsyncCompState;

static AsyncCompState s_async_state = ASYNC_IDLE;
static ID s_swap_old_id = 0;
static ID s_swap_new_id = 0;
static ID s_swap_clear_id = 0;
static ID s_dirty_win_id = 0;
static UH s_async_idx = 0;

static bool windows_overlap(const Window *a, const Window *b)
{
    if (!a || !b) return false;
    if (a->state == WIN_STATE_CLOSED || a->state == WIN_STATE_MINIMIZED) return false;
    if (b->state == WIN_STATE_CLOSED || b->state == WIN_STATE_MINIMIZED) return false;
    if (a->x + a->w <= b->x || b->x + b->w <= a->x) return false;
    if (a->y + a->h <= b->y || b->y + b->h <= a->y) return false;
    return true;
}

static bool get_intersection(const Window *a, const Window *b,
                             uint16_t *ix, uint16_t *iy, uint16_t *iw, uint16_t *ih)
{
    if (!windows_overlap(a, b)) return false;
    uint16_t x1 = (a->x > b->x) ? a->x : b->x;
    uint16_t y1 = (a->y > b->y) ? a->y : b->y;
    uint16_t x2 = ((a->x + a->w) < (b->x + b->w)) ? (a->x + a->w) : (b->x + b->w);
    uint16_t y2 = ((a->y + a->h) < (b->y + b->h)) ? (a->y + a->h) : (b->y + b->h);
    *ix = x1;
    *iy = y1;
    *iw = x2 - x1;
    *ih = y2 - y1;
    return true;
}

static void op_rect_diff(const Window *r,
                         uint16_t ix, uint16_t iy, uint16_t iw, uint16_t ih)
{
    uint16_t rx = r->x, ry = r->y, rw = r->w, rh = r->h;
    uint16_t rx2 = rx + rw, ry2 = ry + rh;
    uint16_t ix2 = ix + iw, iy2 = iy + ih;

    if (iy > ry)   vdp_save_underlay_rect(rx, ry, rw, iy - ry);
    if (ry2 > iy2) vdp_save_underlay_rect(rx, iy2, rw, ry2 - iy2);
    if (ix > rx)   vdp_save_underlay_rect(rx, iy, ix - rx, ih);
    if (rx2 > ix2) vdp_save_underlay_rect(ix2, iy, rx2 - ix2, ih);
}

static void sync_underlay_if_needed(void)
{
    while (s_async_state == ASYNC_SWAP_FINISH) {
        wm_async_compositor_task();
    }
}

void wm_mark_dirty(void)
{
    sync_underlay_if_needed();
    s_wm_dirty = true;
    s_underlay_win_id = 0;
    s_async_state = ASYNC_SCENE_INIT;
    s_async_idx = 0;
}

void wm_mark_window_dirty(ID win_id)
{
    sync_underlay_if_needed();
    if (s_async_state != ASYNC_SCENE_INIT &&
        s_async_state != ASYNC_SCENE_WIN &&
        s_async_state != ASYNC_SCENE_PRESENT) {
        s_dirty_win_id = win_id;
        s_async_state = ASYNC_REFRESH_WIN;
    }
}

void wm_request_async_underlay(ID win_id)
{
    if (s_underlay_win_id == win_id) return;
    if (s_async_state == ASYNC_IDLE) {
        s_async_state = ASYNC_BAKE_INIT;
        s_async_idx = 0;
    }
}

bool wm_is_async_busy(void)
{
    return (s_async_state != ASYNC_IDLE);
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
    s_async_state = ASYNC_IDLE;
    s_swap_old_id = 0;
    s_swap_new_id = 0;
    s_dirty_win_id = 0;
    s_async_idx = 0;
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
    vdp_fill_rect(win->x + 2, win->y + 2, win->w - 28, 11, title_bg);
    vdp_wait_cmd();
    font_draw_string(win->x + 6, win->y + 3, win->title, title_fg, title_bg);
}

void wm_draw_window(Window *win)
{
    if (!win || win->state == WIN_STATE_CLOSED || win->state == WIN_STATE_MINIMIZED) {
        return;
    }

    /* Outer 3D Bevel Frame */
    vdp_draw_bevel_box(win->x, win->y, win->w, win->h,
                       BTRON_COL_WHITE, BTRON_COL_DARK_GRAY, BTRON_COL_LIGHT_GRAY);

    /* Title Bar & Window Controls: Zoom [+] and Close [X] buttons */
    wm_draw_titlebar(win);
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

void wm_focus_window(ID id)
{
    if (id == s_active_window_id) return;
    Window *new_win = wm_get_window(id);
    if (!new_win || new_win->state == WIN_STATE_CLOSED || new_win->state == WIN_STATE_MINIMIZED) return;

    sync_underlay_if_needed();

    Window *old_win = wm_get_window(s_active_window_id);

    /* Check if new_win was overlapped by any window above it in s_z_stack */
    bool was_occluded = false;
    bool occluded_by_other = false;
    bool seen_new = false;
    for (UH i = 0; i < s_window_count; i++) {
        if (s_z_stack[i] == id) {
            seen_new = true;
        } else if (seen_new) {
            Window *above = wm_get_window(s_z_stack[i]);
            if (windows_overlap(new_win, above)) {
                was_occluded = true;
                if (s_z_stack[i] != s_active_window_id) {
                    occluded_by_other = true;
                }
            }
        }
    }

    wm_set_active_window(id);

    uint8_t prev_page = vdp_get_draw_page();
    vdp_set_draw_page(0);
    if (old_win && old_win->state == WIN_STATE_NORMAL) {
        wm_draw_titlebar(old_win);
    }

    if (old_win && s_underlay_win_id == old_win->win_id && !occluded_by_other) {
        uint16_t ix, iy, iw, ih;
        if (get_intersection(old_win, new_win, &ix, &iy, &iw, &ih)) {
            vdp_swap_pages_rect(ix, iy, iw, ih);
        }
        wm_draw_titlebar(new_win);
        vdp_wait_cmd();
        vdp_set_draw_page(prev_page);

        s_underlay_win_id = 0;
        s_swap_old_id = old_win->win_id;
        s_swap_new_id = id;
        s_async_idx = 0;
        s_async_state = ASYNC_SWAP_FINISH;
        return;
    }

    if (!was_occluded) {
        wm_draw_titlebar(new_win);
        vdp_wait_cmd();
        vdp_set_draw_page(prev_page);
        s_underlay_win_id = 0;
        s_async_state = ASYNC_BAKE_INIT;
        return;
    }

    vdp_set_draw_page(prev_page);
    wm_refresh_window(id);
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
    s_underlay_win_id = 0;
    if (s_async_state == ASYNC_IDLE) {
        s_async_state = ASYNC_BAKE_INIT;
    }
}

void wm_async_compositor_task(void)
{
    if (s_async_state == ASYNC_IDLE) return;

    uint8_t prev_page = vdp_get_draw_page();
    Window *act = wm_get_active_window();

    switch (s_async_state) {
    case ASYNC_SWAP_FINISH: {
        Window *old_w = wm_get_window(s_swap_old_id);
        Window *new_w = wm_get_window(s_swap_new_id);
        if (old_w && new_w) {
            uint16_t ix, iy, iw, ih;
            vdp_set_draw_page(1);
            if (s_swap_clear_id != 0) {
                vdp_save_underlay_rect(old_w->x + 2, old_w->y + 2, old_w->w - 28, 11);
            } else if (!get_intersection(old_w, new_w, &ix, &iy, &iw, &ih)) {
                vdp_save_underlay_rect(old_w->x, old_w->y, old_w->w, old_w->h);
            } else {
                op_rect_diff(old_w, ix, iy, iw, ih);
            }
            s_underlay_win_id = s_swap_new_id;
            s_swap_clear_id = s_swap_new_id;
            s_async_state = ASYNC_IDLE;
        } else {
            s_async_state = ASYNC_BAKE_INIT;
        }
        break;
    }
    case ASYNC_REFRESH_WIN: {
        ID target = s_dirty_win_id;
        s_async_state = ASYNC_IDLE;
        wm_refresh_window(target);
        break;
    }
    case ASYNC_BAKE_INIT: {
        s_swap_clear_id = 0;
        if (!act || act->state != WIN_STATE_NORMAL) {
            s_async_state = ASYNC_IDLE;
            break;
        }
        vdp_save_underlay_rect(0, 13, SCREEN_WIDTH, SCREEN_HEIGHT - 13);
        vdp_set_draw_page(1);
        vdp_fill_rect(act->x, act->y, act->w, act->h, BTRON_COL_LIGHT_GRAY);

        bool any_overlap = false;
        for (UH i = 0; i < s_window_count; i++) {
            ID wid = s_z_stack[i];
            if (wid != act->win_id && windows_overlap(act, wm_get_window(wid))) {
                any_overlap = true;
                break;
            }
        }
        if (!any_overlap) {
            s_underlay_win_id = act->win_id;
            s_async_state = ASYNC_IDLE;
        } else {
            s_async_idx = 0;
            s_async_state = ASYNC_BAKE_WIN;
        }
        break;
    }
    case ASYNC_SCENE_INIT:
        vdp_save_underlay_rect(0, 0, SCREEN_WIDTH, 13);
        vdp_set_draw_page(1);
        vdp_fill_rect(0, 13, SCREEN_WIDTH, SCREEN_HEIGHT - 13, BTRON_COL_LIGHT_GRAY);
        s_async_idx = 0;
        s_async_state = ASYNC_SCENE_WIN;
        break;
    case ASYNC_BAKE_WIN:
    case ASYNC_SCENE_WIN: {
        bool is_bake = (s_async_state == ASYNC_BAKE_WIN);
        if (is_bake && (!act || act->state != WIN_STATE_NORMAL)) {
            s_async_state = ASYNC_IDLE;
            break;
        }
        vdp_set_draw_page(1);
        while (s_async_idx < s_window_count) {
            ID wid = s_z_stack[s_async_idx++];
            if (is_bake && wid == act->win_id) continue;
            Window *w = wm_get_window(wid);
            if (w && w->state != WIN_STATE_CLOSED && w->state != WIN_STATE_MINIMIZED) {
                wm_draw_window(w);
                break;
            }
        }
        if (s_async_idx >= s_window_count) {
            if (is_bake) {
                s_underlay_win_id = act->win_id;
                s_async_state = ASYNC_IDLE;
            } else {
                s_async_state = ASYNC_SCENE_PRESENT;
            }
        }
        break;
    }
    case ASYNC_SCENE_PRESENT:
        vdp_present_backbuffer();
        s_wm_dirty = false;
        s_underlay_win_id = 0;
        s_async_state = ASYNC_BAKE_INIT;
        break;
    default:
        s_async_state = ASYNC_IDLE;
        break;
    }

    vdp_wait_cmd();
    vdp_set_draw_page(prev_page);
}

void wm_begin_drag(ID win_id)
{
    /* Zero-CPU click capture: if Page 1 already holds win_id's underlay, reuse it!
     * Otherwise queue async underlay bake for μITRON Task 2. */
    if (s_underlay_win_id == win_id) {
        return;
    }
    if (s_async_state == ASYNC_IDLE) {
        s_async_state = ASYNC_BAKE_INIT;
    }
}

void wm_end_drag(ID win_id)
{
    (void)win_id;
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

    /* Ensure async underlay task has completed before moving window */
    if (s_swap_clear_id != 0) {
        s_underlay_win_id = 0;
        s_swap_clear_id = 0;
    }
    if (s_underlay_win_id != win_id) {
        if (s_async_state == ASYNC_IDLE) {
            s_async_state = ASYNC_BAKE_INIT;
        }
        while (s_async_state != ASYNC_IDLE) {
            wm_async_compositor_task();
        }
    }

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
    for (UH i = 0; i < s_window_count; i++) {
        Window *w = &s_windows[i];
        w->state = WIN_STATE_NORMAL;
        w->x = (uint16_t)(10u + i * 30u);
        w->y = (uint16_t)(16u + i * 12u);
        w->w = 330u;
        w->h = (uint16_t)(170u - i * 10u);
    }
    wm_set_active_window(1);
}

void wm_tile_windows(void)
{
    for (UH i = 0; i < s_window_count; i++) {
        Window *w = &s_windows[i];
        w->state = WIN_STATE_NORMAL;
        w->x = (i & 1u) ? 258u : 2u;
        w->y = (i & 2u) ? 113u : 14u;
        w->w = 252u;
        w->h = 96u;
    }
    wm_set_active_window(1);
}

void wm_restore_default(void)
{
    for (UH i = 0; i < s_window_count; i++) {
        Window *w = &s_windows[i];
        w->state = (w->win_id == 4) ? WIN_STATE_CLOSED : WIN_STATE_NORMAL;
        w->x = w->orig_x;
        w->y = w->orig_y;
        w->w = w->orig_w;
        w->h = w->orig_h;
    }
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
                wm_mark_dirty();
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
                wm_mark_dirty();
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

