#include "all_headers.h"

// ROM: 0xa8f8  19.0%
uint8_t drv_adc_test(void) {
  uint16_t val = drv_adc_sample();
  PDR1 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = (uint8_t)(val >> 8);
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
  sys_delay_short();

  PDR1 &= ~0x01;
  while (!SSSR_BIT.TDRE)
    ;
  SSTDR = (uint8_t)val;
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
  sys_delay_short();

  val = drv_adc_add_calib_checksum(val);
  save_write_reliable(0x0080, 0x0180, (uint8_t *)&val, 2);
  return 1;
}

// ROM: 0x27c2  67.9%
uint16_t drv_adc_add_calib_checksum(uint16_t val) {
  uint8_t sum;
  sum = (uint8_t)((val >> 4) & 0x0F);
  sum += (uint8_t)((val >> 8) & 0x0F);
  sum += (uint8_t)(val & 0x0F);
  return (val & 0x0FFF) | ((uint16_t)sum << 12);
}

// ROM: 0x27ec  63.7%
uint8_t drv_adc_validate_calib_checksum(uint16_t val) {
  uint8_t sum;
  sum = (uint8_t)((val >> 4) & 0x0F);
  sum += (uint8_t)((val >> 8) & 0x0F);
  sum += (uint8_t)(val & 0x0F);
  sum &= 0x0F;
  if ((uint8_t)(val >> 12) == sum)
    return 1;
  return 0;
}

// ROM: 0x281e  83.0%
int16_t drv_adc_sample(void) {
  int32_t sum = 0;
  uint16_t count = 8;

  PCR8 |= 0x10;
  PDR8 = 0x10;
  sys_delay_short();

  do {
    CKSTPR1 |= 0x10;
    nop();
    nop();
    nop();
    nop();
    nop();
    IENR2 &= 0xBF;
    AMR = (AMR & 0xF0) | 0x07;

    if (statusFlags & 0x10) {
      AMR = (AMR & 0xCF) | 0x20;
    } else {
      AMR = (AMR & 0xCF) | 0x30;
    }

    ADSR_BIT.ADSF = 1;
    while (ADSR_BIT.ADSF)
      ;

    AMR &= 0xF0;
    CKSTPR1 &= 0xEF;

    sum += (uint16_t)ADRR;
  } while (--count != 0);

  PDR8 = 0x00;
  PCR8 &= 0xEF;

  return (int16_t)(sum / 8);
}

// ROM: 0x289a  59.2%
uint8_t drv_adc_check_low_battery(uint16_t threshold) {
  uint16_t calib;
  uint16_t target;

  /* Read calibration data from EEPROM */
  save_read_reliable(0x0080, 0x0180, &calib, 2);

  /* Validate CRC */
  if (drv_adc_validate_calib_checksum(calib) == 0) {
    calib = 0;
    save_write_reliable(0x0080, 0x0180, &calib, 2);
  }

  /* Mask calibration bits and scale threshold */
  calib &= 0x0FFF;
  /* (calib * threshold) / 20 */
  target = (uint16_t)(((uint32_t)calib * threshold) / 20);

  if (drv_adc_sample() > target) {
    return 0; /* Battery OK */
  }
  return 1; /* Battery Low */
}

// ROM: 0x290a  88.7%
void drv_adc_check_battery(void) {
  /* bit 2 of statusFlags = request check */
  if (statusFlags & 0x04) {
    if (drv_adc_check_low_battery(20)) {
      statusFlags |= 0x02; /* set low battery flag */
    } else {
      statusFlags &= ~0x02; /* clear low battery flag */
    }
    statusFlags &= ~0x04; /* clear request flag */
  }
}

