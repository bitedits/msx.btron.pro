/*
 * msxgl_config.h — Configuration for MSXgl minimal subset for 8-bit BTRON.
 * Target: MSX turbo R / MSX2+ with V9958 VDP, R800 CPU, ASCII16 MegaROM.
 */

#ifndef _MSXGL_CONFIG_H_
#define _MSXGL_CONFIG_H_

#define MSXGL_TARGET_TURBO_R    1
#define MSXGL_USE_R800          1
#define MSXGL_USE_V9958         1
#define MSXGL_USE_SCREEN7       1   /* 512x212, 16 colors per pixel (4bpp) */
#define MSXGL_USE_ASCII16       1   /* 1024 KiB ASCII16 MegaROM */

#define SCREEN_WIDTH            512
#define SCREEN_HEIGHT           212
#define SCREEN_BPP              4
#define SCREEN_VRAM_PAGES       2

/* I/O Port Definitions for MSX / V9958 / R800 */
#define PORT_VDP_DATA           0x98
#define PORT_VDP_REG            0x99
#define PORT_VDP_PALETTE        0x9A
#define PORT_VDP_INDIR          0x9B

#define PORT_PSG_REG            0xA0
#define PORT_PSG_WRITE          0xA1
#define PORT_PSG_READ           0xA2

#define PORT_PPI_PA             0xA8    /* Primary Slot Register */
#define PORT_PPI_PB             0xA9    /* Keyboard Row Read */
#define PORT_PPI_PC             0xAA    /* Keyboard Column / Cassette / Caps */

#define PORT_KANJI_ADDR_LO      0xD8    /* Kanji ROM Address Low */
#define PORT_KANJI_ADDR_HI      0xD9    /* Kanji ROM Address High */
#define PORT_KANJI_DATA_L1      0xDA    /* Kanji ROM Level 1 Data */
#define PORT_KANJI_DATA_L2      0xDB    /* Kanji ROM Level 2 Data */

#define PORT_CHGCPU             0x0180  /* BIOS Vector for CPU mode select */

/* MegaROM Bank Switch addresses (ASCII16) */
#define ASCII16_BANK1_REG       0x6000
#define ASCII16_BANK2_REG       0x7000

#endif /* _MSXGL_CONFIG_H_ */

