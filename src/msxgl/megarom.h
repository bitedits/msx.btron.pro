/*
 * megarom.h — ASCII16 MegaROM Mapper (1024 KiB) for MSX.
 */

#ifndef _MSXGL_MEGAROM_H_
#define _MSXGL_MEGAROM_H_

#include <stdint.h>
#include "msxgl_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Bank mapping definitions */
#define BANK_KERNEL_BASE        0   /* Bank 0: CRT0, Kernel, Core drivers (Page 1) */
#define BANK_GUI_MENU           1   /* Bank 1: Window Manager & Cascading Menu */
#define BANK_APPS               2   /* Bank 2: Editor, Terminal, Browser, Cabinet */
#define BANK_FONTS              3   /* Bank 3: Tibetan & Kanji glyph bitmaps */
#define BANK_TAD_DOCS           4   /* Bank 4: Heart Sutra & NORTIA TAD documents */

void megarom_set_bank_page1(uint8_t bank);
void megarom_set_bank_page2(uint8_t bank);
uint8_t megarom_get_bank_page1(void);
uint8_t megarom_get_bank_page2(void);

#ifdef __cplusplus
}
#endif

#endif /* _MSXGL_MEGAROM_H_ */

