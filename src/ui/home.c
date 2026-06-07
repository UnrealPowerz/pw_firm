#include "all_headers.h"

/*
 * Home screen (VIEW_HOME).
 *
 *   ui_handle_home          input — three buttons each shortcut into a
 *                           specific main-menu entry; otherwise tick the
 *                           reward-animation timer and bobbing pokemon
 *   ui_render_route_image   the route background art (top-left), differs
 *                           between solo and co-op modes
 *   ui_render_home_route    main renderer — route + bobbing walker pokemon
 *   ui_render_home_bar      bottom status row — flag/event icons + step counter
 */

// ROM: 0x6a3e  93.4%
void ui_handle_home(void) {
  if (!(g.sys_walkerFlags.BIT.session_active)) {
    if (drv_button_is_triggered(BTN_M) || (g.sys_statusFlags.BIT.pedometer_paused)) {
      ui_enter_ir_session();
    }
  } else {
    if (g.viewstate.Z != 0) {
      if (drv_button_is_triggered(BTN_ANY)) {
        g.viewstate.Z = 0;
        game_process_interaction_reward(g.viewstate.Y.BYTE);
        return;
      } else {
        g.viewstate.Z--;
      }
    }
    /* Home-screen button shortcuts: each jumps straight into a main-menu
       entry, bypassing the cursor. */
    if (drv_button_is_triggered(BTN_M)) {
      drv_sound_play(SND_CONFIRM);
      ui_clear_substate_y();
      g.ui_menuCursor = MENU_CONNECTION;
      ui_set_view(VIEW_MAIN_MENU);
    } else if (drv_button_is_triggered(BTN_R)) {
      g.ui_menuCursor = MENU_SETTINGS;
      drv_sound_play(SND_CONFIRM);
      ui_clear_substate_y();
      ui_set_view(VIEW_MAIN_MENU);
    } else if (drv_button_is_triggered(BTN_L)) {
      g.ui_menuCursor = MENU_POKERADAR;
      drv_sound_play(SND_CONFIRM);
      ui_clear_substate_y();
      ui_set_view(VIEW_MAIN_MENU);
    }
  }
}

// ROM: 0x6b10  80.9%
void ui_render_route_image(void) {
  uint8_t *ptr;
  uint16_t addr;

  sys_init_heap();
  ptr = sbrk(0xC0);
  if ((g.save_settings.BYTE & 1)) {
    addr = 0xC83C;
  } else {
    addr = 0x8FBE;
  }
  drv_eeprom_read_block(addr, ptr, 0xC0);
  drv_lcd_blit(0, 0x18, ptr, 0x20, 0x18);
}

// ROM: 0x6bf8  75.7%
void ui_render_home_route(void) {
  uint8_t *buf;
  uint8_t subA;

  if (g.viewstate.Z != 0) {
    uint8_t idx;
    idx = ROUTE_ICON_INDICES[g.viewstate.Y.BYTE - 1];
    gfx_draw_small_route_icon(idx);
  }
  ui_render_route_image();
  if (!(g.sys_walkerFlags.BIT.walking)) {
    return;
  }
  subA = g.viewstate.A;
  if (!g.viewstate.v.homeStandby.flags.BIT.b1) {
    gfx_draw_home_pokemon(subA, 0);
  } else if (!g.viewstate.v.homeStandby.flags.BIT.b2) {
    gfx_draw_own_pokemon_small(subA, 0x18);
  } else {
    sys_init_heap();
    buf = sbrk(0x10);
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, buf, 0x10);
    if (!((byte_bits_t *)&buf[0x0E])->BIT.b0) {
      gfx_draw_own_pokemon_small_flipped(subA, 0x18);
    } else {
      gfx_draw_own_pokemon_small(subA, 0x18);
    }
  }
  if (g.viewstate.v.homeStandby.flags.BIT.b1) {
    sys_update_standby_state();
  } else {
    sys_enter_standby();
  }
}

