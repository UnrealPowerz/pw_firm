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

/* Reason: prologue/frame mismatch defeats compare_bin alignment.
 * ROM saves er2/er4/er5/er6 (4 long pushes = 16B) + 4B stack alloc.
 * ch38 saves r4/r5/r6 (3 word pushes = 6B) + 4B stack alloc.  ROM uses
 * 32-bit pointer locals throughout; ch38 picks 16-bit registers because
 * our locals are uint16_t/uint8_t*-sized in the small-data model.  Could
 * try forcing 32-bit locals but inconclusive -- compare_bin shows 0%
 * because alignment can't latch onto a function this misaligned at
 * the prologue.  Body is structurally correct (cleaned up vs the
 * original (uint16_t)cast(uint8_t*) gymnastics).
 * Class: cannot-fix-without-compiler-change */
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
  uint16_t addr;
  uint8_t *p;
  uint8_t j;

  marker.set = 0xA5;
  save_write_reliable(0x016F, 0x026F, &marker.set, 1);

  save_commit_staged_data();

  marker.clear = 0;
  save_write_reliable(0x016F, 0x026F, &marker.clear, 1);

  addr = 0xCF0C;
  for (j = 0x18; j != 0; j--) {
    drv_eeprom_write_u8(addr + 0x84, 0);
    addr += 0x88;
  }

  drv_eeprom_fill(0xDE24, 0x1568, 0);

  DAT_f7a0 = 0;
  DAT_f790 = 0;
  DAT_f793 = 0;
  watts = 0;

  save_write_reliable(0x0156, 0x0256, (void *)&totalSteps, 0x18);

  walker_status_flags_BIT.walking = 1;
  walker_status_flags_BIT.session_active = 1;

  p = DAT_f84e;
  save_read_reliable(0x00ED, 0x01ED, p, 0x68);

  p[0x5B] |= 0x01;
  p[0x5B] |= 0x02;
  p[0x5B] &= ~0x04;

  *(uint32_t *)(p + 0x00) = *(uint32_t *)DAT_f7e6;
  *(uint32_t *)(p + 0x04) = DAT_f7ea;
  *(uint16_t *)(p + 0x08) = DAT_f7ee;
  *(uint16_t *)(p + 0x0A) = DAT_f7f0;
  *(uint32_t *)(p + 0x0C) = DAT_f7f2;

  for (j = 0; j < 0x12; j++) {
    p[0x48 + j] = DAT_f82e[j];
  }

  p[0x5C] = DAT_f842;
  p[0x5D] = DAT_f843;
  p[0x5E] = 0;
  p[0x5F] = 2;

  save_write_reliable(0x00ED, 0x01ED, p, 0x68);

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
