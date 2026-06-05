#include "all_headers.h"

/*
 * VIEW_PEER_PLAY — the "your walker met another walker" post-IR-session
 * celebration view. Renders the peer pokemon, plays music-note animations,
 * and shows the calculated reward.
 *
 *   ui_start_peer_play_app          Entry — set up substate, copy the
 *                                   incoming-walker flag, calc reward, rotate
 *                                   the interaction log.
 *   ui_handle_peer_play             Empty per-tick handler — rendering does
 *                                   all the work (advances on its own).
 *   game_calculate_interaction_reward
 *                                   Compute the g.save_watts/item reward tier from
 *                                   step counts; populate the next free
 *                                   dowsing item slot if appropriate.
 *   ui_draw_music_note              Helper to blit a single music-note sprite
 *                                   with optional vertical shift.
 *   ui_render_peer_play             Main renderer — five-phase animation
 *                                   driven by g.gCurSubstateZ (0..4).
 *
 *   game_find_seen_peer             EEPROM scan called from ir_protocol.c
 *                                   during handshake to check if a peer's
 *                                   identity has been logged before.
 */

// ROM: 0x6382  60.8%
void game_calculate_interaction_reward(void) {
  uint32_t score;
  uint8_t *peer_steps;
  uint16_t *item_table;
  uint8_t free_slot;
  uint16_t total_daily_steps;

  /* Read the peer's step contributions (0xF6C0 = 0x38-byte peer-data block;
     first 4 bytes = peer session-steps uint32, then peer recent-steps uint16). */
  sys_init_heap();
  sbrk(0xBE);
  peer_steps = (uint8_t *)sbrk(0x38);
  drv_eeprom_read_block(0xF6C0, peer_steps, 0x38);

  /* Read the dowsed-items table (10 slots x 2 uint16 = 0x28 bytes). */
  item_table = (uint16_t *)sbrk(0x28);
  drv_eeprom_read_block(0xCEC8, item_table, 0x28);

  /* score = own session steps + peer session steps + 10*(own recent + peer recent),
     clamped to 20000. */
  score = g.session_steps + *(uint32_t *)peer_steps;
  total_daily_steps = g.session_recentSteps + *(uint16_t *)(peer_steps + 4);
  score += (uint32_t)total_daily_steps * 10;

  if (score > 20000) {
    score = 20000;
  }

  /* Find the first empty dowsed-item slot. */
  for (free_slot = 0; free_slot < 10; free_slot++) {
    if (item_table[free_slot * 2] == 0) {
      break;
    }
  }

  g.dowsing_item_pos = 0;
  if (free_slot < 10) {
    /* Watts award = score/200, clamped to [1, 99]. */
    g.dowsing_item_pos = (uint8_t)(score / 200);
    if (g.dowsing_item_pos == 0) {
      g.dowsing_item_pos = 1;
    }
    if (g.dowsing_item_pos > 99) {
      g.dowsing_item_pos = 99;
    }
    game_add_watts(g.dowsing_item_pos);
  }

  /* Pick the result-text index (g.DAT_f7d1 = 0x2C..0x30 → TEXT_HAD_ADVENTURES,
     PLAY_BATTLED, etc.) and the dowsing-item index based on score tier. */
  if (score >= 20000) {
    g.DAT_f7d1 = 0x2C;
    if (g.dowsing_item_pos != 0) return;
    g.accelXPos = (g.session_steps > *(uint32_t *)peer_steps) ? 0 : 1;
  } else if (score >= 10000) {
    g.DAT_f7d1 = 0x2D;
    if (g.dowsing_item_pos != 0) return;
    g.accelXPos = (g.session_steps > *(uint32_t *)peer_steps) ? 2 : 3;
  } else if (score >= 5000) {
    g.DAT_f7d1 = 0x2E;
    if (g.dowsing_item_pos != 0) return;
    g.accelXPos = (g.session_steps > *(uint32_t *)peer_steps) ? 4 : 5;
  } else if (score >= 2500) {
    g.DAT_f7d1 = 0x2F;
    if (g.dowsing_item_pos != 0) return;
    g.accelXPos = (g.session_steps > *(uint32_t *)peer_steps) ? 6 : 7;
  } else {
    g.DAT_f7d1 = 0x30;
    if (g.dowsing_item_pos != 0) return;
    g.accelXPos = (g.session_steps > *(uint32_t *)peer_steps) ? 8 : 9;
  }

  /* Write the chosen item id into the free slot. */
  {
    uint16_t item_id = save_get_dowsing_item_id(g.accelXPos);
    item_table[free_slot * 2] = item_id;
    drv_eeprom_write_block(0xCEC8, item_table, 0x28);
  }
}

