#include "all_headers.h"

// ROM: 0x9b34  97.9%
void drv_buttons_init_irqs(void) {
  uint8_t tmp;

  g.btn_inputRaw.BYTE = 0;
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
  g.btn_inputRaw.BYTE = 0;

  if (PDRB_BIT.B0) {
    g.btn_inputRaw.BIT.btn_m = 1;
    if (g.sys_wakeFlag[0]) {
      g.btn_holdDuration++;
    }
  } else {
    g.btn_holdDuration = 0;
  }

  if (g.sys_statusFlags.BIT.button_event) {
    g.btn_inputRaw.BIT.btn_m = 1;
    g.sys_statusFlags.BIT.button_event = 0;
  }

  if (PDRB_BIT.B2) {
    g.btn_inputRaw.BIT.btn_r = 1;
  }

  if (PDRB_BIT.B4) {
    g.btn_inputRaw.BIT.btn_l = 1;
  }

  g.btn_trigger = (g.btn_inputRaw.BYTE ^ g.btn_inputPrevious) & g.btn_inputRaw.BYTE;
  g.btn_inputPrevious = g.btn_inputRaw.BYTE;

  if (g.btn_trigger) {
    g.sys_activityTimer = 0x5A;
    g.accel_sampleCount = 0;
    if ((g.sys_walkerFlags.BYTE & WALKER_MODE_MASK) != WALKER_MODE_DEEP_SLEEP) {
      g.btn_trigger = 0;
    }
  }

  if (g.btn_holdDuration >= 8) {
    if ((g.sys_walkerFlags.BYTE & WALKER_MODE_MASK) != WALKER_MODE_DEEP_SLEEP) {
      sys_enter_deep_sleep();
      g.sys_walkerFlags.BIT.input_pending = 1;
    }
  }
}

// ROM: 0x9c40  100.0%
uint8_t drv_button_is_triggered(uint8_t mask) {
  return g.btn_trigger & mask;
}
