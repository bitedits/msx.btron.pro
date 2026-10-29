/*
 * test_feedback_harness.c — End-to-End Automated Feedback Loop Harness for B-MSX OS.
 *
 * Verifies:
 *   1. Proportional 1:1 Host Mouse -> MSX Screen 7 Coordinate Mapping (0 px error)
 *   2. Zero-Flicker Click Handling (active window no-op click, inactive window promotion)
 *   3. Hardware Coprocessor (HMMM) Window Capture & Dragging (Hold-to-Drag + Sticky Capture)
 *   4. Continuous Frame-by-Frame Ink & Blank/Flash Detector (0 flicker frames across entire run)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "../../../btron/include/emulators/tronmsx.h"
#include "../../../btron/src/emulators/msx/v9938.h"

#define SCREEN_W 256u
#define SCREEN_H 212u

static uint16_t s_fb[SCREEN_W * SCREEN_H];
static uint16_t s_prev_fb[SCREEN_W * SCREEN_H];

extern uint16_t msx_get_pc(void);
extern v9938_t *msx_get_v9938(void);
extern void msx_set_bios_dir(const char *dir);

static uint8_t *read_file(const char *path, uint32_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);
    if (n <= 0) { fclose(f); return NULL; }
    uint8_t *buf = (uint8_t *)malloc((size_t)n);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) { free(buf); fclose(f); return NULL; }
    fclose(f);
    *out_len = (uint32_t)n;
    return buf;
}

static void write_ppm(const char *path, const uint16_t *fb)
{
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%u %u\n255\n", SCREEN_W, SCREEN_H);
    for (uint32_t i = 0; i < SCREEN_W * SCREEN_H; i++) {
        uint16_t p = fb[i];
        uint8_t r = (uint8_t)(((p >> 11) & 0x1Fu) << 3);
        uint8_t g = (uint8_t)(((p >> 5)  & 0x3Fu) << 2);
        uint8_t b = (uint8_t)(( p        & 0x1Fu) << 3);
        fputc(r, f); fputc(g, f); fputc(b, f);
    }
    fclose(f);
}

/* Count non-backdrop pixels to detect any blank/gray flash frames */
static uint32_t count_ink(const uint16_t *fb)
{
    v9938_t *v = msx_get_v9938();
    uint16_t bg = v ? v->pal[8] : fb[0]; /* BTRON Light Gray = palette 8 */
    uint32_t ink = 0;
    for (uint32_t i = 0; i < SCREEN_W * SCREEN_H; i++) {
        if (fb[i] != bg) ink++;
    }
    return ink;
}

/* Read a 4-bit Screen 7 pixel from Page 0 VRAM */
static uint8_t read_page0_pixel(uint16_t x, uint16_t y)
{
    v9938_t *v = msx_get_v9938();
    uint32_t addr = (uint32_t)y * 256u + (uint32_t)(x >> 1);
    uint8_t b = v->vram[addr & (V9938_VRAM_SIZE - 1u)];
    return (x & 1u) ? (uint8_t)(b & 0x0Fu) : (uint8_t)(b >> 4);
}

static uint32_t s_total_monitored_frames = 0;
static uint32_t s_flicker_frames = 0;
static uint32_t s_min_ink = 0xFFFFFFFFu;
static uint32_t s_max_ink = 0u;

static void step_and_monitor(int frames, const char *phase_label)
{
    for (int i = 0; i < frames; i++) {
        msx_run_frame();
        msx_blit_rgb565(s_fb);
        uint32_t ink = count_ink(s_fb);
        s_total_monitored_frames++;
        if (ink < s_min_ink) s_min_ink = ink;
        if (ink > s_max_ink) s_max_ink = ink;

        /* A desktop with 3 open windows always has > 25,000 non-gray pixels.
         * If ink ever drops below 20,000, a screen clear or partial redraw leaked to display! */
        if (ink < 20000u) {
            s_flicker_frames++;
            fprintf(stderr, "  [FLICKER DETECTED] Phase '%s' frame %d: ink dropped to %u!\n",
                    phase_label, i, ink);
        }
    }
}

