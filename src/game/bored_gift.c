#include "all_headers.h"

/*
 * VIEW_BORED_GIFT — random walker-pokemon interaction event.
 *
 * Triggered from system/main.c (via game_check_periodic_events) when the
 * walker has been idle for long enough and the RNG rolls a hit. The reward
 * type is stored in gCurSubstateY and dispatched in game_process_interaction_reward:
 *
 *   type 1   - dowsing-style item gift  (selects an item from the trainer's
 *              table based on recentSessionSteps; populates EEPROM_LOG_ITEMS).
 *   type 2-5 - watts reward (50/20/10/?? depending on type).
 *   type 7   - first-time peer-identity setup (copies sprite ROM regions to
 *              the peer-sprite EEPROM slots; resets nickname).
 *   default  - just shows a "social feeling" text-box.
 *
 * accelPos_X points at a per-type 4-byte record in INTERACTION_REWARD_PTRS:
 *   [0] = flags (bit 0 = exit, bit 1 = show pokemon, bit 2 = show item icon,
 *          bits 3-4 = "use moving text", bits 5-7 = route icon index)
 *   [1] = sound id played by ui_handle_bored_gift on advance
 *   [2] = text-strip kind (0xFC = own poke name, 0xFD = item name,
 *          0xFE = value-with-icon, 0xFF = no top text, else = text-box index)
 *   [3] = bottom text-box index (animated +A if flags bit 3-4 high)
 */

// ROM: 0x5c0a  82.3%  saves: er6
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

    for (i = 0; i < 0x16; i++) {
      temp_buf[0x10 + i] = 0;
    }

    drv_eeprom_write_block(EEPROM_TRAINER_PROFILE, temp_buf, 0xBE);
    sessionTicksElapsed = 0;

    save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
    save_clear_peer_log_slots();
  }
}

// ROM: 0x5d52  78.9%
void game_process_interaction_reward(uint8_t type) {
  void *slot_buf;
  void *trainer_buf;
  uint8_t free_slot_idx;
  uint16_t item_id = 0;       /* passed as val_at_0e to game_log_interaction;
                                 only set on type 1 (item gift) */

  gCurSubstateY = type;
  accelPos_X = ((const uint16_t *)INTERACTION_REWARD_PTRS)[type];
  ui_set_view(VIEW_BORED_GIFT);
  idleSeconds = 0;

  switch (type) {
  case 1:
    if (recentSessionSteps < 4500) {
      gCurSubstateZ = (int8_t)(9 - (recentSessionSteps / 500));
    } else {
      gCurSubstateZ = 0;
    }
    item_id = save_get_dowsing_item_id((uint8_t)gCurSubstateZ);
    sys_init_heap();
    slot_buf = sbrk(0x0C);
    drv_eeprom_read_block(EEPROM_LOG_ITEMS, slot_buf, 0x0C);
    free_slot_idx = save_find_empty_item_slot(slot_buf);
    if (free_slot_idx < 3) {
      drv_eeprom_write_block(EEPROM_LOG_ITEMS, slot_buf, 0x0C);
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
  trainer_buf = sbrk(0xBE);
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);

  {
    uint8_t settings_bit = ((RamCache_settingsByte & 1));
    slot_buf = sbrk(0x88);
    /* ROM: r1h=settings_bit (use_wild_data flag), e1=item_id (val_at_0e),
       push=0 (event_subtype). */
    game_log_interaction(trainer_buf, slot_buf, type + 16, settings_bit,
                         item_id, 0);
  }

  if (type >= 2 && type <= 5) {
    gCurSubstateA = (uint8_t)(sys_get_rng() % 3);
  } else {
    gCurSubstateA = 0;
  }
}

// ROM: 0x5e9e  71.2%
void ui_handle_bored_gift(void) {
  uint8_t *dest;
  if (drv_button_is_triggered(BTN_R)) {
    dest = (uint8_t *)(uintptr_t)accelPos_X;
    if (*dest & 0x01) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
    } else {
      dest += 4;
      accelPos_X = (uint16_t)(uintptr_t)dest;
      if (dest[1] == 0x10)
        return;
      drv_sound_play(dest[1]);
    }
  }
}

