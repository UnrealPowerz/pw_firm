#include "all_headers.h"

/*
 * Watchdog timer (TCSRWD1 / TCWD / TMWD).
 *
 *   sys_wdt_unlock   Unlock the watchdog control register for writes.
 *   sys_wdt_init     Configure + start the watchdog timer.
 *   sys_wdt_kick     Reset the watchdog counter (call periodically to keep
 *                    the WDT from firing a reset).
 */

// ROM: 0x245e  97.4%
void sys_wdt_unlock(void) {
  TCSRWD1 = 0x9E;
  TCSRWD1 = 0xA2;
  TCSRWD1 = 0x8E;
}

// ROM: 0x246c  97.3%
void sys_wdt_init(void) {
  TCSRWD1 = 0x9E;
  TCSRWD1 = 0xA6;
  TCSRWD1 = 0x8E;
  TMWD = 0xF5;
}

// ROM: 0x259e  97.7%
void sys_wdt_kick(void) {
  TCSRWD1 = 0x5E;
  TCWD = 0;
  TCSRWD1 = 0x9E;
}
