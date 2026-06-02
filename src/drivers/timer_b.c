#include "all_headers.h"

/*
 * Timer B — periodic interrupt timer (TMB1 / TCB1_TLB1).
 *
 * Initialized once at boot from system/resetprg.c. The matching ISR
 * (`timer_b1_overflow` in src/system/intprg.c) just clears the request
 * flag, so Timer B serves as a wake-up event source rather than running
 * an explicit handler body.
 *
 * Separate from Timer W (used for sound PWM, lives in src/drivers/sound.c)
 * and the watchdog timer (TCSRWD1, lives in src/system/wdt.c).
 */

// ROM: 0x0078  97.7%
void drv_timer_b_init(void) {
  CKSTPR1 |= 0x04;
  TMB1 = 0xBF;
  TCB1_TLB1 = 0xF8;
  IRR2 &= ~0x04;
  IENR2 |= 0x04;
  TMB1 |= 0x40;
}
