#include "all_headers.h"

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
// ROM: 0x4546  56.9%  saves: r3,r4,er5,er6 -> er5,er6
#pragma option speed =loop=1 /* pragma:auto */
void game_log_interaction(uint8_t *a, uint8_t *b, uint8_t d_low, uint8_t d_high,
                          uint16_t val_high) {
  uint16_t i;
  uint16_t slot_off;
  uint8_t eeprom_val;

  slot_off = (uint16_t)peerSlotIndex * 0x88 + 0xCF0C;
  eeprom_val = drv_eeprom_read_u8(slot_off + 0x84);

  if (eeprom_val != 0) {
    if (d_low == 0x1B) {
      return;
    }
  }

  if (eeprom_val == 0x19) {
    peerSlotIndex = (uint8_t)((int16_t)((uint16_t)peerSlotIndex + 1) % 23);
    slot_off = (uint16_t)peerSlotIndex * 0x88 + 0xCF0C;
  }

  if (d_low > 0x0A) {
    for (i = 0; i < 0x88; i++) {
      b[i] = 0;
    }
  }

  b[0x84] = d_low;
  *((uint16_t *)(b + 0x0E)) = val_high;
  *((uint32_t *)b) = *((uint32_t *)a);
  *((uint16_t *)(b + 0x78)) = recentSessionSteps;
  *((uint32_t *)(b + 0x7C)) = sessionSteps;
  *((uint16_t *)(b + 0x0A)) = *((uint16_t *)a);

  /* Copy nickname bytes: src+0x10 -> dst+0x20, 0x16 bytes */
  for (i = 0; i < 0x16; i++) {
    b[0x20 + i] = a[0x10 + i];
  }

  b[0x77] = a[0x26];

  /* Gender/ability bits from src[0x0D] */
  {
    uint8_t src_byte;
    uint8_t dst_byte;
    src_byte = a[0x0D];
    dst_byte = b[0x85];
    dst_byte &= 0xE0;
    dst_byte |= (src_byte & 0x1F);
    b[0x85] = dst_byte;

    src_byte = (uint8_t)((a[0x0D] << 3) | (a[0x0D] >> 5)); /* Dummy rotate? */
    b[0x85] =
        (uint8_t)((b[0x85] & ~(0x03 << 1)) |
                  (((uint8_t)((src_byte >> 6) | (src_byte << 2)) & 3) << 1));
  }
  /* ROM: bld #1, a[0xE]; bst #7, @r6+0x85  (unconditional bit copy) */
  ((byte_bits_t *)&b[0x85])->BIT.b7 =
      ((byte_bits_t *)&a[0x0E])->BIT.b1;

  if (d_high == 0) {
    b[0x76] = a[0x27];
    for (i = 0; i < 0x2A; i++) {
      b[0x4C + i] = a[0x28 + i];
    }
  } else {
    b[0x76] = drv_eeprom_read_u8(EEPROM_HOUR_MARKER);
    drv_eeprom_read_block(0xBF50, b + 0x4C, 0x2A);
  }

  if (d_high >= 1 && d_high <= 3) {
    uint8_t *move_src;
    uint16_t off;
    uint8_t lo_bits;
    uint8_t hi_bits;
    uint8_t rot_val;

    off = (uint16_t)(d_high - 1) * 0x10;
    move_src = a + 0x52 + off;

    *((uint16_t *)(b + 0x0C)) = *((uint16_t *)move_src);

    lo_bits = move_src[0x0D] & 0x1F;
    hi_bits = b[0x86] & 0xE0;
    b[0x86] = hi_bits | lo_bits;

    rot_val = (uint8_t)((move_src[0x0D] >> 5) & 0x03);
    *(b + 0x86) =
        (uint8_t)((*(b + 0x86) & ~(0x03 << 5)) | ((rot_val & 0x03) << 5));

  } else if (d_high == 4) {
    uint16_t src_addr;
    uint8_t sp_byte;
    uint8_t lo_bits;
    uint8_t hi_bits;
    uint8_t rot_val;

    if (d_low == 0x0F || d_low == 0x10) {
      src_addr = 0xBF08;
    } else {
      src_addr = 0xBA44;
    }
    drv_eeprom_read_block(src_addr, b + 0x0C, 0x2);

    sp_byte = drv_eeprom_read_u8(EEPROM_SPECIAL_BYTE);
    lo_bits = sp_byte & 0x1F;
    hi_bits = b[0x86] & 0xE0;
    b[0x86] = hi_bits | lo_bits;

    rot_val = (uint8_t)(sp_byte >> 5) & 0x03;
    *(b + 0x86) =
        (uint8_t)((*(b + 0x86) & ~(0x03 << 5)) | ((rot_val & 0x03) << 5));
  }

  /* Write buffer to EEPROM slot */
  drv_eeprom_write_block(slot_off, b, 0x88);

  /* Advance slot */
  peerSlotIndex = (uint8_t)((int16_t)((uint16_t)peerSlotIndex + 1) % 23);

  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
}

