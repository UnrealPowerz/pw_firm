#include "all_headers.h"

/*
 * EEPROM persistence — redundant writes + corruption recovery.
 *
 *   --- Boot integrity ---
 *     save_write_magic            Write the 8-byte NINTENDO_MAGIC header at
 *                                 EEPROM 0 (factory-init mark).
 *     save_verify_magic           Check the magic; 0 = corrupt / blank chip.
 *     sys_sync_eeprom_on_startup  Boot-time entry: verify magic, repair from
 *                                 staged data if a crash interrupted a save,
 *                                 factory-reset if no magic at all.
 *
 *   --- Primary+backup reliability ---
 *     save_write_reliable         Write a buffer twice (primary + backup
 *                                 location) with a trailing checksum byte.
 *     save_read_reliable          Read both copies, compare checksums, repair
 *                                 the corrupt side if exactly one is good,
 *                                 zero-fill if both are bad.
 *
 *   --- Trainer record event-bit array (trainer.flags_38, 0x10 bytes) ---
 *     save_set_event_bit          OR a 1 into bit `val` of the array.
 *     save_check_event_bit        Test bit `val`; returns 0/1.
 *
 *   --- Slot finders (3-element arrays) ---
 *     save_find_empty_poke_slot   Stride 16 (caught-pokemon records).
 *     save_find_empty_item_slot   Stride 4  (dowsed-item records).
 *     save_get_dowsing_item_id    Read the per-slot dowsing item id from the
 *                                 trainer profile at offset 0x8C+idx*2.
 *
 *   --- Bulk wipes ---
 *     save_clear_data             Fill EEPROM_LOG_CONTEXT region with 0.
 *     save_clear_peer_log_slots   Zero the 0x84-offset interaction-type byte
 *                                 in all 24 peer-log slots (marks them empty).
 *     save_clear_daily_step_log   Zero the 0x1C-byte daily step counts
 *                                 (7 days x 4 bytes). NOT the bigger
 *                                 EEPROM_STEP_HIST region.
 *     save_commit_staged_data     Copy staging area to permanent EEPROM
 *                                 locations (called at boot after a crash).
 *     sys_factory_reset_eeprom    Full reset — args (b, a):
 *                                   b=1: also clear trainer.flags_38 event bits
 *                                   a=1: also zero totals + wipe log region
 */

// ROM: 0xb0ae  60.5%
void save_write_magic(void) {
  uint8_t i;
  i = 0;
  do {
    drv_eeprom_write_u8(i, NINTENDO_MAGIC[i]);
    i++;
  } while (i < 8);
}

// ROM: 0xb0c8  75.8%
uint8_t save_verify_magic(void) {
  uint8_t i;
  uint8_t eep_val;
  int16_t rom_val;

  i = 0;
  do {
    eep_val = drv_eeprom_read_u8(i);
    rom_val = (int16_t)(int8_t)NINTENDO_MAGIC[i];
    if ((int16_t)(uint16_t)eep_val != rom_val) {
      return 0;
    }
    i++;
  } while (i < 8);
  return 1;
}

// ROM: 0xb1ae  78.9%  saves: r3,r4,er5,r6 -> er5,er6
void sys_factory_reset_eeprom(uint8_t wipe_event_bits, uint8_t wipe_save_data) {
  uint8_t *ptr;
  register volatile uint8_t *flags_ptr;
  uint32_t zero = 0;
  uint16_t i;

  g.session_steps = zero;
  g.session_recentSteps = 0;
  g.walker_status_flags &= ~0x06;

  ptr = (uint8_t *)drv_ir_get_rx_ptr();
  save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, ptr, 0x68);

  /* Zero the 16 bytes of trainer identity (id, loc, etc.) + the 18-byte
     at_48 block — always wiped on any reset. */
  *((uint32_t *)(ptr)) = zero;
  *((uint32_t *)(ptr + 4)) = zero;
  *((uint16_t *)(ptr + 8)) = 0;
  *((uint16_t *)(ptr + 10)) = 0;
  *((uint32_t *)(ptr + 12)) = zero;

  for (i = 0; i < 18; i++) {
    ptr[0x48 + i] = 0;
  }

  /* Conditionally wipe the 16-byte event-bit array (trainer.flags_38). */
  if (wipe_event_bits) {
    for (i = 0; i < 16; i++) {
      ptr[0x38 + i] = 0;
    }
  }

  flags_ptr = ptr + 0x5B;
  *flags_ptr &= ~0x01;
  *flags_ptr &= ~0x02;
  *flags_ptr &= ~0x04;
  ptr[0x5B] &= 0x07;
  ptr[0x5A] = 0;
  ptr[0x5E] = 0;
  ptr[0x5F] = 2;
  *((uint32_t *)(ptr + 0x60)) = zero;

  save_read_reliable(EEPROM_RESV_0083, EEPROM_RESV_0083_BACKUP, ptr + 0x28, 0x10);
  if (wipe_save_data) {
    *((uint32_t *)(ptr + 0x64)) = zero;
  }
  save_write_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, ptr, 0x68);
  game_reset_step_data(wipe_save_data);

  if (wipe_save_data) {
    /* Full wipe: blast the entire log region. */
    drv_eeprom_fill(EEPROM_LOG_REGION, 0x0D4C, 0);
  } else {
    /* Light cleanup: clear only the structured sub-regions. */
    save_clear_data();
    save_clear_peer_log_slots();
    save_clear_daily_step_log();
  }

  if (wipe_event_bits) {
    drv_eeprom_fill(EEPROM_STEP_HIST_FLAGS, 0x06C8, 0);
  }

  drv_eeprom_fill(EEPROM_STEP_HIST, 0x1568, 0);
}

