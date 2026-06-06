#include "all_headers.h"

/*
 * Peer interaction log — 23-slot circular log in EEPROM written when the
 * walker pokemon catches something, dowses an item, completes a peer-play
 * session, etc. Slots are 0x88 bytes each at EEPROM_PEER_SLOT_BASE +
 * g.save_peerSlotIndex * 0x88. g.save_peerSlotIndex wraps mod 23.
 *
 *   game_log_interaction        Low-level: writes one log slot.
 *   game_log_poke_interaction   Wraps log_interaction for a caught pokemon
 *                               from the discard picker.
 *   game_log_item_interaction   Wraps log_interaction for a dowsed item.
 *   game_rotate_interaction_log Shift the entire 10-record log down by one
 *                               slot in EEPROM (oldest evicted).
 *   game_rotate_interaction_log_record
 *                               Build one fresh log record from the EEPROM
 *                               peer-context staging area and write it.
 *
 * The 0x88-byte peer-log slot layout (`b` in game_log_interaction):
 *   0x00 id, 0x0A loc, 0x0C move-id, 0x0E val_high, 0x20 nick (0x16 bytes),
 *   0x4C poke data (0x2A bytes), 0x76 hour, 0x77 misc, 0x78 recent-steps
 *   (uint16), 0x7C session-steps (uint32), 0x84 interaction-type,
 *   0x85/0x86 packed bitfields.
 * Source `a` is a 0xBE-byte trainer profile (EEPROM_TRAINER_PROFILE).
 */

/*
 * `a` is a 0xBE-byte pokemon/profile record (e.g. from EEPROM_TRAINER_PROFILE):
 *   0x00 trainer id, 0x0A loc, 0x0D gender/ability bits, 0x0E flag-byte,
 *   0x10..0x25 nickname (0x16), 0x26..0x27 misc bytes,
 *   0x28..0x51 poke data (0x2A), 0x52+ moves[0x10]
 * `b` is a 0x88-byte peer-log slot (written to EEPROM_PEER_SLOT_BASE+i*0x88):
 *   0x00 id, 0x0A loc, 0x0C move-id, 0x0E val_high, 0x20 nick, 0x4C poke,
 *   0x76 hour, 0x77 misc, 0x78 recent-steps, 0x7C session-steps,
 *   0x84 interaction-type, 0x85/0x86 packed bitfields
 * Not a trainer_record — kept as raw byte access pending peer_log_record type.
 */
// Reason: ROM uses inline prologue `push.w r3; push.w r4; push.l er5; push.l
//   er6; subs #2, r7` (14 bytes); ch38 uses `$sp_regsv$3 + subs #2`. Stack
//   layout differs throughout. Body structure (slot_off calc, EEPROM read,
//   reward zero loop, byte copies, bit-field updates, d_high dispatch)
//   appears correct.
// Class: cannot-fix-without-compiler-change (sp_regsv$3 helper)
// ROM: 0x4546  60.4%  saves: r3,r4,er5,er6 -> er5,er6
/* The 4th arg (d_high) selects between trainer-data copy (a[0x27..0x51]) and
 * the wild-pokemon EEPROM block (HOUR_MARKER + 0xBF50). The 6th arg
 * (event_subtype) is a separate dispatch: 1..3 = picks a move slot from
 * a[0x52 + (event_subtype-1)*0x10]; 4 = special case using EEPROM 0xBA44/0xBF08
 * + EEPROM_SPECIAL_BYTE. Decompiler originally conflated them into a single
 * d_high arg — fixed here so call sites pass the values to the right slot. */
