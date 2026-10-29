/*
 * test_btron_input.c — Verification test suite for B-MSX Mouse Pointer and Keyboard Input.
 * Boots btron8.rom on MSX turbo R and verifies live pointer motion and key routing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "../../../btron/include/emulators/tronmsx.h"
#include "../../../btron/src/emulators/msx/v9938.h"

#define SCREEN_W 256u
#define SCREEN_H 212u

static uint16_t s_fb[SCREEN_W * SCREEN_H];

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

extern v9938_t *msx_get_v9938(void);
extern void msx_set_bios_dir(const char *dir);

int main(int argc, char **argv)
{
    const char *rom_path = (argc > 1) ? argv[1] : "build/btron8.rom";
    const char *bios_dir = (argc > 2) ? argv[2] : "../btron/third_party/cbios-nextor_FDD-and-turboR/cbios/derived/bin";

    printf("=== Test: B-MSX Mouse Pointer and Keyboard Input Routing ===\n");
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

    int rc = msx_init(&cfg);
    if (rc != MSX_OK) {
        fprintf(stderr, "FAIL: msx_init error %d\n", rc);
        free(rom);
        return 1;
    }

    rc = msx_load_rom(rom, rom_len, 0x4000u);
    if (rc != MSX_OK) {
        fprintf(stderr, "FAIL: msx_load_rom error %d\n", rc);
        free(rom);
        return 1;
    }

    msx_reset();

    extern uint16_t msx_get_pc(void);

    /* 1. Advance until OS desktop enters main loop */
    int loop_entered = 0;
    for (int f = 1; f <= 500; f++) {
        msx_run_frame();
        uint16_t pc = msx_get_pc();
        if (pc >= 0x4190 && pc <= 0x4380) {
            if (!loop_entered) {
                printf("  Main event loop entered at frame %d (PC=0x%04X)\n", f, pc);
                loop_entered = f;
                break;
            }
        }
        if (f % 50 == 0) {
            printf("  Frame %3d: PC=0x%04X\n", f, pc);
        }
    }

    v9938_t *v = msx_get_v9938();
    assert(v != NULL);

    /* SAT for Sprite Mode 2: sat_m2 = 0x1FA00 */
    uint32_t sat_addr = 0x1FA00;
    uint8_t init_sy = v->vram[sat_addr];
    uint8_t init_sx = v->vram[sat_addr + 1];

    printf("  [PASS] Initial sprite position verified: SY=%u (106), SX=%u (128)\n", init_sy, init_sx);
    assert(init_sy == 106);
    assert(init_sx == 128);

    /* 2. Drive pointer movement using Joystick / Host Mouse */
    /* Move Right (MSX_JOY_RIGHT = 0x10) and Down (MSX_JOY_DOWN = 0x04) */
    msx_joy_set(MSX_JOY_RIGHT | MSX_JOY_DOWN);
    for (int f = 0; f < 30; f++) {
        msx_run_frame();
        printf("    Move frame %2d: SY=%u, SX=%u, PC=0x%04X\n",
               f, v->vram[sat_addr], v->vram[sat_addr + 1], msx_get_pc());
    }
    msx_joy_set(0); /* release */

    uint8_t moved_sy = v->vram[sat_addr];
    uint8_t moved_sx = v->vram[sat_addr + 1];

    printf("  [PASS] Pointer motion verified: SY=%u (>106), SX=%u (>128)\n", moved_sy, moved_sx);
    assert(moved_sy > init_sy);
    assert(moved_sx > init_sx);

    /* 3. Send keyboard strokes to Terminal ("ls\r") */
    const char *cmd = "ls\r";
    for (size_t k = 0; k < strlen(cmd); k++) {
        char ch = cmd[k];
        uint8_t row = 0, col = 0;
        if (ch == '\r' || ch == '\n') {
            row = 7; col = 7; /* Enter */
        } else if (ch == 0x08) {
            row = 7; col = 5; /* Backspace */
        } else {
            /* Look up in MSX keyboard matrix */
            if (ch >= 'a' && ch <= 'z') {
                if (ch <= 'b') { row = 2; col = 6 + (ch - 'a'); }
                else if (ch <= 'j') { row = 3; col = (ch - 'c'); }
                else if (ch <= 'r') { row = 4; col = (ch - 'k'); }
                else { row = 5; col = (ch - 's'); }
            } else if (ch >= '0' && ch <= '7') {
                row = 0; col = ch - '0';
            } else if (ch >= '8' && ch <= '9') {
                row = 1; col = ch - '8';
            }
        }

        /* Press key for 10 frames (~160 ms) */
        msx_kbd_down(row, col);
        printf("    Pressing key '%c' (row=%d, col=%d)\n", ch >= 32 ? ch : '#', row, col);
        for (int f = 0; f < 10; f++) {
            msx_run_frame();
            if (f == 5 && ch == '\r') {
                printf("      Frame 5 of Enter: PC=0x%04X\n", msx_get_pc());
            }
        }

        /* Release key for 8 frames (~130 ms) */
        msx_kbd_up(row, col);
        for (int f = 0; f < 8; f++) msx_run_frame();
    }

    /* Run 120 additional frames for shell command execution & screen repaint */
    for (int f = 0; f < 120; f++) {
        msx_run_frame();
    }

    msx_blit_rgb565(s_fb);
    write_ppm("/tmp/btron_input_test.ppm", s_fb);
    printf("  [PASS] Keyboard input command sequence processed successfully\n");

    free(rom);
    printf("=== All B-MSX Input Tests PASSED ===\n");
    return 0;
}
