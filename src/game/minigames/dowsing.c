#include "all_headers.h"

/*
 * Dowsing minigame.
 *
 * The player picks one of 6 grid positions; the device reveals whether they
 * found the hidden item. The item slot is rolled at game start
 * (game_init_dowsing). If "co-op mode" (peer dowsing) is active —
 * RamCache_settingsByte bit 0 set — encounter generation and selection follow
 * the peer/special-event path; otherwise the solo path uses the trainer's
 * step-unlock table.
 *
 * State machine driven by gCurSubstateZ:
 *   0 = idle (player picking slot)
 *   1 = digging animation (after OK pressed)
 *   2 = item found
 *   3 = wrong slot
 *   4 = reveal screen (after all attempts exhausted)
 *
 * Globals repurposed for dowsing state:
 *   gCurSubstateY      = result slot index / item kind
 *   gCurSubstateA      = animation tick countdown
 *   accelXPos          = attempts remaining (counts down)
 *   accelYPos          = "marked-wrong" slot
 *   accelZPos_b        = save-slot index for the awarded item
 *   DAT_f7d1           = cursor position (0..5)
 *   DAT_f7d5           = watt reward (nonzero => show watts on found screen)
 *   DAT_f7d8_w         = item id awarded
 *   dowsing_item_pos   = hidden item slot (0..5), chosen at init
 */

// ROM: 0x9c48  79.2%
void game_generate_encounter_dowsing(void) {
  uint32_t steps_required;
  uint8_t *scratch;
  uint8_t slot;
  uint8_t rnd_pct;
  uint8_t *trainer_buf;

  gCurSubstateY = 0;
  if (((RamCache_settingsByte & 1)) != 0) {
    uint8_t peer_evt_seed = drv_eeprom_read_u8(EEPROM_EEP_STR);
    sys_init_heap();
    scratch = sbrk(0x68);
    if (gfx_xor_rect_ram(scratch, peer_evt_seed) == 0) {
      if ((drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS) & 0x20) == 0) {
        sys_init_heap();
        scratch = sbrk(4);
        drv_eeprom_read_block(0xBF44, scratch, 4);
        steps_required = ((uint32_t)scratch[1] << 16) |
                         ((uint32_t)scratch[2] << 8) | scratch[3];
        if (steps_required <= sessionSteps) {
          if ((sys_get_rng() % 100) < scratch[2]) {
            gCurSubstateY = 4;
            accelXPos = ((sys_get_rng() >> 3) & 1) + 3;
            return;
          }
        }
      }
    }
  }

  /* LAB_9cea: solo path — three step-gated encounter tiers. */
  sys_init_heap();
  scratch = sbrk(0x30);
  drv_eeprom_read_block(EEPROM_LOG_CONTEXT, scratch, 0x30);
  rnd_pct = (uint8_t)(sys_get_rng() % 100);

  sys_init_heap();
  trainer_buf = sbrk(0xBE);
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);

  for (slot = 0; slot < 3; slot++) {
    if (!game_check_step_unlock((uint16_t)(slot * 2), 0x82, trainer_buf)) {
      if (trainer_buf[0x88 + slot] > rnd_pct) {
        gCurSubstateY = slot + 1;
        /* Codegen note: ROM emits the add as `-slot + (rng>>3 & 1) + 3`,
           hence the negation order here matters for the score. */
        accelXPos = (slot * -1) + ((sys_get_rng() >> 3) & 1) + 3;
        return;
      }
    }
  }

  gCurSubstateY = 3;
  accelXPos = ((sys_get_rng() >> 3) & 1) + 1;
}

// ROM: 0x4792  82.8%
void game_init_dowsing(void) {
  uint16_t rnd;

  gCurSubstateZ = 0;
  DAT_f7d1 = 0;
  accelXPos = 2;

  rnd = (uint16_t)sys_get_rng();
  rnd <<= 3;
  rnd = (uint16_t)((uint8_t)(rnd >> 8));
  dowsing_item_pos = (uint8_t)((int16_t)rnd % 6);

  accelYPos = 0xFF;
  DAT_f7d5 = 0;
}

