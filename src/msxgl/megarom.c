/*
 * megarom.c — ASCII16 MegaROM Mapper implementation.
 */

#include "megarom.h"

static uint8_t s_current_bank1 = 0;
static uint8_t s_current_bank2 = 0;

void megarom_set_bank_page1(uint8_t bank)
{
    volatile uint8_t *reg = (volatile uint8_t *)ASCII16_BANK1_REG;
    *reg = bank;
    s_current_bank1 = bank;
}

void megarom_set_bank_page2(uint8_t bank)
{
    volatile uint8_t *reg = (volatile uint8_t *)ASCII16_BANK2_REG;
    *reg = bank;
    s_current_bank2 = bank;
}

uint8_t megarom_get_bank_page1(void)
{
    return s_current_bank1;
}

uint8_t megarom_get_bank_page2(void)
{
    return s_current_bank2;
}

