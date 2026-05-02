#include "all_headers.h"

// ROM: 0x69be  73.3%
void gfx_draw_home_pokemon(uint8_t x, uint8_t y) {
  uint8_t *ptr;
  uint16_t addr;
  sys_init_heap();
  ptr = sbrk(0x300);
  drv_eeprom_read_block(0x8F00, ptr, 0x10);
  if (gCurSubstateZ != 0) {
    addr = 0x933E;
  } else {
    addr = (uint16_t)((DAT_f7ac >> 1) & 1) * 0x300 + 0x933E;
  }
  drv_eeprom_read_block(addr, ptr, 0x300);
  drv_lcd_blit(x, y, ptr, 0x40, 0x30);
}

// ROM: 0x6ad8  62.9%
void gfx_draw_small_route_icon(uint8_t a) {
  uint8_t *ptr;
  uint16_t addr;

  sys_init_heap();
  ptr = sbrk(0x60);
  addr = (uint16_t)a * 0x60 + 0x670;
  drv_eeprom_read_block(addr, ptr, 0x60);
  drv_lcd_blit(4, 4, ptr, 0x18, 0x10);
}

// ROM: 0xb0a2  44.7%
void gfx_draw_string_simple(void) {
  gfx_draw_string(0x08, 0x08, (const char *)0xBF93);
}

// ROM: 0x2680  85.8%
void gfx_add_font_border(uint16_t *ptr) {
  *ptr |= 0x101;
  ptr++;
  *ptr |= 0x101;
  ptr++;
  *ptr |= 0x101;
}

// ROM: 0x858a  18.5%
void gfx_draw_string(uint8_t x, uint8_t y_raw, const char *str) {
  uint8_t y_page = y_raw >> 3;
  uint8_t c, idx, i;
  const uint8_t *bmp;

  SSER = 0x80;
  PDR1 &= ~0x01;
  PDR1 &= ~0x02;

  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x10 | (x >> 4);
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = x & 0x0F;

  if (y_page > 7) {
    sleep();
  }

  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xB0 | (DAT_f7e4 * 8 + y_page);
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x02;

  while (*str) {
    c = (uint8_t)(*str++);
    if (c == 0x20) {
      for (i = 0; i < 8; i++) {
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0;
      }
    } else {
      if (c <= 0x39)
        idx = c + 0xD0;
      else
        idx = c + 0xC9;
      bmp = &L_BCF4[idx * 3];
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = bmp[0];
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = bmp[1];
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = bmp[2];
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0;
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0;
    }
    while (!SSSR_BIT.TEND)
      ;
  }
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// ROM: 0x18b6  64.2%
#pragma option speed =register /* pragma:auto */
void gfx_add_borders_to_text(void *buf, uint8_t w, uint8_t h, uint8_t flags) {
  uint16_t width = w;
  uint16_t *data = (uint16_t *)buf;
  uint16_t i;

  /* Plane 0 processing (Borders) */
  if (flags & 0x01) {
    for (i = 0; i < width; i++) {
      data[i] |= 0x0101;
    }
  }

  /* Flags bit 2: Start/End word overrides */
  if (flags & 0x04) {
    data[0] = 0xFFFF;
    data[width] = 0xFFFF;
  }

  /* Flags bit 3: Middle word overrides */
  if (flags & 0x08) {
    data[width * 2 - 1] = 0xFFFF;
    data[width * 4 - 1] = 0xFFFF;
  }

  /* Plane 1 processing (Shading/Highlights) */
  if (flags & 0x02) {
    for (i = 0; i < width; i++) {
      data[width + i] |= 0x8080;
    }
  }
}

// ROM: 0x1936  76.6%
void gfx_draw_own_pokemon_small(uint8_t x, uint8_t y) {
  uint8_t *buf;
  uint16_t addr;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xC0);
  addr = (uint16_t)(DAT_f7ac & 1) * 0xC0 + 0x91BE;
  drv_eeprom_read_block(addr, buf, 0xC0);
  drv_lcd_blit(x, y, buf, 0x20, 0x18);
}

// ROM: 0x1972  79.7%
void gfx_draw_own_pokemon_small_flipped(uint8_t x, uint8_t y) {
  uint8_t *buf;
  uint16_t addr;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xC0);

  addr = 0x91BE + (uint16_t)(DAT_f7ac & 0x01) * 0xC0;
  drv_eeprom_read_block(addr, buf, 0xC0);

  gfx_flip_horiz(0x20, 0x18, buf);
  drv_lcd_blit(x, y, buf, 0x20, 0x18);
}