// ROM: 0x3a68  98.3%
void game_start_peer_session(void) {
  gCurSubstateZ = 1;
  (void)0;
}

// ROM: 0x3a70  71.8%
void game_log_poke_interaction(void) {
  uint8_t *ctx;
  uint8_t *tmp;
  uint16_t sy;
  void *buf;

  if (gCurSubstateY == 0)
    return;

  sys_init_heap();
  ctx = sbrk(0x30);
  drv_eeprom_read_block(EEPROM_LOG_CONTEXT, ctx, 0x30);

  drv_eeprom_read_block(EEPROM_POKEMON_SLOTS + ((gCurSubstateY - 1) * 0x10),
                        ctx + (gCurSubstateZ * 0x10), 0x10);
  drv_eeprom_write_block(EEPROM_LOG_CONTEXT, ctx, 0x30);

  tmp = sbrk(0xBE);
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, tmp, 0xBE);

  sy = gCurSubstateY;
  buf = sbrk(0x88);
  game_log_interaction(ctx, buf, 0x0D, 0x00, (uint16_t)sy);
}

// ROM: 0x3b02  79.0%
void game_log_item_interaction(void) {
  uint8_t *ctx;
  uint8_t *tmp;
  void *buf;
  uint32_t val;

  sys_init_heap();
  ctx = sbrk(0x0C);
  drv_eeprom_read_block(EEPROM_LOG_ITEMS, ctx, 0x0C);

  drv_eeprom_read_block(EEPROM_SUBY_LOOKUP_TABLE + (gCurSubstateY * 2), ctx + (gCurSubstateZ * 4),
                        0x02);
  drv_eeprom_write_block(EEPROM_LOG_ITEMS, ctx, 0x0C);

  tmp = sbrk(0xBE);
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, tmp, 0xBE);

  buf = sbrk(0x88);
  val = ((uint32_t)(*(uint16_t *)(tmp + (gCurSubstateY * 2) + 0x8C)) << 16) |
        0x0B;
  game_log_interaction(tmp, buf, 0x0B, 0x00,
                       *(uint16_t *)(tmp + (gCurSubstateY * 2) + 0x8C));
}

