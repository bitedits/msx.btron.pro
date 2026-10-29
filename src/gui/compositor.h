/*
 * compositor.h — Minimal BTRON Damage & BitBlt Compositor for Screen 7.
 * Implements RENDER.txt damage rectangles and V9938 HMMM window capture/move.
 */

#ifndef _COMPOSITOR_H_
#define _COMPOSITOR_H_

#include "window.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void comp_init(void);

/* Repair only damaged rectangle without wiping the entire desktop */
void comp_repair_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/* Activate and promote window to top without wiping desktop */
void comp_activate_window(Window *win);

/* Hardware BitBlt Window Dragging using offscreen VRAM (lines 214..511) */
void comp_drag_start(Window *win);
void comp_drag_move(Window *win, uint16_t new_x, uint16_t new_y);
void comp_drag_end(Window *win);

#ifdef __cplusplus
}
#endif

#endif /* _COMPOSITOR_H_ */