// ROM: 0x1a58  78.0%
#pragma option speed =register /* pragma:auto */
void gfx_draw_own_pokemon_name(uint8_t x, uint8_t y, uint8_t flags) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);

  if (flags == 0x30) {
    diag_lcd_ssu_test_2();
  } else {
    diag_lcd_ssu_test_1();
  }

  drv_eeprom_read_block(0x993E, buf, 0x140);
  gfx_add_borders_to_text(buf, 0x50, 0x10, flags);
  drv_lcd_blit(x, y, buf, 0x50, 0x10);
}

// ROM: 0x1ab2  63.4%
void gfx_draw_peer_pokemon_name(uint8_t x, uint8_t y, uint8_t flags) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);
  diag_lcd_ssu_test_1();

  drv_eeprom_read_block(0xF580, buf, 0x140);
  gfx_add_borders_to_text(buf, 0x50, 0x10, flags);
  drv_lcd_blit(x, y, buf, 0x50, 0x10);
}

// ROM: 0x1af4  64.4%
void gfx_draw_event_pokemon_info(uint8_t x, uint8_t y, uint8_t flags) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);

  if (y == 0x30) {
    diag_lcd_ssu_test_2();
  } else {
    diag_lcd_ssu_test_1();
  }

  drv_eeprom_read_block(0xBC00, buf, 0x140);
  gfx_add_borders_to_text(buf, 0x50, 0x10, flags);
  drv_lcd_blit(x, y, buf, 0x50, 0x10);
}

// ROM: 0x1b40  75.3%
#pragma option speed =register /* pragma:auto */
void gfx_draw_special_poke_name(uint8_t x, uint8_t y, uint8_t flags) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);

  if (y == 0x30) {
    diag_lcd_ssu_test_2();
  } else {
    diag_lcd_ssu_test_1();
  }

  drv_eeprom_read_block(0xC6FC, buf, 0x140);
  gfx_add_borders_to_text(buf, 0x50, 0x10, flags);
  drv_lcd_blit(x, y, buf, 0x50, 0x10);
}

// ROM: 0x1b94  66.7%
void gfx_draw_item_symbol(uint8_t x, uint8_t y) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x10);

  drv_eeprom_read_block(0x488, buf, 0x10);
  drv_lcd_blit(x, y, buf, 8, 8);
}

// ROM: 0x1bc6  80.4%
#pragma option speed =register /* pragma:auto */
void gfx_draw_route_pokemon_name(uint8_t x, uint8_t y, uint8_t index,
                                 uint8_t flags) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x180);

  if ((uint8_t)flags == 0x30) {
    diag_lcd_ssu_test_2();
  } else {
    diag_lcd_ssu_test_1();
  }

  drv_eeprom_read_block(0xA4FE + (uint16_t)index * 0x140, buf, 0x140);
  gfx_add_borders_to_text(buf, 0x50, 0x10, flags);
  drv_lcd_blit(x, y, buf, 0x50, 0x10);
}

// ROM: 0x1c26  71.3%
#pragma option speed =register /* pragma:auto */
void gfx_draw_item_name(uint8_t x, uint8_t y, uint8_t index, uint8_t flags) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x180);

  drv_eeprom_read_block(0xA8BE + (uint16_t)index * 0x180, buf, 0x180);
  gfx_add_borders_to_text(buf, 0x60, 0x10, flags);
  drv_lcd_blit(x, y, buf, 0x60, 0x10);
}

// ROM: 0x1dca  22.1%
#pragma option speed =loop=1 /* pragma:auto */
void gfx_draw_animated_grass(uint8_t w, uint8_t h, int8_t shift, void *buf) {
  uint16_t width = w;
  uint16_t height = h;
  uint8_t *p = (uint8_t *)buf;
  uint16_t h_idx, w_idx;

  if (shift < 0) {
    uint8_t abs_shift = (uint8_t)(-shift);
    for (h = 0; h < height; h++) {
      for (w = 0; w < width; w++) {
        uint8_t *ptr = &p[h * width + w];
        uint8_t val = *ptr;
        *ptr >>= (abs_shift - 1);
        if (h < height - 1) {
          uint8_t next_val = p[(h + 1) * width + w];
          *ptr |= (next_val << (8 - abs_shift));
        }
      }
    }
  } else if (shift > 0) {
    uint8_t s = (uint8_t)shift;
    for (h = height - 1; h > 0; h--) {
      for (w = 0; w < width; w++) {
        uint8_t *ptr = &p[h * width + w];
        *ptr <<= (s - 1);
        if (h > 0) {
          uint8_t prev_val = p[(h - 1) * width + w];
          *ptr |= (prev_val >> (8 - s));
        }
      }
    }
  }
}

