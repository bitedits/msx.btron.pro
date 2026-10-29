/*
 * menu.c — Top Cascading Menu Bar implementation for Screen 7.
 */

#include "menu.h"
#include "window.h"
#include "compositor.h"
#include "../apps/app_editor.h"
#include "../msxgl/vdp.h"
#include "../msxgl/core.h"
#include "../font/font_latin.h"
#include <stddef.h>

typedef struct {
    const char *title;
    uint16_t x;
    uint16_t w;
    const char *items[6];
    uint8_t item_count;
} RootMenuItem;

static const RootMenuItem s_root_menus[6] = {
    { " TRON ", 4, 60, { "About B-MSX OS", "CPU: R800 Turbo", "VDP: V9958 (S#7)", "Memory: 1024K MegaROM", "Reboot" }, 5 },
    { "File", 68, 44, { "New Real Body", "Open Document", "Save Body", "Close Window" }, 4 },
    { "Edit", 116, 44, { "Cut", "Copy", "Paste", "Select All" }, 4 },
    { "View", 164, 44, { "1. Editor", "2. Terminal", "3. Browser", "4. Cabinet" }, 4 },
    { "Window", 212, 56, { "Cascade", "Tile 2x2", "Maximize", "Restore" }, 4 },
    { "Help", 272, 44, { "Heart Sutra TAD", "NORTIA Letter", "BTRON3 Guide" }, 3 }
};

static int8_t s_open_menu_idx = -1;

void menu_init(void)
{
    s_open_menu_idx = -1;
}

void menu_draw_bar(void)
{
    /* Menu bar rectangle (0, 0, 512, 12) */
    vdp_fill_rect(0, 0, SCREEN_WIDTH, 12, BTRON_COL_LIGHT_GRAY);
    vdp_draw_hline(0, 12, SCREEN_WIDTH, BTRON_COL_DARK_GRAY);

    /* Render root titles */
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t bg = (s_open_menu_idx == (int8_t)i) ? BTRON_COL_TITLE_BLUE : BTRON_COL_LIGHT_GRAY;
        uint8_t fg = (s_open_menu_idx == (int8_t)i) ? BTRON_COL_WHITE : BTRON_COL_BLACK;

        if (s_open_menu_idx == (int8_t)i) {
            vdp_fill_rect(s_root_menus[i].x - 2, 0, s_root_menus[i].w + 4, 12, bg);
        }
        font_draw_string(s_root_menus[i].x, 2, s_root_menus[i].title, fg, bg);
    }

    /* System Status indicator on right: CPU Mode */
    bool turbo = msx_is_turbo_r();
    const char *cpu_str = turbo ? "[R800 TURBO 28MHz]" : "[Z80 3.58MHz]";
    font_draw_string(360, 2, cpu_str, BTRON_COL_TITLE_BLUE, BTRON_COL_LIGHT_GRAY);

    /* If a menu is open, draw dropdown popup */
    if (s_open_menu_idx >= 0 && s_open_menu_idx < 6) {
        const RootMenuItem *m = &s_root_menus[s_open_menu_idx];
        uint16_t popup_x = m->x - 2;
        uint16_t popup_y = 13;
        uint16_t popup_w = 140;
        uint16_t popup_h = (uint16_t)(m->item_count * 14 + 6);

        if (popup_x + popup_w > SCREEN_WIDTH) {
            popup_x = SCREEN_WIDTH - popup_w - 2;
        }

        /* 3D Bevel Popup Frame */
        vdp_draw_bevel_box(popup_x, popup_y, popup_w, popup_h,
                           BTRON_COL_WHITE, BTRON_COL_DARK_GRAY, BTRON_COL_LIGHT_GRAY);

        for (uint8_t it = 0; it < m->item_count; it++) {
            uint16_t item_y = popup_y + 3 + (it * 14);
            font_draw_string(popup_x + 8, item_y, m->items[it], BTRON_COL_BLACK, BTRON_COL_LIGHT_GRAY);
        }
    }
}

void menu_close_popup(void)
{
    s_open_menu_idx = -1;
}

bool menu_is_open(void)
{
    return (s_open_menu_idx >= 0);
}

