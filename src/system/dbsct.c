#include "all_headers.h"

/* The original ch38 DTBL/BTBL + lib3hn _INITSCT pipeline doesn't work in
 * this build: __sectop/__secend relocations resolve to garbage and the
 * lib's _INITSCT has stale section-bound literals. Bypass it entirely.
 *
 * Section layout (see Makefile -start= options):
 *   B_RAM: 0xF780..0xFEEF (struct g)
 *   R section: 0xFEF0..0xFEF1 (2 bytes, initialized from D at boot)
 *   No B section (heap/brk live at pinned addresses 0xF8F0/0xF7BE — see sbrk.c).
 *
 * R contains save_read_reliable's `uint8_t checksums[2] = {1, 1}` initializer.
 * Address must match the linker's -start=B,R/FEF0 setting in the Makefile. */
#pragma section P
void _INITSCT(void) {
  *(volatile uint8_t *)0xFEF0 = 0x01;
  *(volatile uint8_t *)0xFEF1 = 0x01;
}
