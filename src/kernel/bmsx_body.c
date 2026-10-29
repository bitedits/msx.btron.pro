/*
 * bmsx_body.c — BTRON Real Body / Virtual Body Storage implementation.
 */

#include "bmsx_body.h"

static BMSX_RealBody s_real_bodies[MAX_REAL_BODIES];
static UH s_body_count = 0;

void bmsx_body_init(void)
{
    s_body_count = 0;
    for (UH i = 0; i < MAX_REAL_BODIES; i++) {
        s_real_bodies[i].body_id = 0;
        s_real_bodies[i].type = BODY_TYPE_NONE;
        s_real_bodies[i].bank = 0;
        s_real_bodies[i].data = NULL;
        s_real_bodies[i].size = 0;
        s_real_bodies[i].title[0] = '\0';
    }
}

ER bmsx_register_real_body(ID id, BodyType type, UB bank, const UB *data, UH size, const char *title)
{
    if (s_body_count >= MAX_REAL_BODIES || id <= 0) {
        return E_LIMIT;
    }

    UH idx = s_body_count;
    s_real_bodies[idx].body_id = id;
    s_real_bodies[idx].type = type;
    s_real_bodies[idx].bank = bank;
    s_real_bodies[idx].data = data;
    s_real_bodies[idx].size = size;

    UH j = 0;
    if (title) {
        while (title[j] != '\0' && j < 31) {
            s_real_bodies[idx].title[j] = title[j];
            j++;
        }
    }
    s_real_bodies[idx].title[j] = '\0';
    s_body_count++;

    return E_OK;
}

const BMSX_RealBody *bmsx_get_real_body(ID id)
{
    for (UH i = 0; i < s_body_count; i++) {
        if (s_real_bodies[i].body_id == id) {
            return &s_real_bodies[i];
        }
    }
    return NULL;
}

UH bmsx_get_real_body_count(void)
{
    return s_body_count;
}

const BMSX_RealBody *bmsx_get_body_by_index(UH index)
{
    if (index >= s_body_count) {
        return NULL;
    }
    return &s_real_bodies[index];
}