// ROM: 0x5c0a  77.6%  saves: er6
#pragma option speed =register /* pragma:auto */
void game_init_peer_identity(void) {
  register struct trainer_record *rec;
  register uint8_t *temp_buf;
  uint16_t i;

  recentSessionSteps = 0;
  walker_status_flags_BIT.walking = 1;

  sys_init_heap();
  rec = (struct trainer_record *)sbrk(sizeof(*rec));
  save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (uint8_t *)rec, sizeof(*rec));

  if (!(rec->flags_5b & 0x02)) {
    rec->flags_5b |= 0x02;
    rec->id_backup = rec->id;
    rec->loc_backup = rec->loc;
    rec->flags_5b |= 0x04;

    save_write_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (uint8_t *)rec, sizeof(*rec));

    sys_init_heap();
    temp_buf = (uint8_t *)sbrk(0x180);
    drv_eeprom_read_block(0x9D7E, temp_buf, 0x180);
    drv_eeprom_write_block(0x91BE, temp_buf, 0x180);

    drv_eeprom_read_block(0x9EFE, temp_buf, 0x600);
    drv_eeprom_write_block(0x933E, temp_buf, 0x600);

    drv_eeprom_read_block(0xA77E, temp_buf, 0x140);
    drv_eeprom_write_block(0x993E, temp_buf, 0x140);

    sys_init_heap();
    temp_buf = (uint8_t *)sbrk(0xBE);
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, temp_buf, 0xBE);

    for (i = 0; i < 0x10; i++) {
      temp_buf[0x72 + i] = temp_buf[i];
    }
    temp_buf[0x0D] &= ~0x80;
    temp_buf[0x26] = 0x46;

    for (i = 0; i < 6; i++) {
      temp_buf[0x10 + i] = 0;
    }

    drv_eeprom_write_block(EEPROM_TRAINER_PROFILE, temp_buf, 0xBE);
    sessionTicksElapsed = 0;

    save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
    save_init_defaults();
  }
}

// ROM: 0x5d52  73.0%
void game_process_interaction_reward(uint8_t type) {
  void *buf;
  void *settings;
  uint8_t index;

  gCurSubstateY = type;
  accelXPos = ((const uint16_t *)interactionRewardPtrTable)[type];
  ui_set_view(0x0C);
  idleSeconds = 0;

  switch (type) {
  case 1:
    if (recentSessionSteps < 4500) {
      gCurSubstateZ = (int8_t)(9 - (recentSessionSteps / 500));
    } else {
      gCurSubstateZ = 0;
    }
    gfx_get_sprite_addr((uint8_t)gCurSubstateZ);
    sys_init_heap();
    buf = sbrk(0x0C);
    drv_eeprom_read_block(EEPROM_LOG_ITEMS, buf, 0x0C);
    index = save_find_empty_slot_32bit(buf);
    if (index < 3) {
      drv_eeprom_write_block(EEPROM_LOG_ITEMS, buf, 0x0C);
    }
    break;
  case 2:
    gCurSubstateZ = 50;
    game_add_watts(50);
    break;
  case 3:
    gCurSubstateZ = 20;
    game_add_watts(20);
    break;
  case 4:
    gCurSubstateZ = 10;
    game_add_watts(10);
    break;
  case 7:
    game_init_peer_identity();
    break;
  default:
    gCurSubstateZ = 0;
    break;
  }

  sys_init_heap();
  settings = sbrk(0xBE);
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, settings, 0xBE);

  {
    uint8_t param = ((RamCache_settingsByte & 1));
    buf = sbrk(0x88);
    game_log_interaction(settings, buf, type + 16, 0, (uint16_t)param);
  }

  if (type >= 2 && type <= 5) {
    gCurSubstateA = (uint8_t)(sys_get_rng() % 3);
  } else {
    gCurSubstateA = 0;
  }
}

// ROM: 0x5e9e  70.8%
void ui_handle_bored_gift(void) {
  uint8_t *dest;
  if (drv_button_is_triggered(0x02)) {
    dest = (uint8_t *)(uintptr_t)accelXPos;
    if (*dest & 0x01) {
      ui_reset_substate();
      ui_set_view(0);
    } else {
      dest += 4;
      accelXPos = (uint16_t)(uintptr_t)dest;
      if (dest[1] == 0x10)
        return;
      drv_sound_play(dest[1]);
    }
  }
}

