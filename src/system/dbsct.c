#include "all_headers.h"

/* The original ch38 DTBL/BTBL + lib3hn _INITSCT pipeline doesn't work in
 * this build: __sectop/__secend relocations resolve to garbage and the
 * lib's _INITSCT has stale section-bound literals. Bypass it entirely.
 *
 * Section layout (see Makefile -start= options):
 *   R section: 0xFE00..0xFE01 (2 bytes, initialized from D at boot)
 *   No B section (heap/brk live at pinned addresses 0xF8F0/0xF7BE — see sbrk.c).
 *
 * R contains save_read_reliable's `uint8_t checksums[2] = {1, 1}` initializer.
 * We placed R at 0xFE00 so it doesn't collide with the pinned session_save
 * struct at 0xF780, which resetprg.c zeroes on boot. */
#pragma section P
void _INITSCT(void) {
  *(volatile uint8_t *)0xFE00 = 0x01;
  *(volatile uint8_t *)0xFE01 = 0x01;
}
