#include "all_headers.h"

/*
 * Graphics engine.
 *
 * Functions grouped by role (search by name to navigate):
 *
 *   --- Low-level LCD primitives ---
 *     gfx_draw_string                bit-banged SPI text writer
 *     gfx_draw_string_simple         fixed-args wrapper of the above
 *     gfx_fill_rect                  SPI rectangle fill (4 colour modes)
 *     gfx_draw_sprite_simple         SPI blit with sub-page y alignment
 *     gfx_blit_to_buffer             OR a sprite into a destination buffer
 *     gfx_flip_horiz                 in-place horizontal mirror
 *     gfx_alpha_blend                blend a smaller sprite into a larger one
 *
 *   --- Sprite-from-EEPROM renderers ---
 *     gfx_draw_home_pokemon          home-screen walker pokemon (animated)
 *     gfx_draw_own_pokemon_small     small own-pokemon icon (animated)
 *     gfx_draw_own_pokemon_small_flipped   same, mirrored
 *     gfx_draw_route_pokemon         wild pokemon icon for a route slot
 *     gfx_draw_peer_pokemon          peer-event pokemon icon
 *     gfx_draw_small_route_icon      small route-tier icon
 *     gfx_draw_treasure_chest_icon   gift / dowsed-item chest sprite
 *     gfx_draw_present_icon          gift-from-peer sprite
 *     gfx_draw_item_symbol           generic item icon
 *     gfx_draw_battery_low           blinking low-battery icon
 *
 *   --- Text-line renderers (name strips + borders) ---
 *     gfx_add_borders_to_text        decorate the 4 sides of a 0x180 text buf
 *     gfx_add_font_border            3-word OR helper for digit borders
 *     gfx_draw_text_box              load+border+blit one of the 0x2530 strings
 *     gfx_draw_own_pokemon_name      blit "own pokemon" name strip
 *     gfx_draw_peer_pokemon_name     blit peer pokemon name
 *     gfx_draw_event_pokemon_name    blit event-pokemon name from EEPROM cache
 *     gfx_draw_special_pokemon_name     blit special-route pokemon name
 *     gfx_draw_route_pokemon_name    blit wild pokemon name (indexed)
 *     gfx_draw_item_name             blit item name (indexed)
 *     gfx_draw_event_item_name       blit special-event item name
 *
 *   --- Numeric / icon-tagged display ---
 *     gfx_draw_numeric_value         base-10 digits, right-to-left, with borders
 *     gfx_draw_value_with_icon       numeric + small icon on the same row
 *
 *   --- Direct-SPI frame primitives (bypass drv_lcd_blit) ---
 *     gfx_draw_text_frame_upper      rectangle frame on pages B4,B5 (y=0x20)
 *     gfx_draw_text_frame_lower      rectangle frame on pages B6,B7 (y=0x30)
 *     gfx_draw_home_section_divider  single horizontal rule at page B6 top
 *
 *   --- Mini-animations ---
 *     gfx_draw_animated_grass        page-aligned shift-with-spillover for the
 *                                    bobbing grass effect
 *
 * Several functions in this file carry "Reason / Class:
 * cannot-fix-without-compiler-change" comment blocks documenting why the
 * score is stuck. The comments call out specific ch38 vs ROM codegen
 * mismatches (prologue helpers, register allocation, constant hoisting).
 * Edit those functions cautiously — small changes have bounced scores by
 * double-digit points in prior attempts.
 */

