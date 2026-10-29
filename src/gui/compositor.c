/*
 * compositor.c — Minimal BTRON Damage & BitBlt Compositor for Screen 7.
 */

#include "compositor.h"
#include "window.h"
#include "../msxgl/vdp.h"

void comp_init(void)
{
}

void comp_repair_rect(uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh)
{
    vdp_restore_underlay_rect(rx, ry, rw, rh);
}

void comp_activate_window(Window *win)
{
    if (win) wm_focus_window(win->win_id);
}

void comp_drag_start(Window *win)
{
    if (win) wm_begin_drag(win->win_id);
}

void comp_drag_move(Window *win, uint16_t new_x, uint16_t new_y)
{
    if (win) (void)wm_move_window_bitblt(win->win_id, new_x, new_y);
}

void comp_drag_end(Window *win)
{
    if (win) wm_end_drag(win->win_id);
}