// ROM: 0x4546  62.9%  saves: r3,r4,er5,er6 -> er5,er6
void game_log_interaction(uint8_t *trainer, uint8_t *log_slot,
                          uint8_t interaction_type, uint8_t use_wild_data,
                          uint16_t val_at_0e, uint8_t event_subtype) {
  uint16_t i;
  uint16_t slot_off;
  uint8_t prev_type;

  /* Probe the existing slot for its interaction type. */
  slot_off = (uint16_t)g.save_peerSlotIndex * 0x88 + 0xCF0C;
  prev_type = drv_eeprom_read_u8(slot_off + 0x84);

  /* Don't overwrite anything with a 0x1B-typed entry. */
  if (prev_type != 0) {
    if (interaction_type == 0x1B) {
      return;
    }
  }

  /* If the prior slot was the special 0x19 marker, skip to the next one. */
  if (prev_type == 0x19) {
    g.save_peerSlotIndex = (uint8_t)((int16_t)((uint16_t)g.save_peerSlotIndex + 1) % 23);
    slot_off = (uint16_t)g.save_peerSlotIndex * 0x88 + 0xCF0C;
  }

  /* For "fresh" interaction types (>0x0A), zero out the buffer first. */
  if (interaction_type > 0x0A) {
    for (i = 0; i < 0x88; i++) {
      log_slot[i] = 0;
    }
  }

  log_slot[0x84] = interaction_type;
  *((uint16_t *)(log_slot + 0x0E)) = val_at_0e;
  *((uint32_t *)log_slot) = g.save_rtcTime;
  *((uint16_t *)(log_slot + 0x78)) = g.session_recentSteps;
  *((uint32_t *)(log_slot + 0x7C)) = g.session_steps;
  *((uint16_t *)(log_slot + 0x0A)) = *((uint16_t *)trainer);

  /* Copy nickname bytes: trainer+0x10 -> log_slot+0x20, 0x16 bytes */
  for (i = 0; i < 0x16; i++) {
    log_slot[0x20 + i] = trainer[0x10 + i];
  }

  log_slot[0x77] = trainer[0x26];

  /* Gender/ability bits from trainer[0x0D] */
  {
    uint8_t src_byte;
    uint8_t rot_val;
    src_byte = trainer[0x0D];
    log_slot[0x85] = (uint8_t)((log_slot[0x85] & 0xE0) | (src_byte & 0x1F));

    /* ROM: rotl.b #3 then & 3 — extracts bits 5,6 to positions 0,1.
     * The (x<<3)|(x>>5) form coaxes ch38 to emit rotl.b #3 rather than DIVXU. */
    rot_val = (uint8_t)(((trainer[0x0D] << 3) | (trainer[0x0D] >> 5)) & 0x03);
    log_slot[0x85] = (uint8_t)((log_slot[0x85] & ~(0x03 << 1)) | (rot_val << 1));
  }
  /* ROM: bld #1, trainer[0xE]; bst #7, @log_slot+0x85  (unconditional bit copy) */
  ((byte_bits_t *)&log_slot[0x85])->BIT.b7 =
      ((byte_bits_t *)&trainer[0x0E])->BIT.b1;

  if (use_wild_data == 0) {
    /* Copy pokemon data from the trainer's own profile. */
    log_slot[0x76] = trainer[0x27];
    for (i = 0; i < 0x2A; i++) {
      log_slot[0x4C + i] = trainer[0x28 + i];
    }
  } else {
    /* Pull pokemon data from the wild-encounter EEPROM block instead. */
    log_slot[0x76] = drv_eeprom_read_u8(EEPROM_HOUR_MARKER);
    drv_eeprom_read_block(0xBF50, log_slot + 0x4C, 0x2A);
  }

  if (event_subtype >= 1 && event_subtype <= 3) {
    /* Copy one of trainer's 3 move slots (selected by event_subtype-1). */
    uint8_t *move_src;
    uint16_t off;
    uint8_t lo_bits;
    uint8_t hi_bits;
    uint8_t rot_val;

    off = (uint16_t)(event_subtype - 1) * 0x10;
    move_src = trainer + 0x52 + off;

    *((uint16_t *)(log_slot + 0x0C)) = *((uint16_t *)move_src);

    lo_bits = move_src[0x0D] & 0x1F;
    hi_bits = log_slot[0x86] & 0xE0;
    log_slot[0x86] = hi_bits | lo_bits;

    rot_val = (uint8_t)((move_src[0x0D] >> 5) & 0x03);
    *(log_slot + 0x86) =
        (uint8_t)((*(log_slot + 0x86) & ~(0x03 << 5)) | ((rot_val & 0x03) << 5));

  } else if (event_subtype == 4) {
    /* Special-route / IR-receive path: pull the move bytes from a fixed
       EEPROM cache, with the source chosen by interaction_type. */
    uint16_t src_addr;
    uint8_t sp_byte;
    uint8_t lo_bits;
    uint8_t hi_bits;
    uint8_t rot_val;

    if (interaction_type == 0x0F || interaction_type == 0x10) {
      src_addr = 0xBF08;
    } else {
      src_addr = 0xBA44;
    }
    drv_eeprom_read_block(src_addr, log_slot + 0x0C, 0x2);

    sp_byte = drv_eeprom_read_u8(EEPROM_SPECIAL_BYTE);
    lo_bits = sp_byte & 0x1F;
    hi_bits = log_slot[0x86] & 0xE0;
    log_slot[0x86] = hi_bits | lo_bits;

    rot_val = (uint8_t)(sp_byte >> 5) & 0x03;
    *(log_slot + 0x86) =
        (uint8_t)((*(log_slot + 0x86) & ~(0x03 << 5)) | ((rot_val & 0x03) << 5));
  }

  /* Commit the slot to EEPROM and advance the circular index. */
  drv_eeprom_write_block(slot_off, log_slot, 0x88);
  g.save_peerSlotIndex = (uint8_t)((int16_t)((uint16_t)g.save_peerSlotIndex + 1) % 23);

  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&g.save_totalSteps, 0x18);
}

