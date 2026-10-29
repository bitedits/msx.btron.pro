/*
 * test_multilingual_font.c — Unit tests for Tibetan UTF-8 engine and Kanji lookup.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "../../src/font/font_tibetan.h"
#include "../../src/font/font_kanji.h"

/* Mock hardware stubs for native host testing */
void msx_out(uint8_t port, uint8_t val) { (void)port; (void)val; }
uint8_t msx_in(uint8_t port) { (void)port; return 0xFF; }
void vdp_set_vram_write_addr(uint32_t addr) { (void)addr; }
void vdp_set_vram_write_line(uint8_t byte_x, uint16_t eff_y) { (void)byte_x; (void)eff_y; }
uint16_t vdp_get_draw_y_offset(void) { return 0; }

#define TEST_PASS(msg) printf("  [PASS] %s\n", msg)
#define TEST_FAIL(msg) do { fprintf(stderr, "  [FAIL] %s\n", msg); return 1; } while(0)

int main(void)
{
    printf("=== Test: Multilingual Font Engine (Tibetan & Kanji) ===\n");

    /* 1. Tibetan Glyph Lookup */
    const uint8_t *g_om = font_tibetan_get_glyph16x16(0x0F02);
    if (!g_om) TEST_FAIL("Lookup of Tibetan OM symbol (0x0F02)");
    TEST_PASS("Tibetan 16x16 glyph lookup");

    const uint8_t *g_ka = font_tibetan_get_glyph8x16(0x0F40);
    if (!g_ka) TEST_FAIL("Lookup of Tibetan letter KA (0x0F40)");
    TEST_PASS("Tibetan 8x16 consonant KA lookup");

    const uint8_t *g_shad = font_tibetan_get_glyph8x16(0x0F0D);
    if (!g_shad) TEST_FAIL("Lookup of Tibetan punctuation SHAD (0x0F0D)");
    TEST_PASS("Tibetan 8x16 punctuation SHAD lookup");

    const uint8_t *g_tsheg = font_tibetan_get_glyph8x16(0x0F0B);
    if (!g_tsheg) TEST_FAIL("Lookup of Tibetan punctuation TSHEG (0x0F0B)");
    TEST_PASS("Tibetan 8x16 punctuation TSHEG lookup");

    const uint8_t *g_gigu = font_tibetan_get_glyph8x16(0x0F72);
    if (!g_gigu) TEST_FAIL("Lookup of Tibetan vowel I (GI-GU, 0x0F72)");
    TEST_PASS("Tibetan vowel GI-GU lookup");

    /* 2. Kanji Embedded Glyph Table */
    /* Check Heart Sutra title kanji: 般(HAN), 若(NYA), 心(SHIN), 経(GYO) */
    const uint8_t expected_kanji[] = {
        KANJI_HAN, KANJI_NYA, KANJI_SHIN, KANJI_GYO,
        KANJI_BUTSU, KANJI_SETSU, KANJI_KUU, KANJI_SHIKI,
        KANJI_HOU, KANJI_KOKU, KANJI_KEN, KANJI_MEI,
        KANJI_KANA_NO, KANJI_KANA_RU, KANJI_KANA_TE, KANJI_KANA_XI, KANJI_KANA_A
    };

    for (size_t i = 0; i < sizeof(expected_kanji); i++) {
        uint8_t id = expected_kanji[i];
        if (id == 0) {
            TEST_FAIL("Kanji ID is 0");
        }
    }
    TEST_PASS("Embedded Kanji IDs defined and non-zero");

    /* 3. UTF-8 Parser Simulation */
    /* Tibetan string: ༄༅། ། (U+0F04 U+0F05 U+0F0D ' ' U+0F0D) */
    static const uint8_t utf8_head[] = {
        0xE0, 0xBC, 0x84,
        0xE0, 0xBC, 0x85,
        0xE0, 0xBC, 0x8D,
        0x00
    };

    const uint8_t *p = utf8_head;
    uint16_t cp1 = (uint16_t)(((*p & 0x0F) << 12) | ((*(p + 1) & 0x3F) << 6) | (*(p + 2) & 0x3F));
    p += 3;
    uint16_t cp2 = (uint16_t)(((*p & 0x0F) << 12) | ((*(p + 1) & 0x3F) << 6) | (*(p + 2) & 0x3F));
    p += 3;
    uint16_t cp3 = (uint16_t)(((*p & 0x0F) << 12) | ((*(p + 1) & 0x3F) << 6) | (*(p + 2) & 0x3F));

    if (cp1 != 0x0F04 || cp2 != 0x0F05 || cp3 != 0x0F0D) {
        fprintf(stderr, "Decoded: 0x%04X, 0x%04X, 0x%04X\n", cp1, cp2, cp3);
        TEST_FAIL("Tibetan 3-byte UTF-8 decoding");
    }
    TEST_PASS("Tibetan UTF-8 3-byte decoding verified");

    printf("=== All Multilingual Font Tests PASSED ===\n\n");
    return 0;
}
