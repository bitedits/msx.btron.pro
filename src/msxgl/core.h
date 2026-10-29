/*
 * core.h — Hardware control, CPU speed, and I/O port primitives for MSX.
 * Compatible with SDCC z80/r800 targets on macOS and BSD.
 */

#ifndef _MSXGL_CORE_H_
#define _MSXGL_CORE_H_

#include <stdint.h>
#include <stdbool.h>
#include "msxgl_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Low-level I/O primitives */
uint8_t msx_in(uint8_t port);
void msx_out(uint8_t port, uint8_t val);
void msx_di(void);
void msx_ei(void);
void msx_halt(void);

/* Hardware detection & CPU speed control */
uint8_t msx_get_version(void);
bool msx_is_turbo_r(void);
void msx_set_cpu_mode(uint8_t mode);

#define CPU_MODE_Z80         0x00
#define CPU_MODE_R800_ROM     0x82
#define CPU_MODE_R800_DRAM    0x81

/* Delay & Synchronization */
void msx_delay(uint16_t cycles);
void msx_wait_vblank(void);

#ifdef __cplusplus
}
#endif

#endif /* _MSXGL_CORE_H_ */

