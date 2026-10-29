/*
 * test_window_manager.c — Verification test suite for BTRON Window Stacking,
 * Activation, Title-bar Dragging, Zooming, and Close controls.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "gui/window.h"

#define TEST_PASS(msg) printf("  [PASS] %s\n", msg)
#define TEST_FAIL(msg) do { fprintf(stderr, "  [FAIL] %s\n", msg); return 1; } while(0)

/* Mock hardware / font stubs */
static uint8_t s_mock_draw_page = 0;
static uint16_t s_last_copy_dx = 0, s_last_copy_dy = 0;
static uint16_t s_restore_count = 0;

uint8_t vdp_get_draw_page(void) { return s_mock_draw_page; }
void vdp_set_draw_page(uint8_t page) { s_mock_draw_page = page; }
void vdp_wait_cmd(void) {}
void vdp_copy_rect(uint16_t sx, uint16_t sy, uint16_t dx, uint16_t dy, uint16_t w, uint16_t h)
{
    (void)sx; (void)sy; (void)w; (void)h;
    s_last_copy_dx = dx;
    s_last_copy_dy = dy;
}
void vdp_restore_underlay_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    (void)x; (void)y; (void)w; (void)h;
    s_restore_count++;
}
void vdp_save_underlay_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    (void)x; (void)y; (void)w; (void)h;
}
void vdp_swap_pages_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    (void)x; (void)y; (void)w; (void)h;
}
void vdp_present_backbuffer(void) {}
void vdp_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t c)
{
    (void)x; (void)y; (void)w; (void)h; (void)c;
}
void vdp_draw_bevel_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t hi, uint8_t sh, uint8_t bg)
{
    (void)x; (void)y; (void)w; (void)h; (void)hi; (void)sh; (void)bg;
}
void font_draw_string(uint16_t x, uint16_t y, const char *s, uint8_t fg, uint8_t bg)
{
    (void)x; (void)y; (void)s; (void)fg; (void)bg;
}

static void dummy_paint(Window *w) { (void)w; }
static void dummy_click(Window *w, uint16_t lx, uint16_t ly) { (void)w; (void)lx; (void)ly; }

int main(void)
{
    printf("=== Test: BTRON Window Manager & Z-Stack Navigation ===\n");

    /* 1. Initialization */
    wm_init();
    if (wm_get_window_count() != 0) TEST_FAIL("Window count should be 0 on init");
    TEST_PASS("Window manager initialization");

    /* 2. Window Creation */
    Window *w1 = wm_create_window(1, "Editor", 10, 16, 330, 170, dummy_paint, dummy_click);
    Window *w2 = wm_create_window(2, "Terminal", 40, 28, 330, 160, dummy_paint, dummy_click);
    Window *w3 = wm_create_window(3, "Browser", 70, 40, 330, 150, dummy_paint, dummy_click);

    if (!w1 || !w2 || !w3) TEST_FAIL("Failed to create windows");
    if (wm_get_window_count() != 3) TEST_FAIL("Window count mismatch");
    TEST_PASS("Created 3 managed windows");

    /* 3. Initial Active Window & Z-stack ordering */
    wm_set_active_window(2);
    Window *active = wm_get_active_window();
    if (!active || active->win_id != 2) TEST_FAIL("Terminal (ID 2) should be active");
    TEST_PASS("Active window set to Terminal");

    /* 4. Click Detection & Window Promotion */
    /* Click on Editor title bar (x=20, y=20) which is behind Terminal */
    ID drag_win = 0;
    bool clicked = wm_handle_click(20, 20, &drag_win);
    if (!clicked) TEST_FAIL("Click on Editor title bar should succeed");
    if (drag_win != 1) TEST_FAIL("Drag target should be Window 1 (Editor)");
    active = wm_get_active_window();
    if (!active || active->win_id != 1) TEST_FAIL("Window 1 (Editor) should be promoted to active");
    TEST_PASS("Click promoted Editor to active and initiated title-bar drag");

    /* 5. Window Dragging via Hardware BitBlt + Underlay Restore */
    wm_begin_drag(1);
    s_restore_count = 0;
    bool moved = wm_move_window_bitblt(1, 60, 30);
    if (!moved) TEST_FAIL("wm_move_window_bitblt should return true on coordinate change");
    if (w1->x != 60 || w1->y != 30) TEST_FAIL("Window coordinates not updated");
    if (s_last_copy_dx != 60 || s_last_copy_dy != 30) TEST_FAIL("Hardware BitBlt destination mismatch");
    if (s_restore_count != 2) TEST_FAIL("Expected 2 L-shaped underlay strip restores for diagonal move");
    TEST_PASS("Window position updated via hardware BitBlt + Page 1 underlay restore");

    /* 6. Zoom Button Toggle [+] */
    /* Close button is at win->x + win->w - 14 (376). Zoom is at win->x + win->w - 26 (364) */
    uint16_t zoom_x = w1->x + w1->w - 22;
    uint16_t zoom_y = w1->y + 6;
    clicked = wm_handle_click(zoom_x, zoom_y, &drag_win);
    if (!clicked) TEST_FAIL("Click on Zoom button should be handled");
    if (w1->state != WIN_STATE_ZOOMED) TEST_FAIL("Window state should be ZOOMED");
    if (w1->w != 508 || w1->h != 194) TEST_FAIL("Zoomed window should occupy full workspace");
    TEST_PASS("Window zoomed to full screen bounds");

    /* Toggle zoom back to normal */
    zoom_x = w1->x + w1->w - 22;
    zoom_y = w1->y + 6;
    clicked = wm_handle_click(zoom_x, zoom_y, &drag_win);
    if (w1->state != WIN_STATE_NORMAL) TEST_FAIL("Window state should return to NORMAL");
    if (w1->w != w1->orig_w || w1->h != w1->orig_h) TEST_FAIL("Window dimensions should revert");
    TEST_PASS("Window un-zoomed to original bounds");

    /* 7. Close Button [X] */
    uint16_t close_x = w1->x + w1->w - 10;
    uint16_t close_y = w1->y + 6;
    clicked = wm_handle_click(close_x, close_y, &drag_win);
    if (!clicked) TEST_FAIL("Click on Close button should be handled");
    if (w1->state != WIN_STATE_CLOSED) TEST_FAIL("Window state should be CLOSED");
    TEST_PASS("Window closed via [X] title-bar control");

    /* 8. Cascade and Tile Layouts */
    wm_cascade_windows();
    if (w2->x != 40 || w2->y != 28) TEST_FAIL("Cascaded coordinates incorrect for Window 2");
    TEST_PASS("Cascaded window positioning verified");

    wm_tile_windows();
    if (w2->x != 258 || w2->y != 14 || w2->w != 252 || w2->h != 96) TEST_FAIL("Tile coordinates incorrect");
    TEST_PASS("Tiled 2x2 window positioning verified");

    printf("=== All Window Manager Tests PASSED ===\n\n");
    return 0;
}

