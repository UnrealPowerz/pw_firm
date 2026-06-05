#include "all_headers.h"

/*
 * Dowsing minigame.
 *
 * The player picks one of 6 grid positions; the device reveals whether they
 * found the hidden item. The item slot is rolled at game start
 * (game_init_dowsing). If "co-op mode" (peer dowsing) is active —
 * g.save_settings.BYTE bit 0 set — encounter generation and selection follow
 * the peer/special-event path; otherwise the solo path uses the trainer's
 * step-unlock table.
 *
 * State machine driven by g.viewstate.Z:
 *   0 = idle (player picking slot)
 *   1 = digging animation (after OK pressed)
 *   2 = item found
 *   3 = wrong slot
 *   4 = reveal screen (after all attempts exhausted)
 *
 * Globals repurposed for dowsing state:
 *   g.viewstate.Y.BYTE      = result slot index / item kind
 *   g.viewstate.A      = animation tick countdown
 *   g.viewstate.v.dowsing.attemptsRemaining          = attempts remaining (counts down)
 *   g.viewstate.v.dowsing.markedWrongSlot          = "marked-wrong" slot
 *   g.viewstate.v.dowsing.saveSlot        = save-slot index for the awarded item
 *   g.viewstate.v.dowsing.cursor           = cursor position (0..5)
 *   g.viewstate.v.dowsing.wattReward           = watt reward (nonzero => show g.save_watts on found screen)
 *   DAT_f7d8_w         = item id awarded
 *   g.viewstate.v.dowsing.hiddenSlot   = hidden item slot (0..5), chosen at init
 */

// ROM: 0x4792  82.8%
void game_init_dowsing(void) {
  uint16_t rnd;

  g.viewstate.Z = 0;
  g.viewstate.v.dowsing.cursor = 0;
  g.viewstate.v.dowsing.attemptsRemaining = 2;

  rnd = (uint16_t)sys_get_rng();
  rnd <<= 3;
  rnd = (uint16_t)((uint8_t)(rnd >> 8));
  g.viewstate.v.dowsing.hiddenSlot = (uint8_t)((int16_t)rnd % 6);

  g.viewstate.v.dowsing.markedWrongSlot = 0xFF;
  g.viewstate.v.dowsing.wattReward = 0;
}

// ROM: 0x47ce  82.0%
void ui_handle_dowsing(void) {
  uint8_t state;

  state = g.viewstate.Z;

  if (state == 0) {
    goto state_idle;
  } else if (state == 1) {
    goto state_digging;
  } else if (state == 2) {
    goto state_found;
  } else if (state == 3) {
    goto state_miss;
  } else if (state == 4) {
    goto state_reveal;
  }
  return;

state_idle:
  if (drv_button_is_triggered(BTN_M)) {
    if (g.viewstate.v.dowsing.cursor == g.viewstate.v.dowsing.markedWrongSlot) {
      /* Already marked as wrong — beep but don't advance. */
      drv_sound_play(SND_BACK);
      return;
    }
    drv_sound_play(SND_CONFIRM);
    g.viewstate.Z = 1;
    g.viewstate.A = 4;
    return;
  }
  if (drv_button_is_triggered(BTN_R)) {
    g.viewstate.v.dowsing.cursor = (uint8_t)(((int32_t)((uint16_t)g.viewstate.v.dowsing.cursor + 5)) % 6);
    drv_sound_play(SND_CURSOR);
  }
  if (drv_button_is_triggered(BTN_L)) {
    g.viewstate.v.dowsing.cursor = (uint8_t)(((int32_t)((uint16_t)g.viewstate.v.dowsing.cursor + 1)) % 6);
    drv_sound_play(SND_CURSOR);
    return;
  }
  return;

state_digging:
  if (g.viewstate.A != 0) {
    return;
  }
  if (drv_sound_is_playing()) {
    return;
  }
  if (g.viewstate.v.dowsing.cursor == g.viewstate.v.dowsing.hiddenSlot) {
    g.viewstate.Z = 2;
    ui_handle_dowsing_selection();
    drv_sound_play(SND_DOWSE_HIT);
    return;
  }
  /* Wrong slot. */
  g.viewstate.Z = 3;
  drv_sound_play(SND_FAIL);
  if (g.viewstate.v.dowsing.attemptsRemaining == 2) {
    g.viewstate.v.dowsing.markedWrongSlot = g.viewstate.v.dowsing.cursor;
  }
  g.viewstate.v.dowsing.attemptsRemaining--;
  return;

state_found:
  if (!drv_button_is_triggered(BTN_ANY)) {
    return;
  }
  if ((g.save_settings.BYTE & 1)) {
    goto exit_to_home;
  }
  {
    uint8_t save_slot = g.viewstate.v.dowsing.saveSlot;
    if (save_slot == 3) {
      /* No room left — kick into peer-session caught-stats view. */
      g.viewstate.A = 1;
      ui_init_discard_cursor();
      ui_set_view(VIEW_DISCARD_PICKER);
      return;
    }
    drv_eeprom_write_block((uint16_t)save_slot * 4 + EEPROM_LOG_ITEMS,
                           (void *)&DAT_f7d8_w, 0x2);
  }
  if (g.sys_walkerFlags.BIT.walking) {
    uint8_t *trainer_buf = (uint8_t *)sbrk(0xBE);
    uint8_t *gift_buf;
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);
    gift_buf = (uint8_t *)sbrk(0x88);
    game_log_interaction(trainer_buf, gift_buf, 0x0B, 0x00, DAT_f7d8_w, 0);
  }

