/*
 * menu.h — Top Cascading Menu Bar for BTRON Screen 7.
 */

#ifndef _MENU_H_
#define _MENU_H_

#include "../kernel/btron_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void menu_init(void);
void menu_draw_bar(void);
bool menu_handle_click(uint16_t x, uint16_t y);
void menu_close_popup(void);
bool menu_is_open(void);

#ifdef __cplusplus
}
#endif

#endif /* _MENU_H_ */

