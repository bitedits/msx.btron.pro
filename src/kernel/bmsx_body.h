/*
 * bmsx_body.h — BTRON Real Body & Virtual Body Hyperdata Storage Model.
 */

#ifndef _BMSX_BODY_H_
#define _BMSX_BODY_H_

#include "btron_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BODY_TYPE_NONE   = 0,
    BODY_TYPE_TAD    = 1,   /* TRON Application Databus (TAD) rich text/vector */
    BODY_TYPE_TEXT   = 2,   /* Plain text */
    BODY_TYPE_FOLDER = 3,   /* Directory/folder of virtual bodies */
    BODY_TYPE_EXEC   = 4    /* Executable app */
} BodyType;

typedef struct {
    ID       body_id;
    BodyType type;
    UB       bank;          /* MegaROM bank containing content */
    const UB *data;         /* Memory pointer within bank */
    UH       size;          /* Byte size */
    char     title[32];     /* Human readable title */
} BMSX_RealBody;

typedef struct {
    ID       link_id;       /* Virtual body link identifier */
    ID       target_body_id;/* ID of target Real Body */
    char     label[24];     /* Display anchor label */
    UB       icon_id;       /* Icon type: 0=doc, 1=folder, 2=exec */
} BMSX_VirtBody;

void bmsx_body_init(void);
ER   bmsx_register_real_body(ID id, BodyType type, UB bank, const UB *data, UH size, const char *title);
const BMSX_RealBody *bmsx_get_real_body(ID id);
UH   bmsx_get_real_body_count(void);
const BMSX_RealBody *bmsx_get_body_by_index(UH index);

#ifdef __cplusplus
}
#endif

#endif /* _BMSX_BODY_H_ */

