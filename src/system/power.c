#include "all_headers.h"

// ROM: 0x6b4c  83.0%
void sys_enter_standby(void) {
  if (!(DAT_f7d1 & 0x04)) {
    gCurSubstateA += 0xFC;
    if (gCurSubstateA <= 0x20) {
      gCurSubstateA = 0x20;
    }
  } else {
    gCurSubstateA += 0x04;
    if (gCurSubstateA >= 0x60) {
      DAT_f7d1 |= 0x02;
      DAT_f7d1 &= ~0x04;
    }
  }
  if (statusFlags & 0x80) {
    if (gCurSubstateA <= 0x20) {
      DAT_f7d1 |= 0x04;
      DAT_f7d1 |= 0x01;
    }
  }
}

// ROM: 0x6ba0  85.6%
void sys_update_standby_state(void) {
  uint8_t s;
  if ((DAT_f7ac & 0x03) != 0) {
    return;
  }
  if (!(DAT_f7d1 & 0x04)) {
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
  DAT_f7d1 ^= 0x04;
LAB_6bde:
  if (!(statusFlags & 0x80)) {
    gCurSubstateA = 0x68;
    DAT_f7d1 &= ~0x02;
    DAT_f7d1 &= ~0x04;
  }
}

// ROM: 0x259e  97.7%
void sys_wdt_kick(void) {
  TCSRWD1 = 0x5E;
  TCWD = 0;
  TCSRWD1 = 0x9E;
}

// ROM: 0xa180  96.9%
void sys_power_save_low_power(void) {
  CKSTPR1 |= 0x04;
  walker_status_flags = (walker_status_flags & 0xE7) | 0x08;
  RTCCR2 |= 0x01;
  stepTimer = 0x1E;
  statusFlags &= ~0x80;
}

// ROM: 0xa29c  97.3%
void sys_enter_deep_sleep(void) {
  activityTimer = 0x3C;
  stepTimer = 0x5A;
  if ((walker_status_flags & 0x18) != 0x10) {
    if ((walker_status_flags & 0x18) == 0) {
      accelSampleCount = 0;
    }
    walker_status_flags = (walker_status_flags & 0xE7) | 0x10;
    RTCCR2 |= 0x01;
    drv_lcd_reset();
  }
}

// ROM: 0xa2da  96.6%
void sys_wake_from_low_power(void) {
  drv_timerw_disable();
  CKSTPR1 &= ~0x04;
  RTCCR2 &= ~0x01;
  walker_status_flags &= 0xE7;
}

// ROM: 0x256e  94.3%
void sys_enter_sleep(uint16_t mode) {
  if (mode == 0) {
    SYSCR1 = 0xA7;
    SYSCR2 = 0xE0;
    statusFlags |= 0x10;
    sleep();
  } else if (mode == 1) {
    SYSCR1 = 0xAF;
    SYSCR2 = 0xE3;
    statusFlags &= ~0x10;
    sleep();
  }
}

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
