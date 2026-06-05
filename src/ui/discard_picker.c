#include "all_headers.h"

/*
 * Discard picker (VIEW_DISCARD_PICKER).
 *
 * Shown only when the player has just earned a new reward but their storage
 * is full — all 3 caught-pokemon slots (from a battle win) or all 3
 * dowsed-item slots (from dowsing) are already occupied. The screen lists
 * the 3 existing entries; pressing R replaces the highlighted one with the
 * new reward and logs the interaction via game/social.
 *
 * Entry points (both call ui_init_discard_cursor right before activating
 * the view, which preps g.viewstate.Z = 1 — the cursor starts on slot 1):
 *   game_battle_handle_finish      -> when save_find_empty_poke_slot >= 3
 *   ui_handle_dowsing (state_found) -> when accel_zPosition_byte == 3
 *
 * Normal-case rewards (storage not full) never reach this view; they slot
 * silently into the first empty position.
 *
 * Cursor state:
 *   g.viewstate.Z = 0..2   index of the existing slot to discard
 *   g.viewstate.A = 0      replacing a caught pokemon
 *                  != 0    replacing a dowsed item
 *
 * ROM cluster: 0x3B94..0x3DBC, immediately after game_log_item_interaction.
 * ui_init_discard_cursor lives a bit earlier at 0x3A68.
 */

// ROM: 0x3a68  100.0%
void ui_init_discard_cursor(void) {
  g.viewstate.Z = 1;
  (void)0;
}

// ROM: 0x3b94  94.6%
void ui_handle_discard_picker(void) {
  if (drv_button_is_triggered(BTN_M)) {
    {
      uint8_t cursor = g.viewstate.Z;
      if (cursor == 0) {
        ui_reset_substate();
        ui_set_view(VIEW_HOME);
        drv_sound_play(SND_BACK);
        return;
      }
      g.viewstate.Z = cursor - 1;
    }
    drv_sound_play(SND_CURSOR);
  }

  if (drv_button_is_triggered(BTN_L)) {
    {
      uint8_t cursor = g.viewstate.Z;
      if (cursor == 2) {
        /* Already on the last reward slot. */
        drv_sound_play(SND_BACK);
        return;
      }
      g.viewstate.Z = cursor + 1;
    }
    drv_sound_play(SND_CURSOR);
  }

  if (drv_button_is_triggered(BTN_R)) {
    /* Commit the reward — branch on g.viewstate.A (set by the caller to
       distinguish a pokemon caught vs an item dowsed). */
    if (g.viewstate.A == 0) {
      game_log_poke_interaction();
    } else {
      game_log_item_interaction();
    }
    ui_reset_substate();
    ui_set_view(VIEW_HOME);
    drv_sound_play(SND_CONFIRM);
  }
}

// ROM: 0x3c0a  81.0%
void ui_render_discard_poke_slot(void) {
  uint16_t *buf;
  uint8_t i;
  uint16_t routes_addr = 0x8f52;
  uint16_t routes_len = 0x30;

  sys_init_heap();
  buf = sbrk(0x40);
  /* Routes table (3 entries x 0x10 bytes) followed by the just-caught
     record at +0x30. */
  drv_eeprom_read_block(routes_addr, buf, routes_len);
  drv_eeprom_read_block(EEPROM_LOG_CONTEXT + ((int8_t)g.viewstate.Z * 0x10),
                        (uint8_t *)buf + 0x30, 0x10);

  /* Find the matching route slot and draw its name. */
  for (i = 0; i < 3; i++) {
    if (*(uint16_t *)((uint8_t *)buf + 0x30) == buf[i * 8]) {
      gfx_draw_route_pokemon_name(0, 0x30, i, 0x07);
      break;
    }
  }
}

// ROM: 0x3c76  79.6%
void ui_render_discard_item_slot(void) {
  uint32_t caught_item;
  uint16_t *lookup;
  uint8_t i;

  sys_init_heap();
  lookup = sbrk(0x14);
  drv_eeprom_read_block(EEPROM_LOG_ITEMS + ((int8_t)g.viewstate.Z * 4),
                        &caught_item, 4);
  drv_eeprom_read_block(EEPROM_SUBY_LOOKUP_TABLE, lookup, 0x14);

  for (i = 0; i < 10; i++) {
    if (*(uint16_t *)&caught_item == lookup[i]) {
      gfx_draw_item_name(0, 0x30, i, 0x0F);
      break;
    }
  }
}

// ROM: 0x3cd8  80.8%
void ui_render_discard_picker(void) {
  void *buf;
  volatile uint16_t base = 0x280;
  /* Function-pointer alias — see same pattern in ui_render_battle. */
  void (*blit)(uint8_t, uint8_t, void *, uint8_t, uint8_t) =
      (void (*)(uint8_t, uint8_t, void *, uint8_t, uint8_t))drv_lcd_blit;

  sys_init_heap();
  buf = sbrk(0x180);

  /* Left gutter chevron. */
  drv_eeprom_read_block(base + 0x378, buf, 0x20);
  blit(0, 0, buf, 8, 0x10);

  /* "Caught!" header. */
  drv_eeprom_read_block(base + 0x88b0, buf, 0x140);
  blit(8, 0, buf, 0x50, 0x10);

  /* Cursor sprite (2-frame blink) over the currently-selected reward slot. */
  drv_eeprom_read_block(base + 0x2a8, buf, 0x20);
  blit((uint8_t)(0x18 + (g.viewstate.Z * 0x14)), 0x18,
       (uint8_t *)buf + ((g.ui_animationTick & 1) * 0x10), 8, 8);

  /* Three reward-slot icons. Use the pokemon icon set (A==0) or the
     item icon set (A!=0). */
  if (g.viewstate.A == 0) {
    drv_eeprom_read_block(base + 0x1e0, buf, 0x10);
  } else {
    drv_eeprom_read_block(base + 0x208, buf, 0x10);
  }

  blit(0x18, 0x20, buf, 8, 8);
  blit(0x2C, 0x20, buf, 8, 8);
  blit(0x40, 0x20, buf, 8, 8);

  /* Detail panel — pokemon name or item name for the selected slot. */
  if (g.viewstate.Z <= 2) {
    if (g.viewstate.A == 0) {
      ui_render_discard_poke_slot();
    } else {
      ui_render_discard_item_slot();
    }
    gfx_draw_battery_low(0, 0);
  }
}
