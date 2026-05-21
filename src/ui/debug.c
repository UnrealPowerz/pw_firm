#include "all_headers.h"

// ROM: 0xa72a  56.2%
#pragma option speed =loop=1 /* pragma:auto */
uint8_t diag_eeprom_factory_test(uint32_t addr) {
  uint8_t *buf;
  uint16_t i, j;
  uint8_t val = 0;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x100);

  for (i = 0; i < 256; i++) {
    sys_wdt_kick();
    for (j = 0; j < 0x100; j++) {
      buf[j] = val++;
    }
    drv_eeprom_write_block(addr + (i << 8), buf, 0x100);
  }

  val = 0;
  for (i = 0; i < 256; i++) {
    sys_wdt_kick();
    drv_eeprom_read_block(addr + (i << 8), buf, 0x100);
    for (j = 0; j < 0x100; j++) {
      if (buf[j] != val++)
        return 0;
    }
  }

  for (i = 0; i < 256; i++) {
    sys_wdt_kick();
    drv_eeprom_write_u8_reliable(addr + (i << 8), 0xFF);
  }

  return 1;
}

// ROM: 0x5990  89.9%  saves: er2,er3,er4,er5,er6
#pragma option speed =loop=1 /* pragma:auto */
void sys_factory_test(void) {
  uint8_t res, i;
  set_ccr(0x80);
  SSER = 0xC0;
  SSMR = (SSMR & 0xF8) | 0x06;
  if (SSSR_BIT.ORER)
    SSSR_BIT.ORER = 0;

  PDR1 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0xE2;
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
  while (!SSSR_BIT.RDRF)
    ;
  res = SSRDR;
  SSER = 0x80;
  sys_delay_short();

  if (res != 0xAA) {
    PDR1 &= ~0x01;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xB0;
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x01;
    sys_delay_short();
    goto cleanup;
  }

  PDR1 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = *((volatile uint8_t *)0x5C);
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
  sys_delay_short();

  PDR1 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = *((volatile uint8_t *)0x5D);
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
  sys_delay_short();

  for (i = 0; i < 12; i++) {
    PDR1 &= ~0x01;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = *((volatile uint8_t *)(0x50 + i));
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x01;
    sys_delay_short();
  }

  TCSRWD1 = 0x9E;
  TCSRWD1 = 0xA2;
  TCSRWD1 = 0x8E;

  PDR1 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x04;
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
  if (!diag_eeprom_factory_test(0)) {
    PDR1 &= ~0x01;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xF4;
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x01;
    while (1)
      ;
  }

  sys_factory_reset_eeprom(1, 1);
  PDR1 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x03;
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
  if (!drv_rtc_wait_sec()) {
    PDR1 &= ~0x01;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xF3;
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x01;
    while (1)
      ;
  }

  PDR1 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x02;
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
  drv_accel_factory_test();

  PDR1 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x01;
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
  if (!drv_adc_test()) {
    PDR1 &= ~0x01;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xF1;
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x01;
    while (1)
      ;
  }

  PDR1 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x00;
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
  PDR9 &= ~0x01;
  PDR1 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x0A;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x01;
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
  PDR9 |= 0x01;

  set_ccr(0x80);
  CKSTPR1 &= ~0x55;
  CKSTPR2 = 0;
  PDR3 = 1;
  sys_enter_sleep(0);

cleanup:
  PDR1 |= 0x01;
  set_ccr(0x00);
}

// ROM: 0xaa42  97.2%
void diag_init_test_mode(void) {
  gCurSubstateY = 0;
  gCurSubstateA = 0;
  accelYPos = 1;
  RamCache_settingsByte = (RamCache_settingsByte & 0xF9) | 0x04;
  drv_sound_set_volume(2);
  drv_lcd_set_contrast(4);
}

