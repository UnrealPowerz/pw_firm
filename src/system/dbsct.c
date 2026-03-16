#include "all_headers.h"

#pragma section $DSEC
struct {
  uint8_t *rom_s; /* Start address of the initialized data section in ROM */
  uint8_t *rom_e; /* End address of the initialized data section in ROM   */
  uint8_t *ram_s; /* Start address of the initialized data section in RAM */
} DTBL[] = {
    {__sectop("D"), __secend("D"), __sectop("R")},
};

#pragma section $BSEC
struct {
  uint8_t *b_s; /* Start address of non-initialized data section */
  uint8_t *b_e; /* End address of non-initialized data section */
} BTBL[] = {
    {__sectop("B"), __secend("B")},
};
