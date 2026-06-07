#include "all_headers.h"

/*
 * Main menu (VIEW_MAIN_MENU) — the 6-entry scrolling menu reached from home.
 *
 * Cursor is `g.ui_menuCursor` (0..5, see enum main_menu_item in
 * include/menu_consts.h). g.viewstate.Y.BYTE tracks any overlay popup (see
 * enum main_menu_popup below) that suppresses the normal display until
 * a button is pressed.
 *
 * Also here: ui_enter_ir_session — the IR-app launcher invoked from
 * MENU_CONNECTION and from the home-screen R-button shortcut.
 */

/* Main-menu overlay popups (g.viewstate.Y.BYTE in ui_handle_main_menu /
   ui_render_main_menu). NONE = normal cursor/g.save_watts display. */
enum main_menu_popup {
    MENU_POPUP_NONE         = 0,
    MENU_POPUP_NEED_WATTS   = 1,
    MENU_POPUP_NO_POKEMON   = 2,
    MENU_POPUP_NOTHING_HELD = 3
};

// ROM: 0x694c  40.0%
void ui_enter_ir_session(void) {
  sys_begin_ir_session(g.sys_tickHandler, sys_main_loop_low_power);
}

// ROM: 0x9756  74.7%
void ui_handle_main_menu(void) {
  uint16_t cost;

  /* A popup is active — any button dismisses it back to the normal menu. */
  if (g.viewstate.Y.BYTE != MENU_POPUP_NONE) {
    if (drv_button_is_triggered(BTN_ANY)) {
      g.viewstate.Y.BYTE = MENU_POPUP_NONE;
      drv_sound_play(SND_CURSOR);
    }
    return;
  }

  if (drv_button_is_triggered(BTN_M)) {
    uint16_t watts = g.save_watts;
    const uint8_t *costTable = MENU_ITEM_COSTS;
    cost = costTable[g.ui_menuCursor];
    if (watts < cost) {
      g.viewstate.Y.BYTE = MENU_POPUP_NEED_WATTS;
      drv_sound_play(SND_CURSOR);
      return;
    }

    switch (g.ui_menuCursor) {
      case MENU_POKERADAR:
        if (!(g.sys_walkerFlags.BIT.walking)) {
          g.viewstate.Y.BYTE = MENU_POPUP_NO_POKEMON;
          drv_sound_play(SND_CURSOR);
          return;
        }
        cost = costTable[g.ui_menuCursor];
        if (g.save_watts < cost) {
          g.save_watts = 0;
        } else {
          g.save_watts -= cost;
        }
        save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (uint8_t *)&g.save_totalSteps, 0x18);
        ui_set_view(VIEW_POKERADAR);
        game_pokeradar_init();
        return;
      case MENU_DOWSING:
        cost = costTable[g.ui_menuCursor];
        if (g.save_watts < cost) {
          g.save_watts = 0;
        } else {
          g.save_watts -= cost;
        }
        save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (uint8_t *)&g.save_totalSteps, 0x18);
        game_init_dowsing();
        ui_set_view(VIEW_DOWSING);
        return;
      case MENU_CONNECTION:
        ui_enter_ir_session();
        return;
      case MENU_TRAINER_CARD:
        ui_set_view(VIEW_TRAINER_CARD);
        ui_reset_trainer_card_state();
        return;
      case MENU_INVENTORY: {
        uint16_t mask[2];
        ui_load_inventory_mask(mask);
        if (mask[0] != 0) {
          *(volatile uint16_t *)&g.viewstate.A = mask[0];
          accel_xPosition_word = mask[1];
          ui_inventory_cursor_reset();
          ui_set_view(VIEW_POKE_ITEMS);
          return;
        }
        if (mask[1] != 0) {
          *(volatile uint16_t *)&g.viewstate.A = mask[0];
          accel_xPosition_word = mask[1];
          ui_inventory_jump_to_items();
          ui_set_view(VIEW_GIFTS);
          return;
        }
        /* Nothing in either pokemon or item slots — show the empty popup. */
        g.viewstate.Y.BYTE = MENU_POPUP_NOTHING_HELD;
        drv_sound_play(SND_CURSOR);
        return;
      }
      case MENU_SETTINGS:
        ui_reset_settings_substate();
        ui_set_view(VIEW_SETTINGS);
        return;
    }
  }

  /* Cursor scroll (with wrap). M past MENU_POKERADAR and L past MENU_SETTINGS
     exit back to home. */
  if (drv_button_is_triggered(BTN_R)) {
    if (g.ui_menuCursor == MENU_POKERADAR) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
      drv_sound_play(SND_BACK);
      return;
    }
    g.ui_menuCursor = (uint8_t)((g.ui_menuCursor + 5) % 6);
    drv_sound_play(SND_CURSOR);
  }

  if (drv_button_is_triggered(BTN_L)) {
    if (g.ui_menuCursor == MENU_SETTINGS) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
      drv_sound_play(SND_BACK);
    } else {
      g.ui_menuCursor = (uint8_t)((g.ui_menuCursor + 1) % 6);
    }
    drv_sound_play(SND_CURSOR);
  }
}