// ROM: 0xaa6c  71.1%
void ui_handle_debug_input(void) {
  uint8_t subY;
  uint8_t subA;

  activityTimer = 0x3C;
  stepTimer = 0x1E;

  subA = gCurSubstateA;
  subY = gCurSubstateY;

  if (subY > 0x12) {
    return;
  }

  switch (subY) {
  case 0x00:
    if (gCurSubstateZ == 0) {
      return;
    }
    subY = gCurSubstateY + 1;
    gCurSubstateY = subY;
    return;

  case 0x01:
    if (subA < 4) {
      return;
    }
    if (drv_button_is_triggered(0x0A) != 0) {
      goto do_sound_and_inc;
    }
    if (gCurSubstateY == 1) {
      return;
    }
    if (drv_button_is_triggered(0x04) == 0) {
      return;
    }
    drv_sound_set_data((uint8_t *)factoryTestSoundData);
    subY = gCurSubstateY - 1;
    goto set_substate_y_and_clear_a;

  case 0x07:
    if (drv_button_is_triggered(0x04) == 0) {
      return;
    }
    goto do_sound_and_inc;

  case 0x08:
    if (drv_button_is_triggered(0x02) == 0) {
      return;
    }
    goto do_sound_and_inc;

  case 0x09:
    if (drv_button_is_triggered(0x08) == 0) {
      return;
    }
    goto do_sound_and_inc;

  case 0x0A:
    if (subA < 2) {
      return;
    }
    goto do_inc;

  case 0x0B: {
    uint8_t s1;
    uint8_t s2;
    do {
      while (RSECDR & 0x80)
        ;
      s1 = RSECDR;
      s2 = RSECDR;
    } while (s1 != s2);
    DAT_f7d1 = s1;
    diag_eeprom_factory_test(0x300);
    dowsing_item_pos = (uint8_t)diag_eeprom_factory_test(0x300);
    sys_factory_reset_eeprom(1, 1);
    do {
      while (RSECDR & 0x80)
        ;
      s1 = RSECDR;
      s2 = RSECDR;
    } while (s1 != s2);
    *(uint8_t *)(&accelXPos) = s1;
    goto do_inc;
  }

  case 0x0C:
    if (dowsing_item_pos == 0) {
      return;
    }
    if (subA < 4) {
      return;
    }
    goto do_sound_and_inc;

  case 0x0D: {
    uint16_t val;
    save_read_reliable(EEPROM_ACCEL_CAL, EEPROM_ACCEL_CAL_BACKUP, (void *)&accelYPos, 2);
    val = accelYPos;
    dowsing_item_pos = drv_adc_validate_calib_checksum(val);
    if (val != 0) {
      goto do_inc;
    }
    dowsing_item_pos = 0;
    goto set_substate_y_and_clear_a;
  }

  case 0x0E:
    if (dowsing_item_pos == 0) {
      return;
    }
    if (subA < 4) {
      return;
    }
    goto do_sound_and_inc;

  case 0x0F:
    if (DAT_f7d1 != *(uint8_t *)(&accelXPos)) {
      goto do_sound_and_inc;
    }
    return;

  case 0x10:
    drv_accel_init();
    dowsing_item_pos = 0;
    goto do_inc;

  case 0x11:
    if (dowsing_item_pos == 0) {
      return;
    }
    goto do_sound_and_inc;

  case 0x12:
    if (subA < 4) {
      return;
    }
    if (drv_button_is_triggered(0x0E) == 0) {
      return;
    }
    PDR1 &= ~0x01;
    PDR1 &= ~0x02;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xA6;
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x01;
    currentlyActiveView = 0x17;
    sys_init_debug_mode();
    drv_sound_set_data((uint8_t *)factoryTestSoundData);
    return;

  default:
    return;
  }

do_sound_and_inc:
  drv_sound_set_data((uint8_t *)factoryTestSoundData);
do_inc:
  subY = gCurSubstateY + 1;
set_substate_y_and_clear_a:
  gCurSubstateY = subY;
  gCurSubstateA = 0;
}