// ROM: 0x5edc  57.0%
void ui_render_bored_gift(void) {
  uint8_t *event_rec;
  uint8_t flags;
  uint8_t prize_count;

  sys_init_heap();
  sbrk(0xC0);

  /* accelPos_X points into INTERACTION_REWARD_PTRS: a 4-byte event record.
     event_rec[0] = flag byte (bit 1 = show own poke, bit 2 = show item icon,
       bits 3-4 = animated bottom text, bits 5-7 = route icon index, 0xE0
       sentinel = no route icon).
     event_rec[2] = top-strip kind (0xFC own poke name, 0xFD item name,
       0xFE value+icon, 0xFF no top text, else = text-box index).
     event_rec[3] = bottom text-box index. */

  event_rec = (uint8_t *)(uintptr_t)accelPos_X;
  if (*event_rec & 0x02) {
    gfx_draw_own_pokemon_small(0x20, 0x04);
  }

  event_rec = (uint8_t *)(uintptr_t)accelPos_X;
  flags = *event_rec;
  if ((flags & 0xE0) != 0xE0) {
    /* Top 3 bits = route icon index (rotl 3 to extract). */
    gfx_draw_small_route_icon((uint8_t)(((flags << 3) | (flags >> 5)) & 0x07));
  }

  event_rec = (uint8_t *)(uintptr_t)accelPos_X;
  if (*event_rec & 0x04) {
    gfx_draw_item_symbol(0x14, 0x14);
  }

  /* gCurSubstateZ doubles as the prize count (watts amount or item index)
     for the value/item displays. */
  prize_count = gCurSubstateZ;
  event_rec = (uint8_t *)(uintptr_t)accelPos_X;
  flags = event_rec[2];
  if (flags == 0xFC) {
    gfx_draw_own_pokemon_name(0x00, 0x20, 5);
  } else if (flags == 0xFD) {
    gfx_draw_item_name(0x00, 0x20, prize_count, 0x0D);
  } else if (flags == 0xFE) {
    gfx_draw_value_with_icon(0x02, 0x20, 0x0D, (uint16_t)prize_count);
  } else if (flags != 0xFF) {
    gfx_draw_text_box(0x20, flags, TEXT_BOX_NO_SHADOW, TEXT_BOX_STATIC);
  }

  /* Bottom text: animated +A variant if flags bits 3-4 are set; otherwise a
     plain index. (FF on the top-strip implies a full-width text box.) */
  event_rec = (uint8_t *)(uintptr_t)accelPos_X;
  flags = *event_rec;
  if ((flags & 0x18) > 8) {
    gfx_draw_text_box(0x30, (uint8_t)(event_rec[3] + gCurSubstateA), TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
  } else if (event_rec[2] == 0xFF) {
    gfx_draw_text_box(0x30, event_rec[3], TEXT_BOX_FULL, TEXT_BOX_BLINK);
  } else {
    gfx_draw_text_box(0x30, event_rec[3], TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
  }
  gfx_draw_battery_low(0, 0);
}

// ROM: 0x5fc2  76.8%  saves: r2,r5,r6 -> sys_epilogue_r2_r5_r6
void game_check_periodic_events(void) {
  volatile uint16_t daily_steps;
  uint8_t prob;
  uint8_t *buf;

  if ((walker_status_flags & WALKER_MODE_MASK) != WALKER_MODE_DEEP_SLEEP)
    return;
  if (currentlyActiveView != VIEW_HOME)
    return;
  if (!(walker_status_flags_BIT.input_pending))
    return;

  walker_status_flags_BIT.input_pending = 0;
  gCurSubstateY = 0;
  gCurSubstateZ = 0;

  daily_steps = recentSessionSteps;
  (void)daily_steps;

  prob = (uint8_t)(sys_get_rng() % 100);
  if (prob >= 40)
    return;

  if (!(walker_status_flags_BIT.walking)) {
    if (recentSessionSteps < 300)
      return;
    gCurSubstateY = 0x07;
  } else {
    if (idleSeconds < 3600)
      return;

    sys_init_heap();
    buf = (uint8_t *)sbrk(0xBE);
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, buf, 0xBE);
    prob = buf[0x26];

    sys_init_heap();
    buf = (uint8_t *)sbrk(0x0C);
    drv_eeprom_read_block(EEPROM_LOG_ITEMS, buf, 0x0C);

    daily_steps = recentSessionSteps;
    if (save_find_empty_item_slot(buf) < 3 && prob >= 90 &&
        daily_steps >= 500) {
      gCurSubstateY = 0x01;
    } else if (prob >= 80 && daily_steps >= 250) {
      gCurSubstateY = 0x02;
    } else if (daily_steps >= 200) {
      gCurSubstateY = 0x03;
    } else if (daily_steps >= 100) {
      gCurSubstateY = 0x04;
    } else if (sessionTicksElapsed >= 60 && daily_steps <= 50) {
      gCurSubstateY = 0x05;
    } else {
      return;
    }
  }
  gCurSubstateZ = 0x30;
}
