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
    /*
     * CHGCPU BIOS routine at 0x0180 (called via CALSLT 0x001C or inter-slot call):
     * A = 0x00 (Z80), 0x81 (R800 DRAM), 0x82 (R800 ROM)
     */
    (void)mode;
    __asm
    push af
    ld   iy, #0x0000        ;; Slot 0 (Main BIOS)
    ld   ix, #0x0180        ;; CHGCPU entry
    call 0x001C             ;; CALSLT
    pop  af
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

void msx_wait_vblank(void)
{
    /* Read VDP Status Register 0 (bit 7 is VBLANK interrupt flag) */
    /* Set VDP register pointer to S#0: write 0 to reg 15 */
    msx_out(PORT_VDP_REG, 0x00);
    msx_out(PORT_VDP_REG, 0x80 | 15);

    /* Wait until bit 7 is set */
    uint16_t timeout = 50000;
    while (timeout--) {
        uint8_t s0 = msx_in(PORT_VDP_REG);
        if (s0 & 0x80) {
            break;
        }
    }
}

