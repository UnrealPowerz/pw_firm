#include "all_headers.h"

/*
 * Walk-session lifecycle.
 *
 *   game_sync_walk_status   Pull `flags_5b` bits 0/1 from the trainer record
 *                           into the g.walker_status_flags cache.
 *   game_start_walk         Begin a new walking session: stage the marker,
 *                           clear all peer-log slots, reset session counters,
 *                           pull staged peer-IR data into the trainer record,
 *                           log the session-start event.
 *   game_end_walk           End the session: clear flags, zero g.save_watts, wipe
 *                           per-session EEPROM regions, re-init defaults.
 *   game_clear_stats        Lighter wipe: zero g.save_watts + commit save block.
 *
 * `trainer_record.flags_5b` bit layout (set/cleared by these functions and
 * by game_init_peer_identity in bored_gift.c):
 *   bit 0 (0x01) - session_active (set on first-ever walk, never cleared)
 *   bit 1 (0x02) - walking        (set during an active walking session)
 *   bit 2 (0x04) - peer-init done (cleared at session start/end; set by
 *                                  game_init_peer_identity after first peer
 *                                  exchange)
 *
 * The `DAT_f7e6 .. DAT_f843` globals are a RAM staging area where the IR
 * comm-loop drops a received trainer record before game_start_walk commits
 * the fields into the persistent trainer record.
 */

// ROM: 0xb176  80.2%
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
  uint16_t slot_addr;
  struct trainer_record *rec;
  uint8_t i;

  /* Stage marker: write 0xA5, commit staged data, clear marker. This is the
     "session start in progress" sentinel for crash recovery. */
  marker.set = 0xA5;
  save_write_reliable(EEPROM_STAGE_MARKER, EEPROM_STAGE_MARKER_BACKUP, &marker.set, 1);

  save_commit_staged_data();

  marker.clear = 0;
  save_write_reliable(EEPROM_STAGE_MARKER, EEPROM_STAGE_MARKER_BACKUP, &marker.clear, 1);

  /* Clear the interaction-type byte (offset 0x84) in all 0x18 peer-log slots,
     marking them empty. */
  slot_addr = 0xCF0C;
  for (i = 0x18; i != 0; i--) {
    drv_eeprom_write_u8(slot_addr + 0x84, 0);
    slot_addr += 0x88;
  }

  drv_eeprom_fill(EEPROM_STEP_HIST, 0x1568, 0);

  g.recentSessionSteps = 0;
  g.save_sessionTicksElapsed = 0;
  g.save_peerSlotIndex = 0;
  g.save_watts = 0;

  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&g.save_totalSteps, 0x18);

  walker_status_flags_BIT.walking = 1;
  walker_status_flags_BIT.session_active = 1;

  /* Pull the IR-staged trainer record fields (DAT_f7e6..DAT_f843) into the
     persistent trainer record + commit. */
  rec = (struct trainer_record *)trainerRecBuf;
  save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, rec, sizeof(*rec));

  rec->flags_5b |= 0x01;         /* session_active */
  rec->flags_5b |= 0x02;         /* walking */
  rec->flags_5b &= ~0x04;        /* clear peer-init-done */

  rec->id        = *(uint32_t *)DAT_f7e6;
  rec->id_backup = DAT_f7ea;
  rec->loc       = DAT_f7ee;
  rec->loc_backup = DAT_f7f0;
  *(uint32_t *)rec->at_0c = DAT_f7f2;

  for (i = 0; i < 0x12; i++) {
    rec->at_48[i] = DAT_f82e[i];
  }

  rec->at_5c = DAT_f842;
  rec->at_5d = DAT_f843;
  rec->at_5e = 0;
  rec->at_5f = 2;

  save_write_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, rec, sizeof(*rec));

  /* Log the session-start event (interaction_type 0x19, use_wild_data flag
     mirrors the co-op settings bit). */
  sys_init_heap();
  trainer_buf = (uint8_t *)sbrk(0xBE);
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);

  settings_bit = ((g.save_settings & 1)) ? 1 : 0;
  extra_buf = (uint8_t *)sbrk(0x88);
  game_log_interaction(trainer_buf, extra_buf, 0x19, settings_bit, 0, 0);

  save_clear_data();
}

// ROM: 0x0636  82.3%  saves: r3,r6
void game_end_walk(void) {
  struct trainer_record *rec = (struct trainer_record *)DAT_f7e6;

  walker_status_flags_BIT.walking = 0;
  g.save_settings &= ~0x01;

  g.save_peerSlotIndex = 0;
  g.save_watts = 0;

  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&g.save_totalSteps, 0x18);
  save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, rec, sizeof(*rec));

  rec->id_backup = 0;
  rec->loc_backup = 0;

  rec->flags_5b &= ~0x02;        /* clear walking */
  rec->flags_5b &= ~0x04;        /* clear peer-init-done */

  save_write_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, rec, sizeof(*rec));
  save_clear_data();
  save_clear_peer_log_slots();

  drv_eeprom_fill(EEPROM_STEP_HIST_FLAGS, 0x06C8, 0);
  drv_eeprom_fill(EEPROM_STEP_HIST, 0x1568, 0);
  drv_eeprom_fill(EEPROM_TRAINER_PROFILE, 0x0010, 0);
}

// ROM: 0x06de  98.4%
void game_clear_stats(void) {
  g.save_watts = 0;
  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&g.save_totalSteps, 0x18);
  save_clear_data();
}