// ROM: 0x5edc  5.1%
void ui_render_social_feelings(void) {
  uint8_t flags;
  uint8_t *ptr;
  uint8_t r6l;

  sys_init_heap();
  sbrk(0xC0);
  ptr = (uint8_t *)(uintptr_t)accelXPos;

  if (*ptr & 0x02) {
    gfx_draw_own_pokemon_small(0x20, 0x18);
  }

  flags = *ptr;
  if ((flags & 0xE0) != 0xE0) {
    uint8_t r0l = (uint8_t)((flags << 3) | (flags >> 5)) & 0x07;
    gfx_draw_small_route_icon(r0l);
  }

  if (*ptr & 0x04) {
    gfx_draw_item_symbol(0x14, 0x14);
  }

  r6l = gCurSubstateZ;
  if (flags == 0xFC) {
    gfx_draw_own_pokemon_name(0x00, 0x20, 5);
  } else if (flags == 0xFD) {
    gfx_draw_item_name(0x00, 0x20, 0x0D, r6l);
  } else if (flags == 0xFE) {
    gfx_draw_value_with_icon(2, 0x20, 0x0D, (uint16_t)r6l);
  } else if (flags != 0xFF) {
    gfx_draw_text_box((uint8_t)(flags >> 8), (uint8_t)(flags & 0xFF),
                      (uint8_t)(flags >> 8), (uint8_t)flags);
    gfx_draw_text_box(0x20, (uint8_t)flags, 0x0D, 0);
  }

  flags = *ptr;
  if ((flags & 0x18) > 8) {
    gfx_draw_text_box(0x30, 0x10, 0x0E, (uint8_t)(ptr[3] + gCurSubstateA));
  } else {
    if (ptr[2] == 0xFF) {
      gfx_draw_text_box(0x30, 0x10, ptr[3], 0x0F);
    } else {
      gfx_draw_text_box(0x30, 0x10, ptr[3], 0x0E);
    }
  }
  gfx_draw_battery_low(0, 0);
}

// ROM: 0x5fc2  51.5%  saves: r2,r5,r6 -> sys_epilogue_r2_r5_r6
#pragma option speed =register /* pragma:auto */
void game_check_periodic_events(void) {
  uint16_t dailyStepCap;
  uint8_t prob;
  uint8_t *pEeprom;

  if ((walker_status_flags & 0x18) != 0x10)
    return;
  if (currentlyActiveView != 0)
    return;
  if (!(walker_status_flags_BIT.input_pending))
    return;

  walker_status_flags_BIT.input_pending = 0;
  gCurSubstateY = 0;
  gCurSubstateZ = 0;

  prob = (uint8_t)(sys_get_rng() % 100);
  if (prob >= 40)
    return;

  if ((sys_get_rng() % 100) >= 40)
    return;

  if (!(walker_status_flags_BIT.walking)) {
    dailyStepCap = recentSessionSteps;
    if (dailyStepCap < 300)
      return;
    gCurSubstateY = 0x07;
  } else {
    if (idleSeconds < 3600)
      return;

    sys_init_heap();
    pEeprom = (uint8_t *)sbrk(0xBE);
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, pEeprom, 0xBE);
    prob = pEeprom[0x26];

    sys_init_heap();
    pEeprom = (uint8_t *)sbrk(0x0C);
    drv_eeprom_read_block(EEPROM_LOG_ITEMS, pEeprom, 0x0C);

    dailyStepCap = recentSessionSteps;
    if (save_find_empty_slot_32bit(pEeprom) < 3 && prob >= 90 &&
        dailyStepCap >= 500) {
      gCurSubstateY = 0x01;
    } else if (prob >= 80 && dailyStepCap >= 250) {
      gCurSubstateY = 0x02;
    } else if (dailyStepCap >= 200) {
      gCurSubstateY = 0x03;
    } else if (dailyStepCap >= 100) {
      gCurSubstateY = 0x04;
    } else if (sessionTicksElapsed >= 60 && dailyStepCap <= 50) {
      gCurSubstateY = 0x05;
    } else {
      return;
    }
  }
  gCurSubstateZ = 0x30;
}

// ROM: 0x6380  100.0%
void ui_handle_peer_play(void) { (void)0; }