// ROM: 0xad06  54.9%
void ui_render_debug(void) {
  uint8_t buf[6];
  void (*fn858a)(uint8_t, uint8_t, const char *);
  uint8_t subY;
  uint8_t subA;

  fn858a = gfx_draw_string;
  subY = gCurSubstateY;

  if (subY > 0x12) {
    goto case_d;
  }

  switch (subY) {
  case 0x00:
    if (gCurSubstateZ != 0) {
      goto case_d;
    }
    fn858a(0x20, 0x08, factoryStr_NG1);
    goto case_d;

  case 0x01:
    drv_lcd_clear(3);
    goto case_d;

  case 0x02:
    drv_lcd_clear(2);
    goto case_d;

  case 0x03:
    drv_lcd_clear(1);
    goto case_d;

  case 0x04:
    drv_lcd_clear(0);
    goto case_d;

  case 0x05:
    drv_lcd_test_spi();
    goto case_d;

  case 0x06:
    drv_lcd_test_pixels();
    goto case_d;

  case 0x07:
    if (((uint16_t)animTick >> 1) & 1) {
      goto case_d;
    }
    fn858a(0x06, 0x38, factoryStr_V);
    goto case_d;

  case 0x08:
    if (((uint16_t)animTick >> 1) & 1) {
      goto case_d;
    }
    fn858a(0x2D, 0x38, factoryStr_V);
    goto case_d;

  case 0x09:
    if (((uint16_t)animTick >> 1) & 1) {
      goto case_d;
    }
    fn858a(0x55, 0x38, factoryStr_V);
    goto case_d;

  case 0x0A:
    fn858a(0x20, 0x08, factoryStr_EEP);
    goto case_d;

  case 0x0C:
    if (dowsing_item_pos != 0) {
      goto case_d;
    }
    fn858a(0x20, 0x08, factoryStr_NG2);
    goto case_d;

  case 0x0E:
    if (dowsing_item_pos != 0) {
      goto case_d;
    }
    fn858a(0x20, 0x08, factoryStr_NG3);
    goto case_d;

  case 0x0F:
    if (DAT_f7d1 != *(uint8_t *)(&accelXPos)) {
      goto case_d;
    }
    fn858a(0x20, 0x08, factoryStr_NG4);
    goto case_d;

  case 0x11:
    if (dowsing_item_pos != 0) {
      goto case_d;
    }
    fn858a(0x20, 0x08, factoryStr_NG5);
    goto case_d;

  case 0x12:
    /* SPI command 0xA7 */
    PDR1 &= ~0x01;
    PDR1 &= ~0x02;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xA7;
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x01;

    /* Draw hex digits of accelYPos */
    {
      const uint8_t *hexTable = hexDigits;
      uint16_t val = accelYPos;

      fn858a(0x20, 0x00, factoryStr_OK);

      buf[4] = 0;
      buf[0] = hexTable[(val >> 12) & 0xF];
      buf[1] = hexTable[(val >> 8) & 0xF];
      buf[2] = hexTable[(val >> 4) & 0xF];
      buf[3] = hexTable[val & 0xF];
      buf[4] = 0;

      fn858a(0x20, 0x18, (const char *)buf);
    }

    if (!(((uint16_t)animTick >> 1) & 1)) {
      fn858a(0x06, 0x38, factoryStr_V);
      fn858a(0x2D, 0x38, factoryStr_V);
      fn858a(0x55, 0x38, factoryStr_V);
    }
    goto case_d;

  default:
    goto case_d;
  }

case_d:
  subA = gCurSubstateA;
  if (subA < 4) {
    gCurSubstateA = subA + 1;
  }
}

// ROM: 0xaebc  96.2%
void sys_init_debug_mode(void) {
  accelSampleCount = 0;
  gCurSubstateY = 0x10;
  gCurSubstateA = 0;
  DAT_f7d1 = 0;
  accelXPos = 0;
  accelYPos = 0;
  accelZPos = 0;
  drv_eeprom_read_block(8, (void *)&DAT_f7d8, 8);
  walker_status_flags_BIT.session_active = 1;
}

// ROM: 0xaef8  100.0%
void sys_noop(void) {}