// ROM: 0x1c80  59.9%
void gfx_draw_event_item_name(uint8_t x, uint8_t y, uint8_t index,
                              uint8_t flags) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x180);

  drv_eeprom_read_block(0xBD48, buf, 0x180);
  gfx_add_borders_to_text(buf, 0x60, 0x10, flags);
  drv_lcd_blit(x, y, buf, 0x60, 0x10);
}

// ROM: 0x1cbe  74.3%
void gfx_draw_treasure_chest_icon(uint8_t x, uint8_t y) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xC0);

  drv_eeprom_read_block(0x1910, buf, 0xC0);
  drv_lcd_blit(x, y, buf, 0x18, 0x08);
}

// ROM: 0x1cf0  74.9%
void gfx_draw_present_icon(uint8_t x, uint8_t y) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xC0);

  drv_eeprom_read_block(0x1A90, buf, 0xC0);
  drv_lcd_blit(x, y, buf, 0x18, 0x08);
}

// ROM: 0x1d7a  60.7%
#pragma option speed =register /* pragma:auto */
uint8_t gfx_xor_rect_ram(void *ptr, uint8_t val) {
  uint8_t *p = (uint8_t *)ptr;
  uint8_t bit = val & 0x7;
  uint8_t offset = val >> 3;

  if (val == 0)
    return 0;
  save_read_reliable(0x00ED, 0x01ED, p, 0x68);
  if (p[0x38 + offset] & (1 << bit)) {
    return 1;
  }
  return 0;
}

// ROM: 0x1fee  80.9%
#pragma option speed =register /* pragma:auto */
void gfx_draw_numeric_value(uint8_t x, uint8_t y, uint32_t number,
                            uint8_t flags) {
  {
    uint16_t *ptr;
    uint8_t i;
    uint8_t *digit_buf;

    sys_init_heap();
    digit_buf = (uint8_t *)sbrk(0x140);
    drv_eeprom_read_block(0x280, digit_buf, 0x140);

    if (flags & 0xFF) {
      ptr = (uint16_t *)digit_buf;
      for (i = 10; i != 0; i--) {
        gfx_add_font_border(ptr);
        gfx_add_font_border(ptr);
        ((volatile uint16_t *)ptr)[0] |= 0x101;
        ptr++;
        ((volatile uint16_t *)ptr)[0] |= 0x101;
        ptr = (uint16_t *)((uint8_t *)ptr + 18);
      }
    }

    if (number == 0) {
      drv_lcd_blit(x, y, digit_buf, 0x10, 8);
    } else {
      do {
        uint8_t digit = (uint8_t)(number % 10);
        drv_lcd_blit(x, y, digit_buf + (digit * 32), 0x10, 8);
        number /= 10;
        x -= 8;
      } while (number != 0);
    }
  }
}

/* Reason: caller-saved register usage in ROM compiler.
 * ROM keeps `index` in r2 and `buf` in r3 across the eeprom_read_block
 * call -- both r2 and r3 are caller-saved per H8/300H ABI, so no PUSH/POP
 * is needed.  Our ch38 puts them in r5/r6 (callee-saved) and emits
 * PUSH.W R6 / PUSH.W R5 in the prologue.  The original compiler must have
 * done inter-procedural analysis (proving drv_eeprom_read_block doesn't
 * clobber r2/r3) or used a different ABI convention; ch38 is more
 * conservative and assumes any function call clobbers caller-saved regs.
 * Try `#pragma option speed=register` carefully -- but note pragmas have
 * file-global scope and clobbering one tends to regress neighbouring
 * functions (we saw -13% on gfx_draw_text_box this way).
 * Class: cannot-fix-without-compiler-change */
// ROM: 0x1eee  52.8%
uint16_t gfx_get_sprite_addr(uint8_t index) {
  uint8_t *buf;
  uint16_t result;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xBE);
  drv_eeprom_read_block(0x8F00, buf, 0xBE);

  result = *(uint16_t *)(buf + 0x8C + (uint16_t)index * 2);
  return result;
}

