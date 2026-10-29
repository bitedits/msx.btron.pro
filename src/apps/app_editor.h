/*
 * app_editor.h — BTRON Document Viewer / Editor Application.
 */

#ifndef _APP_EDITOR_H_
#define _APP_EDITOR_H_

#include "../gui/window.h"

#ifdef __cplusplus
extern "C" {
#endif

void app_editor_init(void);
void app_editor_paint(Window *win);
void app_editor_on_click(Window *win, uint16_t local_x, uint16_t local_y);
void app_editor_load_body(ID body_id);

#ifdef __cplusplus
}
#endif

#endif /* _APP_EDITOR_H_ */

