/*
 * ihx2rom.c — Portable Intel HEX to MSX MegaROM binary converter.
 * Compiles on macOS, FreeBSD, OpenBSD, NetBSD, and Linux.
 * Part of 8-bit BTRON for MSX.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define DEFAULT_ROM_SIZE (1024 * 1024) /* 1 MiB (64 x 16KB banks) */
#define ROM_BASE_ADDR    0x4000u

static uint8_t s_rom_buf[DEFAULT_ROM_SIZE];

static int parse_hex_byte(const char *s)
{
    char buf[3];
    buf[0] = s[0];
    buf[1] = s[1];
    buf[2] = '\0';
    return (int)strtoul(buf, NULL, 16);
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.ihx> <output.rom> [rom_size_kb]\n", argv[0]);
        return 1;
    }

    const char *in_path = argv[1];
    const char *out_path = argv[2];
    size_t rom_size = DEFAULT_ROM_SIZE;

    if (argc >= 4) {
        int kb = atoi(argv[3]);
        if (kb > 0) rom_size = (size_t)kb * 1024;
    }

    FILE *fin = fopen(in_path, "r");
    if (!fin) {
        fprintf(stderr, "Error: cannot open input file '%s'\n", in_path);
        return 1;
    }

    /* Initialize with 0xFF (standard flash/EPROM unprogrammed value) */
    memset(s_rom_buf, 0xFF, sizeof(s_rom_buf));

    char line[512];
    uint32_t ext_addr = 0;
    uint32_t min_addr = 0xFFFFFFFFu;
    uint32_t max_addr = 0;

    while (fgets(line, sizeof(line), fin)) {
        char *p = line;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (*p != ':') continue;

        size_t len = strlen(p);
        if (len < 11) continue;

        int count = parse_hex_byte(p + 1);
        int addr_hi = parse_hex_byte(p + 3);
        int addr_lo = parse_hex_byte(p + 5);
        int rectype = parse_hex_byte(p + 7);
        uint16_t addr16 = (uint16_t)((addr_hi << 8) | addr_lo);

        if (rectype == 0x00) {
            /* Data record */
            uint32_t full_addr = ext_addr + addr16;
            /* In MSX 64KB CPU space: 0x4000..0xBFFF is ROM (Page 1 & 2),
             * 0xC000..0xFFFF is system RAM. Ignore records targeting RAM. */
            if (ext_addr == 0 && full_addr >= 0xC000u) {
                continue;
            }
            /* If compiled with base 0x4000, map to offset 0 */
            uint32_t offset = full_addr;
            if (offset >= ROM_BASE_ADDR && ext_addr == 0) {
                offset -= ROM_BASE_ADDR;
            }

            for (int i = 0; i < count; i++) {
                int byte_val = parse_hex_byte(p + 9 + i * 2);
                if (offset + i < sizeof(s_rom_buf)) {
                    s_rom_buf[offset + i] = (uint8_t)byte_val;
                    if (offset + i < min_addr) min_addr = offset + i;
                    if (offset + i > max_addr) max_addr = offset + i;
                }
            }
        } else if (rectype == 0x01) {
            /* EOF record */
            break;
        } else if (rectype == 0x02) {
            /* Extended Segment Address */
            int seg_hi = parse_hex_byte(p + 9);
            int seg_lo = parse_hex_byte(p + 11);
            ext_addr = (uint32_t)(((seg_hi << 8) | seg_lo) << 4);
        } else if (rectype == 0x04) {
            /* Extended Linear Address */
            int lin_hi = parse_hex_byte(p + 9);
            int lin_lo = parse_hex_byte(p + 11);
            ext_addr = (uint32_t)(((lin_hi << 8) | lin_lo) << 16);
        }
    }

    fclose(fin);

    /* Verify standard MSX ROM header signature "AB" at offset 0 */
    if (s_rom_buf[0] != 0x41 || s_rom_buf[1] != 0x42) {
        fprintf(stderr, "Warning: MSX ROM header signature 'AB' (0x41, 0x42) not found at offset 0.\n");
        fprintf(stderr, "Injecting standard 'AB' header...\n");
        s_rom_buf[0] = 0x41; /* 'A' */
        s_rom_buf[1] = 0x42; /* 'B' */
        if (s_rom_buf[2] == 0xFF && s_rom_buf[3] == 0xFF) {
            /* Default init entry to 0x4010 */
            s_rom_buf[2] = 0x10;
            s_rom_buf[3] = 0x40;
        }
    }

    FILE *fout = fopen(out_path, "wb");
    if (!fout) {
        fprintf(stderr, "Error: cannot create output file '%s'\n", out_path);
        return 1;
    }

    size_t written = fwrite(s_rom_buf, 1, rom_size, fout);
    fclose(fout);

    if (written != rom_size) {
        fprintf(stderr, "Error: failed to write complete ROM image\n");
        return 1;
    }

    printf("Successfully built MSX MegaROM: '%s' (%zu KiB, data range 0x%05X-0x%05X)\n",
           out_path, rom_size / 1024, min_addr, max_addr);
    return 0;
}
