/*
 * test_vdp_page_flip.c — Verification test for V9938/V9958 hardware double-buffering & page flipping.
 * Validates independent Page 0 / Page 1 drawing, atomic page flipping via R#2, and HMMM page synchronization.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "../../../btron/src/emulators/msx/v9938.h"

#define TEST_PASS(msg) printf("  [PASS] %s\n", msg)
#define TEST_FAIL(msg) do { fprintf(stderr, "  [FAIL] %s\n", msg); return 1; } while(0)

static void write_reg(v9938_t *v, uint8_t r, uint8_t val)
{
    v9938_write(v, 1, val);                 /* Port 0x99 */
    v9938_write(v, 1, 0x80u | (r & 0x3Fu));
}

static void dispatch_cmd(v9938_t *v, uint8_t cmd_code, uint16_t sx, uint16_t sy,
                         uint16_t dx, uint16_t dy, uint16_t nx, uint16_t ny,
                         uint8_t col, uint8_t arg)
{
    write_reg(v, 17, 32); /* indirect auto-increment from R#32 */

    v9938_write(v, 3, (uint8_t)(sx & 0xFF));
    v9938_write(v, 3, (uint8_t)((sx >> 8) & 0x01));
    v9938_write(v, 3, (uint8_t)(sy & 0xFF));
    v9938_write(v, 3, (uint8_t)((sy >> 8) & 0x03));
    v9938_write(v, 3, (uint8_t)(dx & 0xFF));
    v9938_write(v, 3, (uint8_t)((dx >> 8) & 0x01));
    v9938_write(v, 3, (uint8_t)(dy & 0xFF));
    v9938_write(v, 3, (uint8_t)((dy >> 8) & 0x03));
    v9938_write(v, 3, (uint8_t)(nx & 0xFF));
    v9938_write(v, 3, (uint8_t)((nx >> 8) & 0x03));
    v9938_write(v, 3, (uint8_t)(ny & 0xFF));
    v9938_write(v, 3, (uint8_t)((ny >> 8) & 0x03));
    v9938_write(v, 3, col);
    v9938_write(v, 3, arg);
    v9938_write(v, 3, cmd_code);
}

