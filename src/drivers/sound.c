#include "all_headers.h"

// ROM: 0x369c  88.3%
uint8_t drv_sound_is_playing(void) {
  if (soundData == NULL) {
    return 0;
  }
  return 1;
}

// ROM: 0x36aa  94.7%
void drv_timerw_init(void) {
  soundHeader = 0x78;
  volume = 0;
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
  soundData = NULL;
}

// ROM: 0x37d6  91.4%
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
}

// ROM: 0x3810  96.7%
void drv_timerw_disable(void) {
  TIERW &= ~0x01;
  TMRW = 0;
  TCRW = 0xC0;
  TSRW &= ~0x01;
  CKSTPR2 &= ~0x40;
}

// ROM: 0x3832  97.5%
void drv_sound_set_volume(uint8_t v) {
  volume = v;
  (void)0;
}

// ROM: 0x3838  72.4%
void drv_sound_set_freq_pwm(uint8_t freq) {
  uint16_t f = freq;
  switch (volume) {
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

// ROM: 0x37c6  97.0%
void drv_sound_set_data(uint8_t *data) {
  soundData = data;
  noteDuration = 0;
  isSeparateNote = 0;
}

// ROM: 0x36f2  75.5%
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

  if (volume == 0)
    return;

  TIERW &= ~0x01;
  drv_eeprom_read_block(0x8CB0 + (sound_idx * 4), &metadata, 4);

  offset = (metadata.offset >> 8) | (metadata.offset << 8);
  src_ptr = (uint8_t *)0x8CF0 + offset;

  if (metadata.len > 0xC0) {
    goto end;
  }

  soundData = ACCEL_SAMPLES_X;
  drv_eeprom_read_block((uint16_t)(uintptr_t)src_ptr, soundData, metadata.len);

  sum = 0;
  i = 0;
  while (i < (metadata.len >> 1)) {
    sum += soundData[i * 2];
    sum += soundData[i * 2 + 1];
    i++;
  }

  if (sum == metadata.chk) {
    if ((soundData[(metadata.len >> 1) * 2 - 1] & 0x7F) >= 0x7E) {
      noteDuration = 0;
      isSeparateNote = 0;
    } else {
      soundData = NULL;
    }
  } else {
    soundData = NULL;
  }

end:
  TIERW |= 0x01;
}

// ROM: 0x388c  78.1%
#pragma option noregexpansion  /* pragma:auto */
void drv_sound_update(void) {
  if (soundData == NULL)
    return;

  if (noteDuration != 0) {
    noteDuration--;
    if (noteDuration == 1) {
      if ((soundData[1] & 0x7F) == 0x7F) {
        TMRW = 0x80;
        TIOR0 = 0x10;
        TIOR1 = 0x01;
      }
    }
    if (noteDuration != 0)
      return;
  }

  if (noteDuration == 0) {
    if (isSeparateNote != 0) {
      GRA = 0x140;
      GRB = 0x140;
      GRC = 0x140;
      TCNT = 0;
      if (isSeparateNote != 0) {
        isSeparateNote--;
      }
      return;
    }
  }

  if ((soundData[1] & 0x7F) == 0x7F) {
    soundData = NULL;
    return;
  }

  if ((soundData[1] & 0x7F) == 0x7B) {
    soundHeader = soundData[0];
    soundData += 2;
  }

  if ((soundData[1] & 0x7F) == 0x7E) {
    soundData = ACCEL_SAMPLES_X;
    return;
  }

  if ((soundData[1] & 0x7F) == 0x7D) {
    uint32_t t = (uint32_t)0x14000 * soundData[0];
    uint16_t d = (uint16_t)(t / soundHeader);
    uint8_t divisor = PERIODTAB[soundData[1] & 0x7F];
    noteDuration = (uint32_t)d / divisor;
    drv_sound_set_freq_pwm(0);
    soundData += 2;
    return;
  }

  if (soundData[1] & 0x80) {
    uint32_t t = (uint32_t)0x14000 * soundData[0];
    uint16_t d = (uint16_t)(t / soundHeader);
    uint8_t divisor = PERIODTAB[soundData[1] & 0x7F];
    noteDuration = (uint32_t)d / divisor;
    isSeparateNote = 0;
  } else {
    uint32_t t = (uint32_t)0x14000 * soundData[0];
    uint16_t d = (uint16_t)(t / soundHeader);
    uint8_t divisor = PERIODTAB[soundData[1] & 0x7F];
    d -= 0x140;
    noteDuration = (uint32_t)d / divisor;
    isSeparateNote = 1;
  }

  if (soundData != ACCEL_SAMPLES_X) {
    if ((soundData[-1] & 0x80) != 0x80) {
      TMRW = 0x83;
      TCRW = 0xC2;
      drv_sound_set_freq_pwm(PERIODTAB[soundData[1] & 0x7F]);
    }
  }

  soundData += 2;
}