// ROM: 0xb2e2  82.4%  saves: er4,er5,er6
void sys_sync_eeprom_on_startup(void) {
  uint8_t magic;

  if (save_verify_magic() != 0) {
    save_read_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&g.save_totalSteps, 0x18);
    if ((g.save_settings & 0x78) > 0x48) {
      g.save_settings = (g.save_settings & 0x87) | 0x20;
    }
    game_sync_walk_status();
  } else {
    sys_factory_reset_eeprom(1, 1);
    drv_sound_set_volume((g.save_settings >> 1) & 0x3);
    drv_lcd_set_contrast((g.save_settings >> 3) & 0xF);
    save_write_magic();
  }

  save_read_reliable(EEPROM_STAGE_MARKER, EEPROM_STAGE_MARKER_BACKUP, &magic, 1);
  if (magic == 0xA5) {
    save_commit_staged_data();
    magic = 0;
    save_write_reliable(EEPROM_STAGE_MARKER, EEPROM_STAGE_MARKER_BACKUP, &magic, 1);
  }
}

// ROM: 0x0426  82.4%
#pragma option noregexpansion  /* pragma:auto */
void save_commit_staged_data(void) {
  void *buf;
  uint16_t src1;
  uint32_t dst1;
  uint16_t count2;
  uint16_t src2;
  uint32_t dst2;

  sys_init_heap();
  buf = sbrk(0x80);

  src1 = 0xD700;
  dst1 = 0x528F00;
  count2 = (uint16_t)(dst1 >> 16);

  while (count2 != 0) {
    drv_eeprom_read_block(src1, buf, 0x80);
    drv_eeprom_write_block(dst1 & 0xFFFF, buf, 0x80);
    src1 += 0x80;
    dst1 += 0x80;
    count2--;
  }

  src2 = 0xD480;
  dst2 = 0x5CC00;
  count2 = 0;

  while (count2 < (uint16_t)(dst2 >> 16)) {
    drv_eeprom_read_block(src2, buf, 0x80);
    drv_eeprom_write_block(dst2 & 0xFFFF, buf, 0x80);
    src2 += 0x80;
    dst2 += 0x80;
    count2++;
  }
}

// ROM: 0x50d8  84.0%  saves: er3,er4,er5,er6
void save_write_reliable(uint16_t primary, uint16_t backup, uint8_t *buf,
                               uint16_t size) {
  uint8_t checksum = 1;
  uint16_t i;

  drv_eeprom_write_block(primary, buf, size);
  for (i = 0; i < size; i++)
    checksum += buf[i];
  drv_eeprom_write_u8(primary + size, checksum);
  drv_eeprom_write_block(backup, buf, size);
  drv_eeprom_write_u8(backup + size, checksum);
}

