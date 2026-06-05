#include "all_headers.h"

// ROM: 0x369c  89.2%
uint8_t drv_sound_is_playing(void) {
  if (g.sound_dataPointer == NULL) {
    return 0;
  }
  return 1;
}

// ROM: 0x36aa  95.4%
void drv_timerw_init(void) {
  g.sound_header = 0x78;
  g.sound_volume = 0;
  PCR8 |= 0x0C;
  PDR8 &= ~0x04;
  PDR8 &= ~0x08;
  CKSTPR2 |= 0x40;
  TCRW = 0xC0;
  TIOR0 = 0x10;
  TIOR1 = 0x01;
  GRA = 0;
  GRB = 0;
  GRC = 0;
  CKSTPR2 &= ~0x40;
  g.sound_dataPointer = NULL;
}

// ROM: 0x37d6  80.3%
void drv_timerw_enable(void) {
  CKSTPR2 |= 0x40;
  TIERW &= ~0x01;
  TCRW = 0xC0;
  TIOR0 = 0x10;
  TIOR1 = 0x01;
  TSRW &= ~0x01;
  IEGR |= 0x01;
  TCNT = 0;
  TMRW = 0x80;
  TIERW |= 0x01;
}

// ROM: 0x3810  96.7%
void drv_timerw_disable(void) {
  TIERW &= ~0x01;
  TMRW = 0;
  TCRW = 0xC0;
  TSRW &= ~0x01;
  CKSTPR2 &= ~0x40;
}

// ROM: 0x3832  100.0%
void drv_sound_set_volume(uint8_t v) {
  g.sound_volume = v;
  (void)0;
}

// ROM: 0x3838  72.6%
void drv_sound_set_freq_pwm(uint8_t freq) {
  uint16_t f = freq;
  switch (g.sound_volume) {
  case 0:
    GRA = f;
    GRB = f;
    GRC = f;
    break;
  case 1:
    GRA = f;
    GRB = f >> 1;
    GRC = f;
    break;
  case 2:
    GRA = f;
    f >>= 1;
    GRB = f;
    GRC = f;
    break;
  }
  TCNT = 0;
}

// ROM: 0x37c6  100.0%
void drv_sound_set_data(uint8_t *data) {
  g.sound_dataPointer = data;
  g.sound_noteDuration = 0;
  g.sound_isSeparateNote = 0;
}

// ROM: 0x36f2  71.0%  saves: er3,er4,er5,er6
void drv_sound_play(uint8_t sound_idx) {
  uint16_t offset;
  uint8_t *src_ptr;
  register uint8_t i;
  register uint8_t sum;
  struct {
    uint16_t offset;
    uint8_t len;
    uint8_t chk;
  } metadata;

  if (g.sound_volume == 0)
    return;

  TIERW &= ~0x01;
  drv_eeprom_read_block(0x8CB0 + (sound_idx * 4), &metadata, 4);

  offset = (metadata.offset >> 8) | (metadata.offset << 8);
  src_ptr = (uint8_t *)0x8CF0 + offset;

  if (metadata.len > 0xC0) {
    goto end;
  }

  g.sound_dataPointer = ACCEL_SAMPLES_X;
  drv_eeprom_read_block((uint16_t)(uintptr_t)src_ptr, g.sound_dataPointer, metadata.len);

  sum = 0;
  i = 0;
  while (i < (metadata.len >> 1)) {
    sum += g.sound_dataPointer[i * 2];
    sum += g.sound_dataPointer[i * 2 + 1];
    i++;
  }

  if (sum == metadata.chk) {
    if ((g.sound_dataPointer[(metadata.len >> 1) * 2 - 1] & 0x7F) >= 0x7E) {
      g.sound_noteDuration = 0;
      g.sound_isSeparateNote = 0;
    } else {
      g.sound_dataPointer = NULL;
    }
  } else {
    g.sound_dataPointer = NULL;
  }

end:
  TIERW |= 0x01;
}

// ROM: 0x388c  62.8%  saves: r6,r5
#pragma option noregexpansion  /* pragma:auto */
void drv_sound_update(void) {
  if (g.sound_dataPointer == NULL)
    return;

  if (g.sound_noteDuration != 0) {
    g.sound_noteDuration--;
    if (g.sound_noteDuration == 1) {
      if ((g.sound_dataPointer[1] & 0x7F) == 0x7F) {
        TMRW = 0x80;
        TIOR0 = 0x10;
        TIOR1 = 0x01;
      }
    }
    if (g.sound_noteDuration != 0)
      return;
  }

  if (g.sound_noteDuration == 0) {
    if (g.sound_isSeparateNote != 0) {
      GRA = 0x140;
      GRB = 0x140;
      GRC = 0x140;
      TCNT = 0;
      if (g.sound_isSeparateNote != 0) {
        g.sound_isSeparateNote--;
      }
      return;
    }
  }

  if ((g.sound_dataPointer[1] & 0x7F) == 0x7F) {
    g.sound_dataPointer = NULL;
    return;
  }

  if ((g.sound_dataPointer[1] & 0x7F) == 0x7B) {
    g.sound_header = g.sound_dataPointer[0];
    g.sound_dataPointer += 2;
  }

  if ((g.sound_dataPointer[1] & 0x7F) == 0x7E) {
    g.sound_dataPointer = ACCEL_SAMPLES_X;
    return;
  }

  if ((g.sound_dataPointer[1] & 0x7F) == 0x7D) {
    uint32_t t = (uint32_t)0x14000 * g.sound_dataPointer[0];
    uint16_t d = (uint16_t)(t / g.sound_header);
    uint8_t divisor = SOUND_PERIOD_TABLE[g.sound_dataPointer[1] & 0x7F];
    g.sound_noteDuration = (uint32_t)d / divisor;
    drv_sound_set_freq_pwm(0);
    g.sound_dataPointer += 2;
    return;
  }

  if (g.sound_dataPointer[1] & 0x80) {
    uint32_t t = (uint32_t)0x14000 * g.sound_dataPointer[0];
    uint16_t d = (uint16_t)(t / g.sound_header);
    uint8_t divisor = SOUND_PERIOD_TABLE[g.sound_dataPointer[1] & 0x7F];
    g.sound_noteDuration = (uint32_t)d / divisor;
    g.sound_isSeparateNote = 0;
  } else {
    uint32_t t = (uint32_t)0x14000 * g.sound_dataPointer[0];
    uint16_t d = (uint16_t)(t / g.sound_header);
    uint8_t divisor = SOUND_PERIOD_TABLE[g.sound_dataPointer[1] & 0x7F];
    d -= 0x140;
    g.sound_noteDuration = (uint32_t)d / divisor;
    g.sound_isSeparateNote = 1;
  }

  if (g.sound_dataPointer != ACCEL_SAMPLES_X) {
    if ((g.sound_dataPointer[-1] & 0x80) != 0x80) {
      TMRW = 0x83;
      TCRW = 0xC2;
      drv_sound_set_freq_pwm(SOUND_PERIOD_TABLE[g.sound_dataPointer[1] & 0x7F]);
    }
  }

  g.sound_dataPointer += 2;
}

