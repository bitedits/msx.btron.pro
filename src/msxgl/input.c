/*
 * input.c — Keyboard matrix scanning and mouse pointer emulation for MSX.
 */

#include "input.h"
#include "core.h"
#include "vdp.h"

static PointerState s_pointer;
static KeyState s_keys;

static uint8_t scan_kbd_row(uint8_t row)
{
    /* Preserve upper nibble of PPI Port C, set lower nibble to row */
    uint8_t c = msx_in(PORT_PPI_PC) & 0xF0;
    msx_out(PORT_PPI_PC, c | (row & 0x0F));
    /* Active low: 0 = pressed, 1 = unpressed */
    return msx_in(PORT_PPI_PB);
}

void input_init(void)
{
    s_pointer.x = 256;      /* Center screen */
    s_pointer.y = 106;
    s_pointer.btn_left = false;
    s_pointer.btn_right = false;
    s_pointer.btn_prev_left = false;

    vdp_set_sprite_pos(0, s_pointer.x, s_pointer.y, 0, BTRON_COL_BLACK);
}

void input_poll(void)
{
    s_pointer.btn_prev_left = s_pointer.btn_left;

    /* Scan Keyboard Matrix */
    uint8_t row8 = scan_kbd_row(8);
    uint8_t row7 = scan_kbd_row(7);

    s_keys.space = !(row8 & (1u << 0));
    s_keys.left  = !(row8 & (1u << 4));
    s_keys.up    = !(row8 & (1u << 5));
    s_keys.down  = !(row8 & (1u << 6));
    s_keys.right = !(row8 & (1u << 7));

    s_keys.enter = !(row7 & (1u << 0));
    s_keys.tab   = !(row7 & (1u << 4));
    s_keys.esc   = !(row7 & (1u << 5));

    /* Move pointer with cursor keys (speed acceleration: 4 pixels per frame) */
    const uint8_t speed = 4;
    if (s_keys.left) {
        if (s_pointer.x > speed) s_pointer.x -= speed;
        else s_pointer.x = 0;
    }
    if (s_keys.right) {
        if (s_pointer.x + speed < SCREEN_WIDTH - 1) s_pointer.x += speed;
        else s_pointer.x = SCREEN_WIDTH - 1;
    }
    if (s_keys.up) {
        if (s_pointer.y > speed) s_pointer.y -= speed;
        else s_pointer.y = 0;
    }
    if (s_keys.down) {
        if (s_pointer.y + speed < (uint8_t)(SCREEN_HEIGHT - 1)) s_pointer.y += speed;
        else s_pointer.y = (uint8_t)(SCREEN_HEIGHT - 1);
    }

    /* Left click button: Space or Enter */
    s_pointer.btn_left = s_keys.space || s_keys.enter;

    /* Read alphanumeric keys for Terminal */
    s_keys.ascii_char = 0;
    uint8_t row4 = scan_kbd_row(4);
    uint8_t row3 = scan_kbd_row(3);
    uint8_t row5 = scan_kbd_row(5);

    if (!(row4 & 0x01)) s_keys.ascii_char = 'a';
    else if (!(row4 & 0x02)) s_keys.ascii_char = 's';
    else if (!(row4 & 0x04)) s_keys.ascii_char = 'd';
    else if (!(row4 & 0x08)) s_keys.ascii_char = 'f';
    else if (!(row4 & 0x10)) s_keys.ascii_char = 'g';
    else if (!(row4 & 0x20)) s_keys.ascii_char = 'h';
    else if (!(row4 & 0x40)) s_keys.ascii_char = 'j';
    else if (!(row4 & 0x80)) s_keys.ascii_char = 'k';
    else if (!(row3 & 0x01)) s_keys.ascii_char = 'q';
    else if (!(row3 & 0x02)) s_keys.ascii_char = 'w';
    else if (!(row3 & 0x04)) s_keys.ascii_char = 'e';
    else if (!(row3 & 0x08)) s_keys.ascii_char = 'r';
    else if (!(row3 & 0x10)) s_keys.ascii_char = 't';
    else if (!(row3 & 0x20)) s_keys.ascii_char = 'y';
    else if (!(row5 & 0x01)) s_keys.ascii_char = 'z';
    else if (!(row5 & 0x02)) s_keys.ascii_char = 'x';
    else if (!(row5 & 0x04)) s_keys.ascii_char = 'c';
    else if (!(row5 & 0x08)) s_keys.ascii_char = 'v';
    else if (!(row5 & 0x10)) s_keys.ascii_char = 'b';
    else if (!(row5 & 0x20)) s_keys.ascii_char = 'n';
    else if (!(row5 & 0x40)) s_keys.ascii_char = 'm';

    /* Update hardware sprite pointer on screen */
    vdp_set_sprite_pos(0, s_pointer.x, s_pointer.y, 0, BTRON_COL_BLACK);
}

const PointerState *input_get_pointer(void)
{
    return &s_pointer;
}

const KeyState *input_get_keys(void)
{
    return &s_keys;
}

bool input_is_clicked(void)
{
    /* Rising edge: clicked now but wasn't clicked previously */
    return (s_pointer.btn_left && !s_pointer.btn_prev_left);
}
