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
 * `g.sys_walkerFlags` bits 3-4 are the 2-bit power-mode field (mask 0x18):
 *   0x00 = active
 *   0x08 = low power     (set by sys_enter_low_power)
 *   0x10 = deep sleep    (set by sys_enter_deep_sleep)
 *   0x18 = reserved / unused
 *
 *   sys_enter_standby /
 *   sys_update_standby_state  Two periodic ticks driving the standby (LCD
 *     fade-out / fade-in) state machine, both touching g.DAT_f7d1 bits 0/1/2
 *     and g.gCurSubstateA. Called alternately from ui_render_home_route based
 *     on g.DAT_f7d1.b1 — _standby runs while b1 is clear (entry/exit phase),
 *     _update_standby_state runs while b1 is set (active-standby tick at
 *     every 4th g.animTick, with a periodic b2 toggle for visual blink).
 */

// ROM: 0x6b4c  88.6%
void sys_enter_standby(void) {
  if (!DAT_f7d1_BIT.b2) {
    g.gCurSubstateA += 0xFC;
    if (g.gCurSubstateA <= 0x20) {
      g.gCurSubstateA = 0x20;
    }
  } else {
    g.gCurSubstateA += 0x04;
    if (g.gCurSubstateA >= 0x60) {
      DAT_f7d1_BIT.b1 = 1;
      DAT_f7d1_BIT.b2 = 0;
    }
  }
  if (sys_statusFlags_BIT.sleeping) {
    if (g.gCurSubstateA <= 0x20) {
      DAT_f7d1_BIT.b2 = 1;
      DAT_f7d1_BIT.b0 = 1;
    }
  }
}

// ROM: 0x6ba0  91.1%
void sys_update_standby_state(void) {
  uint8_t s;
  if ((g.animTick & 0x03) != 0) {
    return;
  }
  if (!DAT_f7d1_BIT.b2) {
    s = g.gCurSubstateA + 0xFC;
    g.gCurSubstateA = s;
    if (s > 0x20)
      goto LAB_6bde;
    s = 0x20;
    goto LAB_6bd2;
  }
  s = g.gCurSubstateA + 0x04;
  g.gCurSubstateA = s;
  if (s < 0x40)
    goto LAB_6bde;
  s = 0x40;
LAB_6bd2:
  g.gCurSubstateA = s;
  g.DAT_f7d1 ^= 0x04;  /* ROM emits `bnot #2,@r0`; ch38 doesn't pattern-match
                        either `^= 0x04` or `bN = !bN` to bnot */
LAB_6bde:
  if (!(sys_statusFlags_BIT.sleeping)) {
    g.gCurSubstateA = 0x68;
    DAT_f7d1_BIT.b1 = 0;
    DAT_f7d1_BIT.b2 = 0;
  }
}

// ROM: 0xa180  98.5%
void sys_enter_low_power(void) {
  CKSTPR1 |= 0x04;
  g.sys_walkerFlags = (g.sys_walkerFlags & 0xE7) | WALKER_MODE_LOW_POWER;
  RTCCR2 |= 0x01;
  g.ped_stepTimer = 0x1E;
  sys_statusFlags_BIT.sleeping = 0;
}

// ROM: 0xa29c  99.0%
void sys_enter_deep_sleep(void) {
  g.ped_activityTimer = 0x3C;
  g.ped_stepTimer = 0x5A;
  if ((g.sys_walkerFlags & WALKER_MODE_MASK) != WALKER_MODE_DEEP_SLEEP) {
    if ((g.sys_walkerFlags & WALKER_MODE_MASK) == WALKER_MODE_ACTIVE) {
      g.ped_sampleCount = 0;
    }
    g.sys_walkerFlags = (g.sys_walkerFlags & 0xE7) | WALKER_MODE_DEEP_SLEEP;
    RTCCR2 |= 0x01;
    drv_lcd_reset();
  }
}

// ROM: 0xa2da  97.9%
void sys_wake_from_low_power(void) {
  drv_timerw_disable();
  CKSTPR1 &= ~0x04;
  RTCCR2 &= ~0x01;
  g.sys_walkerFlags &= 0xE7;
}

// ROM: 0x256e  94.7%
void sys_enter_sleep(uint16_t mode) {
  if (mode == 0) {
    SYSCR1 = 0xA7;
    SYSCR2 = 0xE0;
    sys_statusFlags_BIT.lcd_dirty = 1;
    sleep();
  } else if (mode == 1) {
    SYSCR1 = 0xAF;
    SYSCR2 = 0xE3;
    sys_statusFlags_BIT.lcd_dirty = 0;
    sleep();
  }
}

