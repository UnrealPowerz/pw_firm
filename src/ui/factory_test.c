#include "all_headers.h"

/*
 * Factory test (VIEW_FACTORY_TEST, entered via IR command 0xF0).
 *
 *   diag_init_test_mode        reset state, set vol/contrast.
 *   ui_handle_factory_test     progress through the 0x13-stage test sequence
 *                              driven by g.viewstate.Y.
 *   ui_render_factory_test     per-stage UI (LCD fills, button indicators,
 *                              NG/OK strings drawn via gfx_draw_string).
 *
 *   sys_factory_test           the bare-metal factory-bringup routine called
 *                              from system/resetprg.c. Bit-bangs commands over
 *                              SSU to a host fixture, runs sub-tests (EEPROM
 *                              stress, RTC, accel, ADC), and reports pass/fail.
 *                              Not reached in normal firmware operation.
 *
 *   diag_eeprom_factory_test   the EEPROM-stress sub-test (write-all,
 *                              read-and-verify, then fill 0xFF).
 *
 * Factory-test stages (g.viewstate.Y values 0..0x12) progress sequentially.
 * Each stage either auto-advances after a frame counter (g.viewstate.A >= 4)
 * or waits for a specific button. The stage numbers are referenced both by
 * the handler (decide-when-to-advance) and the renderer (decide-what-to-show);
 * see per-case inline comments below for the role of each stage.
 *
 * The companion VIEW_ACCEL_DEBUG view (entered from the final factory-test
 * stage) lives in src/ui/accel_debug.c.
 */

// ROM: 0xa72a  29.3%
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

// ROM: 0x5990  89.5%  saves: er2,er3,er4,er5,er6
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

// ROM: 0xaa42  98.4%
void diag_init_test_mode(void) {
  g.viewstate.Y = 0;
  g.viewstate.A = 0;
  g.viewstate.v.bytes.at_d4 = 1;
  g.save_settings = (g.save_settings & 0xF9) | 0x04;
  drv_sound_set_volume(2);
  drv_lcd_set_contrast(4);
}

