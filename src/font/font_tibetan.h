/*
 * font_tibetan.h — Tibetan Script Renderer for MSX Screen 7 (512x212).
 * Source: GNU Unifont / BTRON 3.20 Canonical Tibetan Unicode Glyphs (U+0F00..U+0FFF).
 */

#ifndef _FONT_TIBETAN_H_
#define _FONT_TIBETAN_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void font_tibetan_init(void);
const uint8_t *font_tibetan_get_glyph8x16(uint16_t codepoint);
const uint8_t *font_tibetan_get_glyph16x16(uint16_t codepoint);
void font_draw_tibetan_glyph(uint16_t x, uint16_t y, uint16_t codepoint, uint8_t fg_col, uint8_t bg_col);
uint16_t font_render_tibetan_utf8(uint16_t x, uint16_t y, const char *utf8_str, uint8_t fg_col, uint8_t bg_col);

#ifdef __cplusplus
}
#endif

#endif /* _FONT_TIBETAN_H_ */