// ROM: 0x69be  73.7%
void gfx_draw_home_pokemon(uint8_t x, uint8_t y) {
  uint8_t *ptr;
  uint16_t addr;
  sys_init_heap();
  ptr = sbrk(0x300);
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, ptr, 0x10);
  if (gCurSubstateZ != 0) {
    addr = 0x933E;
  } else {
    addr = (uint16_t)((animTick >> 1) & 1) * 0x300 + 0x933E;
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

// Reason: ROM uses the bespoke `$sp_regsv$3` / `$spregld2$3` prologue/epilogue
//   helpers (saves er3/er4/er5/er6 via a single jsr to a shared helper); ch38
//   emits its own inline prologue that saves a different register set.
//   ROM also reorders the SSER/PDR1 setup to happen BEFORE the y_page shift
//   chain; ch38 does the shifts first. The str pointer is held in r4 by ROM
//   (using `mov.b @er4+, r6l` for the autoincrement read in the loop); ch38
//   emits separate `mov.b @erN, ...` + `adds.l #1, erN`. Body structure
//   matches; alignment broken by the prologue helper + reordering.
// Class: cannot-fix-without-compiler-change (sp_regsv$3 helper + instruction
//   scheduling differences; see score_focus.md Tier 3)
// ROM: 0x858a  17.1%  saves: er3,er4,er5,er6
void gfx_draw_string(uint8_t x, uint8_t y_raw, const char *str) {
  uint8_t y_page;
  uint8_t ch, glyph_idx, i;
  const uint8_t *glyph;

  SSER = 0x80;
  PDR1 &= ~0x01;
  y_page = y_raw >> 3;
  PDR1 &= ~0x02;

  /* Set column address (low 4 + high 3 bits). */
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x10 | ((x >> 4) & 0x07);
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = x & 0x0F;

  if (y_page > 7) {
    sleep();
  }

  /* Set page address. */
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xB0 | (uint8_t)(lcdPageOffset * 8 + y_page);
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x02;

  while (*str) {
    ch = (uint8_t)(*str++);
    if (ch == 0x20) {
      /* Space: 8 blank columns. */
      for (i = 0; i < 8; i++) {
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0;
      }
    } else {
      /* Map ASCII to glyph table. Digits '0'..'9' (0x30..0x39) and the
         letters above use slightly different offsets into font3ByteGlyphs. */
      if (ch <= 0x39)
        glyph_idx = ch + 0xD0;
      else
        glyph_idx = ch + 0xC9;
      glyph = &font3ByteGlyphs[glyph_idx * 3];
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = glyph[0];
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = glyph[1];
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = glyph[2];
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

// ROM: 0x18b6  68.9%  saves: r6,r5
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
    data[width - 1] = 0xFFFF;
    data[width * 2 - 1] = 0xFFFF;
  }

  /* Plane 1 processing (Shading/Highlights) */
  if (flags & 0x02) {
    for (i = 0; i < width; i++) {
      data[width + i] |= 0x8080;
    }
  }
}

/* The three "draw text frame" primitives below bit-bang directly to the
 * SSD1854 over the SSU (synchronous serial unit), bypassing drv_lcd_blit.
 * Each one draws a rectangular frame on a fixed pair of LCD pages:
 *
 *   gfx_draw_text_frame_upper        pages B4,B5  (pixel rows 32..47) - top
 *                                    edge horizontal line + left/right rails.
 *                                    Used by every name-strip / value-with-icon
 *                                    renderer when drawing at y=0x20.
 *
 *   gfx_draw_text_frame_lower        pages B6,B7  (pixel rows 48..63) - top
 *                                    edge + bottom edge + corners. Used by
 *                                    the same renderers when drawing at y=0x30.
 *
 *   gfx_draw_home_section_divider    page B6      (pixel row 48 only) - single
 *                                    full-width horizontal rule separating the
 *                                    home route image from the home status row.
 *                                    Used by ui_render_home_bar.
 *
 * Byte-pattern decode: writing 0x01 across a column lights bit 0 = topmost
 * pixel of the page (horizontal line at the top of that page); writing 0x80
 * across lights bit 7 = bottom pixel (line at the bottom); 0xFF at column 0
 * and the last column draws vertical rails.
 */

// ROM: 0x8766  96.4%
void gfx_draw_text_frame_upper(void) {
  uint8_t i;
  SSER = 0x80;
  PDR1 &= ~0x01;
  PDR1 &= ~0x02;

  /* Page B4: top edge horizontal line, with FF corners. */
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x10;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x00;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xB4 + (lcdPageOffset * 8);
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x02;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  for (i = 0; i < 0xBC; i++) {
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x01;
  }
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  while (!SSSR_BIT.TEND)
    ;
  PDR1 &= ~0x02;

  /* Page B5: left rail. */
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x10;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x00;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xB5 + (lcdPageOffset * 8);
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x02;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  while (!SSSR_BIT.TEND)
    ;
  PDR1 &= ~0x02;

  /* Page B5: right rail. */
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x15;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x0F;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xB5 + (lcdPageOffset * 8);
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x02;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  while (!SSSR_BIT.TEND)
    ;

  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// ROM: 0x88e2  95.4%
void gfx_draw_text_frame_lower(void) {
  uint8_t i;
  SSER = 0x80;
  PDR1 &= ~0x01;
  PDR1 &= ~0x02;

  /* Page B6: top edge horizontal line. */
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x10;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x00;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xB6 + (lcdPageOffset * 8);
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x02;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  for (i = 0; i < 0xBC; i++) {
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x01;
  }
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  while (!SSSR_BIT.TEND)
    ;
  PDR1 &= ~0x02;

  /* Page B7: bottom edge horizontal line. */
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x10;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x00;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xB7 + (lcdPageOffset * 8);
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x02;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  while (!SSSR_BIT.TEND)
    ;

  for (i = 0; i < 0xBC; i++) {
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x80;
  }
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xFF;
  while (!SSSR_BIT.TEND)
    ;

  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// ROM: 0x8a36  94.0%
void gfx_draw_home_section_divider(void) {
  uint8_t i;
  SSER = 0x80;
  PDR1 &= ~0x01;
  PDR1 &= ~0x02;

  /* Page B6: full-width horizontal line at the top. */
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x10;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x00;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xB6 + (lcdPageOffset * 8);
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x02;
  for (i = 0; i < 0xC0; i++) {
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x01;
  }
  while (!SSSR_BIT.TEND)
    ;

  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// ROM: 0x1936  88.2%
void gfx_draw_own_pokemon_small(uint8_t x, uint8_t y) {
  uint8_t *buf;
  uint16_t addr;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xC0);
  addr = (uint16_t)(animTick & 1) * 0xC0 + 0x91BE;
  drv_eeprom_read_block(addr, buf, 0xC0);
  drv_lcd_blit(x, y, buf, 0x20, 0x18);
}

// ROM: 0x1972  89.9%
void gfx_draw_own_pokemon_small_flipped(uint8_t x, uint8_t y) {
  uint8_t *buf;
  uint16_t addr;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xC0);

  addr = 0x91BE + (uint16_t)(animTick & 0x01) * 0xC0;
  drv_eeprom_read_block(addr, buf, 0xC0);

  gfx_flip_horiz(0x20, 0x18, buf);
  drv_lcd_blit(x, y, buf, 0x20, 0x18);
}

// ROM: 0x1a58  69.2%  saves: r4,r5,r6
void gfx_draw_own_pokemon_name(uint8_t x, uint8_t y, uint8_t borders) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);

  if (y == 0x30) {
    gfx_draw_text_frame_lower();
  } else {
    gfx_draw_text_frame_upper();
  }

  drv_eeprom_read_block(0x993E, buf, 0x140);
  gfx_add_borders_to_text(buf, 0x50, 0x10, borders);
  drv_lcd_blit(x, y, buf, 0x50, 0x10);
}

// ROM: 0x1ab2  83.4%
void gfx_draw_peer_pokemon_name(uint8_t x, uint8_t y, uint8_t borders) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);
  gfx_draw_text_frame_upper();

  drv_eeprom_read_block(0xF580, buf, 0x140);
  gfx_add_borders_to_text(buf, 0x50, 0x10, borders);
  drv_lcd_blit(x, y, buf, 0x50, 0x10);
}

