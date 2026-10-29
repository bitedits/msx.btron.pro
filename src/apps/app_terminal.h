/*
 * app_terminal.h — B-MSX Command Shell Terminal Application.
 */

#ifndef _APP_TERMINAL_H_
#define _APP_TERMINAL_H_

#include "../gui/window.h"

#ifdef __cplusplus
extern "C" {
#endif

void app_terminal_init(void);
void app_terminal_paint(Window *win);
void app_terminal_on_click(Window *win, uint16_t local_x, uint16_t local_y);
void app_terminal_input_char(char c);

#ifdef __cplusplus
}
#endif

#endif /* _APP_TERMINAL_H_ */