// ROM: 0xaa6c  67.2%
void ui_handle_factory_test(void) {
  uint8_t subY;
  uint8_t subA;

  g.ped_activityTimer = 0x3C;
  g.ped_stepTimer = 0x1E;

  subA = g.viewstate.A;
  subY = g.viewstate.Y;

  if (subY > 0x12) {
    return;
  }

  switch (subY) {
  case 0x00:
    /* Idle entry — wait for g.viewstate.Z to be poked non-zero by the IR
       command path, then advance to sound test. */
    if (g.viewstate.Z == 0) {
      return;
    }
    subY = g.viewstate.Y + 1;
    g.viewstate.Y = subY;
    return;

  case 0x01:
    /* Sound test. After a 4-frame settle, BTN_LM advances + plays the
       factory sound. The unreachable BTN_M-decrement branch below is
       dead code (g.viewstate.Y == 1 always true in this case). */
    if (subA < 4) {
      return;
    }
    if (drv_button_is_triggered(BTN_LM) != 0) {
      goto do_sound_and_inc;
    }
    if (g.viewstate.Y == 1) {
      return;
    }
    if (drv_button_is_triggered(BTN_M) == 0) {
      return;
    }
    drv_sound_set_data((uint8_t *)FACTORY_TEST_SOUND);
    subY = g.viewstate.Y - 1;
    goto set_substate_y_and_clear_a;

  /* Stages 0x02..0x06 (LCD fills + SPI/pixel tests) have no handler case —
     they fall to default and stick until something external advances
     g.viewstate.Y. The render path keeps showing the test pattern. */

  case 0x07:
    /* Middle-button test. */
    if (drv_button_is_triggered(BTN_M) == 0) {
      return;
    }
    goto do_sound_and_inc;

  case 0x08:
    /* Right-button test. */
    if (drv_button_is_triggered(BTN_R) == 0) {
      return;
    }
    goto do_sound_and_inc;

  case 0x09:
    /* Left-button test. */
    if (drv_button_is_triggered(BTN_L) == 0) {
      return;
    }
    goto do_sound_and_inc;

  case 0x0A:
    /* "EEPROM" header shown — short settle, then advance to the test run. */
    if (subA < 2) {
      return;
    }
    goto do_inc;

  case 0x0B: {
    /* EEPROM stress test: read RTC sec twice (debounce), stash, run the
       64K-EEPROM pattern test, run it again, reset EEPROM to defaults,
       read RTC sec again as a "soak time" marker. */
    uint8_t s1;
    uint8_t s2;
    do {
      while (RSECDR & 0x80)
        ;
      s1 = RSECDR;
      s2 = RSECDR;
    } while (s1 != s2);
    g.viewstate.v.bytes.at_d1 = s1;
    diag_eeprom_factory_test(0x300);
    g.viewstate.v.bytes.at_d3 = (uint8_t)diag_eeprom_factory_test(0x300);
    sys_factory_reset_eeprom(1, 1);
    do {
      while (RSECDR & 0x80)
        ;
      s1 = RSECDR;
      s2 = RSECDR;
    } while (s1 != s2);
    *(uint8_t *)(&g.viewstate.v.bytes.at_d2) = s1;
    goto do_inc;
  }

  case 0x0C:
    /* EEPROM result gate — `g.viewstate.v.bytes.at_d3` here doubles as a generic
       pass-flag (1=passed, 0=failed). Wait for the renderer to settle
       then advance with sound. NG2 is shown if it failed. */
    if (g.viewstate.v.bytes.at_d3 == 0) {
      return;
    }
    if (subA < 4) {
      return;
    }
    goto do_sound_and_inc;

  case 0x0D: {
    /* Accel calibration validate — read EEPROM cal block, verify checksum. */
    uint16_t val;
    save_read_reliable(EEPROM_ACCEL_CAL, EEPROM_ACCEL_CAL_BACKUP, (void *)&g.viewstate.v.bytes.at_d4, 2);
    val = g.viewstate.v.bytes.at_d4;
    g.viewstate.v.bytes.at_d3 = drv_adc_validate_calib_checksum(val);
    if (val != 0) {
      goto do_inc;
    }
    g.viewstate.v.bytes.at_d3 = 0;
    goto set_substate_y_and_clear_a;
  }

  case 0x0E:
    /* Accel calibration result gate. NG3 if g.viewstate.v.bytes.at_d3 == 0. */
    if (g.viewstate.v.bytes.at_d3 == 0) {
      return;
    }
    if (subA < 4) {
      return;
    }
    goto do_sound_and_inc;

  case 0x0F:
    /* Accel sample check — advance only once samples diverge from the
       stashed value (gives the technician time to wiggle the device). */
    if (g.viewstate.v.bytes.at_d1 != *(uint8_t *)(&g.viewstate.v.bytes.at_d2)) {
      goto do_sound_and_inc;
    }
    return;

  case 0x10:
    /* Re-init the accel driver and arm the result check. */
    drv_accel_init();
    g.viewstate.v.bytes.at_d3 = 0;
    goto do_inc;

  case 0x11:
    /* Accel init result gate. NG5 if pass-flag still 0. */
    if (g.viewstate.v.bytes.at_d3 == 0) {
      return;
    }
    goto do_sound_and_inc;

  case 0x12:
    /* Final stage — wait, then any button transitions to the accel-debug
       view for live threshold inspection. */
    if (subA < 4) {
      return;
    }
    if (drv_button_is_triggered(BTN_ANY) == 0) {
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
    g.ui_activeView = VIEW_ACCEL_DEBUG;
    sys_init_accel_debug();
    drv_sound_set_data((uint8_t *)FACTORY_TEST_SOUND);
    return;

  default:
    return;
  }

do_sound_and_inc:
  drv_sound_set_data((uint8_t *)FACTORY_TEST_SOUND);
do_inc:
  subY = g.viewstate.Y + 1;
set_substate_y_and_clear_a:
  g.viewstate.Y = subY;
  g.viewstate.A = 0;
}

// ROM: 0xad06  50.3%
void ui_render_factory_test(void) {
  uint8_t buf[6];
  void (*draw_string)(uint8_t, uint8_t, const char *);
  uint8_t subY;
  uint8_t subA;

  draw_string = gfx_draw_string;
  subY = g.viewstate.Y;

  if (subY > 0x12) {
    goto case_d;
  }

  switch (subY) {
  case 0x00:
    if (g.viewstate.Z != 0) {
      goto case_d;
    }
    draw_string(0x20, 0x08, FACTORY_STR_NG1);
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
    if (((uint16_t)g.ui_animationTick >> 1) & 1) {
      goto case_d;
    }
    draw_string(0x06, 0x38, FACTORY_STR_V);
    goto case_d;

  case 0x08:
    if (((uint16_t)g.ui_animationTick >> 1) & 1) {
      goto case_d;
    }
    draw_string(0x2D, 0x38, FACTORY_STR_V);
    goto case_d;

  case 0x09:
    if (((uint16_t)g.ui_animationTick >> 1) & 1) {
      goto case_d;
    }
    draw_string(0x55, 0x38, FACTORY_STR_V);
    goto case_d;

  case 0x0A:
    draw_string(0x20, 0x08, FACTORY_STR_EEP);
    goto case_d;

  case 0x0C:
    if (g.viewstate.v.bytes.at_d3 != 0) {
      goto case_d;
    }
    draw_string(0x20, 0x08, FACTORY_STR_NG2);
    goto case_d;

  case 0x0E:
    if (g.viewstate.v.bytes.at_d3 != 0) {
      goto case_d;
    }
    draw_string(0x20, 0x08, FACTORY_STR_NG3);
    goto case_d;

  case 0x0F:
    if (g.viewstate.v.bytes.at_d1 != *(uint8_t *)(&g.viewstate.v.bytes.at_d2)) {
      goto case_d;
    }
    draw_string(0x20, 0x08, FACTORY_STR_NG4);
    goto case_d;

  case 0x11:
    if (g.viewstate.v.bytes.at_d3 != 0) {
      goto case_d;
    }
    draw_string(0x20, 0x08, FACTORY_STR_NG5);
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

    /* Draw hex digits of g.viewstate.v.bytes.at_d4 */
    {
      const uint8_t *hexTable = HEX_DIGITS;
      uint16_t val = g.viewstate.v.bytes.at_d4;

      draw_string(0x20, 0x00, FACTORY_STR_OK);

      buf[4] = 0;
      buf[0] = hexTable[(val >> 12) & 0xF];
      buf[1] = hexTable[(val >> 8) & 0xF];
      buf[2] = hexTable[(val >> 4) & 0xF];
      buf[3] = hexTable[val & 0xF];
      buf[4] = 0;

      draw_string(0x20, 0x18, (const char *)buf);
    }

    if (!(((uint16_t)g.ui_animationTick >> 1) & 1)) {
      draw_string(0x06, 0x38, FACTORY_STR_V);
      draw_string(0x2D, 0x38, FACTORY_STR_V);
      draw_string(0x55, 0x38, FACTORY_STR_V);
    }
    goto case_d;

  default:
    goto case_d;
  }

case_d:
  subA = g.viewstate.A;
  if (subA < 4) {
    g.viewstate.A = subA + 1;
  }
}
