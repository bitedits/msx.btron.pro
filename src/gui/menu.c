/*
 * menu.c — Top Cascading Menu Bar implementation for Screen 7.
 */

#include "menu.h"
#include "window.h"
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

static void popup_bg_xfer(int8_t idx, bool save)
{
    if (idx < 0 || idx >= 6) return;
    const RootMenuItem *m = &s_root_menus[idx];
    uint16_t popup_x = (uint16_t)((m->x - 2u) & (uint16_t)~1u);
    uint16_t popup_w = 140u;
    uint16_t popup_h = (uint16_t)(m->item_count * 14u + 6u);
    if (popup_x + popup_w > SCREEN_WIDTH) {
        popup_x = (uint16_t)((SCREEN_WIDTH - popup_w - 2u) & (uint16_t)~1u);
    }

    uint8_t prev_page = vdp_get_draw_page();
    vdp_set_draw_page(0);
    uint16_t h1 = (popup_h > 38u) ? 38u : popup_h;
    if (save) {
        vdp_copy_rect(popup_x, 13u, 0u, 212u, popup_w, h1);
        if (popup_h > 38u) {
            vdp_copy_rect(popup_x, 51u, 140u, 212u, popup_w, (uint16_t)(popup_h - 38u));
        }
    } else {
        vdp_copy_rect(0u, 212u, popup_x, 13u, popup_w, h1);
        if (popup_h > 38u) {
            vdp_copy_rect(140u, 212u, popup_x, 51u, popup_w, (uint16_t)(popup_h - 38u));
        }
    }
    vdp_wait_cmd();
    vdp_set_draw_page(prev_page);
}

void menu_init(void)
{
    s_open_menu_idx = -1;
}

static void draw_root_item(uint8_t i)
{
    bool open = (s_open_menu_idx == (int8_t)i);
    uint8_t bg = open ? BTRON_COL_TITLE_BLUE : BTRON_COL_LIGHT_GRAY;
    uint8_t fg = open ? BTRON_COL_WHITE : BTRON_COL_BLACK;
    vdp_fill_rect(s_root_menus[i].x - 2, 0, s_root_menus[i].w + 4, 12, bg);
    vdp_wait_cmd();
    font_draw_string(s_root_menus[i].x, 2, s_root_menus[i].title, fg, bg);
}

static void draw_popup(void)
{
    if (s_open_menu_idx >= 0 && s_open_menu_idx < 6) {
        const RootMenuItem *m = &s_root_menus[s_open_menu_idx];
        uint16_t popup_x = (uint16_t)((m->x - 2u) & (uint16_t)~1u);
        uint16_t popup_y = 13;
        uint16_t popup_w = 140;
        uint16_t popup_h = (uint16_t)(m->item_count * 14 + 6);

        if (popup_x + popup_w > SCREEN_WIDTH) {
            popup_x = (uint16_t)((SCREEN_WIDTH - popup_w - 2u) & (uint16_t)~1u);
        }

        vdp_draw_bevel_box(popup_x, popup_y, popup_w, popup_h,
                           BTRON_COL_WHITE, BTRON_COL_DARK_GRAY, BTRON_COL_LIGHT_GRAY);

        for (uint8_t it = 0; it < m->item_count; it++) {
            uint16_t item_y = popup_y + 3 + (it * 14);
            font_draw_string(popup_x + 8, item_y, m->items[it], BTRON_COL_BLACK, BTRON_COL_LIGHT_GRAY);
        }
    }
}

void menu_draw_bar(void)
{
    /* Menu bar rectangle (0, 0, 512, 12) */
    vdp_fill_rect(0, 0, SCREEN_WIDTH, 12, BTRON_COL_LIGHT_GRAY);
    vdp_draw_hline(0, 12, SCREEN_WIDTH, BTRON_COL_DARK_GRAY);
    vdp_wait_cmd();

    for (uint8_t i = 0; i < 6; i++) {
        draw_root_item(i);
    }

    bool turbo = msx_is_turbo_r();
    const char *cpu_str = turbo ? "[R800 TURBO 28MHz]" : "[Z80 3.58MHz]";
    font_draw_string(360, 2, cpu_str, BTRON_COL_TITLE_BLUE, BTRON_COL_LIGHT_GRAY);

    draw_popup();
}

