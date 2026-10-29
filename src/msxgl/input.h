/*
 * input.h — Keyboard and mouse/pointer polling for MSX.
 */

#ifndef _MSXGL_INPUT_H_
#define _MSXGL_INPUT_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t x;             /* Pointer X (0..511) */
    uint8_t  y;             /* Pointer Y (0..211) */
    bool     btn_left;      /* Primary click button */
    bool     btn_right;     /* Secondary click button */
    bool     btn_prev_left; /* Previous state for click edge detect */
} PointerState;

typedef struct {
    bool up;
    bool down;
    bool left;
    bool right;
    bool space;
    bool enter;
    bool esc;
    bool tab;
    char ascii_char;        /* Last typed ASCII key if any */
} KeyState;

void input_init(void);
void input_poll(void);
const PointerState *input_get_pointer(void);
const KeyState *input_get_keys(void);
bool input_is_clicked(void);

#ifdef __cplusplus
}
#endif

#endif /* _MSXGL_INPUT_H_ */

