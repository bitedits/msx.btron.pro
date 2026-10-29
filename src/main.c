/*
 * main.c — 8-bit BTRON Operating System for MSX turbo R (R800 / V9958).
 * Entry point and micro-ITRON real-time event loop.
 */

#include "msxgl/msxgl_config.h"
#include "msxgl/core.h"
#include "msxgl/vdp.h"
#include "msxgl/input.h"
#include "msxgl/megarom.h"
#include "kernel/btron_types.h"
#include "kernel/itron_task.h"
#include "kernel/bmsx_body.h"
#include "font/font_latin.h"
#include "font/font_kanji.h"
#include "font/font_tibetan.h"
#include "gui/window.h"
#include "gui/menu.h"
#include "apps/app_editor.h"
#include "apps/app_terminal.h"
#include "apps/app_browser.h"
#include "apps/app_cabinet.h"
#include "data/doc_heart_sutra.h"
#include "data/doc_nortia_letter.h"

/* Real-time background task */
static void task_system_monitor(void)
{
    /* Micro-ITRON periodic 1Hz heartbeat; yields remaining ticks to AsyncComp */
    itron_task_sleep(60);
}

int main(void)
{
    /* 1. Hardware Initialization */
    msx_di();
    msx_set_cpu_mode(CPU_MODE_R800_ROM);
    vdp_init_screen7();
    vdp_init_sprites();
    input_init();
    font_latin_init();
    font_kanji_init();
    font_tibetan_init();
    msx_ei();

    /* 2. Kernel & Storage Subsystems */
    itron_init();
    bmsx_body_init();

    /* Register System Real Bodies in MegaROM */
    bmsx_register_real_body(1, BODY_TYPE_TAD, BANK_TAD_DOCS,
                            g_doc_heart_sutra_tad, g_doc_heart_sutra_tad_len,
                            "Heart Sutra (TAD)");

    bmsx_register_real_body(2, BODY_TYPE_TAD, BANK_TAD_DOCS,
                            g_doc_nortia_letter_tad, g_doc_nortia_letter_tad_len,
                            "NORTIA Letter (TAD)");

    bmsx_register_real_body(3, BODY_TYPE_EXEC, BANK_APPS,
                            NULL, 0,
                            "Shell Terminal");

    bmsx_register_real_body(4, BODY_TYPE_EXEC, BANK_APPS,
                            NULL, 0,
                            "Storage Cabinet");

    /* Register Micro-ITRON System & Asynchronous Compositor Tasks */
    itron_task_create(1, task_system_monitor, 1, "SysMon");
    itron_task_create(2, wm_async_compositor_task, 2, "AsyncComp");

    /* 3. Window Manager & Desktop Setup */
    wm_init();
    menu_init();
    app_editor_init();
    app_terminal_init();
    app_browser_init();
    app_cabinet_init();

    /* Window 1: Document Editor (Primary) */
    wm_create_window(1, "Heart Sutra (TAD)", 6, 16, 330, 188,
                     app_editor_paint, app_editor_on_click);

    /* Window 2: B-MSX Shell Terminal */
    wm_create_window(2, "Shell Terminal", 342, 16, 164, 100,
                     app_terminal_paint, app_terminal_on_click);

    /* Window 3: Hypermedia Browser */
    wm_create_window(3, "Hyper Browser", 342, 120, 164, 84,
                     app_browser_paint, app_browser_on_click);

    /* Window 4: Storage Cabinet (closed initially) */
    Window *win_cab = wm_create_window(4, "Storage Cabinet", 80, 40, 350, 140,
                                       app_cabinet_paint, app_cabinet_on_click);
    if (win_cab) {
        win_cab->state = WIN_STATE_CLOSED;
    }

    /* Set Shell Terminal as initial active window */
    wm_set_active_window(2);

    /* Initial Desktop Render via Offscreen Page 1 -> Atomic HMMM Present to Page 0 */
    vdp_set_display_page(0);
    vdp_set_draw_page(1);
    wm_draw_all();
    menu_draw_bar();
    vdp_present_backbuffer();
    vdp_set_draw_page(0);
    (void)wm_consume_dirty();
    wm_request_async_underlay(2);

    /* 4. Main Event Loop */
    while (1) {
        msx_wait_vblank();
        itron_task_tick();
        input_poll();

        bool trigger_click = false;
        ID   new_drag_win = 0;

        const PointerState *ptr = input_get_pointer();
        const KeyState *keys = input_get_keys();
        Window *active = wm_get_active_window();

        /* Window Capture & Dragging state */
        static ID s_dragging_win_id = 0;
        static int16_t s_drag_offset_x = 0;
        static int16_t s_drag_offset_y = 0;
        static bool s_drag_moved = false;
        static bool s_drag_sticky = false;

        bool cursor_in_term = (active && active->win_id == 2 && s_dragging_win_id == 0 &&
                               ptr->x >= active->x && ptr->x < active->x + active->w &&
                               ptr->y >= active->y && ptr->y < active->y + active->h);

        /* Global Window Focus Cycling: Tab key */
        static bool s_prev_tab = false;
        if (keys->tab && !s_prev_tab) {
            s_dragging_win_id = 0;
            s_drag_sticky = false;
            Window *curr = wm_get_active_window();
            ID next_id = (curr && curr->win_id < 3) ? (curr->win_id + 1) : 1;
            wm_focus_window(next_id);
        }
        s_prev_tab = keys->tab;

        /* Esc key drops any captured window or closes open menu */
        if (keys->esc) {
            if (s_dragging_win_id > 0) {
                s_dragging_win_id = 0;
                s_drag_sticky = false;
                s_drag_moved = false;
            }
            if (menu_is_open()) {
                menu_close_popup();
            }
        }

        /* Keyboard input routing with single-press edge detection and auto-repeat */
        static char s_prev_char = 0;
        static uint8_t s_char_repeat = 0;
        char ch = keys->ascii_char;

        if (ch != 0) {
            bool fire_char = false;
            if (ch != s_prev_char) {
                fire_char = true;
                s_char_repeat = 0;
            } else {
                s_char_repeat++;
                if (s_char_repeat > 20 && (s_char_repeat & 3) == 0) {
                    fire_char = true;
                }
            }

            if (fire_char) {
                if (active && active->win_id == 2 && cursor_in_term) {
                    app_terminal_input_char(ch);
                } else {
                    if (ch == '1' || ch == '2' || ch == '3') {
                        s_dragging_win_id = 0;
                        wm_focus_window((ID)(ch - '0'));
                    } else if (ch == ' ') {
                        /* Space outside terminal acts as a click at pointer position */
                        trigger_click = true;
                    } else if (active && active->win_id == 2 && s_dragging_win_id == 0) {
                        app_terminal_input_char(ch);
                    }
                }
            }
        } else {
            s_char_repeat = 0;
        }
        s_prev_char = ch;

        /* Enter key */
        static bool s_prev_enter = false;
        if (keys->enter) {
            if (!s_prev_enter) {
                if (active && active->win_id == 2 && cursor_in_term) {
                    app_terminal_input_char('\r');
                } else {
                    /* Enter outside terminal acts as a click at pointer position */
                    trigger_click = true;
                }
            }
            s_prev_enter = true;
        } else {
            s_prev_enter = false;
        }

        /* Backspace key with repeat */
        static bool s_prev_bs = false;
        static uint8_t s_bs_repeat = 0;
        if (keys->bs) {
            bool fire_bs = false;
            if (!s_prev_bs) {
                fire_bs = true;
                s_bs_repeat = 0;
            } else {
                s_bs_repeat++;
                if (s_bs_repeat > 20 && (s_bs_repeat & 3) == 0) {
                    fire_bs = true;
                }
            }
            if (fire_bs && active && active->win_id == 2 && s_dragging_win_id == 0) {
                app_terminal_input_char(0x08);
            }
            s_prev_bs = true;
        } else {
            s_prev_bs = false;
            s_bs_repeat = 0;
        }

        /* Check Mouse / Pointer Click (Joy Trg A, host mouse click, or 'Z' key outside terminal) */
        if (input_is_clicked()) {
            if (!(cursor_in_term && (ch == 'z' || ch == 'Z'))) {
                trigger_click = true;
            }
        }

        /* Process Unified Click or Sticky Window Drop */
        if (trigger_click) {
            if (s_dragging_win_id > 0 && s_drag_sticky) {
                /* Clicking while a window is sticky-captured drops it */
                s_dragging_win_id = 0;
                s_drag_sticky = false;
                s_drag_moved = false;
            } else {
                if (!menu_handle_click(ptr->x, ptr->y)) {
                    wm_handle_click(ptr->x, ptr->y, &new_drag_win);
                }
            }
        }

        /* Initiate Window Capture / Hardware BitBlt Dragging if Title Bar was clicked */
        if (new_drag_win > 0) {
            Window *dw = wm_get_window(new_drag_win);
            if (dw && dw->state == WIN_STATE_NORMAL) {
                s_dragging_win_id = new_drag_win;
                s_drag_offset_x = (int16_t)(ptr->x - dw->x);
                s_drag_offset_y = (int16_t)(ptr->y - dw->y);
                s_drag_moved = false;
                s_drag_sticky = !ptr->btn_left;
                wm_begin_drag(new_drag_win);
            }
        } else if (s_dragging_win_id > 0) {
            Window *win = wm_get_window(s_dragging_win_id);
            if (win && win->state == WIN_STATE_NORMAL) {
                int16_t target_x = (int16_t)ptr->x - s_drag_offset_x;
                int16_t target_y = (int16_t)ptr->y - s_drag_offset_y;

                if (target_x < 2) target_x = 2;
                if (target_x + (int16_t)win->w > 510) target_x = 510 - (int16_t)win->w;
                if (target_y < 14) target_y = 14;
                if (target_y + (int16_t)win->h > 210) target_y = 210 - (int16_t)win->h;

                if (wm_move_window_bitblt(s_dragging_win_id, (uint16_t)target_x, (uint16_t)target_y)) {
                    s_drag_moved = true;
                }

                if (!ptr->btn_left && !s_drag_sticky) {
                    if (s_drag_moved) {
                        /* Standard press-drag-release completed */
                        s_dragging_win_id = 0;
                        s_drag_moved = false;
                    } else {
                        /* Quick click on title bar: enter sticky window capture mode! */
                        s_drag_sticky = true;
                    }
                }
            } else {
                s_dragging_win_id = 0;
                s_drag_sticky = false;
                s_drag_moved = false;
            }
        }

        /* Cooperative Task Dispatch slice (yield click frame to foreground) */
        if (!trigger_click) {
            itron_task_dispatch();
        }
    }
}
