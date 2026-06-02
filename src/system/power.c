#include "all_headers.h"

/*
 * Power state transitions.
 *
 *   sys_enter_low_power     Drop into low-power mode (LCD on, slowed clock).
 *   sys_enter_deep_sleep    Drop into deep-sleep mode (LCD off, RTCCR2 wakeup).
 *   sys_wake_from_low_power Exit low-power/deep-sleep back to active.
 *   sys_enter_sleep         Per-mode CPU SLEEP instruction (mode 0 / 1).
 *
 * (Watchdog control lives in src/system/wdt.c.)
 *
 * `walker_status_flags` bits 3-4 are the 2-bit power-mode field (mask 0x18):
 *   0x00 = active
 *   0x08 = low power     (set by sys_enter_low_power)
 *   0x10 = deep sleep    (set by sys_enter_deep_sleep)
 *   0x18 = reserved / unused
 *
 *   sys_enter_standby /
 *   sys_update_standby_state  Two periodic ticks driving the standby (LCD
 *     fade-out / fade-in) state machine, both touching DAT_f7d1 bits 0/1/2
 *     and gCurSubstateA. Called alternately from ui_render_home_route based
 *     on DAT_f7d1.b1 — _standby runs while b1 is clear (entry/exit phase),
 *     _update_standby_state runs while b1 is set (active-standby tick at
 *     every 4th animTick, with a periodic b2 toggle for visual blink).
 */

// ROM: 0x6b4c  88.6%
void sys_enter_standby(void) {
  if (!DAT_f7d1_BIT.b2) {
    gCurSubstateA += 0xFC;
    if (gCurSubstateA <= 0x20) {
      gCurSubstateA = 0x20;
    }
  } else {
    gCurSubstateA += 0x04;
    if (gCurSubstateA >= 0x60) {
      DAT_f7d1_BIT.b1 = 1;
      DAT_f7d1_BIT.b2 = 0;
    }
  }
  if (statusFlags_BIT.sleeping) {
    if (gCurSubstateA <= 0x20) {
      DAT_f7d1_BIT.b2 = 1;
      DAT_f7d1_BIT.b0 = 1;
    }
  }
}

// ROM: 0x6ba0  91.1%
void sys_update_standby_state(void) {
  uint8_t s;
  if ((animTick & 0x03) != 0) {
    return;
  }
  if (!DAT_f7d1_BIT.b2) {
    s = gCurSubstateA + 0xFC;
    gCurSubstateA = s;
    if (s > 0x20)
      goto LAB_6bde;
    s = 0x20;
    goto LAB_6bd2;
  }
  s = gCurSubstateA + 0x04;
  gCurSubstateA = s;
  if (s < 0x40)
    goto LAB_6bde;
  s = 0x40;
LAB_6bd2:
  gCurSubstateA = s;
  DAT_f7d1 ^= 0x04;  /* ROM emits `bnot #2,@r0`; ch38 doesn't pattern-match
                        either `^= 0x04` or `bN = !bN` to bnot */
LAB_6bde:
  if (!(statusFlags_BIT.sleeping)) {
    gCurSubstateA = 0x68;
    DAT_f7d1_BIT.b1 = 0;
    DAT_f7d1_BIT.b2 = 0;
  }
}

// ROM: 0xa180  98.5%
void sys_enter_low_power(void) {
  CKSTPR1 |= 0x04;
  walker_status_flags = (walker_status_flags & 0xE7) | WALKER_MODE_LOW_POWER;
  RTCCR2 |= 0x01;
  stepTimer = 0x1E;
  statusFlags_BIT.sleeping = 0;
}

// ROM: 0xa29c  99.0%
void sys_enter_deep_sleep(void) {
  activityTimer = 0x3C;
  stepTimer = 0x5A;
  if ((walker_status_flags & WALKER_MODE_MASK) != WALKER_MODE_DEEP_SLEEP) {
    if ((walker_status_flags & WALKER_MODE_MASK) == WALKER_MODE_ACTIVE) {
      accelSampleCount = 0;
    }
    walker_status_flags = (walker_status_flags & 0xE7) | WALKER_MODE_DEEP_SLEEP;
    RTCCR2 |= 0x01;
    drv_lcd_reset();
  }
}

// ROM: 0xa2da  97.9%
void sys_wake_from_low_power(void) {
  drv_timerw_disable();
  CKSTPR1 &= ~0x04;
  RTCCR2 &= ~0x01;
  walker_status_flags &= 0xE7;
}

// ROM: 0x256e  94.7%
void sys_enter_sleep(uint16_t mode) {
  if (mode == 0) {
    SYSCR1 = 0xA7;
    SYSCR2 = 0xE0;
    statusFlags_BIT.lcd_dirty = 1;
    sleep();
  } else if (mode == 1) {
    SYSCR1 = 0xAF;
    SYSCR2 = 0xE3;
    statusFlags_BIT.lcd_dirty = 0;
    sleep();
  }
}