// ROM: 0xaefa  41.7%
#pragma option noregexpansion /* pragma:auto */
void ui_render_accel_debug(void) {
  uint8_t buf[6];
  void (*fn858a)(uint8_t, uint8_t, const char *);
  uint8_t *hexTable;
  uint8_t *p;
  uint8_t *q;

  fn858a = gfx_draw_string;
  activityTimer = 0x3C;
  stepTimer = 0x1E;

  /* Use buf[0..1] for 2-char strings via stack pointer */
  p = buf;
  q = p + 1;

  /* Draw gCurSubstateA as ASCII digit at position 0x4C */
  *p = (uint8_t)(gCurSubstateA + 0x30);
  *q = 0;
  fn858a(0x4C, 0x00, (const char *)p);

  /* Draw DAT_f7d8 as ASCII digit at position 0x54 */
  *p = (uint8_t)(DAT_f7d8 + 0x30);
  *q = 0;
  fn858a(0x54, 0x00, (const char *)p);

  hexTable = (uint8_t *)hexDigits;

  /* Draw hex digits of axisStepThresholdLo at 0x820 */
  {
    uint16_t val = axisStepThresholdLo;
    uint16_t tmp;
    uint8_t *d;
    d = p;
    tmp = val / 0x1000;
    *d = hexTable[tmp & 0xF];
    d = q;
    tmp = val;
    *d = hexTable[tmp & 0xF];

    d = (p + 2);
    tmp = (val >> 4);
    *d = hexTable[tmp & 0xF];
    d = (p + 3);
    tmp = (val >> 4);
    *d = hexTable[tmp & 0xF];

    /* Reset and recompute properly */
    *p = hexTable[(val >> 12) & 0xF];
    *q = hexTable[(val >> 8) & 0xF];
    *(p + 2) = hexTable[(val >> 4) & 0xF];
    *(p + 3) = hexTable[val & 0xF];
    *(p + 4) = 0;
    fn858a(0x20, 0x08, (const char *)p);
  }

  /* Draw hex digits of axisStepThresholdHi at 0x840 */
  {
    uint16_t val = axisStepThresholdHi;
    *p = hexTable[(val >> 12) & 0xF];
    *q = hexTable[(val >> 8) & 0xF];
    *(p + 2) = hexTable[(val >> 4) & 0xF];
    *(p + 3) = hexTable[val & 0xF];
    *(p + 4) = 0;
    fn858a(0x40, 0x08, (const char *)p);
  }

  /* Draw hex digits of axisIdleThreshold at 0x1020 */
  {
    uint16_t val = axisIdleThreshold;
    *p = hexTable[(val >> 12) & 0xF];
    *q = hexTable[(val >> 8) & 0xF];
    *(p + 2) = hexTable[(val >> 4) & 0xF];
    *(p + 3) = hexTable[val & 0xF];
    *(p + 4) = 0;
    fn858a(0x20, 0x10, (const char *)p);
  }

  /* Draw DAT_f7d1 as ASCII digit at 0x104C */
  *p = (uint8_t)(DAT_f7d1 + 0x30);
  *q = 0;
  fn858a(0x4C, 0x10, (const char *)p);

  /* Draw DAT_f7d8_1 (the byte after DAT_f7d8) at 0x1054 */
  *p = DAT_f7d8_1;
  fn858a(0x54, 0x10, (const char *)p);

  /* If DAT_f7d1 == DAT_f7d8_1, send SPI command 0xA7 and draw check mark */
  if (DAT_f7d1 == DAT_f7d8_1) {
    PDR1 &= ~0x01;
    PDR1 &= ~0x02;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xA7;
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x01;
    fn858a(0x08, 0x20, factoryStr_OK);
  }
}

// ROM: 0x8766  96.4%
void diag_lcd_ssu_test_1(void) {
  uint8_t i;
  SSER = 0x80;
  PDR1 &= ~0x01;
  PDR1 &= ~0x02;

  /* Page B4 */
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

  /* Page B5 */
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

// ROM: 0x88e2  95.3%
void diag_lcd_ssu_test_2(void) {
  uint8_t i;
  SSER = 0x80;
  PDR1 &= ~0x01;
  PDR1 &= ~0x02;

  /* Page B6 */
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

  /* Page B7 */
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

// ROM: 0x8a36  93.9%
void diag_lcd_ssu_test_3(void) {
  uint8_t i;
  SSER = 0x80;
  PDR1 &= ~0x01;
  PDR1 &= ~0x02;

  /* Page B6 */
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
