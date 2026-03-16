#include "all_headers.h"

// ROM: 0x9b34  97.0%
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

// ROM: 0x9b84  87.5%
void drv_button_read(void) {
  buttonInputRaw = 0;

  if (PDRB & 0x01) {
    buttonInputRaw |= 0x02;
    if (wakeupFlagMaybe) {
      buttonHoldDuration++;
    }
  } else {
    buttonHoldDuration = 0;
  }

  if (statusFlags & 0x08) {
    buttonInputRaw |= 0x02;
    statusFlags &= ~0x08;
  }

  if (PDRB & 0x04) {
    buttonInputRaw |= 0x04;
  }

  if (PDRB & 0x10) {
    buttonInputRaw |= 0x08;
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
      walker_status_flags |= 0x01;
    }
  }
}

// ROM: 0x9c40  98.3%
uint8_t drv_button_is_triggered(uint8_t mask) {
  return buttonTrigger & mask;
}
