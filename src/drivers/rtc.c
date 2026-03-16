#include "all_headers.h"

// ROM: 0xa800  66.7%
uint8_t drv_rtc_wait_sec(void) {
  uint16_t i;
  uint8_t s1;
  uint8_t s2;

  drv_rtc_set_time(0);
  i = 0x2710;
  do {
    sys_delay_short();
    i--;
  } while (i != 0);

  /* Read RSECDR stably: wait for bit 7 clear, then read twice for stability */
  do {
    while (RSECDR & 0x80)
      ;
    s1 = RSECDR;
    s2 = RSECDR;
  } while (s1 != s2);

  if (s1 != 0) {
    return 1;
  }
  return 0;
}

// ROM: 0xb390  46.2%
void drv_rtc_load(void) {
  uint16_t i;

  i = 0x3A98;
  do {
    sys_wdt_kick();
    sys_delay_short();
    i--;
  } while (i != 0);
  drv_rtc_set_time(DAT_f788);
}

// ROM: 0x0078  97.7%
void drv_rtc_init_timer_b(void) {
  CKSTPR1 |= 0x04;
  TMB1 = 0xBF;
  TCB1_TLB1 = 0xF8;
  IRR2 &= ~0x04;
  IENR2 |= 0x04;
  TMB1 |= 0x40;
}

// ROM: 0xa4fe  0.0%
void drv_rtc_set_time(uint32_t time_sec) {
  uint32_t t1, q;
  uint16_t t2;
  uint8_t sec_bcd, min_bcd, hr_bcd;

  t1 = time_sec / 60;
  t2 = (uint16_t)(time_sec - t1 * 60);
  q = t2 / 10;
  sec_bcd = (uint8_t)((t2 - q * 10) | q * 16);

  time_sec = t1;
  t1 = time_sec / 60;
  t2 = (uint16_t)(time_sec - t1 * 60);
  q = t2 / 10;
  min_bcd = (uint8_t)((t2 - q * 10) | q * 16);

  t2 = (uint16_t)(t1 - (t1 % 24) * 24);
  q = t2 / 10;
  hr_bcd = (uint8_t)((t2 - q * 10) | q * 16);

  DAT_f7a6 = hr_bcd;
  DAT_f7a5 = min_bcd;
  DAT_f7a4 = sec_bcd;

  CKSTPR1 |= 0x01;
  RTCCR1 &= ~0x80;
  RTCCR1 |= 0x10;
  RTCCR1 &= ~0x10;

  RSECDR = sec_bcd;
  RMINDR = min_bcd;
  RHRDR = hr_bcd;

  RTCCR1 |= 0x40;
  RTCCR1 |= 0x08;
  RTCCR2 = 0x1C;
  RTCCR1 |= 0x01;
  RTCCR1 |= 0x80;
}

// ROM: 0xa5d4  94.5%
#pragma option noregexpansion  /* pragma:auto */
void drv_rtc_get_time(uint8_t *hr_out, uint8_t *min_out, uint8_t *sec_out) {
  uint8_t times[6];
  uint8_t i;

  while (1) {
    for (i = 0; i < 2; i++) {
      while (RSECDR & 0x80) {
      }
      times[i * 3 + 0] = RSECDR;

      while (RMINDR & 0x80) {
      }
      times[i * 3 + 1] = RMINDR;

      while (RHRDR & 0x80) {
      }
      times[i * 3 + 2] = RHRDR;
    }

    if (times[0] == times[3] && times[1] == times[4] && times[2] == times[5]) {
      *hr_out = times[2];
      *min_out = times[1];
      *sec_out = times[0];
      return;
    }
  }
}

