/*
 * font_kanji.h — 16x16 Kanji Font Renderer for MSX Screen 7.
 * Supports MSX Kanji ROM (Ports 0xD8..0xDB) with embedded fallback bitmaps.
 */

#ifndef _FONT_KANJI_H_
#define _FONT_KANJI_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void font_kanji_init(void);
bool font_has_kanji_rom(void);
void font_draw_kanji(uint16_t x, uint16_t y, uint16_t jis_or_id, uint8_t fg_col, uint8_t bg_col);

/* Pre-defined indices for embedded document Kanji/Kana */
#define KANJI_HAN       0x01 /* 般 */
#define KANJI_NYA       0x02 /* 若 */
#define KANJI_SHIN      0x03 /* 心 */
#define KANJI_GYO       0x04 /* 経 */
#define KANJI_BUTSU     0x05 /* 仏 */
#define KANJI_SETSU     0x06 /* 説 */
#define KANJI_KUU       0x07 /* 空 */
#define KANJI_SHIKI     0x08 /* 色 */
#define KANJI_HOU       0x09 /* 報 */
#define KANJI_KOKU      0x0A /* 告 */
#define KANJI_KEN       0x0B /* 件 */
#define KANJI_MEI       0x0C /* 名 */
#define KANJI_EN        0x0D /* 円 */
#define KANJI_KANA_NO   0x0E /* ノ */
#define KANJI_KANA_RU   0x0F /* ル */
#define KANJI_KANA_TE   0x10 /* テ */
#define KANJI_KANA_XI   0x11 /* ィ */
#define KANJI_KANA_A    0x12 /* ア */

#ifdef __cplusplus
}
#endif

#endif /* _FONT_KANJI_H_ */

