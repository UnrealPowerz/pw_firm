#include "all_headers.h"

// ROM: 0xa830  76.6%
uint8_t drv_accel_factory_test(void) {
  uint8_t buf[6];
  uint8_t *p;
  uint16_t i;

  p = buf;

  if (drv_accel_init() == 0) {
    return 0;
  }

  /* Read register 0x14, set bit 3, write back */
  drv_accel_read_reg(0x14, 0x602, p);
  buf[0] &= 0xE0;
  buf[0] |= 0x08;
  drv_accel_write_reg(0x14, buf[0]);
  drv_accel_write_reg(0x00, 0x0A);

  /* Wait 0x1F4 iterations */
  i = 0x1F4;
  do {
    sys_delay_short();
    i--;
  } while (i != 0);

  /* Read 6 bytes of accel data starting at reg 0x02 */
  drv_accel_read_reg(0x02, 0x602, p);

  /* Send buf[1] via SPI */
  PDR1 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = buf[1];
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
  sys_delay_short();

  /* Send buf[3] via SPI */
  PDR1 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = buf[3];
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
  sys_delay_short();

  /* Send buf[5] via SPI */
  PDR1 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = buf[5];
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
  sys_delay_short();

  return 1;
}

// ROM: 0x76aa  77.1%
void drv_accel_sample(void) {
  uint8_t buf[6];
  register uint8_t *pBuf;
  register uint8_t prev_count;
  register uint16_t r4_dummy;

  r4_dummy = (uint16_t)pBuf;

  SSMR = 0x87;
  PDR9 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x0A;
  while (!SSSR_BIT.TEND)
    ;
  SSTDR = 0x00;

  TCSRWD1 = 0x5E;
  TCWD = 0x00;
  TCSRWD1 = 0x9E;

  SYSCR1 = 0xA7;
  SYSCR2 = 0xEB;
  statusFlags |= 0x10;
  PDR9 |= 0x01;
  SSMR = 0x86;
  sleep();

  pBuf = buf;
  drv_accel_read_reg(0x02, 6, pBuf);

  PDR9 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x0A;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = 0x01;
  while (!SSSR_BIT.TEND)
    ;
  PDR9 |= 0x01;

  accelXSamples[accelSampleCount] = (int8_t)buf[1];
  accelYSamples[accelSampleCount] = (int8_t)buf[3];
  accelZSamples[accelSampleCount] = (int8_t)buf[5];

  if ((uint8_t)currentlyActiveView == 0x17) {
    prev_count = (accelSampleCount + 0x3F) & 0x3F;
    if (accelSampleCount == 0) {
      accelXPos = 0;
      accelYPos = 0;
      accelZPos = 0;
    }

    if ((int16_t)accelXSamples[accelSampleCount] -
            (int16_t)accelXSamples[prev_count] <
        0) {
      accelXPos += (uint16_t)(-accelXSamples[accelSampleCount] +
                              accelXSamples[prev_count]);
    } else {
      accelXPos += (uint16_t)(accelXSamples[accelSampleCount] -
                              accelXSamples[prev_count]);
    }

    if ((int16_t)accelYSamples[accelSampleCount] -
            (int16_t)accelYSamples[prev_count] <
        0) {
      accelYPos += (uint16_t)(-accelYSamples[accelSampleCount] +
                              accelYSamples[prev_count]);
    } else {
      accelYPos += (uint16_t)(accelYSamples[accelSampleCount] -
                              accelYSamples[prev_count]);
    }

    if ((int16_t)accelZSamples[accelSampleCount] -
            (int16_t)accelZSamples[prev_count] <
        0) {
      accelZPos += (uint16_t)(-accelZSamples[accelSampleCount] +
                              accelZSamples[prev_count]);
    } else {
      accelZPos += (uint16_t)(accelZSamples[accelSampleCount] -
                              accelZSamples[prev_count]);
    }
  }
}

// ROM: 0x26a0  81.1%
uint8_t drv_accel_read_reg(uint8_t addr, uint16_t count, uint8_t *dst) {
  uint8_t ssr;
  uint8_t tmp_addr;

  tmp_addr = addr;
  tmp_addr |= 0x80;

  ssr = SSSR;
  ssr &= 0x04;
  SSSR = ssr;

  SSER = 0xC0;
  PDR9 &= ~0x01;

  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = tmp_addr;

  while (!SSSR_BIT.RDRF)
    ;
  ssr = SSRDR;

  while (count != 0) {
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xFF;

    while (!SSSR_BIT.RDRF)
      ;
    *dst = SSRDR;
    dst++;
    count--;
  }

  while (!SSSR_BIT.TEND)
    ;
  PDR9 |= 0x01;
  SSER = 0x80;

  return 0;
}

// ROM: 0x270a  98.5%
void drv_accel_write_reg(uint8_t addr, uint8_t val) {
  SSER = 0x80;
  PDR9 &= ~0x01;

  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = addr;

  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = val;

  while (!SSSR_BIT.TEND)
    ;
  PDR9 |= 0x01;
}

// ROM: 0x273c  72.0%
uint8_t drv_accel_init(void) {
  uint8_t flags;
  uint8_t tmp;
  uint8_t ssr;

  ssr = SSSR;
  ssr &= 0x04;
  SSSR = ssr;

  drv_accel_write_reg(0x15, 0x80);

  drv_accel_read_reg(0x02, 2, &flags);

  if ((flags & 0x07) == 0x02) {
    drv_accel_read_reg(0x14, 1, &tmp);
    tmp = (tmp & 0xE0) | 0x06;
    drv_accel_write_reg(0x14, tmp);

    drv_accel_write_reg(0x0B, 0x00);
    drv_accel_write_reg(0x0A, 0x10);

    drv_accel_read_reg(0x1E, 1, &tmp);
    tmp |= 0x80;
    drv_accel_write_reg(0x1E, tmp);

    drv_accel_write_reg(0x0A, 0x00);
    PDR9 |= 0x01;

    return 1;
  }

  PDR9 |= 0x01;
  return 0;
}

// ROM: 0x60da  16.6%
#pragma option speed=loop=2  /* pragma:auto */
void drv_accel_fft(void *samples) {
  int16_t *real_buf;
  int16_t *imag_buf;
  int8_t *src = (int8_t *)samples;
  uint8_t i;

  sys_init_heap();
  real_buf = (int16_t *)sbrk(0x80);
  imag_buf = (int16_t *)sbrk(0x80);

  for (i = 0; i < 64; i += 2) {
    real_buf[i] = (int16_t)src[i];
    real_buf[i + 1] = (int16_t)src[i + 1];
  }

  for (i = 0; i < 64; i += 2) {
    imag_buf[i] = 0;
    imag_buf[i + 1] = 0;
  }

  /* 64-point FFT butterfly logic */
  /* Magnitude calculation into fft_results */
  for (i = 0; i < 32; i++) {
    int16_t mag = (real_buf[i] < 0 ? -real_buf[i] : real_buf[i]) +
                  (imag_buf[i] < 0 ? -imag_buf[i] : imag_buf[i]);
    fft_results[i] = mag;
  }
}
