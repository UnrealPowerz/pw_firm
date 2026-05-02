#include "all_headers.h"

// ROM: 0xb176  80.0%
void game_sync_walk_status(void) {
  /* The save record byte at offset 0x5B holds a few flag bits.  Declared
   * here MSB-first so that ".b1" really means bit 1 and ".b0" means bit 0. */
  typedef struct {
    uint8_t      : 5;
    uint8_t b2   : 1;
    uint8_t b1   : 1;
    uint8_t b0   : 1;
  } save_flag_byte_t;

  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x68);
  save_read_reliable(0x00ED, 0x01ED, buf, 0x68);

  walker_status_flags_BIT.session_active =
      ((save_flag_byte_t *)&buf[0x5B])->b0;
  walker_status_flags_BIT.walking =
      ((save_flag_byte_t *)&buf[0x5B])->b1;
}

// ROM: 0x048c  0.0%
#pragma option speed=register  /* pragma:auto */
void game_start_walk(void) {
  struct {
    uint16_t pad;
    uint8_t clear;
    uint8_t set;
  } marker;
  uint8_t settings_bit;
  uint8_t *trainer_buf;
  uint8_t *extra_buf;
  uint16_t r6;
  uint8_t j;

  marker.set = 0xA5;
  save_write_reliable(0x016F, 0x026F, &marker.set, 1);

  save_commit_staged_data();

  marker.clear = 0;
  save_write_reliable(0x016F, 0x026F, &marker.clear, 1);

  r6 = 0xCF0C;
  for (j = 0x18; j != 0; j--) {
    drv_eeprom_write_u8(r6 + 0x84, 0);
    r6 += 0x88;
  }

  drv_eeprom_fill(0xDE24, 0x1568, 0);

  DAT_f7a0 = 0;
  DAT_f790 = 0;
  DAT_f793 = 0;
  watts = 0;

  save_write_reliable(0x0156, 0x0256, (void *)&totalSteps, 0x18);

  walker_status_flags_BIT.walking = 1;
  walker_status_flags_BIT.session_active = 1;

  r6 = (uint16_t)DAT_f84e;
  save_read_reliable(0x00ED, 0x01ED, (uint8_t *)r6, 0x68);

  ((uint8_t *)r6)[0x5B] |= 0x01;
  ((uint8_t *)r6)[0x5B] |= 0x02;
  ((uint8_t *)r6)[0x5B] &= ~0x04;

  *(uint32_t *)(r6 + 0x00) = *(uint32_t *)DAT_f7e6;
  *(uint32_t *)(r6 + 0x04) = DAT_f7ea;
  *(uint16_t *)(r6 + 0x08) = DAT_f7ee;
  *(uint16_t *)(r6 + 0x0A) = DAT_f7f0;
  *(uint32_t *)(r6 + 0x0C) = DAT_f7f2;

  for (j = 0; j < 0x12; j++) {
    ((uint8_t *)r6)[0x48 + j] = DAT_f82e[j];
  }

  ((uint8_t *)r6)[0x5C] = DAT_f842;
  ((uint8_t *)r6)[0x5D] = DAT_f843;
  ((uint8_t *)r6)[0x5E] = 0;
  ((uint8_t *)r6)[0x5F] = 2;

  save_write_reliable(0x00ED, 0x01ED, (uint8_t *)r6, 0x68);

  sys_init_heap();
  trainer_buf = (uint8_t *)sbrk(0xBE);
  drv_eeprom_read_block(0x8F00, trainer_buf, 0xBE);

  settings_bit = ((RamCache_settingsByte & 1)) ? 1 : 0;
  extra_buf = (uint8_t *)sbrk(0x88);
  game_log_interaction(trainer_buf, extra_buf, 0x19, settings_bit, 0);

  save_clear_data();
}

// ROM: 0x0636  81.9%
void game_end_walk(void) {
  void *poke_base = (void *)DAT_f7e6;

  walker_status_flags_BIT.walking = 0;
  RamCache_settingsByte &= ~0x01;

  DAT_f793 = 0;
  watts = 0;

  save_write_reliable(0x0156, 0x0256, (void *)&totalSteps, 0x18);
  save_read_reliable(0x00ED, 0x01ED, poke_base, 0x68);

  *(uint32_t *)((uint8_t *)poke_base + 0x04) = 0;
  *(uint16_t *)((uint8_t *)poke_base + 0x0A) = 0;

  *(volatile uint8_t *)((uint8_t *)poke_base + 0x5B) &= ~0x02;
  *(volatile uint8_t *)((uint8_t *)poke_base + 0x5B) &= ~0x04;

  save_write_reliable(0x00ED, 0x01ED, poke_base, 0x68);
  save_clear_data();
  save_init_defaults();

  drv_eeprom_fill(0xB800, 0x06C8, 0);
  drv_eeprom_fill(0xDE24, 0x1568, 0);
  drv_eeprom_fill(0x8F00, 0x0010, 0);
}

// ROM: 0x06de  97.1%
void game_clear_stats(void) {
  watts = 0;
  save_write_reliable(0x0156, 0x0256, (void *)&totalSteps, 0x18);
  save_clear_data();
}
