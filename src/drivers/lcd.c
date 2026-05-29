#include "all_headers.h"

// ROM: 0xa962  79.9%
void drv_lcd_test_pixels(void) {
  uint8_t row;
  uint8_t col;
  int16_t tmp;
  int16_t dv;
  int16_t q;
  int16_t r;

  SSER = 0x80;
  PDR1 &= ~0x01;
  PDR1 |= 0x02;

  row = 0;
  do {
    drv_lcd_set_page_addr(row, 0);
    PDR1 |= 0x02;

    col = 0;
    do {
      tmp = (int16_t)(uint16_t)col;
      dv = 0x18;
      /* signed divide: q=quotient, r=remainder */
      q = tmp / dv;
      r = (int16_t)((int8_t)(tmp % dv));
      if (r != 0) {
        goto next_col;
      }
      switch (q) {
      case 0:
        /* 0x00, 0x00 */
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0x00;
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0x00;
        break;
      case 1:
        /* 0x00, 0xFF */
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0x00;
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0xFF;
        break;
      case 2:
        /* 0xFF, 0x00 */
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0xFF;
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0x00;
        break;
      case 3:
      default:
        /* 0xFF, 0xFF */
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0xFF;
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0xFF;
        break;
      }
    next_col:
      col++;
    } while (col < 0x60);

    /* Wait for TEND between rows */
    while (!SSSR_BIT.TEND)
      ;
    row++;
  } while (row < 8);

  /* Final TEND wait then CS high */
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// ROM: 0xac1e  85.3%
void drv_lcd_test_spi(void) {
  uint16_t i;
  uint8_t row;

  SSER = 0x80;
  PDR1 &= ~0x01;
  drv_lcd_set_page_addr(1, 0);
  PDR1 |= 0x02;

  /* Send 0xBC bytes of value 0x01 */
  i = 0xBC;
  do {
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x01;
    i--;
  } while (i != 0);

  /* Wait for TEND */
  while (!SSSR_BIT.TEND)
    ;

  /* For each of 8 rows: send row cmd then 2 bytes 0xFF */
  row = 0;
  do {
    drv_lcd_set_page_addr(0, row);
    PDR1 |= 0x02;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xFF;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xFF;
    while (!SSSR_BIT.TEND)
      ;
    row++;
  } while (row < 8);

  /* For each of 8 rows: send row+0x5F cmd then 2 bytes 0xFF */
  row = 0;
  do {
    drv_lcd_set_page_addr(0x5F, row);
    PDR1 |= 0x02;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xFF;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xFF;
    while (!SSSR_BIT.TEND)
      ;
    row++;
  } while (row < 8);

  /* Send command 0x701 then 0xBC bytes of 0x80 */
  drv_lcd_set_page_addr(1, 7);
  PDR1 |= 0x02;
  i = 0xBC;
  do {
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x80;
    i--;
  } while (i != 0);

  /* Wait for TEND twice */
  while (!SSSR_BIT.TEND)
    ;
  while (!SSSR_BIT.TEND)
    ;

  PDR1 |= 0x01;
}

// ROM: 0x7b44  98.2%
void drv_lcd_send_u8(uint8_t data) {
  PDR1 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = data;
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// ROM: 0x7b64  56.0%
void drv_lcd_delay(void) {
  uint16_t i = 0x64;
  do {
    sys_delay_short();
  } while (--i != 0);
}

// ROM: 0x7b72  70.1%  saves: r2,er6
void drv_lcd_init(void) {
  uint8_t *buf;
  uint8_t *ptr;
  uint8_t cmd;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x40);
  save_read_reliable(EEPROM_LCD_INIT_SEQ, EEPROM_LCD_INIT_SEQ_BACKUP, buf, 0x40);

  SSER = 0x80;
  PDR1 &= ~0x02;
  drv_lcd_send_u8(0xE1);

  ptr = buf;
  if (*ptr == 0 || *ptr == 0xFF) {
    ptr = (uint8_t *)lcdInitFallbackSeq;
  }

  lcdShadeBase = *ptr++;

  while (1) {
    cmd = *ptr++;
    if (cmd == 0xFE)
      break;
    if (cmd == 0xFD) {
      uint16_t delay = (uint16_t)*ptr++;
      uint16_t i;
      for (i = 0; i < delay; i++) {
        drv_lcd_delay();
      }
    } else {
      drv_lcd_send_u8(cmd);
    }
  }

  drv_lcd_send_u8(0xA6);
  drv_lcd_set_contrast((RamCache_settingsByte >> 3) & 0xF);
  lcdPageOffset = 1;
  drv_lcd_clear_pages(0x40);
  lcdPageOffset = 0;
  drv_lcd_clear_pages(0x40);
  PDR1 &= ~0x02;
  drv_lcd_send_u8(0xAF);
}

// ROM: 0x7c24  90.8%  saves: r6
void drv_lcd_set_contrast(uint8_t shade) {
  SSER = 0x80;
  PDR1 &= ~0x01;
  PDR1 &= ~0x02;
  drv_lcd_send_u8(0x81);
  drv_lcd_send_u8(lcdShadeBase + shade);
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// ROM: 0x7c56  88.8%
void drv_lcd_set_page_addr(uint8_t x, uint8_t p) {
  PDR1 &= ~0x02;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x10 | ((x >> 4) & 0x07);
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = x & 0x0F;
  if (p > 7) {
    sleep();
  }
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xB0 | (uint8_t)(p + (lcdPageOffset * 8));
  while (!SSSR_BIT.TEND)
    ;
}

// ROM: 0x7cac  94.1%
void drv_lcd_flip(void) {
  SSER = 0x80;
  PDR1 &= ~0x01;
  PDR1 &= ~0x02;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x40;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = lcdPageOffset * 0x40;
  while (!SSSR_BIT.TEND)
    ;
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
  lcdPageOffset ^= 1;
}

// ROM: 0x7cfa  94.4%
void drv_lcd_set_start(uint8_t page) {
  if (page <= 1) {
    SSER = 0x80;
    PDR1 &= ~0x01;
    PDR1 &= ~0x02;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x40;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = page * 0x40;
    while (!SSSR_BIT.TEND)
      ;
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x01;
    lcdPageOffset = page ^ 1;
  }
}

// ROM: 0x7d4a  83.2%
void drv_lcd_clear(uint8_t color) {
  uint8_t p, col;

  SSER = 0x80;
  PDR1 &= ~0x01;
  for (p = 0; p < 8; p++) {
    PDR1 &= ~0x02;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x10;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x00;
    if (p > 7) {
      sleep();
    }
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xB0 | (p + (uint8_t)(lcdPageOffset * 8));
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x02;

    for (col = 0x60; col != 0; col--) {
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
    while (!SSSR_BIT.TEND)
      ;
  }
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// ROM: 0x7fb8  97.5%
void drv_lcd_reset(void) {
  SSER = 0x80;
  PDR1 &= ~0x01;
  PDR1 &= ~0x02;
  drv_lcd_send_u8(0xE1);
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// ROM: 0x7fda  97.5%
void drv_lcd_power_save(void) {
  SSER = 0x80;
  PDR1 &= ~0x01;
  PDR1 &= ~0x02;
  drv_lcd_send_u8(0xA9);
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// ROM: 0x7ffc  77.1%  saves: r6
void drv_lcd_clear_pages(uint8_t height_pixels) {
  uint8_t p;
  uint8_t col;
  int16_t pages = (int16_t)((uint16_t)height_pixels) >> 3;

  SSER = 0x80;
  PDR1 &= ~0x01;
  for (p = 0; p < pages; p++) {
    PDR1 &= ~0x02;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x10;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0;
    if (p > 7) {
      sleep();
    }
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xB0 | (uint8_t)(p + (lcdPageOffset << 3));
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x02;
    col = 0x60;
    do {
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0;
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0;
      col--;
    } while (col != 0);
    while (!SSSR_BIT.TEND)
      ;
  }
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// Reason: ROM uses `$sp_regsv$3` helper (saves er3-er6 via shared jsr); ch38
//   inlines `push.l ER6/ER5/ER4/ER3/ER2 + subs #18`. Stack-arg byte offsets
//   thus diverge throughout the body. ROM uses `exts.l er2; divxs.w e4, er2`
//   (32-bit signed divide by 8); ch38 uses three `shar.w` shifts. Both
//   correct for valid y; instruction encoding differs.
//   ⚠️ Widely called (44+ callers). Do NOT change the C signature or
//   semantics — would cascade-break.
// Class: cannot-fix-without-compiler-change (sp_regsv$3 helper + signed
//   divide idiom)
// ROM: 0x80ac  28.7%  saves: er3,er4,er5,er6
void drv_lcd_blit(uint8_t x, uint8_t y, void *buffer, uint8_t w,
                        uint8_t h) {
  uint16_t p, col;
  uint8_t y_off = y & 7;
  uint16_t p_start = (uint16_t)((int16_t)(int8_t)y / 8);
  uint16_t p_end = ((int16_t)(int8_t)y + h + 7) / 8;
  uint16_t stride = (uint16_t)w << 1;
  uint8_t shift_r = 8 - y_off;
  uint8_t *ptr = (uint8_t *)buffer;

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
    SSTDR = 0xB0 | (uint8_t)(p + (lcdPageOffset << 3));
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x02; // A0 high

    for (col = 0; col < (uint16_t)w; col++) {
      uint8_t v0, v1;
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
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = v0;
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = v1;
      ptr += 2;
    }
  }

  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01; // CS high
}
