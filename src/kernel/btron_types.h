/*
 * btron_types.h — Canonical TRON Architecture Types for 8-bit MSX BTRON.
 * Complies with JPL Power of Ten coding rules: bounded sizes, explicit widths.
 */

#ifndef _BTRON_TYPES_H_
#define _BTRON_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TRON Standard Primitive Types */
typedef int8_t              B;      /* Signed 8-bit integer */
typedef uint8_t             UB;     /* Unsigned 8-bit integer */
typedef int16_t             H;      /* Signed 16-bit half-word */
typedef uint16_t            UH;     /* Unsigned 16-bit half-word */
typedef int32_t             W;      /* Signed 32-bit word */
typedef uint32_t            UW;     /* Unsigned 32-bit word */

typedef uint16_t            TC;     /* TRON Character Code (16-bit) */
typedef int16_t             ER;     /* Error code */
typedef int16_t             ID;     /* Object / Task / Window ID */
typedef int8_t              PRI;    /* Task Priority */
typedef uint8_t             BOOL;   /* Boolean (TRUE / FALSE) */

#ifndef TRUE
#define TRUE                1
#endif
#ifndef FALSE
#define FALSE               0
#endif

#ifndef NULL
#define NULL                ((void *)0)
#endif

/* TRON Canonical Error Codes */
#define E_OK                0       /* Normal completion */
#define E_SYS               (-5)    /* System error */
#define E_NOMEM             (-10)   /* Insufficient memory */
#define E_NOSPT             (-17)   /* Feature not supported */
#define E_PAR               (-20)   /* Parameter error */
#define E_ID                (-33)   /* Invalid ID number */
#define E_LIMIT             (-34)   /* System limit exceeded */
#define E_NOEXS             (-52)   /* Object does not exist */

/* Static Bounded Architecture Limits (JPL Rule 2: strictly bounded) */
#define MAX_TASKS           6
#define MAX_WINDOWS         6
#define MAX_REAL_BODIES     16
#define MAX_MENU_ITEMS      8
#define MAX_TAD_SEGMENTS    64

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_TYPES_H_ */

