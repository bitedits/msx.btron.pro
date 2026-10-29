/*
 * app_cabinet.h — BTRON Storage Cabinet / Volume Viewer Application.
 */

#ifndef _APP_CABINET_H_
#define _APP_CABINET_H_

#include "../gui/window.h"

#ifdef __cplusplus
extern "C" {
#endif

void app_cabinet_init(void);
void app_cabinet_paint(Window *win);
void app_cabinet_on_click(Window *win, uint16_t local_x, uint16_t local_y);

#ifdef __cplusplus
}
#endif

#endif /* _APP_CABINET_H_ */