exit_to_home:
  drv_sound_play(SND_CONFIRM);
  ui_reset_substate();
  ui_set_view(VIEW_HOME);
  return;

state_miss:
  if (!drv_button_is_triggered(BTN_ANY)) {
    return;
  }
  if (g.viewstate.v.dowsing.attemptsRemaining != 0) {
    /* Still have attempts left — bounce back to idle. */
    drv_sound_play(SND_CONFIRM);
    g.viewstate.Z = 4;
    return;
  }
  drv_sound_play(SND_CONFIRM);
  ui_reset_substate();
  ui_set_view(VIEW_HOME);
  return;

state_reveal:
  if (!drv_button_is_triggered(BTN_ANY)) {
    return;
  }
  drv_sound_play(SND_CONFIRM);
  g.viewstate.Z = 0;
  return;
}

// ROM: 0x499c  52.5%
void game_read_wild_poke(void *ram_dst) {
  drv_eeprom_read_block(EEPROM_WILD_POKE, ram_dst, 0x188);
}

// ROM: 0x49ae  52.5%
void game_write_wild_poke(void *ram_src) {
  drv_eeprom_write_block(EEPROM_WILD_POKE, ram_src, 0x188);
}

// ROM: 0x49c0  83.5%
#pragma option noregexpansion /* pragma:auto */
void game_check_wild_encounter(void) {
  uint8_t *battle_buf;
  uint8_t *encounter_data;
  uint8_t *route_data;
  uint32_t steps_required;
  uint16_t rnd_pct;
  uint16_t encounter_rate;

  sys_init_heap();
  battle_buf = (uint8_t *)sbrk(0x68);
  encounter_data = (uint8_t *)sbrk(0x188);
  route_data = (uint8_t *)sbrk(0x7C);

  drv_eeprom_read_block(0xBF00, route_data, 0x7C);

  /* Byte-swap big-endian 16-bit value at offset 0x4A into the step
     requirement. (EEPROM stores multi-byte fields BE.) */
  {
    uint16_t w = *((uint16_t *)(route_data + 0x4A));
    steps_required = (uint32_t)(((w & 0xFF00) >> 8) | ((w & 0xFF) << 8));
  }

  if (g.session_steps < steps_required) {
    goto no_encounter;
  }

  rnd_pct = (uint16_t)((int16_t)((uint16_t)sys_get_rng() >> 3) % 100);
  encounter_rate = (uint16_t)route_data[0x4C];
  if (rnd_pct >= encounter_rate) {
    goto no_encounter;
  }

  game_read_wild_poke(encounter_data);
  if (*((uint16_t *)(encounter_data + 0x6)) != 0) {
    /* Already an encounter pending — don't overwrite it. */
    goto no_encounter;
  }

  if (save_check_event_bit(battle_buf, route_data[0x7B]) == 0) {
    goto encounter;
  }

no_encounter:
  g.viewstate.v.dowsing.wattReward = (uint8_t)((g.viewstate.v.dowsing.attemptsRemaining << 2) + 2);
  game_add_watts(10);
  return;

encounter:
  save_set_event_bit(battle_buf, route_data[0x7B]);

  {
    uint8_t *trainer_buf = (uint8_t *)sbrk(0xBE);
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);

    g.viewstate.Y.BYTE = 0x0A;

    *((uint32_t *)encounter_data) = *((uint32_t *)route_data);
    *((uint16_t *)(encounter_data + 0x4)) = *((uint16_t *)(route_data + 0x4));
    *((uint16_t *)(encounter_data + 0x6)) = *((uint16_t *)(route_data + 0x48));

    drv_eeprom_read_block(0xCA3C, encounter_data + 0x8, 0x180);
    game_write_wild_poke(encounter_data);

    {
      uint8_t flags = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      flags |= 0x40;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, flags);
    }

    if (g.sys_walkerFlags.BIT.walking) {
      uint8_t *gift_buf = (uint8_t *)sbrk(0x88);
      game_log_interaction(trainer_buf, gift_buf, 0x0C, 0x01, DAT_f7d8_w, 0);
    }
  }
}