// ROM: 0x1af4  81.3%
void gfx_draw_event_pokemon_name(uint8_t x, uint8_t y, uint8_t borders) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);

  if (y == 0x30) {
    gfx_draw_text_frame_lower();
  } else {
    gfx_draw_text_frame_upper();
  }

  drv_eeprom_read_block(0xBC00, buf, 0x140);
  gfx_add_borders_to_text(buf, 0x50, 0x10, borders);
  drv_lcd_blit(x, y, buf, 0x50, 0x10);
}

// ROM: 0x1b40  76.0%  saves: r5,r6
void gfx_draw_special_pokemon_name(uint8_t x, uint8_t y, uint8_t borders) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);

  if (y == 0x30) {
    gfx_draw_text_frame_lower();
  } else {
    gfx_draw_text_frame_upper();
  }

  drv_eeprom_read_block(0xC6FC, buf, 0x140);
  gfx_add_borders_to_text(buf, 0x50, 0x10, borders);
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

// ROM: 0x1bc6  81.3%  saves: r5,r6
void gfx_draw_route_pokemon_name(uint8_t x, uint8_t y, uint8_t index,
                                 uint8_t borders) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x180);

  if (y == 0x30) {
    gfx_draw_text_frame_lower();
  } else {
    gfx_draw_text_frame_upper();
  }

  drv_eeprom_read_block(0xA4FE + (uint16_t)index * 0x140, buf, 0x140);
  gfx_add_borders_to_text(buf, 0x50, 0x10, borders);
  drv_lcd_blit(x, y, buf, 0x50, 0x10);
}

// ROM: 0x1c26  63.7%  saves: r4,r5,er6
void gfx_draw_item_name(uint8_t x, uint8_t y, uint8_t index, uint8_t borders) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x180);

  drv_eeprom_read_block(0xA8BE + (uint16_t)index * 0x180, buf, 0x180);
  gfx_add_borders_to_text(buf, 0x60, 0x10, borders);
  drv_lcd_blit(x, y, buf, 0x60, 0x10);
}