// ROM: 0x3a70  82.4%
void game_log_poke_interaction(void) {
  uint8_t *log_block;
  uint8_t *trainer_buf;
  uint16_t sub_y;
  void *slot_buf;

  if (g.viewstate.Y.BYTE == 0)
    return;

  /* Copy the chosen pokemon slot into the log context at the discard cursor's
     position (g.viewstate.Z * 0x10), then write the whole block back. */
  sys_init_heap();
  log_block = sbrk(0x30);
  drv_eeprom_read_block(EEPROM_LOG_CONTEXT, log_block, 0x30);

  drv_eeprom_read_block(EEPROM_POKEMON_SLOTS + ((g.viewstate.Y.BYTE - 1) * 0x10),
                        log_block + (g.viewstate.Z * 0x10), 0x10);
  drv_eeprom_write_block(EEPROM_LOG_CONTEXT, log_block, 0x30);

  trainer_buf = sbrk(0xBE);
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);

  sub_y = g.viewstate.Y.BYTE;
  slot_buf = sbrk(0x88);
  /* ROM r0=trainer_buf, e0=slot_buf. sub_y is the event_subtype (6th arg,
     pushed); val_at_0e (e1) is 0. */
  game_log_interaction(trainer_buf, slot_buf, 0x0D, 0x00, 0, (uint8_t)sub_y);
}

// ROM: 0x3b02  74.7%
void game_log_item_interaction(void) {
  uint8_t *log_block;
  uint8_t *trainer_buf;
  void *slot_buf;
  uint32_t scratch_val;

  /* Pick the dowsed item id from the SUBY lookup table into the log block at
     the discard cursor's position. */
  sys_init_heap();
  log_block = sbrk(0x0C);
  drv_eeprom_read_block(EEPROM_LOG_ITEMS, log_block, 0x0C);

  drv_eeprom_read_block(EEPROM_SUBY_LOOKUP_TABLE + (g.viewstate.Y.BYTE * 2),
                        log_block + (g.viewstate.Z * 4), 0x02);
  drv_eeprom_write_block(EEPROM_LOG_ITEMS, log_block, 0x0C);

  trainer_buf = sbrk(0xBE);
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);

  slot_buf = sbrk(0x88);
  /* `scratch_val` is unused locally but the assignment is preserved because
     ch38 allocates a stack slot to match the ROM's frame. */
  scratch_val = ((uint32_t)(*(uint16_t *)(trainer_buf + (g.viewstate.Y.BYTE * 2) + 0x8C)) << 16) |
                0x0B;
  (void)scratch_val;
  game_log_interaction(trainer_buf, slot_buf, 0x0B, 0x00,
                       *(uint16_t *)(trainer_buf + (g.viewstate.Y.BYTE * 2) + 0x8C), 0);
}