// ROM: 0x4af2  81.3%
void ui_handle_dowsing_selection(void) {
  uint8_t *item_table;
  uint8_t *trainer_buf;
  uint8_t slot;

  sys_init_heap();
  item_table = (uint8_t *)sbrk(0x0C);
  drv_eeprom_read_block(EEPROM_LOG_ITEMS, item_table, 0x0C);

  g.viewstate.v.dowsing.saveSlot = save_find_empty_item_slot(item_table);

  if ((g.save_settings.BYTE & 1)) {
    game_check_wild_encounter();
    return;
  }

  trainer_buf = (uint8_t *)sbrk(0xBE);
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);

  /* Walk the step-unlock table; first slot whose roll passes is the award. */
  for (slot = 0; slot < 10; slot++) {
    uint16_t rnd_pct;
    uint16_t slot_rate;

    if (game_check_step_unlock((uint16_t)slot * 2, 0xA0, trainer_buf)) {
      continue;
    }

    rnd_pct = (uint16_t)((int16_t)((uint16_t)sys_get_rng() >> 3) % 100);
    slot_rate = (uint16_t)trainer_buf[0xB4 + slot];

    if (rnd_pct < slot_rate) {
      break;
    }
  }

  if (slot > 9) {
    slot = 9;
  }

  g.viewstate.Y.BYTE = slot;
  DAT_f7d8_w = *((uint16_t *)(trainer_buf + 0x8C + (uint16_t)slot * 2));
}