bool menu_handle_click(uint16_t x, uint16_t y)
{
    /* If click is on the top bar */
    if (y <= 12) {
        for (uint8_t i = 0; i < 6; i++) {
            if (x >= s_root_menus[i].x - 2 && x <= s_root_menus[i].x + s_root_menus[i].w + 2) {
                if (s_open_menu_idx == (int8_t)i) {
                    s_open_menu_idx = -1; /* Toggle close */
                } else {
                    s_open_menu_idx = (int8_t)i;
                }
                menu_draw_bar();
                return true;
            }
        }
        s_open_menu_idx = -1;
        menu_draw_bar();
        return true;
    }

    /* If menu dropdown is open and clicked inside */
    if (s_open_menu_idx >= 0 && s_open_menu_idx < 6) {
        const RootMenuItem *m = &s_root_menus[s_open_menu_idx];
        uint16_t popup_x = m->x - 2;
        uint16_t popup_y = 13;
        uint16_t popup_w = 140;
        uint16_t popup_h = (uint16_t)(m->item_count * 14 + 6);

        if (popup_x + popup_w > SCREEN_WIDTH) {
            popup_x = SCREEN_WIDTH - popup_w - 2;
        }

        if (x >= popup_x && x <= popup_x + popup_w &&
            y >= popup_y && y <= popup_y + popup_h) {

            uint8_t selected_item = (uint8_t)((y - (popup_y + 3)) / 14);
            if (selected_item < m->item_count) {
                /* Execute menu selection */
                if (s_open_menu_idx == 0) {
                    /* TRON Menu */
                    if (selected_item == 0) {
                        /* About: focus Terminal */
                        Window *win = wm_get_window(2);
                        if (win) {
                            win->state = WIN_STATE_NORMAL;
                            wm_set_active_window(2);
                        }
                    } else if (selected_item == 3) {
                        /* Memory / Storage Cabinet */
                        Window *win = wm_get_window(4);
                        if (win) {
                            win->state = WIN_STATE_NORMAL;
                            wm_set_active_window(4);
                        }
                    } else if (selected_item == 4) {
                        /* Reboot */
                        msx_reboot();
                    }
                } else if (s_open_menu_idx == 1) {
                    /* File Menu */
                    if (selected_item == 0 || selected_item == 1) {
                        Window *win = wm_get_window(1);
                        if (win) {
                            win->state = WIN_STATE_NORMAL;
                            wm_set_active_window(1);
                        }
                    } else if (selected_item == 3) {
                        Window *act = wm_get_active_window();
                        if (act) {
                            act->state = WIN_STATE_CLOSED;
                        }
                    }
                } else if (s_open_menu_idx == 3) {
                    /* View Menu: Open Windows */
                    Window *win = NULL;
                    if (selected_item == 0) win = wm_get_window(1); /* Editor */
                    if (selected_item == 1) win = wm_get_window(2); /* Terminal */
                    if (selected_item == 2) win = wm_get_window(3); /* Browser */
                    if (selected_item == 3) win = wm_get_window(4); /* Cabinet */

                    if (win) {
                        win->state = WIN_STATE_NORMAL;
                        wm_set_active_window(win->win_id);
                    }
                } else if (s_open_menu_idx == 4) {
                    /* Window Menu */
                    if (selected_item == 0) {
                        wm_cascade_windows();
                    } else if (selected_item == 1) {
                        wm_tile_windows();
                    } else if (selected_item == 2) {
                        Window *act = wm_get_active_window();
                        if (act && act->state == WIN_STATE_NORMAL) {
                            act->state = WIN_STATE_ZOOMED;
                            act->x = 2; act->y = 14; act->w = 508; act->h = 194;
                        }
                    } else if (selected_item == 3) {
                        wm_restore_default();
                    }
                } else if (s_open_menu_idx == 5) {
                    /* Help Menu: Switch document in Editor */
                    if (selected_item == 0) {
                        app_editor_load_body(1);
                    } else if (selected_item == 1) {
                        app_editor_load_body(2);
                    }
                    Window *win = wm_get_window(selected_item == 2 ? 3 : 1);
                    if (win) {
                        win->state = WIN_STATE_NORMAL;
                        wm_set_active_window(win->win_id);
                    }
                }
            }
            s_open_menu_idx = -1;
            comp_repair_rect(popup_x, popup_y, popup_w, popup_h);
            menu_draw_bar();
            return true;
        }

        /* Click outside closed the popup */
        s_open_menu_idx = -1;
        comp_repair_rect(popup_x, popup_y, popup_w, popup_h);
        menu_draw_bar();
        return true;
    }

    return false;
}

