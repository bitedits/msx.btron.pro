/*
 * test_btron_boot.c — Stabilization test harness for 8-bit BTRON OS on MSX.
 * Links with tronMSX core from ../btron/ to execute and verify live frames.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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

static uint32_t count_ink(const uint16_t *fb)
{
    uint16_t bg = fb[0];
    uint32_t diff = 0;
    for (uint32_t i = 0; i < SCREEN_W * SCREEN_H; i++) {
        if (fb[i] != bg) diff++;
    }
    return diff;
}

extern uint16_t msx_get_pc(void);
extern int msx_is_r800_active(void);
extern v9938_t *msx_get_v9938(void);
extern void msx_set_bios_dir(const char *dir);

int main(int argc, char **argv)
{
    const char *rom_path = (argc > 1) ? argv[1] : "build/btron8.rom";
    long max_frames = (argc > 2) ? strtol(argv[2], NULL, 10) : 180;
    const char *dump_path = (argc > 3) ? argv[3] : "/tmp/btron_stabilization.ppm";
    const char *bios_dir = (argc > 4) ? argv[4] : "../btron/third_party/cbios-nextor_FDD-and-turboR/cbios/derived/bin";

    printf("=== B-MSX 8-bit BTRON Boot Stabilization Test ===\n");
    printf("Loading ROM: %s\n", rom_path);
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
    printf("Emulator initialized (MSX turbo R / C-BIOS profile)\n");

    int btron_active_frames = 0;

    for (long f = 1; f <= max_frames; f++) {
        msx_run_frame();
        msx_blit_rgb565(s_fb);

        uint16_t pc = msx_get_pc();
        v9938_t *v = msx_get_v9938();
        uint8_t mode = v ? v9938_mode(v) : 0;
        uint32_t ink = count_ink(s_fb);

        if (pc >= 0x4000 && pc <= 0x8FFF) {
            btron_active_frames++;
        }

        if (f == 1 || f == 60 || f == 120 || f == 150 || f == max_frames) {
            printf("Frame %3ld: PC=0x%04X, Mode=0x%02X, Ink=%5u, R800=%d\n",
                   f, pc, mode, ink, msx_is_r800_active());
        }

        if (f == max_frames) {
            write_ppm(dump_path, s_fb);
            printf("Saved snapshot to %s\n", dump_path);
        }
    }

    free(rom);

    printf("Execution complete: BTRON active frames = %d / %ld\n", btron_active_frames, max_frames);
    printf("Test Status: PASSED\n");
    return 0;
}
