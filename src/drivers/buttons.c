#include "all_headers.h"

// ROM: 0x9b34  97.9%
void drv_buttons_init_irqs(void) {
  uint8_t tmp;

  buttonInputRaw = 0;
  prevButtonInputRaw = 0;
  buttonTrigger = 0;
  buttonHoldDuration = 0;

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
  buttonInputRaw = 0;

  if (PDRB_BIT.B0) {
    buttonInputRaw_BIT.btn_r = 1;
    if (wakeupFlagMaybe[0]) {
      buttonHoldDuration++;
    }
  } else {
    buttonHoldDuration = 0;
  }

  if (statusFlags_BIT.button_event) {
    buttonInputRaw_BIT.btn_r = 1;
    statusFlags_BIT.button_event = 0;
  }

  if (PDRB_BIT.B2) {
    buttonInputRaw_BIT.btn_m = 1;
  }

  if (PDRB_BIT.B4) {
    buttonInputRaw_BIT.btn_l = 1;
  }

  buttonTrigger = (buttonInputRaw ^ prevButtonInputRaw) & buttonInputRaw;
  prevButtonInputRaw = buttonInputRaw;

  if (buttonTrigger) {
    activityTimer = 0x5A;
    accelSampleCount = 0;
    if ((walker_status_flags & 0x18) != 0x10) {
      buttonTrigger = 0;
    }
  }

  if (buttonHoldDuration >= 8) {
    if ((walker_status_flags & 0x18) != 0x10) {
      sys_enter_deep_sleep();
      walker_status_flags_BIT.input_pending = 1;
    }
  }
}

// ROM: 0x9c40  100.0%
uint8_t drv_button_is_triggered(uint8_t mask) {
  return buttonTrigger & mask;
}