// ROM: 0x19b8  41.8%
void gfx_draw_route_pokemon(uint8_t x, uint8_t y, uint8_t index) {
  uint8_t *buf;
  uint16_t addr;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xC0);

  addr = (uint16_t)index * 0x180 + (uint16_t)(DAT_f7ac & 1) * 0xC0 + 0x9A7E;
  drv_eeprom_read_block(addr, buf, 0xC0);

  drv_lcd_blit(x, y, buf, 0x20, 0x18);
}

// ROM: 0x2096  34.7%
#pragma option speed =register /* pragma:auto */
void gfx_draw_text_box(uint8_t y, uint8_t index, uint8_t borders,
                       uint8_t flags) {
  uint8_t *buf, *e16_buf;
  uint16_t i;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x180);
  e16_buf = (uint8_t *)sbrk(0x18);

  drv_eeprom_read_block(0x2530 + (uint16_t)index * 0x180, buf, 0x180);
  gfx_add_borders_to_text(buf, 0x60, 0x10, borders);

  if (flags != 0 && ((DAT_f7ac >> 1) & 0x01)) {
    drv_eeprom_read_block(0x638, e16_buf, 0x18);
    for (i = 0; i < 8; i++) {
      uint16_t off = 0x170 + i * 2;
      uint8_t mask = e16_buf[0x10 + i];
      buf[off] &= mask;
      buf[off + 1] &= mask;
    }
    for (i = 0; i < 16; i++) {
      buf[0x170 + i] |= e16_buf[i];
    }
  }

  drv_lcd_blit(0, y, buf, 0x60, 0x10);
}

// ROM: 0x1f6c  58.2%
void gfx_draw_value_with_icon(uint8_t x, uint8_t y, uint8_t subtype,
                              uint16_t val) {
  uint8_t *buf;
  uint16_t *p16;
  uint16_t i;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);
  diag_lcd_ssu_test_1();

  gfx_fill_rect(8, 0x5E, 0x28, 1, 0);

  gfx_draw_numeric_value((uint8_t)(x + 8), y, (uint32_t)val, 0);

  drv_eeprom_read_block(0x420, buf, 0x40);
  p16 = (uint16_t *)buf;
  for (i = 0; i < 0x10; i++) {
    p16[i] |= 0x0101;
  }

  drv_lcd_blit(x + 16, y, buf, 0x10, 0x10);
}

// ROM: 0x21fe  90.3%
void gfx_draw_battery_low(uint8_t x, uint8_t y) {
  uint8_t *buf;

  if (!(statusFlags_BIT.low_battery)) {
    return;
  }

  /* Blinking logic: DAT_f7ac >> 2 bit 0 */
  if ((DAT_f7ac >> 2) & 0x01) {
    return;
  }

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x10);
  drv_eeprom_read_block(0x660, buf, 0x10);
  drv_lcd_blit(x, y, buf, 8, 8);
}

// ROM: 0x1a0a  63.6%
void gfx_draw_peer_pokemon(uint8_t x, uint8_t y, uint8_t flip) {
  uint8_t *buf;
  uint16_t addr;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xC0);

  addr = (uint16_t)(DAT_f7ac & 1) * 0xC0 + 0xF400;
  drv_eeprom_read_block(addr, buf, 0xC0);

  if (flip) {
    gfx_flip_horiz(0x20, 0x18, buf);
  }

  gfx_draw_sprite_simple(x, y, 0x20, 0x18, buf);
}

// ROM: 0x224c  11.2%
void gfx_alpha_blend(void *buf1, uint8_t w, uint8_t h, void *buf2, void *buf3,
                     uint8_t x, uint8_t y, uint8_t flags) {
  uint8_t *dst = (uint8_t *)buf1;
  uint8_t *mask_buf = (uint8_t *)buf2;
  uint8_t *data_buf = (uint8_t *)buf3;
  uint16_t y_page = y / 8;
  uint16_t shift = y & 7;
  uint16_t hi, wi;

  for (hi = 0; hi < h; hi++) {
    uint8_t *d_p = dst + (uint16_t)(y_page + hi) * 96 + x;
    for (wi = 0; wi < w; wi++) {
      if (mask_buf) {
        uint8_t m = mask_buf[hi * w + wi];
        d_p[wi] &= ~(m << shift);
        if (shift) {
          d_p[wi + 96] &= ~(m >> (8 - shift));
        }
      }
      if (data_buf) {
        uint8_t d = data_buf[hi * w + wi];
        d_p[wi] |= (d << shift);
        if (shift) {
          d_p[wi + 96] |= (d >> (8 - shift));
        }
      }
    }
  }
}