// ROM: 0x6382  70.1%
void game_calculate_interaction_reward(void) {
  uint32_t steps_val;
  uint8_t *items_info;
  uint16_t *item_table;
  uint8_t r5l;
  uint16_t total_daily_steps;

  sys_init_heap();
  sbrk(0xBE);
  items_info = (uint8_t *)sbrk(0x38);
  drv_eeprom_read_block(0xF6C0, items_info, 0x38);

  item_table = (uint16_t *)sbrk(0x28);
  drv_eeprom_read_block(0xCEC8, item_table, 0x28);

  steps_val = sessionSteps + *(uint32_t *)items_info;
  total_daily_steps = recentSessionSteps + *(uint16_t *)(items_info + 4);
  steps_val += (uint32_t)total_daily_steps * 10;

  if (steps_val > 20000) {
    steps_val = 20000;
  }

  for (r5l = 0; r5l < 10; r5l++) {
    if (item_table[r5l * 2] == 0) {
      break;
    }
  }

  dowsing_item_pos = 0;
  if (r5l < 10) {
    dowsing_item_pos = (uint8_t)(steps_val / 200);
    if (dowsing_item_pos == 0) {
      dowsing_item_pos = 1;
    }
    if (dowsing_item_pos > 99) {
      dowsing_item_pos = 99;
    }
    game_add_watts(dowsing_item_pos);
  }

  if (steps_val >= 20000) {
    DAT_f7d1 = 0x2C;
    if (dowsing_item_pos == 0) {
      if (sessionSteps > *(uint32_t *)items_info) {
        accelXPos = 0;
      } else {
        accelXPos = 1;
      }
      goto calc_end;
    }
  } else if (steps_val >= 10000) {
    DAT_f7d1 = 0x2D;
    if (dowsing_item_pos == 0) {
      if (sessionSteps > *(uint32_t *)items_info) {
        accelXPos = 2;
      } else {
        accelXPos = 3;
      }
      goto calc_end;
    }
  } else if (steps_val >= 5000) {
    DAT_f7d1 = 0x2E;
    if (dowsing_item_pos == 0) {
      if (sessionSteps > *(uint32_t *)items_info) {
        accelXPos = 4;
      } else {
        accelXPos = 5;
      }
      goto calc_end;
    }
  } else if (steps_val >= 2500) {
    DAT_f7d1 = 0x2F;
  } else {
    DAT_f7d1 = 0x30;
  }

  if (dowsing_item_pos != 0) {
    if (sessionSteps > *(uint32_t *)items_info) {
      accelXPos = 10;
    } else {
      accelXPos = 11;
    }
  }

calc_end: {
  uint16_t sprite_addr = gfx_get_sprite_addr(accelXPos);
  item_table[r5l * 2] = sprite_addr;
  drv_eeprom_write_block(0xCEC8, item_table, 0x28);
}
}

// ROM: 0x67de  47.8%
#pragma option speed =loop=1 /* pragma:auto */
void game_rotate_interaction_log(void) {
  uint16_t r6 = 0x224;
  void *r5;
  uint16_t e5 = 0xEF44;
  uint16_t e6 = 0xF168;
  uint8_t r4l;

  sys_init_heap();
  r5 = sbrk(r6);
  for (r4l = 10; r4l > 0; r4l--) {
    drv_eeprom_read_block(e5, r5, r6);
    drv_eeprom_write_block(e6, r5, r6);
    e5 -= r6;
    e6 -= r6;
  }
}

