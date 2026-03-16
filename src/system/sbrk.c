#include "all_headers.h"

#define HEAPSIZE 2 // actually 0x400

#pragma pack 2
static union {
  uint16_t dummy;
  uint8_t heap[HEAPSIZE];
} heap_area;
#pragma unpack

static uint8_t *brk = (uint8_t *)&heap_area;

// ROM: 0x247e  96.7%
void sys_init_heap(void) { brk = (uint8_t *)&heap_area; }

// ROM: 0x2488  71.6%
#pragma option speed =register /* pragma:auto */
uint8_t *sbrk(size_t size) {
  uint8_t *next_brk;

  next_brk = brk + size;
  brk = next_brk;

  if (next_brk - &heap_area.heap[0] > HEAPSIZE) {
    sleep();
  }

  return next_brk;
}