// ROM: 0x4b9c  87.2%  saves: r5
void ui_render_dowsing_grass(void) {
  uint8_t *buf;
  volatile uint16_t sprites_base;
  uint16_t i;

  sprites_base = 0x280;
  sys_init_heap();
  buf = (uint8_t *)sbrk(0x180);

  /* Player sprite (frame selected by g.viewstate.v.dowsing.attemptsRemaining). */
  drv_eeprom_read_block(sprites_base + (uint16_t)g.viewstate.v.dowsing.attemptsRemaining * 0x20, buf, 0x20);
  drv_lcd_blit(0x40, 0, buf, 0x08, 0x10);

  /* Background top strip. */
  drv_eeprom_read_block(0x1950 + sprites_base, buf, 0x80);
  drv_lcd_blit(0x20, 0, buf, 0x20, 0x10);

  drv_eeprom_read_block(0x19D0 + sprites_base, buf, 0x60);
  drv_lcd_blit(0x48, 0, buf, 0x18, 0x10);

  /* Grass background — different art in co-op vs solo. */
  {
    uint16_t bg_addr;
    if ((g.save_settings.BYTE & 1)) {
      bg_addr = 0xC83C;
    } else {
      bg_addr = 0x8FBE;
    }
    drv_eeprom_read_block(bg_addr, buf, 0xC0);
  }
  drv_lcd_blit(0, 0, buf, 0x20, 0x18);

  /* Tick down dig animation. */
  {
    uint8_t a = g.viewstate.A;
    if (a != 0) {
      g.viewstate.A = a - 1;
    }
  }

  /* Six selection circles across the grid. */
  drv_eeprom_read_block(0x18D0 + sprites_base, buf, 0x80);
  for (i = 0; i < 6; i++) {
    uint8_t x = (uint8_t)(i * 0x10);
    if ((uint8_t)i == g.viewstate.v.dowsing.markedWrongSlot) {
      /* "Wrong-slot" marker. */
      drv_lcd_blit(x, 0x18, buf + 0x40, 0x10, 0x10);
    } else if ((uint8_t)i != g.viewstate.v.dowsing.cursor) {
      drv_lcd_blit(x, 0x18, buf, 0x10, 0x10);
    }
  }

  /* Bobbing grass overlay — g.ui_animationTick selects one of 4 wobble offsets. */
  {
    uint8_t anim = g.ui_animationTick & 0x03;
    gfx_draw_animated_grass(
        0x10, 0x10, (int8_t)*((volatile uint8_t *)(0xBD82 + anim)), buf);
  }

  /* Highlighted cursor on top. */
  {
    uint8_t cursor_x = (uint8_t)(g.viewstate.v.dowsing.cursor * 0x10);
    drv_lcd_blit(cursor_x, 0x18, buf, 0x10, 0x10);
  }

  gfx_draw_text_box(0x30, TEXT_DISCOVER_AN_ITEM, TEXT_BOX_FULL, TEXT_BOX_STATIC);
}