// ROM: 0x6816  51.4%
#pragma option speed =loop=1 /* pragma:auto */
void game_rotate_interaction_log_record(void) {
  uint8_t *r3_settings;
  uint8_t *r5_contact;
  uint8_t *r6_gift;
  uint8_t r1l, r1h;
  uint8_t r4l;

  sys_init_heap();
  r3_settings = (uint8_t *)sbrk(0xBE);
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, r3_settings, 0xBE);

  r5_contact = (uint8_t *)sbrk(0x38);
  drv_eeprom_read_block(0xF6C0, r5_contact, 0x38);

  r6_gift = (uint8_t *)sbrk(0x88);

  *(uint32_t *)(r6_gift + 4) = *(uint32_t *)(r5_contact + 8);
  *(uint16_t *)(r6_gift + 8) = *(uint16_t *)(r5_contact + 12);
  *(uint16_t *)(r6_gift + 12) = *(uint16_t *)(r5_contact + 14);

  r1l = (r5_contact[0x36] & 0x1F);
  r1h = (r6_gift[0x86] & 0xE0);
  r6_gift[0x86] = r1h | r1l;

  r1l = (r5_contact[0x36] >> 5) & 3;
  *(r6_gift + 0x86) =
      (uint8_t)((*(r6_gift + 0x86) & ~(0x03 << 5)) | ((r1l & 0x03) << 5));

  /* ROM: bld #7, r5[0x36]; bst #7, @r6+0x86  (unconditional bit copy) */
  ((byte_bits_t *)&r6_gift[0x86])->BIT.b7 =
      ((byte_bits_t *)&r5_contact[0x36])->BIT.b7;

  *(uint16_t *)(r6_gift + 0x7A) = *(uint16_t *)(r5_contact + 4);
  *(uint32_t *)(r6_gift + 0x80) = *(uint32_t *)r5_contact;

  for (r4l = 0; r4l < 0x16; r4l++) {
    r6_gift[0x36 + r4l] = r5_contact[0x10 + r4l];
  }
  for (r4l = 0; r4l < 0x12; r4l++) {
    r6_gift[0x10 + r4l] = r5_contact[0x26 + r4l];
  }

  if (dowsing_item_pos == 0) {
    uint8_t accel_val = accelXPos;
    if (accel_val < 10) {
      game_log_interaction(r3_settings, r6_gift, accel_val + 1, 0,
                           (uint16_t)((RamCache_settingsByte & 1)));
    }
  }
}

// ROM: 0x632c  63.7%  saves: er2,er3,r4,er5,er6 -> sys_epilogue_5
void ui_start_peer_play_app(void) {
  uint8_t *buf;
  sys_init_heap();
  buf = sbrk(0x38);
  drv_eeprom_read_block(0xF6C0, buf, 0x38);
  /* Copy bit 0 of buf[0x37] into bit 1 of gCurSubstateY -- the ROM uses
   * bld+bst here, which means the destination bit is unconditionally set
   * to the source bit (not OR'd as the original C suggested). */
  ((byte_bits_t *)&gCurSubstateY)->BIT.b1 =
      ((byte_bits_t *)&buf[0x37])->BIT.b0;
  gCurSubstateZ = 0;
  gCurSubstateA = 0;
  game_calculate_interaction_reward();
  game_rotate_interaction_log();
  game_rotate_interaction_log_record();
}

// ROM: 0x6528  57.8%  saves: r6
void ui_draw_music_note(uint8_t x, uint8_t y, uint8_t shift) {
  uint8_t *buf;
  uint8_t i;
  uint16_t eeprom_addr = 0x2470;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x10);
  drv_eeprom_read_block(eeprom_addr, buf, 0x10);

  for (i = 0; i < 0x10; i++) {
    *(buf + i) >>= (int8_t)shift;
  }
  drv_lcd_blit(x, y, buf, 8, 8);
}

