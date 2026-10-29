/*
 * doc_nortia_letter.c — NORTIA Order TAD Letter binary representation.
 */

#include "doc_nortia_letter.h"
#include "../tad/tad_segment.h"
#include "../msxgl/vdp.h"
#include "../font/font_kanji.h"

const uint8_t g_doc_nortia_letter_tad[] = {
    /* 1. Header: 件名：報告 (Ken, Mei, Hou, Koku) */
    TAD_SEG_TEXT, TAD_LANG_KANJI, BTRON_COL_TITLE_BLUE, BTRON_COL_WHITE,
    4, 0,
    KANJI_KEN, KANJI_MEI, KANJI_HOU, KANJI_KOKU,

    TAD_SEG_NEWLINE,

    /* 2. Subtitle: NORTIA Order / TAD Working Group */
    TAD_SEG_TEXT, TAD_LANG_LATIN, BTRON_COL_DARK_GRAY, BTRON_COL_WHITE,
    33, 0,
    'T','o',':',' ','N','O','R','T','I','A',' ','T','A','D',' ','W','o','r','k','i',
    'n','g',' ','G','r','o','u','p',0,

    TAD_SEG_NEWLINE,

    /* 3. Vector line */
    TAD_SEG_VECTOR, BTRON_COL_TITLE_BLUE,
    0, 0, 0, 0, 0, 0, 0, 0, 0,

    TAD_SEG_NEWLINE,

    /* 4. Letter Body Paragraph 1 */
    TAD_SEG_TEXT, TAD_LANG_LATIN, BTRON_COL_BLACK, BTRON_COL_WHITE,
    46, 0,
    'B','T','R','O','N','3',' ','E','n','v','i','r','o','n','m','e','n','t',' ','f',
    'o','r',' ','M','S','X',' ','t','u','r','b','o',' ','R',' ','(','8','-','b','i',
    't',' ','O','S',')',0,

    TAD_SEG_NEWLINE,

    /* 5. Feature List */
    TAD_SEG_TEXT, TAD_LANG_LATIN, BTRON_COL_CHARCOAL, BTRON_COL_WHITE,
    38, 0,
    '-',' ','A','S','C','I','I',' ','R','8','0','0',' ','T','u','r','b','o',' ','C',
    'P','U',' ','(','2','8','.','6',' ','M','H','z',')',0,

    TAD_SEG_NEWLINE,

    TAD_SEG_TEXT, TAD_LANG_LATIN, BTRON_COL_CHARCOAL, BTRON_COL_WHITE,
    37, 0,
    '-',' ','V','9','9','5','8',' ','S','c','r','e','e','n',' ','7',' ','(','5','1',
    '2','x','2','1','2','x','1','6','c','o','l',')',0,

    TAD_SEG_NEWLINE,

    TAD_SEG_TEXT, TAD_LANG_LATIN, BTRON_COL_CHARCOAL, BTRON_COL_WHITE,
    35, 0,
    '-',' ','R','e','a','l','/','V','i','r','t',' ','B','o','d','y',' ','H','y','p',
    'e','r','d','a','t','a',' ','M','o','d','e','l',0,

    TAD_SEG_NEWLINE,

    TAD_SEG_TEXT, TAD_LANG_LATIN, BTRON_COL_CHARCOAL, BTRON_COL_WHITE,
    38, 0,
    '-',' ','m','I','T','R','O','N',' ','R','e','a','l','-','T','i','m','e',' ','T',
    'a','s','k',' ','K','e','r','n','e','l',0,

    TAD_SEG_NEWLINE,

    TAD_SEG_TEXT, TAD_LANG_LATIN, BTRON_COL_CHARCOAL, BTRON_COL_WHITE,
    38, 0,
    '-',' ','M','u','l','t','i','l','i','n','g','u','a','l',':',' ','K','a','n','j',
    'i',' ','&',' ','T','i','b','e','t','a','n',0,

    TAD_SEG_NEWLINE,

    /* 6. Vector line */
    TAD_SEG_VECTOR, BTRON_COL_MED_GRAY,
    0, 0, 0, 0, 0, 0, 0, 0, 0,

    TAD_SEG_NEWLINE,

    /* 7. Hypermedia Links */
    TAD_SEG_LINK,
    1, 0,   /* target Real Body ID: 1 (Heart Sutra) */
    17,
    'H','e','a','r','t',' ','S','u','t','r','a',' ','(','T','A','D',')',

    TAD_SEG_NEWLINE,

    TAD_SEG_LINK,
    3, 0,   /* target Real Body ID: 3 (Terminal) */
    15,
    'S','h','e','l','l',' ','T','e','r','m','i','n','a','l',0,

    TAD_SEG_NEWLINE,

    TAD_SEG_LINK,
    4, 0,   /* target Real Body ID: 4 (Cabinet) */
    15,
    'S','t','o','r','a','g','e',' ','C','a','b','i','n','e','t',

    TAD_SEG_NEWLINE,

    TAD_SEG_END
};

const uint16_t g_doc_nortia_letter_tad_len = sizeof(g_doc_nortia_letter_tad);