// ROM: 0x67de  85.9%
void game_rotate_interaction_log(void) {
  uint16_t chunk_size = 0x224;
  void *buf;
  uint16_t src_addr;
  uint16_t dst_addr;
  uint8_t i;

  /* Shift the 10-record peer log down by one slot in EEPROM (oldest
     evicted). Each record is 0x224 bytes; the topmost record at 0xEF44
     becomes the slot at 0xF168 = 0xEF44 + 0x224. */
  sys_init_heap();
  buf = sbrk(chunk_size);
  src_addr = 0xEF44;
  dst_addr = 0xF168;
  for (i = 10; i > 0; i--) {
    drv_eeprom_read_block(src_addr, buf, chunk_size);
    drv_eeprom_write_block(dst_addr, buf, chunk_size);
    src_addr -= chunk_size;
    dst_addr -= chunk_size;
  }
}

// ROM: 0x6816  9.1%
void game_rotate_interaction_log_record(void) {
  uint8_t *trainer_buf;
  uint8_t *peer_data;     /* the 0x38-byte peer-context block at EEPROM 0xF6C0 */
  uint8_t *log_record;    /* the 0x88-byte fresh log record being built       */
  uint8_t lo_bits, hi_bits;
  uint8_t i;

  sys_init_heap();
  trainer_buf = (uint8_t *)sbrk(0xBE);
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);

  peer_data = (uint8_t *)sbrk(0x38);
  drv_eeprom_read_block(0xF6C0, peer_data, 0x38);

  log_record = (uint8_t *)sbrk(0x88);

  *(uint32_t *)(log_record + 4) = *(uint32_t *)(peer_data + 8);
  *(uint16_t *)(log_record + 8) = *(uint16_t *)(peer_data + 12);
  *(uint16_t *)(log_record + 12) = *(uint16_t *)(peer_data + 14);

  lo_bits = (peer_data[0x36] & 0x1F);
  hi_bits = (log_record[0x86] & 0xE0);
  log_record[0x86] = hi_bits | lo_bits;

  lo_bits = (peer_data[0x36] >> 5) & 3;
  *(log_record + 0x86) =
      (uint8_t)((*(log_record + 0x86) & ~(0x03 << 5)) | ((lo_bits & 0x03) << 5));

  /* ROM: bld #7, peer_data[0x36]; bst #7, @log_record+0x86 (unconditional copy) */
  ((byte_bits_t *)&log_record[0x86])->BIT.b7 =
      ((byte_bits_t *)&peer_data[0x36])->BIT.b7;

  *(uint16_t *)(log_record + 0x7A) = *(uint16_t *)(peer_data + 4);
  *(uint32_t *)(log_record + 0x80) = *(uint32_t *)peer_data;

  for (i = 0; i < 0x16; i++) {
    log_record[0x36 + i] = peer_data[0x10 + i];
  }
  for (i = 0; i < 0x12; i++) {
    log_record[0x10 + i] = peer_data[0x26 + i];
  }

  if (g.viewstate.v.peerPlay.wattsAwarded == 0) {
    uint8_t accel_val = g.viewstate.v.peerPlay.subTextOrSlot;
    if (accel_val < 10) {
      /* ROM: r1h=settings_bit (d_high), e1=trainer[0x8C+g.viewstate.v.peerPlay.subTextOrSlot*2] uint16
       * (val_at_0e), push=0 (event_subtype). */
      game_log_interaction(
          trainer_buf, log_record, accel_val + 1,
          (uint8_t)((g.save_settings.BYTE & 1)),
          *(uint16_t *)(trainer_buf + 0x8C + (uint16_t)accel_val * 2), 0);
    }
  }
}
