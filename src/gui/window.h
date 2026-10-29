/*
 * window.h — BTRON 3D Bevel Window Manager for Screen 7 (512x212).
 */

#ifndef _WINDOW_H_
#define _WINDOW_H_

#include "../kernel/btron_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIN_STATE_NORMAL    = 0,
    WIN_STATE_ZOOMED    = 1,
    WIN_STATE_MINIMIZED = 2,
    WIN_STATE_CLOSED    = 3
} WindowState;

typedef struct Window {
    ID          win_id;
    char        title[24];
    uint16_t    x;
    uint16_t    y;
    uint16_t    w;
    uint16_t    h;
    uint16_t    orig_x;
    uint16_t    orig_y;
    uint16_t    orig_w;
    uint16_t    orig_h;
    uint16_t    scroll_y;
    WindowState state;
    bool        is_active;
    void      (*paint)(struct Window *win);
    void      (*on_click)(struct Window *win, uint16_t local_x, uint16_t local_y);
} Window;

void wm_init(void);
Window *wm_create_window(ID id, const char *title, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                         void (*paint)(Window *win),
                         void (*on_click)(Window *win, uint16_t local_x, uint16_t local_y));
Window *wm_get_window(ID id);
Window *wm_get_active_window(void);
void wm_set_active_window(ID id);
void wm_draw_window(Window *win);
void wm_draw_all(void);
bool wm_handle_click(uint16_t x, uint16_t y);

#ifdef __cplusplus
}
#endif

#endif /* _WINDOW_H_ */