int main(void)
{
    printf("=== Test: V9938 Hardware Double-Buffering & Page Flipping ===\n");

    v9938_t v;
    memset(&v, 0, sizeof(v));
    v9938_init(&v, 1); /* 1 = V9958 */

    /* Setup 16-color palette */
    for (uint8_t i = 0; i < 16; i++) {
        v.pal[i] = (uint16_t)i * 0x1000u; /* distinct color tags in RGB565 */
    }

    /* Initialize Screen 7 (Mode 0x14: 512x212 4bpp, 256 bytes per scanline) */
    write_reg(&v, 0, 0x0A); /* M5=1, M4=0, M3=1 */
    write_reg(&v, 1, 0x62); /* Screen on, 16x16 sprites */
    write_reg(&v, 2, 0x1F); /* Page 0 Base address 0x00000 */
    write_reg(&v, 8, 0x08); /* 128K VRAM */
    write_reg(&v, 9, 0x80); /* 212 lines */

    uint8_t mode = v9938_mode(&v);
    if (mode != 0x14) {
        TEST_FAIL("Screen 7 initialization");
    }
    TEST_PASS("Screen 7 mode verification (Mode 0x14)");

    /* ── Step 1: Fill Page 0 with 0x88 (Gray) and Page 1 with 0x00 (Zero) ── */
    dispatch_cmd(&v, 0xC0, 0, 0, 0, 0, 512, 212, 0x88, 0x00);
    dispatch_cmd(&v, 0xC0, 0, 0, 0, 256, 512, 212, 0x00, 0x00);

    /* Verify Page 0 is 0x88 and Page 1 is 0x00 */
    uint32_t line_pitch = 256u;
    for (uint32_t y = 0; y < 212; y++) {
        for (uint32_t x = 0; x < 256; x++) {
            if (v.vram[y * line_pitch + x] != 0x88) {
                TEST_FAIL("Page 0 initial fill accuracy");
            }
            if (v.vram[(y + 256) * line_pitch + x] != 0x00) {
                TEST_FAIL("Page 1 initial fill accuracy");
            }
        }
    }
    TEST_PASS("Page 0 and Page 1 independent VRAM regions initialized");

    /* ── Step 2: Draw on Page 0 (active display) ─────────────────────────── */
    /* Draw 40x20 rect with 0x22 (Title Blue) at DX=100 (byte 50), DY=20 */
    dispatch_cmd(&v, 0xC0, 0, 0, 100, 20, 40, 20, 0x22, 0x00);

    /* Verify Page 0 has 0x22, Page 1 still has 0x00 */
    for (uint32_t y = 20; y < 40; y++) {
        for (uint32_t x = 50; x < 70; x++) {
            if (v.vram[y * line_pitch + x] != 0x22) {
                TEST_FAIL("Page 0 drawing check");
            }
            if (v.vram[(y + 256) * line_pitch + x] != 0x00) {
                TEST_FAIL("Page 1 isolation breach during Page 0 draw");
            }
        }
    }
    TEST_PASS("Page 0 drawn without mutating Page 1");

    /* Render display to framebuffer */
    uint16_t fb[256 * 212];
    memset(fb, 0, sizeof(fb));
    v9938_render(&v, fb);
    /* In Screen 7, byte 50 has 2 pixels. Left pixel is 0x2. Check fb at (50, 20) */
    if (fb[20 * 256 + 50] != v.pal[2]) {
        TEST_FAIL("Page 0 display render mismatch");
    }
    TEST_PASS("Display renders Page 0 contents when R#2 = 0x1F");

    /* ── Step 3: Draw on Page 1 (offscreen buffer) ────────────────────────── */
    /* Draw 40x20 rect with 0x33 (Dark Red) at DX=100 (byte 50), DY=256+20 */
    dispatch_cmd(&v, 0xC0, 0, 0, 100, 256 + 20, 40, 20, 0x33, 0x00);

    /* Verify Page 0 still has 0x22 (unchanged) */
    for (uint32_t y = 20; y < 40; y++) {
        for (uint32_t x = 50; x < 70; x++) {
            if (v.vram[y * line_pitch + x] != 0x22) {
                TEST_FAIL("Page 0 isolation breach during Page 1 draw");
            }
            if (v.vram[(y + 256) * line_pitch + x] != 0x33) {
                TEST_FAIL("Page 1 draw check");
            }
        }
    }
    TEST_PASS("Offscreen Page 1 drawn without disturbing visible Page 0");

    /* While R#2 is still 0x1F, re-render and ensure display still shows Page 0 (0x22) */
    v9938_render(&v, fb);
    if (fb[20 * 256 + 50] != v.pal[2]) {
        TEST_FAIL("Visible page leaked offscreen Page 1 drawing");
    }
    TEST_PASS("Visible page completely immune to background Page 1 rendering");

    /* ── Step 4: Atomic Page Flip via R#2 ─────────────────────────────────── */
    /* Flip display to Page 1 (R#2 = 0x3F) */
    write_reg(&v, 2, 0x3F);

    /* Render display to framebuffer */
    v9938_render(&v, fb);
    if (fb[20 * 256 + 50] != v.pal[3]) {
        fprintf(stderr, "FB pixel = 0x%04X, expected 0x%04X\n", fb[20 * 256 + 50], v.pal[3]);
        TEST_FAIL("Page 1 display render mismatch after flip");
    }
    TEST_PASS("Zero-flicker Page Flip verified: R#2=0x3F instantly switches visible display");

    /* ── Step 5: Hardware Page Synchronization (HMMM BitBlt) ─────────────── */
    /* Blit Page 1 contents back to Page 0 so both pages are in sync */
    dispatch_cmd(&v, 0xD0, 0, 256, 0, 0, 512, 212, 0x00, 0x00);

    /* Verify Page 0 now has 0x33 at the test rect */
    for (uint32_t y = 20; y < 40; y++) {
        for (uint32_t x = 50; x < 70; x++) {
            if (v.vram[y * line_pitch + x] != 0x33) {
                TEST_FAIL("Page sync BitBlt mismatch");
            }
        }
    }
    TEST_PASS("HMMM hardware page synchronization verified");

    printf("=== All V9938 Page Flip Tests PASSED ===\n\n");
    return 0;
}
