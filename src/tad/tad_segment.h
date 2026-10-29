/*
 * tad_segment.h — TRON Application Databus (TAD) 8-bit Segment Specification.
 */

#ifndef _TAD_SEGMENT_H_
#define _TAD_SEGMENT_H_

#include "../kernel/btron_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TAD Segment Identifiers */
#define TAD_SEG_TEXT        0x01    /* Text stream */
#define TAD_SEG_LINK        0x02    /* Virtual Body link anchor */
#define TAD_SEG_VECTOR      0x03    /* Vector graphics primitive */
#define TAD_SEG_STYLE       0x04    /* Color & style change */
#define TAD_SEG_NEWLINE     0x05    /* Paragraph break */
#define TAD_SEG_END         0xFF    /* Stream terminator */

/* Language codes for TAD_SEG_TEXT */
#define TAD_LANG_LATIN      0x00
#define TAD_LANG_KANJI      0x01
#define TAD_LANG_TIBETAN    0x02

typedef struct {
    UB type;
    UB lang;
    UB fg_color;
    UB bg_color;
    ID target_body_id;  /* For TAD_SEG_LINK */
    const char *text;
    UH length;
    /* Vector parameters */
    H  x1, y1, x2, y2;
} TAD_Segment;

typedef struct {
    const UB *stream;
    UH       offset;
    UH       total_len;
} TAD_Reader;

void tad_reader_init(TAD_Reader *reader, const UB *stream, UH len);
bool tad_read_next_segment(TAD_Reader *reader, TAD_Segment *seg);

#ifdef __cplusplus
}
#endif

#endif /* _TAD_SEGMENT_H_ */