// Reason: ROM uses a very lean prologue (`push.w r4; push.w r6` only — 4
//   bytes saved). ch38 emits `push.w r6; push.l er5; push.l er4; push.l er3;
//   push.l er2; subs.l #4, sp` (16 bytes plus 4 locals). The extra register
//   saves shift the alignment of every body instruction. ROM also calls the
//   compiler helpers `$DSRUC$3` and `$DSLC$3` for the bit-shift inner loops;
//   ch38 inlines `dec.b/shll.b/bra` shift loops instead — both produce the
//   correct value, but cause different byte sequences. Body structure
//   (shift<0 / shift>0 branches with row/column nested loops, carry-bit blend
//   into adjacent row) appears correct.
// Class: cannot-fix-without-compiler-change (calling-convention helper
//   mismatch + missing $DSRUC$3 / $DSLC$3 helper emission)
// ROM: 0x1dca  26.1%  saves: r4,r6
void gfx_draw_animated_grass(uint8_t w, uint8_t h, int8_t shift, void *buf) {
  uint16_t stride = (uint16_t)w << 1;   /* bytes per byte-row (e6 = w*2) */
  uint16_t rows = (uint16_t)h >> 3;     /* byte-rows (e4 = h/8) */
  uint8_t *p = (uint8_t *)buf;
  uint16_t r, c;

  if (shift < 0) {
    uint8_t abs_shift = (uint8_t)(-shift);
    for (r = 0; r < rows; r++) {
      for (c = 0; c < stride; c++) {
        uint8_t *ptr = &p[r * stride + c];
        *ptr >>= abs_shift;
        if (r != rows - 1) {
          uint8_t next_val = p[(r + 1) * stride + c];
          *ptr |= (uint8_t)(next_val << (8 - abs_shift));
        }
      }
    }
  } else if (shift > 0) {
    uint8_t s = (uint8_t)shift;
    for (r = rows; r > 0; r--) {
      uint16_t cur = r - 1;
      for (c = 0; c < stride; c++) {
        uint8_t *ptr = &p[cur * stride + c];
        *ptr <<= s;
        if (cur != 0) {
          uint8_t prev_val = p[(cur - 1) * stride + c];
          *ptr |= (uint8_t)(prev_val >> (8 - s));
        }
      }
    }
  }
}

// ROM: 0x1c80  80.9%
void gfx_draw_event_item_name(uint8_t x, uint8_t y, uint8_t index,
                              uint8_t borders) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x180);

  drv_eeprom_read_block(0xBD48, buf, 0x180);
  gfx_add_borders_to_text(buf, 0x60, 0x10, borders);
  drv_lcd_blit(x, y, buf, 0x60, 0x10);
}

// ROM: 0x1cbe  74.4%
void gfx_draw_treasure_chest_icon(uint8_t x, uint8_t y) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xC0);

  drv_eeprom_read_block(0x1910, buf, 0xC0);
  drv_lcd_blit(x, y, buf, 0x20, 0x18);
}

// ROM: 0x1cf0  74.9%
void gfx_draw_present_icon(uint8_t x, uint8_t y) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xC0);

  drv_eeprom_read_block(0x1A90, buf, 0xC0);
  drv_lcd_blit(x, y, buf, 0x20, 0x18);
}

// ROM: 0x1fee  54.8%  saves: r3,r4,r5,r6
void gfx_draw_numeric_value(uint8_t x, uint8_t y, uint32_t number,
                            uint8_t add_border) {
  {
    uint16_t *cursor;
    uint8_t i;
    uint8_t *digit_sheet;

    sys_init_heap();
    /* Load the 10-digit sheet (each digit = 0x20 bytes = 16 words). */
    digit_sheet = (uint8_t *)sbrk(0x140);
    drv_eeprom_read_block(0x280, digit_sheet, 0x140);

    if (add_border & 0xFF) {
      /* OR a top/bottom border into every digit in the sheet. Each digit
         occupies 16 words; we touch the first 8 (the top half), then skip
         18 bytes (= 16 bytes of the bottom half + 2 padding) to land on the
         next digit. The original calls gfx_add_font_border twice per digit,
         but that helper advances the caller's r6 by 6 each call — a calling-
         convention abuse C cannot express, so the OR sequence is inlined. */
      cursor = (uint16_t *)digit_sheet;
      for (i = 10; i != 0; i--) {
        cursor[0] |= 0x101;
        cursor[1] |= 0x101;
        cursor[2] |= 0x101;
        cursor[3] |= 0x101;
        cursor[4] |= 0x101;
        cursor[5] |= 0x101;
        cursor[6] |= 0x101;
        cursor[7] |= 0x101;
        cursor = (uint16_t *)((uint8_t *)(cursor + 8) + 16);
      }
    }

    if (number == 0) {
      /* Single zero — blit digit 0 only. */
      drv_lcd_blit(x, y, digit_sheet, 8, 0x10);
    } else {
      /* Render right-to-left, peeling decimal digits from `number`. */
      do {
        uint8_t digit = (uint8_t)(number % 10);
        drv_lcd_blit(x, y, digit_sheet + (digit * 32), 8, 0x10);
        number /= 10;
        x -= 8;
      } while (number != 0);
    }
  }
}