void menu_close_popup(void)
{
    if (s_open_menu_idx >= 0) {
        uint8_t old_idx = (uint8_t)s_open_menu_idx;
        popup_bg_xfer(s_open_menu_idx, false);
        s_open_menu_idx = -1;
        uint8_t prev_page = vdp_get_draw_page();
        vdp_set_draw_page(0);
        draw_root_item(old_idx);
        vdp_wait_cmd();
        vdp_set_draw_page(prev_page);
    }
}

bool menu_is_open(void)
{
    return (s_open_menu_idx >= 0);
}

static void activate_or_open_window(ID wid)
{
    Window *win = wm_get_window(wid);
    if (!win) return;
    if (win->state != WIN_STATE_NORMAL) {
        win->state = WIN_STATE_NORMAL;
        wm_set_active_window(wid);
        wm_mark_window_dirty(wid);
    } else {
        wm_focus_window(wid);
    }
}

bool menu_handle_click(uint16_t x, uint16_t y)
{
    /* If click is on the top bar */
    if (y <= 12) {
        for (uint8_t i = 0; i < 6; i++) {
            if (x >= s_root_menus[i].x - 2 && x <= s_root_menus[i].x + s_root_menus[i].w + 2) {
                int8_t prev_open = s_open_menu_idx;
                if (prev_open >= 0) {
                    popup_bg_xfer(prev_open, false);
                }
                if (prev_open == (int8_t)i) {
                    s_open_menu_idx = -1; /* Toggle close */
                } else {
                    s_open_menu_idx = (int8_t)i;
                    popup_bg_xfer(s_open_menu_idx, true);
                }
                uint8_t prev_page = vdp_get_draw_page();
                vdp_set_draw_page(0);
                if (prev_open >= 0 && prev_open != (int8_t)i) {
                    draw_root_item((uint8_t)prev_open);
                }
                draw_root_item(i);
                draw_popup();
                vdp_wait_cmd();
                vdp_set_draw_page(prev_page);
                return true;
            }
        }
        menu_close_popup();
        return true;
    }

    /* If menu dropdown is open and clicked inside */
    if (s_open_menu_idx >= 0 && s_open_menu_idx < 6) {
        int8_t menu_idx = s_open_menu_idx;
        const RootMenuItem *m = &s_root_menus[menu_idx];
        uint16_t popup_x = (uint16_t)((m->x - 2u) & (uint16_t)~1u);
        uint16_t popup_y = 13;
        uint16_t popup_w = 140;
        uint16_t popup_h = (uint16_t)(m->item_count * 14 + 6);

        if (popup_x + popup_w > SCREEN_WIDTH) {
            popup_x = (uint16_t)((SCREEN_WIDTH - popup_w - 2u) & (uint16_t)~1u);
        }

        /* Restore popup background first */
        menu_close_popup();

        if (x >= popup_x && x <= popup_x + popup_w &&
            y >= popup_y && y <= popup_y + popup_h) {

            uint8_t selected_item = (uint8_t)((y - (popup_y + 3)) / 14);
            if (selected_item < m->item_count) {
                /* Execute menu selection */
                if (menu_idx == 0) {
                    /* TRON Menu */
                    if (selected_item == 0) {
                        activate_or_open_window(2);
                    } else if (selected_item == 3) {
                        activate_or_open_window(4);
                    } else if (selected_item == 4) {
                        msx_reboot();
                    }
                } else if (menu_idx == 1) {
                    /* File Menu */
                    if (selected_item == 0 || selected_item == 1) {
                        activate_or_open_window(1);
                    } else if (selected_item == 3) {
                        Window *act = wm_get_active_window();
                        if (act) {
                            act->state = WIN_STATE_CLOSED;
                            wm_mark_dirty();
                        }
                    }
                } else if (menu_idx == 3) {
                    /* View Menu: Open Windows */
                    activate_or_open_window((ID)(selected_item + 1));
                } else if (menu_idx == 4) {
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
                    wm_mark_dirty();
                } else if (menu_idx == 5) {
                    /* Help Menu: Switch document in Editor */
                    if (selected_item == 0) {
                        app_editor_load_body(1);
                    } else if (selected_item == 1) {
                        app_editor_load_body(2);
                    }
                    activate_or_open_window(selected_item == 2 ? 3 : 1);
                }
            }
            return true;
        }

        return true;
    }

    return false;
}