// ROM: 0x74bc  86.9%
void ui_render_home_bar(void) {
  uint8_t *buf;
  uint8_t flags;
  int i;
  uint8_t *itemArea;  gfx_draw_home_section_divider();
  flags = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);

  sys_init_heap();
  buf = sbrk(0x180);

  if (flags & 0x20) {
    /* "Event pokemon held" indicator (flag bit 0x20 = walker contains event poke). */
    drv_eeprom_read_block(SPR_OFF(pokeball_event), buf, SPR_SIZE(pokeball_event));
    for (i = 0; i < 0x10; i++) {
      buf[i] |= 0x01;
    }
    drv_lcd_blit(0, 0x30, buf, 8, 8);
  }

  if (flags & 0x40) {
    uint16_t hasRoute;
    drv_eeprom_read_block(EEPROM_WILD_POKE, buf, 8);
    hasRoute = *(uint16_t *)(buf + 6);
    if (hasRoute != 0) {
      /* "Event item held" indicator (flag bit 0x40 = walker contains event item). */
      drv_eeprom_read_block(SPR_OFF(item_symbol_event), buf, SPR_SIZE(item_symbol_event));
      for (i = 0; i < 0x10; i++) {
        buf[i] |= 0x01;
      }
      drv_lcd_blit(8, 0x30, buf, 8, 8);
    }
  }

  /* 4 stamp-suit glyphs (heart/spade/diamond/club, 8x8 each, total 0x40 bytes).
   * Each suit is one of the four "stamp received" indicators (flag bits 0x01-0x08). */
  drv_eeprom_read_block(SPR_OFF(card_faces), buf, SPR_SIZE(card_faces));
  for (i = 0; i < 0x40; i++) {
    buf[i] |= 0x01;
  }
  if (flags & 0x01) {
    drv_lcd_blit(0x10, 0x30, buf, 8, 8);
  }

  itemArea = buf + 0x10;

  if (flags & 0x02) {
    drv_lcd_blit(0x18, 0x30, itemArea, 8, 8);
  }

  if (flags & 0x04) {
    drv_lcd_blit(0x20, 0x30, buf + 0x20, 8, 8);
  }

  if (flags & 0x08) {
    drv_lcd_blit(0x28, 0x30, buf + 0x30, 8, 8);
  }

  /* Pokeball glyph — bottom row pokemon-slot indicators. */
  drv_eeprom_read_block(SPR_OFF(pokeball), buf, SPR_SIZE(pokeball));
  drv_eeprom_read_block(EEPROM_LOG_CONTEXT, itemArea, 0x30);

  for (i = 0; i < 3; i++) {
    uint16_t entry;
    entry = *(uint16_t *)(itemArea + i * 0x10);
    if (entry != 0) {
      uint8_t xpos;
      xpos = (uint8_t)(i * 8);
      drv_lcd_blit(xpos, 0x38, buf, 8, 8);
    }
  }

  /* Item-symbol glyph — bottom row item-slot indicators. */
  drv_eeprom_read_block(SPR_OFF(item_symbol), buf, SPR_SIZE(item_symbol));
  drv_eeprom_read_block(EEPROM_LOG_ITEMS, itemArea, 0x0C);

  for (i = 0; i < 3; i++) {
    uint16_t entry;
    entry = *(uint16_t *)(itemArea + i * 4);
    if (entry != 0) {
      uint8_t xpos;
      xpos = (uint8_t)(i * 8) + 0x18;
      drv_lcd_blit(xpos, 0x38, buf, 8, 8);
    }
  }

  if (flags & 0x10) {
    /* Tiny map icon — "special map received" indicator. */
    drv_eeprom_read_block(SPR_OFF(map_icon_tiny), buf, SPR_SIZE(map_icon_tiny));
    drv_lcd_blit(0x30, 0x38, buf, 8, 8);
  }

  gfx_draw_numeric_value(0x58, 0x30, g.session_steps, 1);

  gfx_draw_battery_low(0, 0);
}