// ROM: 0x47ce  82.0%
void ui_handle_dowsing(void) {
  uint8_t state;

  state = gCurSubstateZ;

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
  if (drv_button_is_triggered(BTN_R)) {
    if (DAT_f7d1 == accelYPos) {
      /* Already marked as wrong — beep but don't advance. */
      drv_sound_play(SND_BACK);
      return;
    }
    drv_sound_play(SND_CONFIRM);
    gCurSubstateZ = 1;
    gCurSubstateA = 4;
    return;
  }
  if (drv_button_is_triggered(BTN_M)) {
    DAT_f7d1 = (uint8_t)(((int32_t)((uint16_t)DAT_f7d1 + 5)) % 6);
    drv_sound_play(SND_CURSOR);
  }
  if (drv_button_is_triggered(BTN_L)) {
    DAT_f7d1 = (uint8_t)(((int32_t)((uint16_t)DAT_f7d1 + 1)) % 6);
    drv_sound_play(SND_CURSOR);
    return;
  }
  return;

state_digging:
  if (gCurSubstateA != 0) {
    return;
  }
  if (drv_sound_is_playing()) {
    return;
  }
  if (DAT_f7d1 == dowsing_item_pos) {
    gCurSubstateZ = 2;
    ui_handle_dowsing_selection();
    drv_sound_play(SND_DOWSE_HIT);
    return;
  }
  /* Wrong slot. */
  gCurSubstateZ = 3;
  drv_sound_play(SND_FAIL);
  if (accelXPos == 2) {
    accelYPos = DAT_f7d1;
  }
  accelXPos--;
  return;

state_found:
  if (!drv_button_is_triggered(BTN_ANY)) {
    return;
  }
  if ((RamCache_settingsByte & 1)) {
    goto exit_to_home;
  }
  {
    uint8_t save_slot = accelZPos_b;
    if (save_slot == 3) {
      /* No room left — kick into peer-session caught-stats view. */
      gCurSubstateA = 1;
      game_start_peer_session();
      ui_set_view(VIEW_CAUGHT_STATS);
      return;
    }
    drv_eeprom_write_block((uint16_t)save_slot * 4 + EEPROM_LOG_ITEMS,
                           (void *)&DAT_f7d8_w, 0x2);
  }
  if (walker_status_flags_BIT.walking) {
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
  if (accelXPos != 0) {
    /* Still have attempts left — bounce back to idle. */
    drv_sound_play(SND_CONFIRM);
    gCurSubstateZ = 4;
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
  gCurSubstateZ = 0;
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

  if (sessionSteps < steps_required) {
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

  if (gfx_xor_rect_ram(battle_buf, route_data[0x7B]) == 0) {
    goto encounter;
  }

no_encounter:
  DAT_f7d5 = (uint8_t)((accelXPos << 2) + 2);
  game_add_watts(10);
  return;

encounter:
  save_set_event_bit(battle_buf, route_data[0x7B]);

  {
    uint8_t *trainer_buf = (uint8_t *)sbrk(0xBE);
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);

    gCurSubstateY = 0x0A;

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

    if (walker_status_flags_BIT.walking) {
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

  accelZPos_b = save_find_empty_slot_32bit(item_table);

  if ((RamCache_settingsByte & 1)) {
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

  gCurSubstateY = slot;
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

  /* Player sprite (frame selected by accelXPos). */
  drv_eeprom_read_block(sprites_base + (uint16_t)accelXPos * 0x20, buf, 0x20);
  drv_lcd_blit(0x40, 0, buf, 0x08, 0x10);

  /* Background top strip. */
  drv_eeprom_read_block(0x1950 + sprites_base, buf, 0x80);
  drv_lcd_blit(0x20, 0, buf, 0x20, 0x10);

  drv_eeprom_read_block(0x19D0 + sprites_base, buf, 0x60);
  drv_lcd_blit(0x48, 0, buf, 0x18, 0x10);

  /* Grass background — different art in co-op vs solo. */
  {
    uint16_t bg_addr;
    if ((RamCache_settingsByte & 1)) {
      bg_addr = 0xC83C;
    } else {
      bg_addr = 0x8FBE;
    }
    drv_eeprom_read_block(bg_addr, buf, 0xC0);
  }
  drv_lcd_blit(0, 0, buf, 0x20, 0x18);

  /* Tick down dig animation. */
  {
    uint8_t a = gCurSubstateA;
    if (a != 0) {
      gCurSubstateA = a - 1;
    }
  }

  /* Six selection circles across the grid. */
  drv_eeprom_read_block(0x18D0 + sprites_base, buf, 0x80);
  for (i = 0; i < 6; i++) {
    uint8_t x = (uint8_t)(i * 0x10);
    if ((uint8_t)i == accelYPos) {
      /* "Wrong-slot" marker. */
      drv_lcd_blit(x, 0x18, buf + 0x40, 0x10, 0x10);
    } else if ((uint8_t)i != DAT_f7d1) {
      drv_lcd_blit(x, 0x18, buf, 0x10, 0x10);
    }
  }

  /* Bobbing grass overlay — animTick selects one of 4 wobble offsets. */
  {
    uint8_t anim = animTick & 0x03;
    gfx_draw_animated_grass(
        0x10, 0x10, (int8_t)*((volatile uint8_t *)(0xBD82 + anim)), buf);
  }

  /* Highlighted cursor on top. */
  {
    uint8_t cursor_x = (uint8_t)(DAT_f7d1 * 0x10);
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

  if (gCurSubstateZ == 1) {
    ui_render_dowsing_grass();
    goto end;
  }

  sys_init_heap();
  sprite_sheet = (uint8_t *)sbrk(0x140);
  buf = (uint8_t *)sbrk(0x180);

  /* Player sprite — load full sheet, then point at frame accelXPos. */
  drv_eeprom_read_block(sprites_base, sprite_sheet, 0x140);
  sprite_sheet += (uint16_t)accelXPos * 0x20;
  drv_lcd_blit(0x40, 0, sprite_sheet, 0x08, 0x10);

  /* Background pieces. */
  drv_eeprom_read_block(0x1950 + sprites_base, buf, 0x80);
  drv_lcd_blit(0x20, 0, buf, 0x20, 0x10);

  drv_eeprom_read_block(0x19D0 + sprites_base, buf, 0x60);
  drv_lcd_blit(0x48, 0, buf, 0x18, 0x10);

  /* Grass background. */
  {
    uint16_t bg_addr;
    if ((RamCache_settingsByte & 1)) {
      bg_addr = 0xC83C;
    } else {
      bg_addr = 0x8FBE;
    }
    drv_eeprom_read_block(bg_addr, buf, 0xC0);
  }
  drv_lcd_blit(0, 0, buf, 0x20, 0x18);

  {
    uint8_t a = gCurSubstateA;
    if (a != 0) {
      gCurSubstateA = a - 1;
    }
  }

  /* Six probe circles. */
  drv_eeprom_read_block(0x18D0 + sprites_base, buf, 0x80);
  for (i = 0; i < 6; i++) {
    uint8_t x = (uint8_t)(i * 0x10);

    if (gCurSubstateZ == 2 && (uint8_t)i == DAT_f7d1) {
      /* Found state hides the cursor slot — item sprite drawn below. */
      continue;
    }
    if ((uint8_t)i == accelYPos) {
      drv_lcd_blit(x, 0x18, buf + 0x40, 0x10, 0x10);
    } else {
      drv_lcd_blit(x, 0x18, buf, 0x10, 0x10);
    }
  }

  /* Per-substate overlay. */
  if (gCurSubstateZ == 0) {
    /* Idle: dowsing rod, two-frame bob. */
    uint8_t frame = animTick & 0x01;
    uint8_t rod_x;
    drv_eeprom_read_block(0x278 + sprites_base + (uint16_t)frame * 0x10, buf,
                          0x10);
    rod_x = (uint8_t)(DAT_f7d1 * 0x10 + 0x04);
    drv_lcd_blit(rod_x, 0x28, buf, 8, 8);
    gfx_draw_text_box(0x30, TEXT_DISCOVER_AN_ITEM, TEXT_BOX_FULL, TEXT_BOX_STATIC);

  } else if (gCurSubstateZ == 2) {
    /* Found item: draw item icon and either watts or item name. */
    uint8_t item_x;
    drv_eeprom_read_block(0x208 + sprites_base, buf, 0x10);
    item_x = (uint8_t)(DAT_f7d1 * 0x10 + 0x04);
    drv_lcd_blit(item_x, 0x18, buf, 8, 8);

    if (DAT_f7d5 != 0) {
      gfx_draw_value_with_icon(0x02, 0x20, 0x0D, (uint16_t)DAT_f7d5);
      gfx_draw_text_box(0x30, TEXT_RECEIVED, TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
    } else {
      uint8_t result_kind = gCurSubstateY;
      if (result_kind >= 0x0A) {
        gfx_draw_event_item_name(0x00, 0x20, 0, 0x0D);
      } else {
        gfx_draw_item_name(0x00, 0x20, result_kind, 0x0D);
      }
      gfx_draw_text_box(0x30, TEXT_FOUND, TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
    }

  } else if (gCurSubstateZ == 3) {
    gfx_draw_text_box(0x30, TEXT_NOTHING_FOUND, TEXT_BOX_FULL, TEXT_BOX_BLINK);

    /* Reveal the hidden item only after all attempts are spent. The loop
       draws the icon 3 times for emphasis (single-frame flash effect). */
    if (accelXPos == 0) {
      uint16_t k;
      drv_eeprom_read_block(0x208 + sprites_base, buf, 0x10);
      for (k = 3; k > 0; k--) {
        uint8_t item_x = (uint8_t)(dowsing_item_pos * 0x10 + 0x04);
        drv_lcd_blit(item_x, 0x16, buf, 8, 8);
      }
    }

  } else if (gCurSubstateZ == 4) {
    /* Proximity indicator: |cursor - hidden| < 2 ⇒ "close", else "far". */
    int16_t diff = (int16_t)(uint16_t)DAT_f7d1 -
                   (int16_t)(uint16_t)dowsing_item_pos;
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
