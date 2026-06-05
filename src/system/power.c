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
 * `g.sys_walkerFlags.BYTE` bits 3-4 are the 2-bit power-mode field (mask 0x18):
 *   0x00 = active
 *   0x08 = low power     (set by sys_enter_low_power)
 *   0x10 = deep sleep    (set by sys_enter_deep_sleep)
 *   0x18 = reserved / unused
 *
 *   sys_enter_standby /
 *   sys_update_standby_state  Two periodic ticks driving the standby (LCD
 *     fade-out / fade-in) state machine, both touching g.viewstate.v.bytes.at_d1.BYTE bits 0/1/2
 *     and g.viewstate.A. Called alternately from ui_render_home_route based
 *     on g.viewstate.v.bytes.at_d1.b1 — _standby runs while b1 is clear (entry/exit phase),
 *     _update_standby_state runs while b1 is set (active-standby tick at
 *     every 4th g.ui_animationTick, with a periodic b2 toggle for visual blink).
 */

// ROM: 0x6b4c  88.6%
void sys_enter_standby(void) {
  if (!g.viewstate.v.bytes.at_d1.BIT.b2) {
    g.viewstate.A += 0xFC;
    if (g.viewstate.A <= 0x20) {
      g.viewstate.A = 0x20;
    }
  } else {
    g.viewstate.A += 0x04;
    if (g.viewstate.A >= 0x60) {
      g.viewstate.v.bytes.at_d1.BIT.b1 = 1;
      g.viewstate.v.bytes.at_d1.BIT.b2 = 0;
    }
  }
  if (g.sys_statusFlags.BIT.sleeping) {
    if (g.viewstate.A <= 0x20) {
      g.viewstate.v.bytes.at_d1.BIT.b2 = 1;
      g.viewstate.v.bytes.at_d1.BIT.b0 = 1;
    }
  }
}

// ROM: 0x6ba0  91.1%
void sys_update_standby_state(void) {
  uint8_t s;
  if ((g.ui_animationTick & 0x03) != 0) {
    return;
  }
  if (!g.viewstate.v.bytes.at_d1.BIT.b2) {
    s = g.viewstate.A + 0xFC;
    g.viewstate.A = s;
    if (s > 0x20)
      goto LAB_6bde;
    s = 0x20;
    goto LAB_6bd2;
  }
  s = g.viewstate.A + 0x04;
  g.viewstate.A = s;
  if (s < 0x40)
    goto LAB_6bde;
  s = 0x40;
LAB_6bd2:
  g.viewstate.A = s;
  g.viewstate.v.bytes.at_d1.BYTE ^= 0x04;  /* ROM emits `bnot #2,@r0`; ch38 doesn't pattern-match
                        either `^= 0x04` or `bN = !bN` to bnot */
LAB_6bde:
  if (!(g.sys_statusFlags.BIT.sleeping)) {
    g.viewstate.A = 0x68;
    g.viewstate.v.bytes.at_d1.BIT.b1 = 0;
    g.viewstate.v.bytes.at_d1.BIT.b2 = 0;
  }
}

// ROM: 0xa180  98.5%
void sys_enter_low_power(void) {
  CKSTPR1 |= 0x04;
  g.sys_walkerFlags.BYTE = (g.sys_walkerFlags.BYTE & 0xE7) | WALKER_MODE_LOW_POWER;
  RTCCR2 |= 0x01;
  g.ped_stepTimer = 0x1E;
  g.sys_statusFlags.BIT.sleeping = 0;
}

// ROM: 0xa29c  99.0%
void sys_enter_deep_sleep(void) {
  g.sys_activityTimer = 0x3C;
  g.ped_stepTimer = 0x5A;
  if ((g.sys_walkerFlags.BYTE & WALKER_MODE_MASK) != WALKER_MODE_DEEP_SLEEP) {
    if ((g.sys_walkerFlags.BYTE & WALKER_MODE_MASK) == WALKER_MODE_ACTIVE) {
      g.accel_sampleCount = 0;
    }
    g.sys_walkerFlags.BYTE = (g.sys_walkerFlags.BYTE & 0xE7) | WALKER_MODE_DEEP_SLEEP;
    RTCCR2 |= 0x01;
    drv_lcd_reset();
  }
}

// ROM: 0xa2da  97.9%
void sys_wake_from_low_power(void) {
  drv_timerw_disable();
  CKSTPR1 &= ~0x04;
  RTCCR2 &= ~0x01;
  g.sys_walkerFlags.BYTE &= 0xE7;
}

// ROM: 0x256e  94.7%
void sys_enter_sleep(uint16_t mode) {
  if (mode == 0) {
    SYSCR1 = 0xA7;
    SYSCR2 = 0xE0;
    g.sys_statusFlags.BIT.lcd_dirty = 1;
    sleep();
  } else if (mode == 1) {
    SYSCR1 = 0xAF;
    SYSCR2 = 0xE3;
    g.sys_statusFlags.BIT.lcd_dirty = 0;
    sleep();
  }
}