// Reason: ROM hoists `mov.w #0x280, r5` at entry as the EEPROM base constant
//   and reuses it via add.w r5,... at multiple read sites; ch38 inlines the
//   immediate at each call site. ROM also uses no prologue helper (starts
//   immediately); ch38 emits `$sp_regsv$3`. ch38 swaps the register choice
//   for the two sbrk buffers (R6 vs ROM's R4, R5 vs R6) — different
//   allocator. Body structure (substate dispatch, sprite blit, item icon
//   draws) matches.
// Class: cannot-fix-without-compiler-change (constant hoisting + sp_regsv$3)
// ROM: 0x4cd6  67.3%
void ui_render_dowsing(void) {
  uint8_t *buf;
  uint8_t *sprite_sheet;
  uint16_t sprites_base;
  uint16_t i;

  sprites_base = 0x280;

  if (g.viewstate.Z == 1) {
    ui_render_dowsing_grass();
    goto end;
  }

  sys_init_heap();
  sprite_sheet = (uint8_t *)sbrk(0x140);
  buf = (uint8_t *)sbrk(0x180);

  /* Player sprite — load full sheet, then point at frame g.viewstate.v.dowsing.attemptsRemaining. */
  drv_eeprom_read_block(sprites_base, sprite_sheet, 0x140);
  sprite_sheet += (uint16_t)g.viewstate.v.dowsing.attemptsRemaining * 0x20;
  drv_lcd_blit(0x40, 0, sprite_sheet, 0x08, 0x10);

  /* Background pieces. */
  drv_eeprom_read_block(0x1950 + sprites_base, buf, 0x80);
  drv_lcd_blit(0x20, 0, buf, 0x20, 0x10);

  drv_eeprom_read_block(0x19D0 + sprites_base, buf, 0x60);
  drv_lcd_blit(0x48, 0, buf, 0x18, 0x10);

  /* Grass background. */
  {
    uint16_t bg_addr;
    if ((g.save_settings.BYTE & 1)) {
      bg_addr = 0xC83C;
    } else {
      bg_addr = 0x8FBE;
    }
    drv_eeprom_read_block(bg_addr, buf, 0xC0);
  }
  drv_lcd_blit(0, 0, buf, 0x20, 0x18);

  {
    uint8_t a = g.viewstate.A;
    if (a != 0) {
      g.viewstate.A = a - 1;
    }
  }

  /* Six probe circles. */
  drv_eeprom_read_block(0x18D0 + sprites_base, buf, 0x80);
  for (i = 0; i < 6; i++) {
    uint8_t x = (uint8_t)(i * 0x10);

    if (g.viewstate.Z == 2 && (uint8_t)i == g.viewstate.v.dowsing.cursor) {
      /* Found state hides the cursor slot — item sprite drawn below. */
      continue;
    }
    if ((uint8_t)i == g.viewstate.v.dowsing.markedWrongSlot) {
      drv_lcd_blit(x, 0x18, buf + 0x40, 0x10, 0x10);
    } else {
      drv_lcd_blit(x, 0x18, buf, 0x10, 0x10);
    }
  }

  /* Per-substate overlay. */
  if (g.viewstate.Z == 0) {
    /* Idle: dowsing rod, two-frame bob. */
    uint8_t frame = g.ui_animationTick & 0x01;
    uint8_t rod_x;
    drv_eeprom_read_block(0x278 + sprites_base + (uint16_t)frame * 0x10, buf,
                          0x10);
    rod_x = (uint8_t)(g.viewstate.v.dowsing.cursor * 0x10 + 0x04);
    drv_lcd_blit(rod_x, 0x28, buf, 8, 8);
    gfx_draw_text_box(0x30, TEXT_DISCOVER_AN_ITEM, TEXT_BOX_FULL, TEXT_BOX_STATIC);

  } else if (g.viewstate.Z == 2) {
    /* Found item: draw item icon and either g.save_watts or item name. */
    uint8_t item_x;
    drv_eeprom_read_block(0x208 + sprites_base, buf, 0x10);
    item_x = (uint8_t)(g.viewstate.v.dowsing.cursor * 0x10 + 0x04);
    drv_lcd_blit(item_x, 0x18, buf, 8, 8);

    if (g.viewstate.v.dowsing.wattReward != 0) {
      gfx_draw_value_with_icon(0x02, 0x20, 0x0D, (uint16_t)g.viewstate.v.dowsing.wattReward);
      gfx_draw_text_box(0x30, TEXT_RECEIVED, TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
    } else {
      uint8_t result_kind = g.viewstate.Y.BYTE;
      if (result_kind >= 0x0A) {
        gfx_draw_event_item_name(0x00, 0x20, 0, 0x0D);
      } else {
        gfx_draw_item_name(0x00, 0x20, result_kind, 0x0D);
      }
      gfx_draw_text_box(0x30, TEXT_FOUND, TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
    }

  } else if (g.viewstate.Z == 3) {
    gfx_draw_text_box(0x30, TEXT_NOTHING_FOUND, TEXT_BOX_FULL, TEXT_BOX_BLINK);

    /* Reveal the hidden item only after all attempts are spent. The loop
       draws the icon 3 times for emphasis (single-frame flash effect). */
    if (g.viewstate.v.dowsing.attemptsRemaining == 0) {
      uint16_t k;
      drv_eeprom_read_block(0x208 + sprites_base, buf, 0x10);
      for (k = 3; k > 0; k--) {
        uint8_t item_x = (uint8_t)(g.viewstate.v.dowsing.hiddenSlot * 0x10 + 0x04);
        drv_lcd_blit(item_x, 0x16, buf, 8, 8);
      }
    }

  } else if (g.viewstate.Z == 4) {
    /* Proximity indicator: |cursor - hidden| < 2 ⇒ "close", else "far". */
    int16_t diff = (int16_t)(uint16_t)g.viewstate.v.dowsing.cursor -
                   (int16_t)(uint16_t)g.viewstate.v.dowsing.hiddenSlot;
    uint16_t dist = diff < 0 ? (uint16_t)(-diff) : (uint16_t)diff;

    if (dist < 2) {
      gfx_draw_text_box(0x30, TEXT_ITS_NEAR, TEXT_BOX_FULL, TEXT_BOX_BLINK);
    } else {
      gfx_draw_text_box(0x30, TEXT_ITS_FAR_AWAY, TEXT_BOX_FULL, TEXT_BOX_BLINK);
    }
  }

end:
  gfx_draw_battery_low(0, 0x58);
}
