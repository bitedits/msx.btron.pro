/*
 * tad_segment.c — TAD 8-bit Segment Parser implementation.
 */

#include "tad_segment.h"
#include <stddef.h>

void tad_reader_init(TAD_Reader *reader, const UB *stream, UH len)
{
    if (!reader) return;
    reader->stream = stream;
    reader->offset = 0;
    reader->total_len = len;
}

bool tad_read_next_segment(TAD_Reader *reader, TAD_Segment *seg)
{
    if (!reader || !seg || !reader->stream) return false;
    if (reader->offset >= reader->total_len) return false;

    const UB *p = reader->stream + reader->offset;
    UB type = *p++;
    reader->offset++;

    seg->type = type;
    seg->lang = TAD_LANG_LATIN;
    seg->fg_color = 0;
    seg->bg_color = 0;
    seg->target_body_id = 0;
    seg->text = NULL;
    seg->length = 0;
    seg->x1 = seg->y1 = seg->x2 = seg->y2 = 0;

    if (type == TAD_SEG_END) {
        return false;
    }

    if (type == TAD_SEG_TEXT) {
        if (reader->offset + 4 > reader->total_len) return false;
        seg->lang = *p++;
        seg->fg_color = *p++;
        seg->bg_color = *p++;
        UH len = *p++;
        len |= (UH)(*p++) << 8;
        reader->offset += 5;

        if (reader->offset + len > reader->total_len) return false;
        seg->text = (const char *)p;
        seg->length = len;
        reader->offset += len;
        return true;
    }

    if (type == TAD_SEG_LINK) {
        if (reader->offset + 3 > reader->total_len) return false;
        seg->target_body_id = (ID)(*p++);
        seg->target_body_id |= (ID)((*p++) << 8);
        UH len = *p++;
        reader->offset += 3;

        if (reader->offset + len > reader->total_len) return false;
        seg->text = (const char *)p;
        seg->length = len;
        reader->offset += len;
        return true;
    }

    if (type == TAD_SEG_VECTOR) {
        if (reader->offset + 9 > reader->total_len) return false;
        seg->fg_color = *p++;
        seg->x1 = (H)(*p++); seg->x1 |= (H)((*p++) << 8);
        seg->y1 = (H)(*p++); seg->y1 |= (H)((*p++) << 8);
        seg->x2 = (H)(*p++); seg->x2 |= (H)((*p++) << 8);
        seg->y2 = (H)(*p++); seg->y2 |= (H)((*p++) << 8);
        reader->offset += 9;
        return true;
    }

    if (type == TAD_SEG_NEWLINE) {
        return true;
    }

    return false;
}

