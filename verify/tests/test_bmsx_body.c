/*
 * test_bmsx_body.c — Unit tests for BTRON Real Body / Virtual Body Storage.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "../../src/kernel/bmsx_body.h"

#define TEST_PASS(msg) printf("  [PASS] %s\n", msg)
#define TEST_FAIL(msg) do { fprintf(stderr, "  [FAIL] %s\n", msg); return 1; } while(0)

static const uint8_t s_dummy_payload[64] = { 0x01, 0x02, 0x03, 0x04 };

int main(void)
{
    printf("=== Test: B-MSX Real Body / Virtual Body Storage ===\n");

    /* 1. Initialization */
    bmsx_body_init();
    if (bmsx_get_real_body_count() != 0) {
        TEST_FAIL("Initial body count non-zero");
    }
    TEST_PASS("Real body registry initialization");

    /* 2. Registration */
    ER err = bmsx_register_real_body(1, BODY_TYPE_TAD, 2, s_dummy_payload, sizeof(s_dummy_payload), "Heart Sutra");
    if (err != E_OK) TEST_FAIL("Body registration ID 1");

    err = bmsx_register_real_body(2, BODY_TYPE_EXEC, 3, NULL, 0, "Terminal");
    if (err != E_OK) TEST_FAIL("Body registration ID 2");

    if (bmsx_get_real_body_count() != 2) {
        TEST_FAIL("Body count mismatch after registration");
    }
    TEST_PASS("Body registration count");

    /* 3. Lookup by ID */
    const BMSX_RealBody *b1 = bmsx_get_real_body(1);
    if (!b1 || b1->type != BODY_TYPE_TAD || b1->bank != 2 || strcmp(b1->title, "Heart Sutra") != 0) {
        TEST_FAIL("Lookup ID 1 content mismatch");
    }

    const BMSX_RealBody *b2 = bmsx_get_real_body(2);
    if (!b2 || b2->type != BODY_TYPE_EXEC || b2->bank != 3 || strcmp(b2->title, "Terminal") != 0) {
        TEST_FAIL("Lookup ID 2 content mismatch");
    }

    const BMSX_RealBody *b_none = bmsx_get_real_body(999);
    if (b_none != NULL) {
        TEST_FAIL("Lookup non-existent body did not return NULL");
    }
    TEST_PASS("Lookup by ID accuracy");

    /* 4. Lookup by Title */
    const BMSX_RealBody *b_title = bmsx_find_body_by_title("Heart Sutra");
    if (!b_title || b_title->body_id != 1) {
        TEST_FAIL("Lookup by title failed");
    }
    TEST_PASS("Lookup by title accuracy");

    /* 5. Boundary: Exceeding MAX_REAL_BODIES */
    for (int i = 3; i <= MAX_REAL_BODIES; i++) {
        bmsx_register_real_body((ID)i, BODY_TYPE_TEXT, 1, NULL, 0, "Extra");
    }
    err = bmsx_register_real_body(MAX_REAL_BODIES + 1, BODY_TYPE_TEXT, 1, NULL, 0, "Overflow");
    if (err != E_LIMIT) {
        TEST_FAIL("Allowed registering past MAX_REAL_BODIES");
    }
    TEST_PASS("Registry capacity boundary enforcement");

    printf("=== All B-MSX Body Storage Tests PASSED ===\n\n");
    return 0;
}
