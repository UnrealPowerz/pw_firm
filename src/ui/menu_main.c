#include "all_headers.h"

// ROM: 0x694c  40.0%
void ui_start_connection_app(void) {
  sys_init_io_ports(currentEventLoopFunc, sys_main_loop_low_power);
}

// ROM: 0x6a3e  93.4%
void ui_handle_home(void) {
  if (!(walker_status_flags_BIT.session_active)) {
    if (drv_button_is_triggered(2) || (statusFlags_BIT.pedometer_paused)) {
      ui_start_connection_app();
    }
  } else {
    if (gCurSubstateZ != 0) {
      if (drv_button_is_triggered(0x0E)) {
        gCurSubstateZ = 0;
        game_process_interaction_reward(gCurSubstateY);
        return;
      } else {
        gCurSubstateZ--;
      }
    }
    if (drv_button_is_triggered(2)) {
      drv_sound_play(0);
      ui_clear_substate_y();
      menu_cursor = 2;
      ui_set_view(VIEW_MAIN_MENU);
    } else if (drv_button_is_triggered(4)) {
      menu_cursor = 5;
      drv_sound_play(0);
      ui_clear_substate_y();
      ui_set_view(VIEW_MAIN_MENU);
    } else if (drv_button_is_triggered(8)) {
      menu_cursor = 0;
      drv_sound_play(0);
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
  if ((RamCache_settingsByte & 1)) {
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

  if (gCurSubstateZ != 0) {
    uint8_t idx;
    idx = routeIconIndices[gCurSubstateY - 1];
    gfx_draw_small_route_icon(idx);
  }
  ui_render_route_image();
  if (!(walker_status_flags_BIT.walking)) {
    return;
  }
  subA = gCurSubstateA;
  if (!DAT_f7d1_BIT.b1) {
    gfx_draw_home_pokemon(subA, 0);
  } else if (!DAT_f7d1_BIT.b2) {
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
  if (DAT_f7d1_BIT.b1) {
    sys_update_standby_state();
  } else {
    sys_enter_standby();
  }
}

// ROM: 0x6c88  100.0%
void ui_show_status_screen(void) {
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
}

// ROM: 0x6c94  92.5%
void ui_handle_settings_menu(void) {
  if (drv_button_is_triggered(4)) {
    if (gCurSubstateZ == 0) {
      drv_sound_play(1);
      ui_clear_substate_y();
      ui_set_view(VIEW_MAIN_MENU);
      return;
    } else {
      gCurSubstateZ = 0;
      drv_sound_play(2);
    }
  }
  if (drv_button_is_triggered(8)) {
    if (gCurSubstateZ != 1) {
      gCurSubstateZ = 1;
      drv_sound_play(2);
    }
  }
}

// ROM: 0x6ce2  65.3%
void ui_handle_volume_menu(void) {
  uint8_t vol;
  uint16_t val;

  if (drv_button_is_triggered(4)) {
    vol = (RamCache_settingsByte >> 1) & 0x03;
    val = (uint16_t)((int16_t)(int8_t)vol + 2);
    val = (uint16_t)((int16_t)val % 3);
    RamCache_settingsByte = (uint8_t)((RamCache_settingsByte & ~(0x03 << 1)) |
                                      (((uint8_t)val & 0x03) << 1));
    vol = (RamCache_settingsByte >> 1) & 0x03;
    drv_sound_set_volume(vol);
    drv_sound_play(2);
  }
  if (drv_button_is_triggered(8)) {
    vol = (RamCache_settingsByte >> 1) & 0x03;
    val = (uint16_t)((int16_t)(int8_t)vol + 1);
    val = (uint16_t)((int16_t)val % 3);
    RamCache_settingsByte = (uint8_t)((RamCache_settingsByte & ~(0x03 << 1)) |
                                      (((uint8_t)val & 0x03) << 1));
    vol = (RamCache_settingsByte >> 1) & 0x03;
    drv_sound_set_volume(vol);
    drv_sound_play(2);
  }
}

// ROM: 0x6d6c  70.6%
void ui_handle_shade_menu(void) {
  uint8_t shade;

  if (drv_button_is_triggered(4)) {
    if ((RamCache_settingsByte & 0x78) != 0) {
      shade = ((RamCache_settingsByte >> 3) & 0x0F) - 1;
      RamCache_settingsByte = (uint8_t)((RamCache_settingsByte & ~(0x0F << 3)) |
                                        ((shade & 0x0F) << 3));
      drv_sound_play(2);
    }
    shade = (RamCache_settingsByte >> 3) & 0x0F;
    drv_lcd_set_contrast(shade);
  }
  if (drv_button_is_triggered(8)) {
    if ((RamCache_settingsByte & 0x78) < 0x48) {
      shade = ((RamCache_settingsByte >> 3) & 0x0F) + 1;
      RamCache_settingsByte = (uint8_t)((RamCache_settingsByte & ~(0x0F << 3)) |
                                        ((shade & 0x0F) << 3));
      drv_sound_play(2);
    }
    shade = (RamCache_settingsByte >> 3) & 0x0F;
    drv_lcd_set_contrast(shade);
  }
}

// ROM: 0x6dfc  78.8%
void ui_handle_settings(void) {
  if (gCurSubstateY == 0) {
    ui_handle_settings_menu();
  } else if (gCurSubstateY == 1) {
    ui_handle_volume_menu();
  } else if (gCurSubstateY == 2) {
    ui_handle_shade_menu();
  }
  if (drv_button_is_triggered(2)) {
    if (gCurSubstateY == 0) {
      drv_sound_play(0);
      gCurSubstateY = gCurSubstateZ + 1;
    } else {
      drv_sound_play(0);
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
    }
  }
}

// ROM: 0x6e62  74.3%
#pragma option noregexpansion /* pragma:auto */
void ui_render_settings(void) {
  uint8_t *buf;
  uint8_t tmp;
  int i;
  uint8_t volVal;
  uint8_t shVal;
  uint8_t shadeOff;
  uint16_t animOff;

  sys_init_heap();
  buf = sbrk(0x140);

  drv_eeprom_read_block(0xF50, buf, 0x140);
  drv_lcd_blit(8, 0, buf, 0x50, 0x10);

  drv_eeprom_read_block(0x1690, buf, 0xA0);
  drv_lcd_blit(8, 0x10, buf, 0x28, 0x10);

  drv_eeprom_read_block(0x1730, buf, 0xA0);
  drv_lcd_blit(0x38, 0x10, buf, 0x28, 0x10);

  drv_eeprom_read_block(0x4F8, buf, 0xC0);

  tmp = gCurSubstateZ * 0x30;

  if (gCurSubstateY == 0) {
    animOff = ((animTick & 0x01) + 9) * 0x10;
    drv_lcd_blit(tmp, 0x14, buf + animOff, 8, 8);
  } else if (gCurSubstateY == 1) {
    drv_lcd_blit(tmp, 0x14, buf + 0xB0, 8, 8);

    volVal = (RamCache_settingsByte >> 1) & 0x03;
    animOff = ((animTick & 0x01) + 9) * 0x10;
    drv_lcd_blit((uint8_t)(volVal * 0x20), 0x2C, buf + animOff, 8, 8);

    drv_eeprom_read_block(0x17D0, buf, 0x120);
    drv_lcd_blit(0x08, 0x28, buf, 0x18, 0x10);
    drv_lcd_blit(0x28, 0x28, buf + 0x60, 0x18, 0x10);
    drv_lcd_blit(0x48, 0x28, buf + 0xC0, 0x18, 0x10);
  } else if (gCurSubstateY == 2) {
    shVal = (RamCache_settingsByte >> 3) & 0x0F;
    shadeOff = shVal * 8 + 8;
    animOff = ((animTick & 0x01) + 3) * 0x10;

    drv_lcd_blit((uint8_t)shadeOff, 0x20, buf + animOff, 8, 8);
    drv_lcd_blit((uint8_t)(gCurSubstateZ * 0x30), 0x14, buf + 0xB0, 8, 8);

    drv_eeprom_read_block(0x18F0, buf, 0x20);
    for (i = 0; i < 0x0A; i++) {
      uint8_t xpos;
      xpos = (uint8_t)(i * 8) + 8;
      drv_lcd_blit(xpos, 0x28, buf, 8, 0x10);
    }
  }

  drv_eeprom_read_block(0x5F8, buf, 0x20);
  drv_lcd_blit(0, 0, buf, 8, 0x10);
  gfx_draw_battery_low(0x58, 0);
}

// ROM: 0x703c  70.4%
void ui_render_empty_eeprom(void) {
  uint8_t *buf;
  uint16_t i;

  sys_init_heap();
  buf = sbrk(0x100);

  for (i = 0; i < 0x100; i++) {
    buf[i] = IMG_POKEWALKER_LARGE[i];
  }

  for (i = 0; i < 0x20; i++) {
    uint8_t pix;
    pix = walkerFaceNeutral[i];
    buf[0x50 + i] |= (pix * 8);
    buf[0x50 + i + 0x40] |= (pix / 0x20);
  }

  {
    uint8_t frame;
    frame = (animTick >> 2) & 1;
    if (frame) {
      for (i = 0; i < 0x10; i++) {
        uint8_t pix;
        pix = walkerEmptyExtraGlyph[i];
        buf[0xD8 + i] |= (pix * 0x10);
      }
      drv_lcd_blit(0x20, 0x10, buf, 0x20, 0x20);

      for (i = 0; i < 0x10; i++) {
        buf[i] = walkerEmptyExtraGlyph[i] / 0x10;
      }
      drv_lcd_blit(0x2C, 0x30, buf, 8, 8);
    } else {
      drv_lcd_blit(0x20, 0x10, buf, 0x20, 0x20);
    }
  }
}

// ROM: 0x711a  24.0%
void ui_render_sad_walker(void) {
  uint8_t *buf;
  uint8_t *dst;
  uint16_t i;

  sys_init_heap();
  buf = sbrk(0x100);

  for (i = 0; i < 0x100; i++) {
    buf[i] = IMG_POKEWALKER_LARGE[i];
  }

  dst = buf + 0x50;
  for (i = 0; i < 0x20; i++) {
    uint8_t pix;
    pix = walkerFaceSad[i];
    dst[i] |= (pix * 8);
    dst[i + 0x40] |= (pix / 0x20);
  }

  drv_lcd_blit(0x20, 0x10, buf, 0x20, 0x20);

  {
    uint8_t tmp = gCurSubstateZ + 1;
    gCurSubstateZ = tmp;
    if (tmp > 0x08) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
    }
  }
}

// ROM: 0x71a4  72.2%
void ui_render_happy_walker(uint8_t show_ir) {
  uint8_t *buf;
  uint8_t *dst;
  uint16_t i;

  sys_init_heap();
  buf = sbrk(0x100);

  if (show_ir) {
    drv_lcd_blit(0x2C, 0, (void *)IMG_POKEWALKER_IR_ACTIVE, 8, 8);
  }

  for (i = 0; i < 0x100; i++) {
    buf[i] = IMG_POKEWALKER_LARGE[i];
  }

  dst = buf + 0x50;
  for (i = 0; i < 0x20; i++) {
    uint8_t pix;
    pix = walkerFaceHappy[i];
    dst[i] |= (pix * 8);
    dst[i + 0x40] |= (pix / 0x20);
  }

  drv_lcd_blit(0x20, 0x10, buf, 0x20, 0x20);
}

// ROM: 0x722c  75.4%
void ui_draw_ir_icon(uint8_t show_ir) {
  uint8_t *buf;

  sys_init_heap();
  buf = sbrk(0x180);

  if (show_ir) {
    drv_eeprom_read_block(0x2450, buf, 0x20);
    drv_lcd_blit(0x2C, 0, buf, 8, 0x10);
  }

  drv_eeprom_read_block(0x2350, buf, 0x100);
  drv_lcd_blit(0x20, 0x10, buf, 0x20, 0x20);
  gfx_draw_text_box(0x30, 0, 0x0F, 0x00);
}

// ROM: 0x74bc  86.9%
void ui_render_home_bar(void) {
  uint8_t *buf;
  uint8_t flags;
  volatile uint16_t base;
  int i;
  uint8_t *itemArea;

  base = 0x280;
  diag_lcd_ssu_test_3();
  flags = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);

  sys_init_heap();
  buf = sbrk(0x180);

  if (flags & 0x20) {
    drv_eeprom_read_block(0x1F0 + base, buf, 0x10);
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
      drv_eeprom_read_block(0x218 + base, buf, 0x10);
      for (i = 0; i < 0x10; i++) {
        buf[i] |= 0x01;
      }
      drv_lcd_blit(8, 0x30, buf, 8, 8);
    }
  }

  drv_eeprom_read_block(0x238 + base, buf, 0x40);
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

  drv_eeprom_read_block(0x1E0 + base, buf, 0x10);
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

  drv_eeprom_read_block(0x208 + base, buf, 0x10);
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
    drv_eeprom_read_block(0x228 + base, buf, 0x10);
    drv_lcd_blit(0x30, 0x38, buf, 8, 8);
  }

  {
    uint8_t fmt;
    fmt = 1;
    gfx_draw_numeric_value(0x58, 0x30, sessionSteps, fmt);
  }

  gfx_draw_battery_low(0, 0);
}