// ROM: 0x19b8  57.9%  saves: r6
void gfx_draw_route_pokemon(uint8_t x, uint8_t y, uint8_t index) {
  uint8_t *buf;
  uint16_t addr;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xC0);

  addr = (uint16_t)index * 0x180 + (uint16_t)(animTick & 1) * 0xC0 + 0x9A7E;
  drv_eeprom_read_block(addr, buf, 0xC0);

  drv_lcd_blit(x, y, buf, 0x20, 0x18);
}

// Reason: ROM saves r3/r4/r5/er6 = 10 bytes; ch38 saves r6/r5/r4/er3/r2 = 12
//   bytes. Different register choice. ROM hoists `mov.w #0x180, e6` (size)
//   and `mov.w #0x2530, r5` (base addr) at entry and reuses them via
//   add.w; ch38 inlines both constants at every use site. Called by ~30
//   functions, so risky to alter signature.
// Class: cannot-fix-without-compiler-change (callee-save register set +
//   constant hoisting)
// ROM: 0x2096  67.8%  saves: r3,r4,r5,er6
void gfx_draw_text_box(uint8_t y, uint8_t index, uint8_t borders,
                       uint8_t blink) {
  uint8_t *buf, *blink_overlay;
  uint16_t i;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x180);
  blink_overlay = (uint8_t *)sbrk(0x18);

  /* Load the indexed 96x16 string + apply borders. */
  drv_eeprom_read_block(0x2530 + (uint16_t)index * 0x180, buf, 0x180);
  gfx_add_borders_to_text(buf, 0x60, 0x10, borders);

  /* Blink: every other animTick beat, mask + OR a small overlay near the
     right edge of the text strip (used as the "press button to continue"
     blinking arrow). */
  if (blink != 0 && ((animTick >> 1) & 0x01)) {
    drv_eeprom_read_block(0x638, blink_overlay, 0x18);
    for (i = 0; i < 8; i++) {
      uint16_t off = 0x170 + i * 2;
      uint8_t mask = blink_overlay[0x10 + i];
      buf[off] &= mask;
      buf[off + 1] &= mask;
    }
    for (i = 0; i < 16; i++) {
      buf[0x170 + i] |= blink_overlay[i];
    }
  }

  drv_lcd_blit(0, y, buf, 0x60, 0x10);
}

// ROM: 0x1f6c  80.6%  saves: r4,er5,er6 -> er5,er6
void gfx_draw_value_with_icon(uint8_t x, uint8_t y, uint8_t subtype,
                              uint16_t val) {
  uint8_t *buf;
  uint16_t *icon16;
  uint16_t i;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);
  gfx_draw_text_frame_upper();

  /* Clear the numeric strip then write the value into it. */
  gfx_fill_rect(1, 0x28, 0x5E, 8, 0);
  gfx_draw_numeric_value((uint8_t)(x + 8), y, (uint32_t)val, 0);

  /* Load the icon sprite from EEPROM and OR in a top/bottom border on each
     16-bit column (0x0101 = bit 0 of both bytes = top + bottom edge pixels). */
  drv_eeprom_read_block(0x420, buf, 0x40);
  icon16 = (uint16_t *)buf;
  for (i = 0; i < 0x10; i++) {
    icon16[i] |= 0x0101;
  }

  drv_lcd_blit(x + 16, y, buf, 0x10, 0x10);
}

// ROM: 0x21fe  90.6%  saves: r6,r5
void gfx_draw_battery_low(uint8_t x, uint8_t y) {
  uint8_t *buf;

  if (!(statusFlags_BIT.low_battery)) {
    return;
  }

  /* Blinking logic: animTick >> 2 bit 0 */
  if ((animTick >> 2) & 0x01) {
    return;
  }

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x10);
  drv_eeprom_read_block(0x660, buf, 0x10);
  drv_lcd_blit(x, y, buf, 8, 8);
}

// ROM: 0x1a0a  79.9%
void gfx_draw_peer_pokemon(uint8_t x, uint8_t y, uint8_t flip) {
  uint8_t *buf;
  uint16_t addr;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xC0);

  addr = (uint16_t)(animTick & 1) * 0xC0 + 0xF400;
  drv_eeprom_read_block(addr, buf, 0xC0);

  if (flip) {
    gfx_flip_horiz(0x20, 0x18, buf);
  }

  gfx_draw_sprite_simple(x, y, 0x18, 0x20, buf);
}

