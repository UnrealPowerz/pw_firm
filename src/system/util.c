#include "all_headers.h"

// ROM: 0x25ac  98.8%
void sys_delay_short(void) {
  uint16_t i;
  if (statusFlags_BIT.lcd_dirty) {
    i = 0x25;
    do {
      nop();
      nop();
      nop();
      nop();
      nop();
    } while (--i);
  }
}

// ROM: 0x25c8  65.0%
void sys_seed_rng(uint32_t seed) { nextRandom = seed; }

// ROM: 0x25d0  47.2%
uint32_t sys_get_rng(void) {
  nextRandom = nextRandom * 1664525 + 1013904223;
  return nextRandom;
}

// ROM: 0x25f6  49.8%
void sys_lzss_decode(uint8_t *src, uint8_t *dst) {
  uint8_t count;
  uint8_t flags;
  int8_t bits;
  uint8_t len;
  int16_t dist;
  uint8_t *s_ptr = src;
  uint8_t *d_ptr = dst;

  count = src[1];
  s_ptr += 4;

  while (count != 0) {
    flags = *s_ptr++;
    for (bits = 8; bits > 0; bits--) {
      if (!(flags & 0x80)) {
        *d_ptr++ = *s_ptr++;
        if (--count == 0)
          return;
      } else {
        uint8_t b = *s_ptr;
        len = (b >> 4) + 3;
        dist = (int16_t)(((uint16_t)(b & 0x0F) << 8) | s_ptr[1]) + 1;
        s_ptr += 2;

        count -= len;
        while (len--) {
          *d_ptr = *(d_ptr - dist);
          d_ptr++;
        }
        if (count == 0)
          return;
      }
      flags <<= 1;
    }
  }
}
