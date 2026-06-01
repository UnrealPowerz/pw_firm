#include "all_headers.h"

/* Heap and brk pointer addresses are fixed to match the original ROM layout
 * (heap at 0xF8F0, brk pointer at 0xF7BE). Using static variables would put
 * them in the B/R sections at 0xF780+, which collides with our pinned
 * session_save struct.
 *
 * The R section's brk-init mechanism is wired up in dbsct.c::_INITSCT so
 * brk starts pointing at heap_area. */
#define HEAP_AREA ((uint8_t *)0xF8F0u)
#define BRK_PTR   (*(uint8_t **)0xF7BEu)

// ROM: 0x247e  99.7%
void sys_init_heap(void) { BRK_PTR = HEAP_AREA; }

// ROM: 0x2488  58.0%  saves: e5
uint8_t *sbrk(size_t size) {
  uint8_t *old_brk = BRK_PTR;

  BRK_PTR = old_brk + size;

  if (BRK_PTR - HEAP_AREA > 0x400) {
    sleep();
  }

  return old_brk;
}
