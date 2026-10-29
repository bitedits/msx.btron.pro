/*
 * app_browser.h — BTRON Hypermedia Real & Virtual Body Navigator Application.
 */

#ifndef _APP_BROWSER_H_
#define _APP_BROWSER_H_

#include "../gui/window.h"

#ifdef __cplusplus
extern "C" {
#endif

void app_browser_init(void);
void app_browser_paint(Window *win);
void app_browser_on_click(Window *win, uint16_t local_x, uint16_t local_y);

#ifdef __cplusplus
}
#endif

#endif /* _APP_BROWSER_H_ */