// ROM: 0x6380  100.0%
void ui_handle_peer_play(void) { (void)0; }

// ROM: 0x632c  64.3%  saves: er2,er3,r4,er5,er6 -> sys_epilogue_5
void ui_start_peer_play_app(void) {
  uint8_t *buf;
  sys_init_heap();
  buf = sbrk(0x38);
  drv_eeprom_read_block(0xF6C0, buf, 0x38);
  /* Copy bit 0 of buf[0x37] into bit 1 of g.gCurSubstateY -- the ROM uses
   * bld+bst here, which means the destination bit is unconditionally set
   * to the source bit (not OR'd as the original C suggested). */
  ((byte_bits_t *)&g.gCurSubstateY)->BIT.b1 =
      ((byte_bits_t *)&buf[0x37])->BIT.b0;
  g.gCurSubstateZ = 0;
  g.gCurSubstateA = 0;
  game_calculate_interaction_reward();
  game_rotate_interaction_log();
  game_rotate_interaction_log_record();
}

// ROM: 0x6528  84.1%  saves: r6
void ui_draw_music_note(uint8_t x, uint8_t y, uint8_t shift) {
  uint8_t *buf;
  uint8_t i;
  uint16_t eeprom_addr = 0x2470;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x10);
  drv_eeprom_read_block(eeprom_addr, buf, 0x10);

  for (i = 0; i < 0x10; i++) {
    *(buf + i) >>= shift;
  }
  drv_lcd_blit(x, y, buf, 8, 8);
}