// ROM: 0x9756  74.7%
void ui_handle_main_menu(void) {
  uint16_t cost;
  const uint8_t *costTable = menuItemCostTable;

  if (gCurSubstateY != 0) {
    if (drv_button_is_triggered(0x0E)) {
      gCurSubstateY = 0;
      drv_sound_play(2);
    }
    return;
  }

  if (drv_button_is_triggered(2)) {
    cost = costTable[menu_cursor];
    if (watts < cost) {
      gCurSubstateY = 1;
      drv_sound_play(2);
      return;
    }

    if (menu_cursor > 5) {
    } else {
      switch (menu_cursor) {
      case 0:
        if (!(walker_status_flags_BIT.walking)) {
          gCurSubstateY = 2;
          drv_sound_play(2);
          return;
        }
        cost = costTable[menu_cursor];
        if (watts < cost) {
          watts = 0;
        } else {
          watts -= cost;
        }
        save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (uint8_t *)&totalSteps, 0x18);
        ui_set_view(VIEW_POKERADAR);
        game_pokeradar_init();
        return;
      case 1:
        cost = costTable[menu_cursor];
        if (watts < cost) {
          watts = 0;
        } else {
          watts -= cost;
        }
        save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (uint8_t *)&totalSteps, 0x18);
        game_init_dowsing();
        ui_set_view(VIEW_DOWSING);
        return;
      case 2:
        ui_start_connection_app();
        return;
      case 3:
        ui_set_view(VIEW_TRAINER_CARD);
        ui_reset_trainer_card_state();
        return;
      case 4: {
        uint16_t mask[2];
        ui_load_inventory_mask(mask);
        if (mask[0] != 0) {
          *(volatile uint16_t *)&gCurSubstateA = mask[0];
          accelPos_X = mask[1];
          ui_stats_reset_cursor();
          ui_set_view(VIEW_POKE_ITEMS);
          return;
        }
        if (mask[1] != 0) {
          *(volatile uint16_t *)&gCurSubstateA = mask[0];
          accelPos_X = mask[1];
          ui_set_stats_view_item();
          ui_set_view(VIEW_GIFTS);
          return;
        }
        gCurSubstateY = 3;
        drv_sound_play(2);
        return;
      }
      case 5:
        ui_show_status_screen();
        ui_set_view(VIEW_SETTINGS);
        return;
      }
    }
  }

  if (drv_button_is_triggered(4)) {
    if (menu_cursor == 0) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
      drv_sound_play(1);
      return;
    }
    menu_cursor = (uint8_t)((menu_cursor + 5) % 6);
    drv_sound_play(2);
  }

  if (drv_button_is_triggered(8)) {
    if (menu_cursor == 5) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
      drv_sound_play(1);
    } else {
      menu_cursor = (uint8_t)((menu_cursor + 1) % 6);
    }
    drv_sound_play(2);
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
  volatile uint16_t base = 0x280;

  sys_init_heap();
  sprite_buf = (uint8_t *)sbrk(0x140);
  e0_buf = (uint8_t *)sbrk(0x80);

  /* Current selection item rendering */
  {
    uint16_t addr = (uint16_t)menu_cursor * 0x140 + 0x690 + base;
    drv_eeprom_read_block(addr, sprite_buf, 0x140);
    drv_lcd_blit(8, 0, sprite_buf, 0x50, 0x10);
  }

  for (i = 0; i < 6; i++) {
    uint16_t j;
    for (j = 0; j < 0x80; j++) {
      e0_buf[j] = 0;
    }

    if ((uint8_t)i == menu_cursor) {
      uint16_t cursor_addr = (uint16_t)((animTick & 1) + 3) * 0x10 + 0x278 + base;
      drv_eeprom_read_block(cursor_addr, sprite_buf, 0x10);
      gfx_blit_to_buffer(8, 8, 4, (uint8_t)(mainMenuYCoords[i] - 8),
                         sprite_buf, e0_buf, 0x10);
    }

    {
      uint16_t item_addr = 0xE10 + base + (uint16_t)i * 0x40;
      drv_eeprom_read_block(item_addr, sprite_buf, 0x40);
      gfx_blit_to_buffer(0x10, 0x10, 0, mainMenuYCoords[i],
                         sprite_buf, e0_buf, 0x10);
    }

    drv_lcd_blit((uint8_t)(i * 0x10), 0x10, e0_buf, 0x10, 0x20);
  }

  if (gCurSubstateY == 0) {
    gfx_draw_numeric_value(0x48, 0x30, watts, 0);
    if (menu_cursor == 0) {
      gfx_draw_numeric_value(0x08, 0x30, 10, 0);
    } else if (menu_cursor == 1) {
      gfx_draw_numeric_value(0x08, 0x30, 3, 0);
    }

    if (menu_cursor < 2) {
      drv_eeprom_read_block(base + 0x1A0, sprite_buf, 0x40);
      drv_lcd_blit(0x50, 0x30, sprite_buf, 0x10, 0x10);
      drv_lcd_blit(0x18, 0x30, sprite_buf, 0x10, 0x10);

      drv_eeprom_read_block(base + 0x180, sprite_buf, 0x20);
      drv_lcd_blit(0x28, 0x30, sprite_buf, 8, 0x10);
    }
  } else if (gCurSubstateY == 1) {
    gfx_draw_text_box(0x30, 0x14, 0x0F, 0x01);
  } else if (gCurSubstateY == 2) {
    gfx_draw_text_box(0x30, 0x15, 0x0F, 0x01);
  } else if (gCurSubstateY == 3) {
    gfx_draw_text_box(0x30, 0x16, 0x0F, 0x01);
  }

  drv_eeprom_read_block(base + 0x338, sprite_buf, 0x40);
  drv_lcd_blit(0, 0, sprite_buf, 8, 0x10);

  drv_lcd_blit(0x58, 0, sprite_buf + 0x20, 8, 0x10);
  gfx_draw_battery_low(0x58, 0);
}
