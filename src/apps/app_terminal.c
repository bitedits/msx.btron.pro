/*
 * app_terminal.c — B-MSX Command Shell Terminal implementation.
 */

#include "app_terminal.h"
#include "../msxgl/vdp.h"
#include "../font/font_latin.h"
#include "../kernel/itron_task.h"
#include "../kernel/bmsx_body.h"
#include <stddef.h>

#define TERM_MAX_LINES  9
#define TERM_LINE_LEN   44

static char s_term_lines[TERM_MAX_LINES][TERM_LINE_LEN];
static uint8_t s_line_count = 0;
static char s_cmd_buf[32];
static uint8_t s_cmd_len = 0;

static void term_append_line(const char *str)
{
    if (s_line_count < TERM_MAX_LINES) {
        uint8_t i = 0;
        while (str[i] != '\0' && i < TERM_LINE_LEN - 1) {
            s_term_lines[s_line_count][i] = str[i];
            i++;
        }
        s_term_lines[s_line_count][i] = '\0';
        s_line_count++;
    } else {
        /* Scroll lines up */
        for (uint8_t l = 0; l < TERM_MAX_LINES - 1; l++) {
            for (uint8_t c = 0; c < TERM_LINE_LEN; c++) {
                s_term_lines[l][c] = s_term_lines[l + 1][c];
            }
        }
        uint8_t i = 0;
        while (str[i] != '\0' && i < TERM_LINE_LEN - 1) {
            s_term_lines[TERM_MAX_LINES - 1][i] = str[i];
            i++;
        }
        s_term_lines[TERM_MAX_LINES - 1][i] = '\0';
    }
}

static void term_execute_cmd(const char *cmd)
{
    term_append_line(cmd);

    if (cmd[0] == 'v' && cmd[1] == 'e' && cmd[2] == 'r') {
        term_append_line("B-MSX OS v1.0 [BTRON3.20]");
        term_append_line("Kernel: uITRON 3.0 / R800 Turbo R");
    } else if (cmd[0] == 'l' && cmd[1] == 's') {
        term_append_line("Real Bodies in MegaROM:");
        UH count = bmsx_get_real_body_count();
        for (UH i = 0; i < count; i++) {
            const BMSX_RealBody *b = bmsx_get_body_by_index(i);
            if (b) {
                term_append_line(b->title);
            }
        }
    } else if (cmd[0] == 't' && cmd[1] == 'a' && cmd[2] == 's' && cmd[3] == 'k') {
        term_append_line("Tasks: 1:Sys 2:Gui 3:App 4:Tad");
        term_append_line("Scheduler: uITRON Cooperative 60Hz");
    } else if (cmd[0] == 'r' && cmd[1] == '8') {
        term_append_line("CPU: ASCII R800 fast mode [28.6 MHz]");
        term_append_line("Bus: 16-bit internal ALUs active");
    } else if (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 's') {
        s_line_count = 0;
    } else if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l') {
        term_append_line("Commands: ver, ls, tasks, r800, cls");
    } else {
        term_append_line("Unknown command. Type 'help'.");
    }
}

void app_terminal_init(void)
{
    s_line_count = 0;
    s_cmd_len = 0;
    s_cmd_buf[0] = '\0';

    term_append_line("B-MSX OS v1.0 Shell (R800 Turbo R)");
    term_append_line("Type 'help' for available commands.");
    term_append_line("----------------------------------");
}

void app_terminal_paint(Window *win)
{
    if (!win) return;

    uint16_t content_x = win->x + 2;
    uint16_t content_y = win->y + 14;
    uint16_t content_w = win->w - 4;
    uint16_t content_h = win->h - 16;

    /* Fill terminal interior with pure Black */
    vdp_fill_rect(content_x, content_y, content_w, content_h, BTRON_COL_BLACK);

    /* Render history lines in Bright Green */
    for (uint8_t i = 0; i < s_line_count; i++) {
        font_draw_string(content_x + 6, content_y + 4 + (i * 12),
                         s_term_lines[i], BTRON_COL_BRIGHT_GREEN, BTRON_COL_BLACK);
    }

    /* Render current command line prompt */
    char prompt[40];
    prompt[0] = 'B'; prompt[1] = 'T'; prompt[2] = 'R'; prompt[3] = 'O';
    prompt[4] = 'N'; prompt[5] = '>'; prompt[6] = ' ';
    uint8_t p = 7;
    for (uint8_t k = 0; k < s_cmd_len && p < 36; k++) {
        prompt[p++] = s_cmd_buf[k];
    }
    prompt[p++] = '_'; /* Blinking block cursor */
    prompt[p] = '\0';

    font_draw_string(content_x + 6, content_y + 4 + (s_line_count * 12),
                     prompt, BTRON_COL_WHITE, BTRON_COL_BLACK);
}

void app_terminal_on_click(Window *win, uint16_t local_x, uint16_t local_y)
{
    (void)win; (void)local_x; (void)local_y;
    /* Click gives focus */
}

void app_terminal_input_char(char c)
{
    if (c == '\r' || c == '\n') {
        s_cmd_buf[s_cmd_len] = '\0';
        term_execute_cmd(s_cmd_buf);
        s_cmd_len = 0;
        s_cmd_buf[0] = '\0';
    } else if (c == 0x08) {
        /* Backspace */
        if (s_cmd_len > 0) s_cmd_len--;
    } else if (c >= 32 && c <= 126 && s_cmd_len < 28) {
        s_cmd_buf[s_cmd_len++] = c;
        s_cmd_buf[s_cmd_len] = '\0';
    }
    Window *win = wm_get_window(2);
    if (win) {
        wm_draw_window(win);
    }
}