int main(int argc, char **argv)
{
    const char *rom_path = (argc > 1) ? argv[1] : "build/btron8.rom";
    const char *bios_dir = (argc > 2) ? argv[2] : "../btron/third_party/cbios-nextor_FDD-and-turboR/cbios/derived/bin";

    printf("=== B-MSX Automated Feedback Loop & Zero-Flicker Harness ===\n");
    msx_set_bios_dir(bios_dir);

    uint32_t rom_len = 0;
    uint8_t *rom = read_file(rom_path, &rom_len);
    if (!rom || rom_len != 1024 * 1024) {
        fprintf(stderr, "FAIL: Could not load 1024 KiB ROM from %s\n", rom_path);
        return 1;
    }

    msx_cfg_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.machine = BMSX_MACHINE_MSXTR_CBIOS;
    cfg.frame_hz = 60;

    if (msx_init(&cfg) != MSX_OK || msx_load_rom(rom, rom_len, 0x4000u) != MSX_OK) {
        fprintf(stderr, "FAIL: Emulator initialization failed\n");
        free(rom);
        return 1;
    }
    msx_reset();

    /* 1. Boot until main event loop is reached and initial desktop is presented on Page 0 */
    v9938_t *v = msx_get_v9938();
    uint32_t sat_addr = 0x1FA00u;
    int boot_frames = 0;
    for (int f = 1; f <= 450; f++) {
        msx_run_frame();
        msx_blit_rgb565(s_fb);
        if (v->vram[sat_addr] == 106 && v->vram[sat_addr + 1] == 128 &&
            read_page0_pixel(470, 20) == 2) {
            boot_frames = f;
            break;
        }
    }
    assert(boot_frames > 0);
    /* Run 2 settle frames and begin strict frame-by-frame flicker monitoring */
    step_and_monitor(2, "post-boot-settle");
    printf("  [PASS] 1. Desktop booted in %d frames (Initial ink = %u px, R#2 = 0x%02X)\n",
           boot_frames, count_ink(s_fb), v->regs[2]);

    /* 2. Test 1:1 Proportional Host Mouse Coordinate Mapping */
    static const struct { uint16_t x; uint16_t y; } mouse_pts[] = {
        {  60,  40 },
        { 180,  22 },
        { 350,  20 },
        { 420,  90 },
        { 120, 150 },
        { 256, 106 }
    };

    for (size_t i = 0; i < sizeof(mouse_pts) / sizeof(mouse_pts[0]); i++) {
        msx_mouse_set(mouse_pts[i].x, mouse_pts[i].y, 0);
        step_and_monitor(2, "proportional-mouse-sweep");
        uint8_t sy = v->vram[sat_addr];
        uint8_t sx = v->vram[sat_addr + 1];
        uint8_t term_y = v->vram[sat_addr + 4];
        if (sy != mouse_pts[i].y || sx != (uint8_t)(mouse_pts[i].x >> 1)) {
            fprintf(stderr, "FAIL: Mouse proportional mismatch at (%u, %u): SAT=(%u, %u)\n",
                    mouse_pts[i].x, mouse_pts[i].y, sx, sy);
            free(rom);
            return 1;
        }
        assert(term_y == 216); /* Sprite 1 terminator must remain 216 */
    }
    printf("  [PASS] 2. Proportional 1:1 Host->MSX Mouse Mapping verified (0 px error across 6 points)\n");

    /* 3. Test Zero-Flicker Click Handling */
    /* 3a: Click inside already-active Shell Terminal content area (400, 60) -> should not redraw */
    msx_mouse_set(400, 60, 0);
    step_and_monitor(2, "move-to-active-term");
    memcpy(s_prev_fb, s_fb, sizeof(s_fb));

    msx_mouse_set(400, 60, 1); /* Left button down */
    step_and_monitor(3, "click-active-term-down");
    msx_mouse_set(400, 60, 0); /* Left button up */
    step_and_monitor(3, "click-active-term-up");

    if (memcmp(s_prev_fb, s_fb, sizeof(s_fb)) != 0) {
        fprintf(stderr, "FAIL: Clicking inside already-active Terminal caused unnecessary redraw!\n");
        free(rom);
        return 1;
    }
    printf("  [PASS] 3a. Clicking inside already-active window caused 0 redundant redraws\n");

    /* 3b: Click inactive Window 1 (Editor) at (160, 80) to promote it -> zero-flicker present */
    msx_mouse_set(160, 80, 0);
    step_and_monitor(2, "move-to-inactive-editor");
    msx_mouse_set(160, 80, 1);
    step_and_monitor(5, "click-promote-editor-down");
    msx_mouse_set(160, 80, 0);
    step_and_monitor(2, "click-promote-editor-up");

    /* Verify Window 1 title bar at (200, 20) is now active BTRON_COL_TITLE_BLUE (2) */
    uint8_t w1_title_col = read_page0_pixel(200, 20);
    if (w1_title_col != 2) {
        fprintf(stderr, "FAIL: Window 1 title bar pixel at (200, 20) is %u (expected 2 = TITLE_BLUE), W2=(470,20)=%u, Page1_W1=%u, PC=0x%04X\n",
                w1_title_col, read_page0_pixel(470, 20),
                (v->vram[(256 + 20) * 256 + 100] >> 4), msx_get_pc());
        free(rom);
        return 1;
    }
    printf("  [PASS] 3b. Clicking inactive Editor promoted focus with 0 flicker\n");

    /* 4. Test Window Capture & Hardware Coprocessor (HMMM) Dragging */
    /* 4a: Hold-to-Drag Window 2 (Shell Terminal, currently at x=342, y=16, w=164, h=100) */
    /* Grab Window 2 title bar at (380, 22) */
    msx_mouse_set(380, 22, 0);
    step_and_monitor(2, "move-to-term-titlebar");
    msx_mouse_set(380, 22, 1); /* Press and hold on title bar */
    step_and_monitor(6, "grab-term-titlebar");

    /* Verify Window 2 is now active (title bar at (470, 20) is color 2) */
    assert(read_page0_pixel(470, 20) == 2);

    /* Drag Window 2 smoothly by (-120, +50) across 10 steps -> new win pos = (222, 66) */
    for (int step = 1; step <= 10; step++) {
        uint16_t mx = (uint16_t)(380 - step * 12);
        uint16_t my = (uint16_t)(22 + step * 5);
        msx_mouse_set(mx, my, 1);
        step_and_monitor(2, "hold-drag-step");
    }
    /* Release mouse button at (260, 72) */
    msx_mouse_set(260, 72, 0);
    step_and_monitor(3, "hold-drag-release");

    /* Verify Window 2 title bar moved to (222..386, 66..77) and old area (470, 20) restored to gray (8) */
    uint8_t moved_title_col = read_page0_pixel(350, 70);
    uint8_t restored_bg_col = read_page0_pixel(470, 20);
    if (moved_title_col != 2 || restored_bg_col != 8) {
        fprintf(stderr, "FAIL: Hold-to-Drag pixel check failed: moved_title=%u (exp 2), restored_bg=%u (exp 8)\n",
                moved_title_col, restored_bg_col);
        free(rom);
        return 1;
    }
    printf("  [PASS] 4a. Hold-to-Drag moved Terminal to (222, 66) via HMMM BitBlt & restored underlay\n");

    /* 4b: Sticky Click-to-Capture / Click-to-Drop (click title bar without moving, then move unpressed) */
    msx_mouse_set(260, 72, 1); /* Click down on Window 2 title bar at (260, 72) */
    step_and_monitor(4, "sticky-capture-click-down");
    msx_mouse_set(260, 72, 0); /* Release immediately without moving -> engages sticky capture! */
    step_and_monitor(3, "sticky-capture-click-up");

    /* Now move mouse with buttons=0 (unpressed!) by (+60, -30) to (320, 42) -> win moves to (282, 36) */
    for (int step = 1; step <= 5; step++) {
        uint16_t mx = (uint16_t)(260 + step * 12);
        uint16_t my = (uint16_t)(72 - step * 6);
        msx_mouse_set(mx, my, 0);
        step_and_monitor(2, "sticky-capture-move");
    }

    /* Click once to drop the captured window at (282, 36) */
    msx_mouse_set(320, 42, 1);
    step_and_monitor(2, "sticky-drop-down");
    msx_mouse_set(320, 42, 0);
    step_and_monitor(2, "sticky-drop-up");

    /* Move mouse away to (100, 100) and verify window stayed dropped at (282, 36) */
    msx_mouse_set(100, 100, 0);
    step_and_monitor(3, "post-drop-move-away");

    uint8_t dropped_title_col = read_page0_pixel(410, 40);
    if (dropped_title_col != 2) {
        fprintf(stderr, "FAIL: Sticky-captured window did not land at (282, 36): col=%u (exp 2)\n",
                dropped_title_col);
        free(rom);
        return 1;
    }
    printf("  [PASS] 4b. Sticky Click-to-Capture & Click-to-Drop moved Terminal to (282, 36)\n");

    /* 5. Test Zero-CPU Click Capture on Occluded Window & Fast Menu Popup (<= 3 frames!) */
    /* 5a: Window 2 (282, 36) currently overlaps Window 1 (6, 16, 330, 188).
     * Click Window 1 at (100, 160) and verify it promotes to front in <= 3 frames! */
    msx_mouse_set(100, 160, 1);
    step_and_monitor(3, "fast-swap-occluded-editor-down");

    /* Verify Window 1 title bar at (200, 20) is immediately active blue (2) and overlap area (290, 90) is white paper (9) */
    if (read_page0_pixel(200, 20) != 2 || read_page0_pixel(290, 90) != 9) {
        fprintf(stderr, "FAIL: Fast HMMM swap on occluded Window 1 exceeded 3-frame budget: title=%u, overlap=%u\n",
                read_page0_pixel(200, 20), read_page0_pixel(290, 90));
        free(rom);
        return 1;
    }
    msx_mouse_set(100, 160, 0);
    step_and_monitor(4, "fast-swap-occluded-editor-up");

    msx_mouse_set(410, 80, 1);
    step_and_monitor(3, "fast-swap-back-to-term-down");
    if (read_page0_pixel(410, 40) != 2) {
        v9938_t *v = msx_get_v9938();
        fprintf(stderr, "FAIL: Fast HMMM swap back to Window 2: title(410,40)=%u, w1_title(200,20)=%u, sx=%u, sy=%u, pc=0x%04X\n",
                read_page0_pixel(410, 40), read_page0_pixel(200, 20),
                v->vram[0x1FA01], v->vram[0x1FA00], msx_get_pc());
        free(rom);
        return 1;
    }
    msx_mouse_set(410, 80, 0);
    step_and_monitor(3, "fast-swap-back-to-term-up");
    printf("  [PASS] 5a. Zero-stall hardware HMMM focus swap on overlapping windows completed in <= 3 frames\n");

    /* 5b: Fast Menu Popup Open & Close in <= 3 frames without desktop redraw */
    msx_mouse_set(130, 6, 1); /* Click "Edit" menu (x=116..160) */
    step_and_monitor(3, "fast-menu-open-down");
    if (read_page0_pixel(200, 50) != 8) { /* Popup body is BTRON_COL_LIGHT_GRAY (8) over Editor's white paper */
        fprintf(stderr, "FAIL: Menu popup did not open in <= 3 frames: pixel(200,50)=%u, pc=0x%04X\n",
                read_page0_pixel(200, 50), msx_get_pc());
        free(rom);
        return 1;
    }
    msx_mouse_set(130, 6, 0);
    step_and_monitor(3, "fast-menu-open-up");

    msx_mouse_set(130, 6, 1);
    step_and_monitor(3, "fast-menu-close-down");
    if (read_page0_pixel(200, 50) != 9) { /* Restored Editor white paper (9) via HMMM */
        fprintf(stderr, "FAIL: Menu popup close did not restore background in <= 3 frames: pixel(200,50)=%u\n",
                read_page0_pixel(200, 50));
        free(rom);
        return 1;
    }
    msx_mouse_set(130, 6, 0);
    step_and_monitor(2, "fast-menu-close-up");
    printf("  [PASS] 5b. Fast menu popup open/close via HMMM scratch completed in <= 3 frames\n");

    write_ppm("/tmp/btron_feedback_harness.ppm", s_fb);

    printf("\n=== Feedback Loop Telemetry Summary ===\n");
    printf("  Monitored Active Frames : %u\n", s_total_monitored_frames);
    printf("  Flicker / Blank Frames  : %u (Target: 0)\n", s_flicker_frames);
    printf("  Frame Ink Range         : %u .. %u px\n", s_min_ink, s_max_ink);
    printf("  Snapshot Saved          : /tmp/btron_feedback_harness.ppm\n");

    assert(s_flicker_frames == 0);
    free(rom);
    printf("=== All Feedback Loop Harness Checks PASSED ===\n\n");
    return 0;
}

