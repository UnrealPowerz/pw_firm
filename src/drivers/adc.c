#include "all_headers.h"

// Reason: ROM stores the sampled value on stack (`mov.w r0, @er7`) and reads
//   the high byte then low byte via `mov.b @er7, r0h` and `mov.b @(1,er7),
//   r0h` — exploiting big-endian stack layout. ch38 keeps the value in a
//   register and uses `(val >> 8)` for the high byte, producing a different
//   instruction stream (shift vs memory access).
// Class: cannot-fix-without-compiler-change (codegen choice between stack
//   spill + byte-aligned read vs register + shift)
// ROM: 0xa8f8  22.2%
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
  save_write_reliable(EEPROM_ACCEL_CAL, EEPROM_ACCEL_CAL_BACKUP, (uint8_t *)&val, 2);
  return 1;
}

// ROM: 0x27c2  67.9%
uint16_t drv_adc_add_calib_checksum(uint16_t val) {
  uint8_t sum;
  val &= 0x0FFF;
  sum = (uint8_t)((val >> 4) & 0x0F);
  sum += (uint8_t)((val >> 8) & 0x0F);
  sum += (uint8_t)(val & 0x0F);
  return val | ((uint16_t)sum << 12);
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

// ROM: 0x281e  85.3%
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

    if (statusFlags_BIT.lcd_dirty) {
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

// ROM: 0x289a  62.0%  saves: er3,r5,er6
uint8_t drv_adc_check_low_battery(uint16_t threshold) {
  uint16_t calib;
  uint16_t target;

  /* Read calibration data from EEPROM */
  save_read_reliable(EEPROM_ACCEL_CAL, EEPROM_ACCEL_CAL_BACKUP, &calib, 2);

  /* Validate CRC */
  if (drv_adc_validate_calib_checksum(calib) == 0) {
    calib = 0;
    save_write_reliable(EEPROM_ACCEL_CAL, EEPROM_ACCEL_CAL_BACKUP, &calib, 2);
  }

  /* Mask calibration bits and scale threshold */
  calib &= 0x0FFF;
  /* (calib * threshold) / 20 */
  target = (uint16_t)(((uint32_t)calib * threshold) / 20);

  if (drv_adc_sample() > target) {
    return 1;
  }
  return 0;
}

// ROM: 0x290a  96.3%
void drv_adc_check_battery(void) {
  if (statusFlags_BIT.battery_check_request) {
    if (drv_adc_check_low_battery(20)) {
      statusFlags_BIT.low_battery = 1;
    } else {
      statusFlags_BIT.low_battery = 0;
    }
    statusFlags_BIT.battery_check_request = 0;
  }
}

