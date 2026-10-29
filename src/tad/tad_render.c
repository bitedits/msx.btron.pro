/*
 * tad_render.c — TAD Stream Renderer implementation.
 */

#include "tad_render.h"
#include "../msxgl/vdp.h"
#include "../font/font_latin.h"
#include "../font/font_kanji.h"
#include "../font/font_tibetan.h"
#include <stddef.h>

void tad_render_init(void)
{
    /* Ready */
}

ID tad_render_document(uint16_t win_x, uint16_t win_y, uint16_t win_w, uint16_t win_h,
                       uint16_t scroll_y, const uint8_t *stream, uint16_t stream_len,
                       uint16_t click_x, uint16_t click_y, bool is_click)
{
    if (!stream || stream_len == 0) return 0;

    TAD_Reader reader;
    tad_reader_init(&reader, stream, stream_len);

    uint16_t cur_x = win_x + 8;
    int16_t  cur_y = (int16_t)(win_y + 8) - (int16_t)scroll_y;
    uint16_t max_x = win_x + win_w - 12;
    int16_t  max_y = (int16_t)(win_y + win_h - 12);

    ID clicked_link_id = 0;
    TAD_Segment seg;

    while (tad_read_next_segment(&reader, &seg)) {
        if (cur_y > max_y) {
            break;
        }

        if (seg.type == TAD_SEG_NEWLINE) {
            cur_x = win_x + 8;
            cur_y += 18;
            continue;
        }

        if (seg.type == TAD_SEG_VECTOR) {
            if (cur_y >= (int16_t)win_y && cur_y + 4 <= max_y) {
                vdp_draw_hline(win_x + 6, (uint16_t)cur_y, win_w - 12, seg.fg_color);
            }
            cur_y += 6;
            cur_x = win_x + 8;
            continue;
        }

        if (seg.type == TAD_SEG_TEXT) {
            if (cur_y >= (int16_t)win_y && cur_y + 16 <= max_y) {
                if (seg.lang == TAD_LANG_TIBETAN) {
                    uint16_t w = font_render_tibetan_utf8(cur_x, (uint16_t)cur_y, seg.text, seg.fg_color, seg.bg_color);
                    cur_x += w + 4;
                } else if (seg.lang == TAD_LANG_KANJI) {
                    /* Read sequence of 8-bit Kanji IDs */
                    const uint8_t *kp = (const uint8_t *)seg.text;
                    for (UH k = 0; k < seg.length; k++) {
                        font_draw_kanji(cur_x, (uint16_t)cur_y, kp[k], seg.fg_color, seg.bg_color);
                        cur_x += 18;
                        if (cur_x + 18 > max_x) {
                            cur_x = win_x + 8;
                            cur_y += 18;
                        }
                    }
                } else {
                    font_draw_string(cur_x, (uint16_t)cur_y, seg.text, seg.fg_color, seg.bg_color);
                    cur_x += font_get_string_width(seg.text) + 4;
                }
            }
            if (cur_x + 24 > max_x) {
                cur_x = win_x + 8;
                cur_y += 18;
            }
            continue;
        }

        if (seg.type == TAD_SEG_LINK) {
            uint16_t label_w = font_get_string_width(seg.text) + 24;
            uint16_t link_h = 14;

            if (cur_x + label_w > max_x) {
                cur_x = win_x + 8;
                cur_y += 18;
            }

            if (cur_y >= (int16_t)win_y && cur_y + link_h <= max_y) {
                /* Draw Virtual Body Bevel Link Button */
                vdp_draw_bevel_box(cur_x, (uint16_t)cur_y, label_w, link_h,
                                   BTRON_COL_WHITE, BTRON_COL_DARK_GRAY, BTRON_COL_LINK_CYAN);
                
                /* Draw small link symbol "[>] " */
                font_draw_string(cur_x + 4, (uint16_t)cur_y + 3, ">", BTRON_COL_BLACK, BTRON_COL_LINK_CYAN);
                font_draw_string(cur_x + 14, (uint16_t)cur_y + 3, seg.text, BTRON_COL_BLACK, BTRON_COL_LINK_CYAN);

                /* Check for click hit */
                if (is_click) {
                    if (click_x >= cur_x && click_x <= cur_x + label_w &&
                        click_y >= (uint16_t)cur_y && click_y <= (uint16_t)cur_y + link_h) {
                        clicked_link_id = seg.target_body_id;
                    }
                }
            }
            cur_x += label_w + 6;
            continue;
        }
    }

    return clicked_link_id;
}