// Reason: ROM saves er2/er3/r4(word)/er5/er6 in that order (18 bytes saved);
//   ch38 saves er6/er5/er4(long)/er3/er2 in reverse order (20 bytes). The
//   2-byte size mismatch on what should be `r4` propagates: every stack-arg
//   access offset shifts (ROM uses @(0x1F/0x21/0x23, er7); ch38's offsets are
//   2 bytes higher). ROM also hoists `e6 = 0xFF` at entry (used as a mask
//   later); ch38 doesn't keep this constant in a register. Body structure
//   (mask AND, data OR, page-shift carry into adjacent row) appears correct.
// Class: cannot-fix-without-compiler-change (push.l vs push.l+push.w mix +
//   constant hoisting)
// ROM: 0x224c  8.5%  saves: er2,er3,r4,er5,er6 -> sys_epilogue_5
/* Blend an 8-row-tall sub-sprite (small_w columns wide) into the dst sprite at
 * (x, y), where dst is stored as 2 bytes per column per page (data plane in
 * byte 0, mask/aux plane in byte 1) with w columns per page-row and a stride
 * of (w*2) bytes per page. buf3 ("fill", 1 byte per col) clears dst pixels
 * inside the sub-sprite outline via AND; buf2 ("outline", 2 bytes per col)
 * adds the visible sprite via OR. Right-edge clipping; vertical spillover
 * into the next page when shift!=0 and y+8 < h. */
// ROM: 0x224c  16.2%  saves: er2,er3,r4,er5,er6 -> sys_epilogue_5
void gfx_alpha_blend(void *buf1, uint8_t w, uint8_t h, void *buf2, void *buf3,
                     uint8_t x, uint8_t y, uint8_t small_w) {
  uint8_t *dst = (uint8_t *)buf1;
  uint8_t *outline = (uint8_t *)buf2;
  uint8_t *fill = (uint8_t *)buf3;

  int16_t x_signed = (int16_t)(int8_t)x;
  int16_t y_signed = (int16_t)(int8_t)y;
  int16_t sw = (int16_t)small_w;
  int16_t eff_w;
  int16_t page_idx;
  uint8_t shift, inv_shift;
  int16_t y_plus_8;
  uint16_t page_stride;
  uint8_t *base;
  int16_t col;

  if (x_signed >= (int16_t)w)
    return;
  if (x_signed + sw > (int16_t)w) {
    eff_w = (int16_t)w - x_signed;
  } else {
    eff_w = sw;
  }

  page_idx = y_signed >> 3;
  shift = (uint8_t)(y_signed & 7);
  inv_shift = (uint8_t)(8 - shift);
  y_plus_8 = y_signed + 8;
  page_stride = (uint16_t)w * 2;
  base = dst + 2 * (page_idx * (int16_t)w + x_signed);

  /* Pass 1: AND dst with shifted fill[col] to clear pixels inside the
   * sub-sprite area. Two writes per column (byte 0 and byte 1 of dst); each
   * uses a different shift (matching the original asm). */
  for (col = 0; col < eff_w; col++) {
    uint8_t fb = fill[col];
    uint8_t shifted_lo = (uint8_t)((uint16_t)fb << inv_shift);
    uint8_t pad_lo = (uint8_t)((uint16_t)0xFF >> shift);
    uint8_t combined0 = shifted_lo | pad_lo;
    uint8_t shifted_hi = (uint8_t)((uint16_t)fb << shift);
    uint8_t pad_hi = (uint8_t)((uint16_t)0xFF >> inv_shift);
    uint8_t combined1 = shifted_hi | pad_hi;

    base[col * 2] &= combined0;
    base[col * 2 + 1] &= combined1;

    if (y_plus_8 < (int16_t)h) {
      uint8_t spill_lo = (uint8_t)((uint16_t)fb >> inv_shift);
      uint8_t spill_pad = (uint8_t)((uint16_t)0xFF << shift);
      uint8_t spill_combined = spill_lo | spill_pad;
      base[col * 2 + page_stride] &= spill_combined;
      base[col * 2 + 1 + page_stride] &= spill_combined;
    }
  }

  /* Pass 2: OR shifted outline[col*2..col*2+1] into dst. Both bytes of the
   * outline pair use the same shift (= y & 7); next-page spillover uses
   * the inverse shift. */
  for (col = 0; col < eff_w; col++) {
    uint8_t ob0 = outline[col * 2];
    uint8_t ob1 = outline[col * 2 + 1];

    base[col * 2] |= (uint8_t)((uint16_t)ob0 << shift);
    base[col * 2 + 1] |= (uint8_t)((uint16_t)ob1 << shift);

    if (y_plus_8 < (int16_t)h) {
      base[col * 2 + page_stride] |= (uint8_t)((uint16_t)ob0 >> inv_shift);
      base[col * 2 + 1 + page_stride] |= (uint8_t)((uint16_t)ob1 >> inv_shift);
    }
  }
}

