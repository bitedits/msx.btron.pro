/*
 * tad_render.h — TAD Stream Renderer for BTRON Window System.
 */

#ifndef _TAD_RENDER_H_
#define _TAD_RENDER_H_

#include "tad_segment.h"

#ifdef __cplusplus
extern "C" {
#endif

void tad_render_init(void);

/*
 * Renders TAD stream inside window content rect.
 * If is_click is true and a Virtual Body link was clicked, returns the target Real Body ID.
 * Returns 0 if no link was clicked.
 */
ID tad_render_document(uint16_t win_x, uint16_t win_y, uint16_t win_w, uint16_t win_h,
                       uint16_t scroll_y, const uint8_t *stream, uint16_t stream_len,
                       uint16_t click_x, uint16_t click_y, bool is_click);

#ifdef __cplusplus
}
#endif

#endif /* _TAD_RENDER_H_ */

