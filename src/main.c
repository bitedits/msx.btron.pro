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
    /* Micro-ITRON periodic heartbeat */
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

    /* Register Micro-ITRON System Task */
    itron_task_create(1, task_system_monitor, 1, "SysMon");

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

    /* Set Editor as initial active window */
    wm_set_active_window(1);

    /* Initial Desktop Render */
    wm_draw_all();
    menu_draw_bar();

    /* 4. Main Event Loop */
    while (1) {
        msx_wait_vblank();
        itron_task_tick();
        input_poll();

        const PointerState *ptr = input_get_pointer();
        const KeyState *keys = input_get_keys();

        /* Pass keyboard input to active window if Terminal */
        if (keys->ascii_char != 0) {
            Window *active = wm_get_active_window();
            if (active && active->win_id == 2) {
                app_terminal_input_char(keys->ascii_char);
            }
        } else if (keys->enter) {
            Window *active = wm_get_active_window();
            if (active && active->win_id == 2) {
                app_terminal_input_char('\r');
            }
        }

        /* Check Mouse / Pointer Click */
        if (input_is_clicked()) {
            if (!menu_handle_click(ptr->x, ptr->y)) {
                if (menu_is_open()) {
                    menu_close_popup();
                    wm_draw_all();
                    menu_draw_bar();
                } else {
                    wm_handle_click(ptr->x, ptr->y);
                }
            }
        }

        /* Cooperative Task Dispatch slice */
        itron_task_dispatch();
    }
}
