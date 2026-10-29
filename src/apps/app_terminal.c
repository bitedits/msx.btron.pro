/*
 * app_terminal.c — B-MSX Command Shell Terminal implementation.
 */

#include "app_terminal.h"
#include "app_editor.h"
#include "../gui/window.h"
#include "../msxgl/vdp.h"
#include "../msxgl/core.h"
#include "../font/font_latin.h"
#include "../kernel/itron_task.h"
#include "../kernel/bmsx_body.h"
#include <stddef.h>

#define TERM_MAX_LINES  5
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
        term_append_line("B-MSX OS v1.0 [BTRON 3.20]");
        term_append_line("Kernel: uITRON 3.0 / R800 Turbo R");
    } else if (cmd[0] == 'l' && cmd[1] == 's') {
        term_append_line("Real Bodies in MegaROM:");
        UH count = bmsx_get_real_body_count();
        for (UH i = 0; i < count; i++) {
            const BMSX_RealBody *b = bmsx_get_body_by_index(i);
            if (b) {
                char item[44];
                item[0] = '#'; item[1] = '0' + (char)b->body_id; item[2] = ' ';
                uint8_t p = 3;
                for (uint8_t k = 0; b->title[k] != '\0' && p < 42; k++) {
                    item[p++] = b->title[k];
                }
                item[p] = '\0';
                term_append_line(item);
            }
        }
    } else if (cmd[0] == 't' && cmd[1] == 'a' && cmd[2] == 's' && cmd[3] == 'k') {
        term_append_line("Tasks: 1:Sys 2:Gui 3:App 4:Tad");
        term_append_line("Scheduler: uITRON Cooperative 60Hz");
    } else if (cmd[0] == 'r' && cmd[1] == '8') {
        term_append_line("CPU: ASCII R800 fast mode [28.6 MHz]");
        term_append_line("Bus: 16-bit internal ALUs active");
    } else if ((cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 's') ||
               (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 'e' && cmd[3] == 'a' && cmd[4] == 'r')) {
        s_line_count = 0;
    } else if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't') {
        if (cmd[4] == '1') {
            term_append_line("Body #1: Heart Sutra (TAD)");
            term_append_line("Lang: Sanskrit, Tibetan, Chinese");
        } else if (cmd[4] == '2') {
            term_append_line("Body #2: NORTIA TAD Letter");
            term_append_line("Recipient: TAD Working Group");
        } else {
            term_append_line("Usage: cat <1|2>");
        }
    } else if ((cmd[0] == 'o' && cmd[1] == 'p' && cmd[2] == 'e' && cmd[3] == 'n') ||
               (cmd[0] == 'e' && cmd[1] == 'd' && cmd[2] == 'i' && cmd[3] == 't')) {
        char arg = cmd[5];
        if (arg >= '1' && arg <= '4') {
            app_editor_load_body(arg - '0');
        } else {
            term_append_line("Usage: open <1..4>");
        }
    } else if (cmd[0] == 't' && cmd[1] == 'i' && cmd[2] == 'l') {
        wm_tile_windows();
        wm_mark_dirty();
    } else if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 's') {
        wm_cascade_windows();
        wm_mark_dirty();
    } else if (cmd[0] == 'r' && cmd[1] == 'e' && cmd[2] == 'b') {
        msx_reboot();
    } else if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l') {
        term_append_line("Commands: ver, ls, cat <id>, open <id>,");
        term_append_line("tile, cascade, tasks, r800, cls, reboot");
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

static void draw_prompt_line(Window *win, bool clear_bg)
{
    uint16_t content_x = win->x + 2;
    uint16_t content_y = win->y + 14;

    char prompt[24];
    prompt[0] = 'B'; prompt[1] = 'T'; prompt[2] = 'R'; prompt[3] = 'O';
    prompt[4] = 'N'; prompt[5] = '>'; prompt[6] = ' ';
    uint8_t p = 7;
    for (uint8_t k = 0; k < s_cmd_len && p < 17; k++) {
        prompt[p++] = s_cmd_buf[k];
    }
    prompt[p++] = '_';
    prompt[p] = '\0';

    uint8_t prompt_line = (s_line_count < TERM_MAX_LINES) ? s_line_count : TERM_MAX_LINES;
    uint16_t py = content_y + 4 + (prompt_line * 12);
    if (clear_bg) {
        vdp_fill_rect(content_x + 6, py, win->w - 16, 12, BTRON_COL_BLACK);
    }
    font_draw_string(content_x + 6, py, prompt, BTRON_COL_WHITE, BTRON_COL_BLACK);
}

void app_terminal_paint(Window *win)
{
    if (!win) return;

    uint16_t content_x = win->x + 2;
    uint16_t content_y = win->y + 14;

    /* Fill terminal interior with pure Black */
    vdp_fill_rect(content_x, content_y, win->w - 4, win->h - 16, BTRON_COL_BLACK);

    /* Render history lines in Bright Green */
    for (uint8_t i = 0; i < s_line_count; i++) {
        s_term_lines[i][18] = '\0';
        font_draw_string(content_x + 6, content_y + 4 + (i * 12),
                         s_term_lines[i], BTRON_COL_BRIGHT_GREEN, BTRON_COL_BLACK);
    }

    draw_prompt_line(win, false);
}

void app_terminal_on_click(Window *win, uint16_t local_x, uint16_t local_y)
{
    (void)win; (void)local_x; (void)local_y;
}

void app_terminal_input_char(char c)
{
    Window *win = wm_get_window(2);
    if (!win) return;

    if (c == '\r' || c == '\n') {
        s_cmd_buf[s_cmd_len] = '\0';
        s_cmd_len = 0;
        term_execute_cmd(s_cmd_buf);
        s_cmd_buf[0] = '\0';
        wm_mark_window_dirty(2);
    } else {
        if (c == 0x08) {
            if (s_cmd_len > 0) s_cmd_len--;
        } else if (c >= 32 && c <= 126 && s_cmd_len < 28) {
            s_cmd_buf[s_cmd_len++] = c;
            s_cmd_buf[s_cmd_len] = '\0';
        }
        draw_prompt_line(win, true);
    }
}

