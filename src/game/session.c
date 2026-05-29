#include "all_headers.h"

// ROM: 0xb176  80.0%
void game_sync_walk_status(void) {
  struct trainer_record *rec;

  sys_init_heap();
  rec = (struct trainer_record *)sbrk(sizeof(struct trainer_record));
  save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, rec, sizeof(struct trainer_record));

  walker_status_flags_BIT.session_active =
      ((byte_bits_t *)&rec->flags_5b)->BIT.b0;
  walker_status_flags_BIT.walking =
      ((byte_bits_t *)&rec->flags_5b)->BIT.b1;
}

/* Reason: prologue saves smaller register set than ROM, defeats alignment.
 * ROM saves er2/er4/er5/er6 (4 long pushes = 16B) + subs #4.  Our build
 * saves r4/r5/r6 (3 word pushes = 6B) + subs #4 -- ch38 picks 16-bit
 * pushes because our pointer locals are 16-bit in -cpu=300HN normal mode.
 * Forcing `addr` to uint32_t got one of the four pushes to ER but didn't
 * close the 0% score (alignment still fails).  300HA advanced mode would
 * give 24-bit pointers natively but fails to build with our linker setup.
 * Body is structurally correct.  Stuck until we find a way to express
 * "register-pressure-induced 32-bit pushes" in C/pragma form.
 * Class: cannot-fix-without-compiler-change */
// ROM: 0x048c  0.0%  saves: er2,er4,er5,er6
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
  struct trainer_record *p;
  uint8_t j;

  marker.set = 0xA5;
  save_write_reliable(EEPROM_STAGE_MARKER, EEPROM_STAGE_MARKER_BACKUP, &marker.set, 1);

  save_commit_staged_data();

  marker.clear = 0;
  save_write_reliable(EEPROM_STAGE_MARKER, EEPROM_STAGE_MARKER_BACKUP, &marker.clear, 1);

  addr = 0xCF0C;
  for (j = 0x18; j != 0; j--) {
    drv_eeprom_write_u8(addr + 0x84, 0);
    addr += 0x88;
  }

  drv_eeprom_fill(EEPROM_STEP_HIST, 0x1568, 0);

  recentSessionSteps = 0;
  sessionTicksElapsed = 0;
  peerSlotIndex = 0;
  watts = 0;

  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);

  walker_status_flags_BIT.walking = 1;
  walker_status_flags_BIT.session_active = 1;

  p = (struct trainer_record *)trainerRecBuf;
  save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, p, sizeof(*p));

  p->flags_5b |= 0x01;
  p->flags_5b |= 0x02;
  p->flags_5b &= ~0x04;

  p->id        = *(uint32_t *)DAT_f7e6;
  p->id_backup = DAT_f7ea;
  p->loc       = DAT_f7ee;
  p->loc_backup = DAT_f7f0;
  *(uint32_t *)p->at_0c = DAT_f7f2;

  for (j = 0; j < 0x12; j++) {
    p->at_48[j] = DAT_f82e[j];
  }

  p->at_5c = DAT_f842;
  p->at_5d = DAT_f843;
  p->at_5e = 0;
  p->at_5f = 2;

  save_write_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, p, sizeof(*p));

  sys_init_heap();
  trainer_buf = (uint8_t *)sbrk(0xBE);
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);

  settings_bit = ((RamCache_settingsByte & 1)) ? 1 : 0;
  extra_buf = (uint8_t *)sbrk(0x88);
  game_log_interaction(trainer_buf, extra_buf, 0x19, settings_bit, 0);

  save_clear_data();
}

// ROM: 0x0636  81.9%  saves: r3,r6
void game_end_walk(void) {
  struct trainer_record *rec = (struct trainer_record *)DAT_f7e6;

  walker_status_flags_BIT.walking = 0;
  RamCache_settingsByte &= ~0x01;

  peerSlotIndex = 0;
  watts = 0;

  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
  save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, rec, sizeof(*rec));

  rec->id_backup = 0;
  rec->loc_backup = 0;

  rec->flags_5b &= ~0x02;
  rec->flags_5b &= ~0x04;

  save_write_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, rec, sizeof(*rec));
  save_clear_data();
  save_init_defaults();

  drv_eeprom_fill(EEPROM_STEP_HIST_FLAGS, 0x06C8, 0);
  drv_eeprom_fill(EEPROM_STEP_HIST, 0x1568, 0);
  drv_eeprom_fill(EEPROM_TRAINER_PROFILE, 0x0010, 0);
}

// ROM: 0x06de  97.1%
void game_clear_stats(void) {
  watts = 0;
  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
  save_clear_data();
}