// Reason: ROM emits NO prologue (zero pushes) — this function trashes the
//   caller's r3/r4/r5/r6 freely, like ui_load_inventory_mask. ch38 emits
//   `push.l er6; push.l er5; push.w r4; push.w r3` (12 bytes), breaking
//   alignment from byte 0. ROM also uses the `bld/bst` bit-copy idiom
//   (`bld #1, g.gCurSubstateY; bst #0, r6l`) for the `r6l = (g.gCurSubstateY >>
//   1) & 1` pattern; ch38 emits the `btst/beq/mov #1` triple from the
//   explicit `if (g.gCurSubstateY & 2) r6l = 1` form. Rewriting that one site
//   with byte_bits_t bit-copy might gain ~2pp but the prologue blocker caps
//   the function regardless. Body's branch structure and call args look
//   correct.
// Class: cannot-fix-without-compiler-change (no-prologue convention; same
//   ABI blocker as ui_load_inventory_mask / gfx_draw_animated_grass)
// ROM: 0x6574  23.1%
void ui_render_peer_play(void) {
  uint8_t z = g.gCurSubstateZ;
  uint8_t peer_facing = 0;     /* 0 = flipped, 1 = native (driven by Y bit 1) */
  uint8_t step, entry_x, r0h;

  if (z < 3) {
    gfx_draw_own_pokemon_small(0x38, 0x08);
    if (g.gCurSubstateY & 0x02) {
      peer_facing = 1;
    }
    if (z == 0) {
      /* Phase 0: peer walks in from the left. x = 8 - 3*(7 - A). */
      step = (uint8_t)(7 - g.gCurSubstateA);
      r0h = 3;
      step *= r0h;
      entry_x = (uint8_t)(8 - step);
      if (peer_facing != 0) {
        gfx_draw_peer_pokemon(entry_x, 8, 0x00);
      } else {
        gfx_draw_peer_pokemon(entry_x, 8, 0x01);
      }
    } else {
      /* Phases 1-2: peer settled at x=8. */
      if (peer_facing != 0) {
        gfx_draw_peer_pokemon(0x08, 0x08, 0x00);
      } else {
        gfx_draw_peer_pokemon(0x08, 0x08, 0x01);
      }
    }
  }

  if (z == 1) {
    gfx_draw_peer_pokemon_name(0x02, 0x20, 1);
    gfx_draw_text_box(0x30, TEXT_WALKER_HAS_ARRIVED, TEXT_BOX_NO_LINES, TEXT_BOX_STATIC);
  } else if (z == 2) {
    uint8_t count = g.gCurSubstateA + 1;
    uint8_t limit = (g.gCurSubstateA >> 1) + 1;
    uint8_t table_idx = 0;
    uint8_t i;

    if (g.DAT_f7d1 == 0x2C) {
      limit = count;
      if (limit > 5)
        limit = 5;
    } else if (g.DAT_f7d1 == 0x2D) {
      if (limit > 4)
        limit = 4;
    } else if (g.DAT_f7d1 == 0x2E) {
      if (limit > 3)
        limit = 3;
    } else if (g.DAT_f7d1 == 0x2F) {
      if (limit > 2)
        limit = 2;
      table_idx = 1;
    } else if (g.DAT_f7d1 == 0x30) {
      ui_draw_music_note(0x2C, MUSIC_NOTE_HEIGHTS[2], 0);  /* peak height */
      goto music_done;
    }

    for (i = 0; i < limit; i++) {
      uint8_t note_y = MUSIC_NOTE_HEIGHTS[i + table_idx];
      ui_draw_music_note((uint8_t)(i * 8 + 0x1C), note_y, 0);
    }
  music_done:
    gfx_draw_text_box(0x30, (uint8_t)g.DAT_f7d1, TEXT_BOX_FULL, TEXT_BOX_STATIC);
  } else if (z == 3) {
    gfx_draw_present_icon(0x20, 0x04);
    gfx_draw_text_box(0x30, TEXT_HERES_A_GIFT, TEXT_BOX_FULL, TEXT_BOX_STATIC);
  } else if (z == 4) {
    gfx_draw_present_icon(0x20, 0x04);
    if (g.dowsing_item_pos != 0) {
      gfx_draw_value_with_icon(0x02, 0x20, 0x0D, (uint16_t)g.dowsing_item_pos);
    } else {
      gfx_draw_item_name(0x00, 0x20, (uint8_t)g.accelXPos, 0x0D);
    }
    gfx_draw_text_box(0x30, TEXT_RECEIVED, TEXT_BOX_NO_LINES, TEXT_BOX_STATIC);
  }

  g.gCurSubstateA++;
  if (g.gCurSubstateA >= 8) {
    g.gCurSubstateZ++;
    g.gCurSubstateA = 0;
    if (g.gCurSubstateZ == 2) {
      drv_sound_play(SND_GIFT);
    } else if (g.gCurSubstateZ == 4) {
      drv_sound_play(SND_ANIM_CUE);
    }
  }

  if (g.gCurSubstateZ >= 5) {
    ui_reset_substate();
    ui_set_view(VIEW_HOME);
  }
  gfx_draw_battery_low(0, 0);
}

// ROM: 0x6784  89.0%  saves: er3,er4,er5,er6
uint8_t game_find_seen_peer(void *trainer_ptr) {
  uint8_t slot;
  uint8_t is_match;
  uint8_t j;
  uint16_t scan_addr = 0xDE24;
  uint8_t *buf = eepromPageScratch;
  uint8_t *target = (uint8_t *)trainer_ptr;

  /* Walk 10 peer-log slots (each 0x224 bytes apart in EEPROM), comparing the
     0x28-byte identity block at offset +8 against the target trainer record.
     Returns 1 on first match, 0 if none found. */
  for (slot = 0; slot < 10; slot++) {
    is_match = 1;
    drv_eeprom_read_block(scan_addr + 8, buf, 0x28);
    for (j = 0; j < 0x28; j++) {
      if (buf[j] != target[j]) {
        is_match = 0;
      }
    }
    if (is_match == 1) {
      return 1;
    }
    scan_addr += 0x224;
  }
  return 0;
}