// ROM: 0x2178  40.5%
void gfx_flip_horiz(uint8_t w, uint8_t h, void *buf) {
  uint16_t width = w;
  uint16_t rows = h / 8;
  uint16_t *data = (uint16_t *)buf;
  uint16_t r, x;

  for (r = 0; r < rows; r++) {
    uint16_t *left = data;
    uint16_t *right = data + width - 1;
    for (x = 0; x < width / 2; x++) {
      // XOR swap logic to match compiler optimization
      *left ^= *right;
      *right ^= *left;
      *left ^= *right;
      left++;
      right--;
    }
    data += width;
  }
}

// ROM: 0x7e58  24.6%
#pragma option speed =loop=1 /* pragma:auto */
void gfx_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
  uint16_t p, col;
  uint16_t p_start = (uint16_t)y / 8;
  uint16_t p_end = (uint16_t)(y + h + 7) / 8;

  SSER = 0x80;
  PDR1 &= ~0x01;

  for (p = p_start; p < p_end; p++) {
    PDR1 &= ~0x02;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x10 | ((x >> 4) & 0x07);
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = x & 0x0F;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xB0 | (uint8_t)(p + (DAT_f7e4 << 3));
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x02;

    for (col = 0; col < (uint16_t)w; col++) {
      uint8_t v0 = 0, v1 = 0;
      if (color == 0) {
        v0 = 0;
        v1 = 0;
      } else if (color == 1) {
        v0 = 0;
        v1 = 0xFF;
      } else if (color == 2) {
        v0 = 0xFF;
        v1 = 0;
      } else if (color == 3) {
        v0 = 0xFF;
        v1 = 0xFF;
      }

      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = v0;
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = v1;
    }
  }
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// ROM: 0x82ea  32.3%
#pragma option noregexpansion /* pragma:auto */
void gfx_draw_sprite_simple(uint8_t x, uint8_t y, uint16_t w, uint16_t h,
                            void *buffer) {
  uint16_t p;
  int16_t col;
  uint16_t p_start = (uint16_t)((int16_t)(int8_t)y / 8);
  uint16_t p_end = (uint16_t)((int16_t)(int8_t)y + h + 7) / 8;
  uint16_t stride = (uint16_t)w << 1;
  uint8_t y_off = y & 7;
  uint8_t shift_r = 8 - y_off;
  uint8_t *base_ptr = (uint8_t *)(uintptr_t)buffer;

  SSER = 0x80;
  PDR1 &= ~0x01; // CS low

  for (p = p_start; p < p_end; p++) {
    PDR1 &= ~0x02; // A0 low
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x10 | ((x >> 4) & 0x07);
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = x & 0x0F;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xB0 | (uint8_t)(p + (DAT_f7e4 << 3));
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x02; // A0 high

    for (col = 0; col < (int16_t)w; col++) {
      uint8_t v0, v1;
      uint8_t *ptr = base_ptr + col * 2;

      if (y_off == 0) {
        v0 = ptr[0];
        v1 = ptr[1];
      } else {
        if (p == p_start) {
          v0 = ptr[0] << y_off;
          v1 = ptr[1] << y_off;
        } else if (p == p_end - 1) {
          uint8_t *prev = ptr - (w << 1);
          v0 = prev[0] >> shift_r;
          v1 = prev[1] >> shift_r;
        } else {
          uint8_t *prev = ptr - (w << 1);
          v0 = (prev[0] >> shift_r) | (ptr[0] << y_off);
          v1 = (prev[1] >> shift_r) | (ptr[1] << y_off);
        }
      }

      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = v0;
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = v1;
    }
  }

  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01; // CS high
}

// ROM: 0x7a40  21.7%
#pragma option speed =register /* pragma:auto */
void gfx_blit_to_buffer(uint8_t x, uint8_t y, uint8_t w, uint8_t h, void *src,
                        void *dst, uint8_t dst_w) {
  uint8_t *s_p = (uint8_t *)src;
  uint8_t *d_p = (uint8_t *)dst;
  uint16_t y_page = y / 8;
  uint16_t shift = y & 7;
  uint16_t hi, wi;

  d_p += (uint16_t)y_page * dst_w + x;

  for (hi = 0; hi < h; hi++) {
    for (wi = 0; wi < w; wi++) {
      uint8_t val = s_p[wi];
      d_p[wi] |= (val << shift);
      if (shift) {
        d_p[wi + dst_w] |= (val >> (8 - shift));
      }
    }
    s_p += w;
    d_p += dst_w;
  }
}
