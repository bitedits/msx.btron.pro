/*
 * compositor.c — Minimal BTRON Damage & BitBlt Compositor for Screen 7.
 */

#include "compositor.h"
#include "window.h"
#include "../msxgl/vdp.h"
#include "../font/font_latin.h"
#include <stddef.h>

#define OFFSCREEN_DRAG_Y 214

static Window *s_drag_win = NULL;
static uint16_t s_prev_x = 0;
static uint16_t s_prev_y = 0;

void comp_init(void)
{
    s_drag_win = NULL;
    s_prev_x = 0;
    s_prev_y = 0;
}

static bool rects_intersect(uint16_t x1, uint16_t y1, uint16_t w1, uint16_t h1,
                            uint16_t x2, uint16_t y2, uint16_t w2, uint16_t h2)
{
    return !(x1 + w1 <= x2 || x2 + w2 <= x1 ||
             y1 + h1 <= y2 || y2 + h2 <= y1);
}

void comp_repair_rect(uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh)
{
    if (rw == 0 || rh == 0) return;

    /* Clamp to desktop client area */
    if (ry < 13) {
        if (ry + rh <= 13) return;
        rh -= (13 - ry);
        ry = 13;
    }
    if (rx + rw > SCREEN_WIDTH) rw = SCREEN_WIDTH - rx;
    if (ry + rh > SCREEN_HEIGHT) rh = SCREEN_HEIGHT - ry;

    /* Fill damaged repair slice with desktop background color */
    vdp_fill_rect(rx, ry, rw, rh, BTRON_COL_LIGHT_GRAY);

    /* Composite intersecting windows back-to-front */
    UH count = wm_get_window_count();
    /* Inactive windows first */
    for (UH i = 0; i < count; i++) {
        Window *w = wm_get_window_by_index(i);
        if (w && !w->is_active && w->state != WIN_STATE_CLOSED && w->state != WIN_STATE_MINIMIZED) {
            if (rects_intersect(rx, ry, rw, rh, w->x, w->y, w->w, w->h)) {
                wm_draw_window(w);
            }
        }
    }
    /* Active window on top */
    for (UH i = 0; i < count; i++) {
        Window *w = wm_get_window_by_index(i);
        if (w && w->is_active && w->state != WIN_STATE_CLOSED && w->state != WIN_STATE_MINIMIZED) {
            if (rects_intersect(rx, ry, rw, rh, w->x, w->y, w->w, w->h)) {
                wm_draw_window(w);
            }
        }
    }
}

void comp_activate_window(Window *win)
{
    if (!win) return;
    if (win->is_active) return; /* Already active: zero work! */

    /* Demote currently active window's title bar to inactive gray */
    Window *old_act = wm_get_active_window();
    if (old_act) {
        old_act->is_active = false;
        vdp_fill_rect(old_act->x + 2, old_act->y + 2, old_act->w - 4, 11, BTRON_COL_MED_GRAY);
        font_draw_string(old_act->x + 6, old_act->y + 3, old_act->title, BTRON_COL_CHARCOAL, BTRON_COL_MED_GRAY);
    }

    /* Promote new window to active blue and draw on top */
    wm_set_active_window(win->win_id);
    wm_draw_window(win);
}

void comp_drag_start(Window *win)
{
    if (!win || win->state != WIN_STATE_NORMAL) return;

    comp_activate_window(win);

    /* Hardware capture window from screen into offscreen VRAM lines 214..511 */
    vdp_cmd_hmmm(win->x, win->y, 0, OFFSCREEN_DRAG_Y, win->w, win->h, 0);

    s_drag_win = win;
    s_prev_x = win->x;
    s_prev_y = win->y;
}

void comp_drag_move(Window *win, uint16_t new_x, uint16_t new_y)
{
    if (!win || win != s_drag_win) return;
    if (new_x == s_prev_x && new_y == s_prev_y) return;

    uint16_t ox = s_prev_x;
    uint16_t oy = s_prev_y;
    uint16_t w  = win->w;
    uint16_t h  = win->h;

    /* Compute and repair exposed strip(s) */
    if (new_x > ox) {
        comp_repair_rect(ox, oy, new_x - ox, h);
    } else if (new_x < ox) {
        comp_repair_rect(new_x + w, oy, ox - new_x, h);
    }

    if (new_y > oy) {
        comp_repair_rect(ox, oy, w, new_y - oy);
    } else if (new_y < oy) {
        comp_repair_rect(ox, new_y + h, w, oy - new_y);
    }

    /* High-speed VRAM-to-VRAM BitBlt captured window to new position */
    vdp_cmd_hmmm(0, OFFSCREEN_DRAG_Y, new_x, new_y, w, h, 0);

    win->x = new_x;
    win->y = new_y;
    s_prev_x = new_x;
    s_prev_y = new_y;
}

void comp_drag_end(Window *win)
{
    if (!win) return;

    /* Single final refresh of active window to ensure crisp pixel bevels */
    wm_draw_window(win);
    s_drag_win = NULL;
}

