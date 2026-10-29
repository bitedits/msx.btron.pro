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

static uint8_t scan_joystick(uint8_t port_idx)
{
    /* Select Joystick Port in PSG Reg 15 (bit 6: 0 = Port 1, 1 = Port 2) */
    msx_out(PORT_PSG_REG, 15);
    uint8_t r15 = msx_in(PORT_PSG_READ);
    if (port_idx == 0) {
        r15 &= (uint8_t)~0x40u;
    } else {
        r15 |= 0x40u;
    }
    msx_out(PORT_PSG_WRITE, r15);

    /* Read Joystick pins from PSG Reg 14 */
    msx_out(PORT_PSG_REG, 14);
    return msx_in(PORT_PSG_READ);
}

void input_init(void)
{
    s_pointer.x = 256;      /* Center screen (512x212) */
    s_pointer.y = 106;
    s_pointer.btn_left = false;
    s_pointer.btn_right = false;
    s_pointer.btn_prev_left = false;

    vdp_set_sprite_pos(0, s_pointer.x, s_pointer.y, 0, BTRON_COL_CHARCOAL);
}

void input_poll(void)
{
    s_pointer.btn_prev_left = s_pointer.btn_left;

    /* 1. Scan Keyboard Matrix */
    uint8_t row8 = scan_kbd_row(8);
    uint8_t row7 = scan_kbd_row(7);

    bool kbd_space = !(row8 & (1u << 0));
    bool kbd_left  = !(row8 & (1u << 4));
    bool kbd_up    = !(row8 & (1u << 5));
    bool kbd_down  = !(row8 & (1u << 6));
    bool kbd_right = !(row8 & (1u << 7));

    s_keys.space = kbd_space;
    s_keys.left  = kbd_left;
    s_keys.up    = kbd_up;
    s_keys.down  = kbd_down;
    s_keys.right = kbd_right;
    s_keys.enter = !(row7 & (1u << 7)); /* Enter: Row 7, Col 7 */
    s_keys.tab   = !(row7 & (1u << 3)); /* Tab: Row 7, Col 3 */
    s_keys.esc   = !(row7 & (1u << 2)); /* Esc: Row 7, Col 2 */
    s_keys.bs    = !(row7 & (1u << 5)); /* BS: Row 7, Col 5 */

    /* 2. Scan Joystick / Mouse Port 1 via PSG Reg 14 */
    uint8_t joy1 = scan_joystick(0);
    bool joy_trga = !(joy1 & 0x01); /* Trigger A / Left click */
    bool joy_up   = !(joy1 & 0x02); /* Up */
    bool joy_down = !(joy1 & 0x04); /* Down */
    bool joy_left = !(joy1 & 0x08); /* Left */
    bool joy_right= !(joy1 & 0x10); /* Right */
    bool joy_trgb = !(joy1 & 0x20); /* Trigger B / Right click */

    /* Precision Pointer Motion:
     * Keyboard arrow keys: precision micro-stepping (1 px on tap, 2 px when held)
     * Joystick / Host Mouse: 3 px horizontally, 2 px vertically */
    static uint8_t s_kbd_hold_x = 0;
    static uint8_t s_kbd_hold_y = 0;

    uint8_t delta_x = 0;
    uint8_t delta_y = 0;

    if (kbd_left || kbd_right) {
        s_kbd_hold_x++;
        delta_x = (s_kbd_hold_x < 5) ? 1 : 2; /* 1 pixel precision tap, 2 px smooth hold */
    } else if (joy_left || joy_right) {
        s_kbd_hold_x = 0;
        delta_x = 3;
    } else {
        s_kbd_hold_x = 0;
    }

    if (kbd_up || kbd_down) {
        s_kbd_hold_y++;
        delta_y = (s_kbd_hold_y < 5) ? 1 : 2; /* 1 pixel precision tap, 2 px smooth hold */
    } else if (joy_up || joy_down) {
        s_kbd_hold_y = 0;
        delta_y = 2;
    } else {
        s_kbd_hold_y = 0;
    }

    if (kbd_left || joy_left) {
        if (s_pointer.x > delta_x) s_pointer.x -= delta_x;
        else s_pointer.x = 0;
    }
    if (kbd_right || joy_right) {
        if (s_pointer.x + delta_x < SCREEN_WIDTH - 1) s_pointer.x += delta_x;
        else s_pointer.x = SCREEN_WIDTH - 1;
    }
    if (kbd_up || joy_up) {
        if (s_pointer.y > delta_y) s_pointer.y -= delta_y;
        else s_pointer.y = 0;
    }
    if (kbd_down || joy_down) {
        if (s_pointer.y + delta_y < (uint8_t)(SCREEN_HEIGHT - 1)) s_pointer.y += delta_y;
        else s_pointer.y = (uint8_t)(SCREEN_HEIGHT - 1);
    }

    /* Left click button: Joystick Trigger A (host mouse left click) */
    s_pointer.btn_left  = joy_trga;
    s_pointer.btn_right = joy_trgb;

    /* 3. Read alphanumeric keys per standard MSX Keyboard Matrix */
    s_keys.ascii_char = 0;
    uint8_t row2 = scan_kbd_row(2);
    uint8_t row3 = scan_kbd_row(3);
    uint8_t row4 = scan_kbd_row(4);
    uint8_t row5 = scan_kbd_row(5);
    uint8_t row0 = scan_kbd_row(0);
    uint8_t row1 = scan_kbd_row(1);

    /* Space key: Row 8, bit 0 */
    if (kbd_space) s_keys.ascii_char = ' ';
    /* Row 2: bit 6 = a, bit 7 = b */
    else if (!(row2 & (1u << 6))) s_keys.ascii_char = 'a';
    else if (!(row2 & (1u << 7))) s_keys.ascii_char = 'b';
    /* Row 3: c, d, e, f, g, h, i, j (bits 0..7) */
    else if (!(row3 & (1u << 0))) s_keys.ascii_char = 'c';
    else if (!(row3 & (1u << 1))) s_keys.ascii_char = 'd';
    else if (!(row3 & (1u << 2))) s_keys.ascii_char = 'e';
    else if (!(row3 & (1u << 3))) s_keys.ascii_char = 'f';
    else if (!(row3 & (1u << 4))) s_keys.ascii_char = 'g';
    else if (!(row3 & (1u << 5))) s_keys.ascii_char = 'h';
    else if (!(row3 & (1u << 6))) s_keys.ascii_char = 'i';
    else if (!(row3 & (1u << 7))) s_keys.ascii_char = 'j';
    /* Row 4: k, l, m, n, o, p, q, r (bits 0..7) */
    else if (!(row4 & (1u << 0))) s_keys.ascii_char = 'k';
    else if (!(row4 & (1u << 1))) s_keys.ascii_char = 'l';
    else if (!(row4 & (1u << 2))) s_keys.ascii_char = 'm';
    else if (!(row4 & (1u << 3))) s_keys.ascii_char = 'n';
    else if (!(row4 & (1u << 4))) s_keys.ascii_char = 'o';
    else if (!(row4 & (1u << 5))) s_keys.ascii_char = 'p';
    else if (!(row4 & (1u << 6))) s_keys.ascii_char = 'q';
    else if (!(row4 & (1u << 7))) s_keys.ascii_char = 'r';
    /* Row 5: s, t, u, v, w, x, y, z (bits 0..7) */
    else if (!(row5 & (1u << 0))) s_keys.ascii_char = 's';
    else if (!(row5 & (1u << 1))) s_keys.ascii_char = 't';
    else if (!(row5 & (1u << 2))) s_keys.ascii_char = 'u';
    else if (!(row5 & (1u << 3))) s_keys.ascii_char = 'v';
    else if (!(row5 & (1u << 4))) s_keys.ascii_char = 'w';
    else if (!(row5 & (1u << 5))) s_keys.ascii_char = 'x';
    else if (!(row5 & (1u << 6))) s_keys.ascii_char = 'y';
    else if (!(row5 & (1u << 7))) s_keys.ascii_char = 'z';
    /* Row 0: 0..7 */
    else if (!(row0 & (1u << 0))) s_keys.ascii_char = '0';
    else if (!(row0 & (1u << 1))) s_keys.ascii_char = '1';
    else if (!(row0 & (1u << 2))) s_keys.ascii_char = '2';
    else if (!(row0 & (1u << 3))) s_keys.ascii_char = '3';
    else if (!(row0 & (1u << 4))) s_keys.ascii_char = '4';
    else if (!(row0 & (1u << 5))) s_keys.ascii_char = '5';
    else if (!(row0 & (1u << 6))) s_keys.ascii_char = '6';
    else if (!(row0 & (1u << 7))) s_keys.ascii_char = '7';
    /* Row 1: 8, 9 */
    else if (!(row1 & (1u << 0))) s_keys.ascii_char = '8';
    else if (!(row1 & (1u << 1))) s_keys.ascii_char = '9';

    /* Update hardware sprite pointer on screen */
    vdp_set_sprite_pos(0, s_pointer.x, s_pointer.y, 0, BTRON_COL_CHARCOAL);
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