// ROM: 0x5128  85.2%  saves: er3,er4,er5,er6
#pragma option noregexpansion  /* pragma:auto */
void save_read_reliable(uint16_t primary, uint16_t backup, uint8_t *buf,
                              uint16_t size) {
  uint8_t status = 0;
  uint8_t checksums[2] = {1, 1}; // 0 = backup, 1 = primary
  uint8_t chk1, chk2;
  uint8_t i;

  drv_eeprom_read_block(primary, buf, size);
  for (i = 0; i < size; i++)
    checksums[1] += buf[i];

  drv_eeprom_read_block(backup, buf, size);
  for (i = 0; i < size; i++)
    checksums[0] += buf[i];

  chk1 = drv_eeprom_read_u8(backup + size);
  if (checksums[0] == chk1)
    status |= 1;

  chk2 = drv_eeprom_read_u8(primary + size);
  if (checksums[1] == chk2)
    status |= 2;

  switch (status) {
  case 0:
    for (i = 0; i < size; i++)
      buf[i] = 0xFF;
    drv_eeprom_write_block(primary, buf, size);
    drv_eeprom_write_u8(primary + size, 0xFF);
    drv_eeprom_write_block(backup, buf, size);
    drv_eeprom_write_u8(backup + size, 0xFF);
    break;
  case 1:
    drv_eeprom_read_block(backup, buf, size);
    drv_eeprom_write_block(primary, buf, size);
    drv_eeprom_write_u8(primary + size, checksums[0]);
    break;
  case 2:
    drv_eeprom_read_block(primary, buf, size);
    drv_eeprom_write_block(backup, buf, size);
    drv_eeprom_write_u8(backup + size, checksums[1]);
    break;
  case 3:
    if (checksums[0] != checksums[1]) {
      drv_eeprom_write_block(primary, buf, size);
      drv_eeprom_write_u8(primary + size, checksums[0]);
    }
    break;
  }
}

// ROM: 0x1d22  68.8%  saves: r6,r5
void save_set_event_bit(void *ptr, uint8_t val) {
  uint8_t *p = (uint8_t *)ptr;
  uint8_t offset;
  uint8_t bit;

  if (val == 0)
    return;

  offset = val >> 3;
  bit = val & 0x7;

  save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, p, 0x68);
  p[0x38 + offset] |= (1 << bit);
  save_write_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, p, 0x68);
}

// ROM: 0x1d7a  79.4%  saves: r6,r5
uint8_t save_check_event_bit(void *ptr, uint8_t val) {
  struct trainer_record *rec = (struct trainer_record *)ptr;
  uint8_t offset;
  uint8_t bit;

  if (val == 0)
    return 0;

  offset = val >> 3;
  bit = val & 0x7;

  save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (uint8_t *)rec, sizeof(*rec));
  if (rec->flags_38[offset] & (1 << bit)) {
    return 1;
  }
  return 0;
}

// ROM: 0x1eca  69.1%
uint8_t save_find_empty_poke_slot(void *ptr) {
  uint8_t *p = (uint8_t *)ptr;
  uint8_t i;

  for (i = 0; i < 3; i++) {
    uint16_t val = *(uint16_t *)(p + (i * 16));
    if (val == 0) {
      return i;
    }
  }
  return 3;
}

/* Reason: ROM keeps locals in caller-saved r2/r3; ch38 picks callee-saved
 * r5/r6 and emits PUSH.W R6 / PUSH.W R5 prologue.
 * Even with -regparam=3 enabled (so er2 IS caller-saved), ch38 chose r5/r6
 * here -- score unchanged at 52.8%.  The original compiler must have done
 * inter-procedural analysis to prove drv_eeprom_read_block doesn't clobber
 * r2/r3, then used them as scratch across the call without saving anything.
 * ch38 doesn't do that analysis and conservatively reaches for callee-saved
 * registers.  Pragmas like `#pragma option speed=register` don't reliably
 * help -- and they have file-global scope, so clobbering one regresses
 * neighbouring functions (we saw -13% on gfx_draw_text_box this way).
 * Class: cannot-fix-without-compiler-change */
// ROM: 0x1eee  52.8%
uint16_t save_get_dowsing_item_id(uint8_t index) {
  uint8_t *buf;
  uint16_t result;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xBE);
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, buf, 0xBE);

  result = *(uint16_t *)(buf + 0x8C + (uint16_t)index * 2);
  return result;
}

// ROM: 0x1f1c  69.4%
uint8_t save_find_empty_item_slot(void *ptr) {
  uint8_t *p = (uint8_t *)ptr;
  uint8_t i;

  for (i = 0; i < 3; i++) {
    uint16_t val = *(uint16_t *)(p + (i * 4));
    if (val == 0) {
      return i;
    }
  }
  return 3;
}

/*
 * Address: 0x187E
 */
// ROM: 0x187e  72.2%
void save_clear_data(void) { drv_eeprom_fill(EEPROM_LOG_CONTEXT, 0x0064, 0); }

#pragma noregsave(save_clear_peer_log_slots)
// ROM: 0x188c  96.9%
void save_clear_peer_log_slots(void) {
  uint16_t addr = 0xCF0C;
  uint8_t i = 0x18;
  do {
    drv_eeprom_write_u8(addr + 0x84, 0);
    addr += 0x88;
  } while (--i != 0);
}

/*
 * Address: 0x18A8
 */
// ROM: 0x18a8  72.2%
void save_clear_daily_step_log(void) { drv_eeprom_fill(EEPROM_LOG_POKE_STATS, 0x001C, 0); }