// ROM: 0x2178  58.5%  saves: r3,r4,er5,r6 -> er5,er6
void gfx_flip_horiz(uint8_t w, uint8_t h, void *buf) {
  uint16_t half = (uint16_t)w >> 1;
  uint16_t rows = (uint16_t)h >> 3;
  uint16_t stride = (uint16_t)w << 1;
  uint8_t *row_ptr = (uint8_t *)buf;
  uint8_t right_start = (uint8_t)(w - 1);
  uint8_t row, left_idx, right_idx;

  /* For each page-row, swap left/right column-pair words via the classic
     three-XOR swap. Each "column" is 2 bytes (data + mask plane). */
  for (row = 0; row < rows; row++) {
    left_idx = 0;
    right_idx = right_start;
    while ((uint16_t)left_idx < half) {
      uint16_t *L = (uint16_t *)(row_ptr + (uint16_t)left_idx * 2);
      uint16_t *R = (uint16_t *)(row_ptr + (uint16_t)right_idx * 2);
      *L ^= *R;
      *R ^= *L;
      *L ^= *R;
      left_idx++;
      right_idx--;
    }
    row_ptr += stride;
  }
}

// Reason: ROM saves with `push.w r4; push.l er5; push.l er6` (longword pushes
//   for ER5/ER6); ch38 saves with five separate `push.w` instructions
//   (r6/r5/r4/r3/r2). Same registers saved but different encoding/order, which
//   breaks alignment for the entire body. ROM also packs the y_page / p_end
//   calculation as `extu.w r1; add.w r1, r2; exts.l er1; divxs.w e0, er1`
//   (sign-extend to 32-bit then signed div) while ch38 uses
//   `add.w r1, r2; add.w #7, r2; shlr.w r2 x3` (unsigned div via shifts).
//   Both produce the right answer for valid inputs but emit different code.
//   The SPI inner-loop polling (`bld #2, @SSSR; bcc ...`) matches correctly.
// Class: cannot-fix-without-compiler-change (push.l vs push.w prologue
//   encoding + integer-division idiom mismatch)
// ROM: 0x7e58  58.1%  saves: r4,er5,er6 -> er5,er6
void gfx_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
  uint16_t p, col;
  uint8_t p_start = y >> 3;
  uint8_t p_end = (uint8_t)(y + h + 7) >> 3;

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
    SSTDR = 0xB0 | (uint8_t)(p + (lcdPageOffset << 3));
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x02;

    for (col = 0; col < (uint16_t)w; col++) {
      if (color == 0) {
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0;
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0;
      } else if (color == 1) {
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0;
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0xFF;
      } else if (color == 2) {
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0xFF;
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0;
      } else if (color == 3) {
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0xFF;
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0xFF;
      }
    }
  }
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// Reason: ROM saves r3/r4 (words) + er5/er6 (longs) = 12 bytes; ch38 saves
//   er2/er3/er4/er5/er6 (5 longs) = 20 bytes. Stack-arg offsets diverge by
//   8 bytes. ROM keeps `buffer` pointer in e4 across the function; ch38
//   spills it to a stack slot. Also stores byte locals (y_off, shift_r) at
//   tight @(0xa,r7)/@(0xb,r7) byte offsets; ch38 word-aligns them. Body
//   structure correct (signed y handling, p_start/p_end pagination, SPI
//   inner loop). Same cluster as gfx_fill_rect / drv_lcd_blit.
// Class: cannot-fix-without-compiler-change (push.w/push.l prologue mix +
//   register allocation for pointer locals)
// ROM: 0x82ea  33.9%  saves: r3,r4,er5,er6 -> er5,er6
#pragma option noregexpansion /* pragma:auto */
void gfx_draw_sprite_simple(uint8_t x, uint8_t y, uint16_t h, uint16_t w,
                            void *buffer) {
  uint16_t p;
  int16_t col;
  int16_t x_signed = (int16_t)(int8_t)x;
  int16_t col_end = x_signed + (int16_t)w;
  uint16_t p_start = (uint16_t)((int16_t)(int8_t)y / 8);
  uint16_t p_end = (uint16_t)((int16_t)(int8_t)y + h + 7) / 8;
  uint16_t stride = (uint16_t)w << 1;
  uint8_t y_off = y & 7;
  uint8_t shift_r = 8 - y_off;
  uint8_t *base_ptr = (uint8_t *)buffer;

  SSER = 0x80;
  PDR1 &= ~0x01; // CS low

  for (p = p_start; p < p_end; p++) {
    uint8_t *row_ptr = base_ptr + (p - p_start) * stride;
    PDR1 &= ~0x02; // A0 low
    while (!SSSR_BIT.TDRE)
      ;
    if (x_signed < 0) {
      SSTDR = 0x10;
    } else {
      SSTDR = 0x10 | ((x >> 4) & 0x07);
    }
    while (!SSSR_BIT.TDRE)
      ;
    if (x_signed < 0) {
      SSTDR = 0;
    } else {
      SSTDR = x & 0x0F;
    }
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xB0 | (uint8_t)(p + (lcdPageOffset << 3));
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x02; // A0 high

    for (col = x_signed; col < col_end; col++) {
      uint8_t v0, v1;
      uint8_t *ptr = row_ptr + (uint16_t)(col - x_signed) * 2;

      if (y_off == 0) {
        v0 = ptr[0];
        v1 = ptr[1];
      } else {
        if (p == p_start) {
          v0 = ptr[0] << y_off;
          v1 = ptr[1] << y_off;
        } else if (p == p_end - 1) {
          uint8_t *prev = ptr - stride;
          v0 = prev[0] >> shift_r;
          v1 = prev[1] >> shift_r;
        } else {
          uint8_t *prev = ptr - stride;
          v0 = (prev[0] >> shift_r) | (ptr[0] << y_off);
          v1 = (prev[1] >> shift_r) | (ptr[1] << y_off);
        }
      }

      if (col < 0)
        continue;

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

/* gfx_blit_to_buffer: OR a (w cols x h pixels) sprite into a destination
 * column-buffer at (x, y) with sub-page bit alignment.
 *
 * Layout convention: both src and dst are arranged as 2 bytes per "column"
 * (i.e. each x-position contributes 2 vertically-stacked bytes per page).
 * A page is 8 pixels tall. h is in pixels (must be a multiple of 8).
 * dst_w is the destination's column stride (bytes-per-page-row = 2*dst_w).
 *
 * Each source byte gets `<< (y & 7)` deposited into the current page and
 * `>> (8 - (y & 7))` deposited into the next page (the "carry into next
 * row"). The function OR-merges into the dst, never overwrites.
 *
 * NB: arg order matches the ROM ABI (w, h, x, y) — the prior C declaration
 * had it as (x, y, w, h) which mis-routed all four byte args; the body was
 * also half-implemented (1 byte per column instead of 2). */
/* Remaining mismatches (capping score at ~32%):
 *   - ROM prologue saves er4/r5/er6 (10 bytes); ch38 saves er6/er5/er4/er3/er2
 *     (20 bytes) plus a larger SUB.W locals reserve. Extra register saves
 *     propagate to byte-offset mismatches in @(N,SP) accesses for stack args
 *     and locals.
 *   - ROM uses byte-sized SHLR.B on r0h for the h/8 computation; ch38 picks
 *     16-bit SHLR.W on a promoted operand. Tried storing h/8 in a uint8_t
 *     local — that regressed because ch38 zero-extends-then-shifts.
 *   - ROM uses signed SHAR.W for y/8 (matters for negative y values from the
 *     cursor branch); ch38 picks SHLR.W (unsigned). Mathematically equivalent
 *     for the small positive y values the real call sites pass, but the
 *     instructions differ. Tried `(int16_t)(int8_t)y >> 3` — ch38 still
 *     emitted SHLR.W.
 *   - ch38 keeps `outer_iters`, `shift_l`, and dst_w on the stack instead of
 *     in scratch registers ROM uses (sp+0, e6, e1). High register pressure.
 *
 * Net of dedicated rewrite session 2026-05-23: 27.2% → 31.8% (+4.6pp);
 * ui_render_main_menu 64.9% → 67.9% (+3pp). Body is now semantically correct
 * (was a half-implementation before — 1 byte/col instead of 2). */
// ROM: 0x7a40  61.9%  saves: er4,r5,er6
void gfx_blit_to_buffer(uint8_t w, uint8_t h, uint8_t x, uint8_t y,
                        void *src, void *dst, uint8_t dst_w) {
  register uint8_t *s_p = (uint8_t *)src;
  register uint8_t *d_p = (uint8_t *)dst;
  register uint16_t shift_l = (uint16_t)(y & 7);
  register uint16_t outer_iters = (uint16_t)h >> 3;
  register uint16_t outer, inner;

  d_p += 2 * (((uint16_t)y >> 3) * (uint16_t)dst_w + (uint16_t)x);

  for (outer = 0; outer < outer_iters; outer++) {
    for (inner = 0; inner < (uint16_t)w; inner++) {
      uint8_t b0 = s_p[2 * inner];
      uint8_t b1 = s_p[2 * inner + 1];

      d_p[2 * inner]     |= (uint8_t)(b0 << shift_l);
      d_p[2 * inner + 1] |= (uint8_t)(b1 << shift_l);

      if (shift_l != 0) {
        d_p[2 * ((uint16_t)dst_w + inner)]     |= (uint8_t)(b0 >> (8 - shift_l));
        d_p[2 * ((uint16_t)dst_w + inner) + 1] |= (uint8_t)(b1 >> (8 - shift_l));
      }
    }
    s_p += 2 * (uint16_t)w;
    d_p += 2 * (uint16_t)dst_w;
  }
}
