/*
 * test_tad_parser.c — Unit tests for TAD (TRON Application Databox) segment parser.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "../../src/tad/tad_segment.h"
#include "../../src/data/doc_heart_sutra.h"
#include "../../src/data/doc_nortia_letter.h"

#define TEST_PASS(msg) printf("  [PASS] %s\n", msg)
#define TEST_FAIL(msg) do { fprintf(stderr, "  [FAIL] %s\n", msg); return 1; } while(0)

int main(void)
{
    printf("=== Test: TAD Binary Segment Parser ===\n");

    /* 1. Synthetic TAD Stream Test */
    static const uint8_t synthetic_tad[] = {
        /* Text segment */
        TAD_SEG_TEXT, TAD_LANG_LATIN, 0x02, 0x09, 5, 0, 'H', 'e', 'l', 'l', 'o',
        /* Newline */
        TAD_SEG_NEWLINE,
        /* Link segment: target_id=1 (2 bytes: 0x01, 0x00), length=4 (1 byte: 4), text="Link" */
        TAD_SEG_LINK, 0x01, 0x00, 4, 'L', 'i', 'n', 'k',
        /* Vector line */
        TAD_SEG_VECTOR, 0x02, 10, 0, 20, 0, 100, 0, 20, 0, 0,
        /* End marker */
        TAD_SEG_END
    };

    TAD_Reader reader;
    tad_reader_init(&reader, synthetic_tad, sizeof(synthetic_tad));

    TAD_Segment seg;
    /* Segment 1: Text */
    if (!tad_read_next_segment(&reader, &seg) || seg.type != TAD_SEG_TEXT) {
        TEST_FAIL("Expected text segment");
    }
    if (seg.lang != TAD_LANG_LATIN || seg.fg_color != 0x02 || seg.bg_color != 0x09 || seg.length != 5) {
        TEST_FAIL("Text segment attributes mismatch");
    }
    if (strncmp(seg.text, "Hello", 5) != 0) {
        TEST_FAIL("Text payload mismatch");
    }
    TEST_PASS("TAD text segment parsing");

    /* Segment 2: Newline */
    if (!tad_read_next_segment(&reader, &seg) || seg.type != TAD_SEG_NEWLINE) {
        TEST_FAIL("Expected newline segment");
    }
    TEST_PASS("TAD newline segment parsing");

    /* Segment 3: Link */
    if (!tad_read_next_segment(&reader, &seg) || seg.type != TAD_SEG_LINK) {
        TEST_FAIL("Expected link segment");
    }
    if (seg.target_body_id != 0x01 || seg.length != 4 || strncmp(seg.text, "Link", 4) != 0) {
        TEST_FAIL("Link segment payload mismatch");
    }
    TEST_PASS("TAD virtual body link segment parsing");

    /* Segment 4: Vector */
    if (!tad_read_next_segment(&reader, &seg) || seg.type != TAD_SEG_VECTOR) {
        TEST_FAIL("Expected vector segment");
    }
    if (seg.x1 != 10 || seg.y1 != 20 || seg.x2 != 100 || seg.y2 != 20) {
        TEST_FAIL("Vector coordinates mismatch");
    }
    TEST_PASS("TAD vector line segment parsing");

    /* Segment 5: End */
    if (tad_read_next_segment(&reader, &seg)) {
        TEST_FAIL("Reader did not terminate on TAD_SEG_END");
    }
    TEST_PASS("TAD end marker termination");

    /* 2. Verification of Heart Sutra TAD Document */
    tad_reader_init(&reader, g_doc_heart_sutra_tad, g_doc_heart_sutra_tad_len);
    int seg_count = 0;
    while (tad_read_next_segment(&reader, &seg)) {
        seg_count++;
    }
    if (seg_count < 4) {
        fprintf(stderr, "Heart Sutra segment count = %d, expected >= 4\n", seg_count);
        TEST_FAIL("Heart Sutra document segment count");
    }
    TEST_PASS("Heart Sutra TAD document integrity verified");

    /* 3. Verification of NORTIA Letter TAD Document */
    tad_reader_init(&reader, g_doc_nortia_letter_tad, g_doc_nortia_letter_tad_len);
    seg_count = 0;
    while (tad_read_next_segment(&reader, &seg)) {
        seg_count++;
    }
    if (seg_count < 4) {
        fprintf(stderr, "NORTIA Letter segment count = %d, expected >= 4\n", seg_count);
        TEST_FAIL("NORTIA Letter document segment count");
    }
    TEST_PASS("NORTIA Letter TAD document integrity verified");

    /* 4. Truncated & Corrupted Stream Handling */
    static const uint8_t truncated_stream[] = { TAD_SEG_TEXT, TAD_LANG_LATIN, 0x01, 0x02, 50, 0 }; /* claims 50 bytes, ends early */
    tad_reader_init(&reader, truncated_stream, sizeof(truncated_stream));
    if (tad_read_next_segment(&reader, &seg)) {
        TEST_FAIL("Parser did not reject truncated text payload");
    }
    TEST_PASS("Truncated payload bounds safety");

    printf("=== All TAD Parser Tests PASSED ===\n\n");
    return 0;
}