// ROM: 0x6574  22.4%
#pragma option speed =loop=1 /* pragma:auto */
void ui_render_peer_play(void) {
  uint8_t z = gCurSubstateZ;
  uint8_t r6l = 0;
  uint8_t r0l, r1l, r0h;

  if (z < 3) {
    gfx_draw_own_pokemon_small(0x38, 0x08);
    if (gCurSubstateY & 0x02) {
      r6l = 1;
    }
    if (z == 0) {
      r0l = (uint8_t)(7 - gCurSubstateA);
      r0h = 3;
      r0l *= r0h;
      r1l = (uint8_t)(8 - r0l);
      if (r6l != 0) {
        gfx_draw_peer_pokemon(r1l, 8, 0x00);
      } else {
        gfx_draw_peer_pokemon(r1l, 1, 0x08);
      }
    } else {
      if (r6l != 0) {
        gfx_draw_peer_pokemon(0x08, 0x08, 0x00);
      } else {
        gfx_draw_peer_pokemon(0x08, 0x01, 0x08);
      }
    }
  }

  if (z == 1) {
    gfx_draw_peer_pokemon_name(0x02, 0x20, 1);
    gfx_draw_text_box(0x30, 0x0E, 0x30, 0x2B);
  } else if (z == 2) {
    uint8_t count = gCurSubstateA + 1;
    uint8_t limit = (gCurSubstateA >> 1) + 1;
    uint8_t table_idx = 0;
    uint8_t i;

    if (DAT_f7d1 == 0x2C) {
      limit = count;
      if (limit > 5)
        limit = 5;
    } else if (DAT_f7d1 == 0x2D) {
      if (limit > 4)
        limit = 4;
    } else if (DAT_f7d1 == 0x2E) {
      if (limit > 3)
        limit = 3;
    } else if (DAT_f7d1 == 0x2F) {
      if (limit > 2)
        limit = 2;
      table_idx = 1;
    } else if (DAT_f7d1 == 0x30) {
      ui_draw_music_note(0x2C, musicNoteInitialState[0], 0);
      goto music_done;
    }

    for (i = 0; i < limit; i++) {
      uint8_t note_y = (table_idx == 0) ? musicNoteYTableA[i] : musicNoteYTableB[i];
      ui_draw_music_note((uint8_t)(i * 8 + 0x1C), note_y, 0);
    }
  music_done:
    gfx_draw_text_box(0x30, 0x0E, 0x30, (uint8_t)DAT_f7d1);
  } else if (z == 3) {
    gfx_draw_present_icon(0x20, 0x04);
    gfx_draw_text_box(0x30, 0x0F, 0x30, 0x31);
  } else if (z == 4) {
    gfx_draw_present_icon(0x20, 0x04);
    if (dowsing_item_pos != 0) {
      gfx_draw_value_with_icon(0x02, 0x20, 0x0D, (uint16_t)dowsing_item_pos);
    } else {
      gfx_draw_item_name(0x00, 0x20, 0x0D, (uint8_t)accelXPos);
    }
    gfx_draw_text_box(0x30, 0x0E, 0x30, 0x0F);
  }

  gCurSubstateA++;
  if (gCurSubstateA >= 8) {
    gCurSubstateZ++;
    gCurSubstateA = 0;
    if (gCurSubstateZ == 2) {
      drv_sound_play(9);
    } else if (gCurSubstateZ == 4) {
      drv_sound_play(6);
    }
  }

  if (gCurSubstateZ >= 5) {
    ui_reset_substate();
    ui_set_view(0);
  }
  gfx_draw_battery_low(0, 0);
}

// ROM: 0x6784  67.1%  saves: er3,er4,er5,er6
uint8_t game_find_seen_peer(void *trainer_ptr) {
  uint8_t r4l;
  uint8_t r6h;
  uint8_t r6l;
  uint16_t r3 = 0xDE24;
  uint8_t *r5 = eepromPageScratch;
  uint8_t *e5 = (uint8_t *)trainer_ptr;

  for (r4l = 0; r4l < 10; r4l++) {
    r6h = 1;
    drv_eeprom_read_block(r3 + 8, r5, 0x28);
    for (r6l = 0; r6l < 0x28; r6l++) {
      if (r5[r6l] != e5[r6l]) {
        r6h = 0;
      }
    }
    if (r6h == 1) {
      return 1;
    }
    r3 += 0x224;
  }
  return 0;
}