// Reason: Two compounding blockers:
//   (1) ROM uses `mov.l #H'400280, er4` to pack constants 0x40 (e4) and 0x280
//       (r4) into a single 32-bit immediate, then computes per-call addresses
//       via `add.w r4, ...` — ch38 cannot be coaxed into this ER-pair packing.
//   (2) gfx_blit_to_buffer's C signature (x, y, w, h, src, dst, dst_w) does not
//       match what ROM passes (two stack pushes per call vs one expected; r0h
//       and r1h are loaded with values that don't fit the documented sig).
//       The C body itself appears to compute the destination offset using
//       different inputs than ROM does, so the wrapper isn't semantically
//       correct either. Untangling this requires re-deriving the real
//       signature of gfx_blit_to_buffer first.
//   Fixed in this pass: drv_lcd_blit arg order at the two affected call sites
//   (the C had them in the wrong order, mirroring the broken function-pointer
//   pattern from the trainer_card.c sister functions).
// Class: cannot-fix-without-compiler-change (ER-register constant packing)
//   + needs gfx_blit_to_buffer signature investigation
// ROM: 0x9930  69.1%
#pragma option noregexpansion /* pragma:auto */
void ui_render_main_menu(void) {
  uint8_t *sprite_buf, *e0_buf;
  uint16_t i;
  sys_init_heap();
  sprite_buf = (uint8_t *)sbrk(0x140);
  e0_buf = (uint8_t *)sbrk(0x80);

  /* Current selection: 80x16 menu heading bar for the selected row. */
  drv_eeprom_read_block((uint16_t)g.ui_menuCursor * 0x140 + SPR_OFF(menu_hdg_pokeradar),
                        sprite_buf, 0x140);
  drv_lcd_blit(8, 0, sprite_buf, 0x50, 0x10);

  for (i = 0; i < 6; i++) {
    uint16_t j;
    for (j = 0; j < 0x80; j++) {
      e0_buf[j] = 0;
    }

    if ((uint8_t)i == g.ui_menuCursor) {
      /* Animated right-pointing arrow next to the selected row.
       * EEP_ARROWS_8x8 sheet: arrows in 3 configs each (normal/offset/inverted);
       * +3 indexes into the right-arrow set, +(tick & 1) toggles between the
       * two animation frames. */
      uint16_t cursor_addr = (uint16_t)((g.ui_animationTick & 1) + 3) * 0x10 + SPR_OFF(arrows_8x8);
      drv_eeprom_read_block(cursor_addr, sprite_buf, 0x10);
      gfx_blit_to_buffer(8, 8, 4, (uint8_t)(MAIN_MENU_Y_COORDS[i] - 8),
                         sprite_buf, e0_buf, 0x10);
    }

    /* Per-row 16x16 menu icon (poke-radar, dowsing, connect, ...). */
    drv_eeprom_read_block(SPR_OFF(menu_icons[i]), sprite_buf, SPR_SIZE(menu_icons[i]));
    gfx_blit_to_buffer(0x10, 0x10, 0, MAIN_MENU_Y_COORDS[i],
                       sprite_buf, e0_buf, 0x10);

    drv_lcd_blit((uint8_t)(i * 0x10), 0x10, e0_buf, 0x10, 0x20);
  }

  switch (g.viewstate.Y.BYTE) {
  case MENU_POPUP_NONE:
    gfx_draw_numeric_value(0x48, 0x30, g.save_watts, 0);
    /* Cost number: dowsing costs 10, radar costs 3. */
    if (g.ui_menuCursor == MENU_POKERADAR) {
      gfx_draw_numeric_value(0x08, 0x30, 10, 0);
    } else if (g.ui_menuCursor == MENU_DOWSING) {
      gfx_draw_numeric_value(0x08, 0x30, 3, 0);
    }

    /* "W" icon next to the current g.save_watts value (right side) — always drawn. */
    drv_eeprom_read_block(SPR_OFF(watt_symbol), sprite_buf, SPR_SIZE(watt_symbol));
    drv_lcd_blit(0x50, 0x30, sprite_buf, 0x10, 0x10);

    /* Cost row (left "W" + arrow) only for the two minigames with a cost. */
    if (g.ui_menuCursor < MENU_CONNECTION) {
      drv_lcd_blit(0x18, 0x30, sprite_buf, 0x10, 0x10);

      /* 0x180 offset = tail of the digit sheet, used as a small arrow glyph.
       * Reads 0x20 bytes which spans into the WATT-symbol bytes. */
      drv_eeprom_read_block(SPR_OFF(digits) + 0x180, sprite_buf, 0x20);
      drv_lcd_blit(0x28, 0x30, sprite_buf, 8, 0x10);
    }
    break;
  case MENU_POPUP_NEED_WATTS:
    gfx_draw_text_box(0x30, TEXT_NEED_MORE_WATTS, TEXT_BOX_FULL, TEXT_BOX_BLINK);
    break;
  case MENU_POPUP_NO_POKEMON:
    gfx_draw_text_box(0x30, TEXT_NO_POKEMON_HELD, TEXT_BOX_FULL, TEXT_BOX_BLINK);
    break;
  case MENU_POPUP_NOTHING_HELD:
    gfx_draw_text_box(0x30, TEXT_NOTHING_HELD, TEXT_BOX_FULL, TEXT_BOX_BLINK);
    break;
  }

  /* Left/right menu-edge chevrons — reads the left+right arrow pair. */
  drv_eeprom_read_block(SPR_OFF(menu_arrow_left), sprite_buf,
                        SPR_SIZE(menu_arrow_left) + SPR_SIZE(menu_arrow_right));
  drv_lcd_blit(0, 0, sprite_buf, 8, 0x10);

  drv_lcd_blit(0x58, 0, sprite_buf + 0x20, 8, 0x10);
  gfx_draw_battery_low(0x58, 0);
}
