#include "all_headers.h"

// ROM: 0x9b34  97.9%
void drv_buttons_init_irqs(void) {
  uint8_t tmp;

  g.btn_inputRaw = 0;
  g.btn_inputPrevious = 0;
  g.btn_trigger = 0;
  g.btn_holdDuration = 0;

  set_ccr(0x80);

  tmp = PFCR;
  tmp &= 0xFC;
  PFCR = tmp;
  IEGR |= 0x01;
  IRR1 &= ~0x01;
  IENR1 |= 0x01;

  tmp = PFCR;
  tmp &= 0xF3;
  PFCR = tmp;
  IEGR |= 0x02;
  IRR1 &= ~0x02;
  IENR1 |= 0x02;

  PDRB |= 0x20;
  PDR8 |= 0x10;
  PCR8 &= ~0x10;

  set_ccr(0x00);
}

// ROM: 0x9b84  96.9%
void drv_button_read(void) {
  g.btn_inputRaw = 0;

  if (PDRB_BIT.B0) {
    btn_inputRaw_BIT.btn_r = 1;
    if (g.wakeupFlagMaybe[0]) {
      g.btn_holdDuration++;
    }
  } else {
    g.btn_holdDuration = 0;
  }

  if (statusFlags_BIT.button_event) {
    btn_inputRaw_BIT.btn_r = 1;
    statusFlags_BIT.button_event = 0;
  }

  if (PDRB_BIT.B2) {
    btn_inputRaw_BIT.btn_m = 1;
  }

  if (PDRB_BIT.B4) {
    btn_inputRaw_BIT.btn_l = 1;
  }

  g.btn_trigger = (g.btn_inputRaw ^ g.btn_inputPrevious) & g.btn_inputRaw;
  g.btn_inputPrevious = g.btn_inputRaw;

  if (g.btn_trigger) {
    g.activityTimer = 0x5A;
    g.accelSampleCount = 0;
    if ((g.walker_status_flags & WALKER_MODE_MASK) != WALKER_MODE_DEEP_SLEEP) {
      g.btn_trigger = 0;
    }
  }

  if (g.btn_holdDuration >= 8) {
    if ((g.walker_status_flags & WALKER_MODE_MASK) != WALKER_MODE_DEEP_SLEEP) {
      sys_enter_deep_sleep();
      walker_status_flags_BIT.input_pending = 1;
    }
  }
}

// ROM: 0x9c40  100.0%
uint8_t drv_button_is_triggered(uint8_t mask) {
  return g.btn_trigger & mask;
}
