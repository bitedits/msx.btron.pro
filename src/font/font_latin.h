/*
 * font_latin.h — 8x8 Standard ASCII Font Renderer for Screen 7 (512x212).
 */

#ifndef _FONT_LATIN_H_
#define _FONT_LATIN_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void font_latin_init(void);
void font_draw_char(uint16_t x, uint16_t y, char c, uint8_t fg_col, uint8_t bg_col);
void font_draw_string(uint16_t x, uint16_t y, const char *str, uint8_t fg_col, uint8_t bg_col);
uint16_t font_get_string_width(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* _FONT_LATIN_H_ */

