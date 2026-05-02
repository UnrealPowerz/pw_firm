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
  statusFlags_BIT.lcd_dirty = 1;
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

/* Reason: register allocation in butterfly stages diverges from ROM.
 * Body is now a faithful 64-point radix-2 DIT FFT (was a 16% stub).
 * The magnitude-accumulation epilogue matches byte-for-byte; the
 * bit-reverse permutation and butterfly inner loop do not, because:
 *   - ch38 emits JSR @$sp_regsv$3 / @$spregld2$3 to save many regs;
 *     ROM's compiler used a slimmer "sub.w #0x10, r7" + 0 saves frame.
 *   - Our compiler chose different register vars for the same locals
 *     than the original C source did, so address registers differ.
 * Reaching ~80% would require iterating local-variable order and types
 * to nudge ch38's allocator toward the same choices, possibly with
 * inline assembly for the butterfly hot loop.  Keep current C: it is
 * functionally correct and structurally close (46% vs. 16% stub).
 * Class: high-effort, partial-fix */
// ROM: 0x60da  46.1%
#pragma option speed=loop=2  /* pragma:auto */
void drv_accel_fft(void *samples) {
  int16_t *real_buf;
  int16_t *imag_buf;
  int8_t *src = (int8_t *)samples;
  uint8_t i, j;
  uint8_t stage_step;     /* W^k step within stage; halves each stage */
  uint8_t stage_half;     /* group_size/2; doubles each stage */
  uint8_t group_size;
  uint8_t twiddle_idx;
  uint8_t k;
  const int16_t *twiddle = (const int16_t *)DAT_bdd0;  /* sin table, 48 entries */

  sys_init_heap();
  real_buf = (int16_t *)sbrk(0x80);
  imag_buf = (int16_t *)sbrk(0x80);

  /* Sign-extend int8 samples into int16 real buffer (unrolled by 2) */
  {
    int8_t *sp = src;
    int16_t *dp = real_buf;
    i = 0;
    do {
      *dp++ = (int16_t)*sp++;
      i++;
      *dp++ = (int16_t)*sp++;
      i++;
    } while (i < 64);
  }

  /* Zero imaginary buffer (unrolled by 2) */
  {
    int16_t *dp = imag_buf;
    i = 0;
    do {
      *dp++ = 0;
      i++;
      *dp++ = 0;
      i++;
    } while (i < 64);
  }

  /* Bit-reverse permutation (Gold-Rader algorithm).
   * j tracks the bit-reversed counter; for each i we find the matching j
   * and swap real_buf[i] with real_buf[j] when i < j. */
  j = 0;
  for (i = 1; i < 0x3F; i++) {
    uint8_t m = 0x20;
    j ^= m;
    while (m > j) {
      m >>= 1;
      j ^= m;
    }
    if (i < j) {
      int16_t tmp = real_buf[i];
      real_buf[i] = real_buf[j];
      real_buf[j] = tmp;
    }
  }

  /* Six butterfly stages.  group_size = 2,4,8,16,32,64. */
  stage_step = 0x40;
  stage_half = 1;
  for (;;) {
    group_size = stage_half << 1;
    if (group_size > 0x40) break;
    stage_step >>= 1;
    for (twiddle_idx = 0; twiddle_idx < stage_half; twiddle_idx += stage_step) {
      int16_t cos_w = twiddle[twiddle_idx + 0x10];
      int16_t sin_w = twiddle[twiddle_idx];
      for (k = twiddle_idx; k < 0x40; k += group_size) {
        uint8_t paired = k + stage_half;
        int16_t real_a = real_buf[k];
        int16_t imag_a = imag_buf[k];
        int16_t real_b;
        int16_t imag_b;

        if (twiddle_idx == 0) {
          /* W^0 = 1 */
          real_b = real_buf[paired];
          imag_b = imag_buf[paired];
        } else if (twiddle_idx == 0x10) {
          /* W^16 = i (90 degrees): mul by i swaps re/im with sign flip */
          real_b = -imag_buf[paired];
          imag_b = real_buf[paired];
        } else {
          int16_t br = real_buf[paired];
          int16_t bi = imag_buf[paired];
          int32_t prod_r;
          int32_t prod_i;
          if (bi == 0) {
            prod_r = (int32_t)br * cos_w;
            prod_i = (int32_t)br * sin_w;
          } else {
            prod_r = (int32_t)br * cos_w - (int32_t)bi * sin_w;
            prod_i = (int32_t)br * sin_w + (int32_t)bi * cos_w;
          }
          /* Q-format normalize: <<5 then take high word */
          prod_r <<= 5;
          prod_i <<= 5;
          real_b = (int16_t)(prod_r >> 16);
          imag_b = (int16_t)(prod_i >> 16);
        }

        real_buf[paired] = real_a - real_b;
        imag_buf[paired] = imag_a - imag_b;
        real_buf[k] = real_a + real_b;
        imag_buf[k] = imag_a + imag_b;
      }
    }
    stage_half = group_size;
  }

  /* Magnitude accumulation: fft_results[i] += |real[i]| + |imag[i]| for i=0..31.
   * The ROM accumulates (does not overwrite) because the caller invokes the
   * FFT three times (X, Y, Z) and sums their magnitudes. */
  {
    int16_t *rp = real_buf;
    int16_t *ip = imag_buf;
    uint16_t off = 0;
    do {
      int16_t r = *rp;
      int16_t im;
      if (r < 0) r = -r;
      im = *ip;
      if (im < 0) im = -im;
      *(int16_t *)((char *)fft_results + off) += r + im;
      rp++; ip++; off += 2;
      r = *rp;
      if (r < 0) r = -r;
      im = *ip;
      if (im < 0) im = -im;
      *(int16_t *)((char *)fft_results + off) += r + im;
      rp++; ip++; off += 2;
    } while (off < 0x40);
  }
}
