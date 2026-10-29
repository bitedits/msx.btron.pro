/*
 * core.c — MSX hardware control implementation for SDCC.
 */

#include "core.h"

uint8_t msx_in(uint8_t port) __naked
{
    (void)port;
    __asm
    ld   c, a
    in   a, (c)
    ret
    __endasm;
}

void msx_out(uint8_t port, uint8_t val) __naked
{
    (void)port;
    (void)val;
    __asm
    ld   c, a
    ld   a, l
    out  (c), a
    ret
    __endasm;
}

void msx_di(void) __naked
{
    __asm
    di
    ret
    __endasm;
}

void msx_ei(void) __naked
{
    __asm
    ei
    ret
    __endasm;
}

void msx_halt(void) __naked
{
    __asm
    halt
    ret
    __endasm;
}

uint8_t msx_get_version(void)
{
    /* MSX BIOS 0x002D holds MSX version: 0=MSX1, 1=MSX2, 2=MSX2+, 3=turbo R */
    volatile const uint8_t *rom_id = (volatile const uint8_t *)0x002D;
    return *rom_id;
}

bool msx_is_turbo_r(void)
{
    return (msx_get_version() == 3);
}

void msx_set_cpu_mode(uint8_t mode)
{
    if (!msx_is_turbo_r()) {
        return;
    }
    (void)mode;
    __asm
    ld   a, #0x82
    call 0x0180             ;; CHGCPU direct entry in Page 0
    __endasm;
}

void msx_delay(uint16_t cycles)
{
    while (cycles > 0) {
        __asm
        nop
        nop
        nop
        nop
        __endasm;
        cycles--;
    }
}

void msx_reboot(void) __naked
{
#ifndef __SDCC
    return;
#else
    __asm
    di
    jp   0x0000
    __endasm;
#endif
}

void msx_wait_vblank(void)
{
    /* Wait for VBlank using V9938 Status Register 2, Bit 6 (Vertical Retrace = ST_VR: 0x40) */
    msx_out(PORT_VDP_REG, 0x02);
    msx_out(PORT_VDP_REG, 0x80 | 15);

    /* 1. If currently in VBlank, wait for active display */
    uint16_t timeout = 2500;
    while (timeout--) {
        if (!(msx_in(PORT_VDP_REG) & 0x40)) break;
    }

    /* 2. Wait until VBlank starts */
    timeout = 2500;
    while (timeout--) {
        if (msx_in(PORT_VDP_REG) & 0x40) break;
    }

    /* Reset VDP status register pointer back to S#0 */
    msx_out(PORT_VDP_REG, 0x00);
    msx_out(PORT_VDP_REG, 0x80 | 15);
}

