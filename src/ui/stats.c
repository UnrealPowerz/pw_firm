#include "all_headers.h"

// ROM: 0x9108  70.0%
void ui_set_stats_view_item(void) {
  gCurSubstateY = 9;
  ui_stats_find_index(accelXPos);
}

// ROM: 0x8d02  72.9%
void ui_handle_poke_items(void) {
  uint8_t sid;
  if (drv_button_is_triggered(0x04) != 0) {
    if (ui_stats_prev_index(gCurSubstateA) != 0) {
      ui_clear_substate_y();
      ui_set_view(VIEW_MAIN_MENU);
      sid = 1;
      goto do_play_sound;
    }
    sid = 2;
    goto do_play_sound;
  }

  if (drv_button_is_triggered(0x08) != 0) {
    if (ui_stats_next_index(gCurSubstateA) != 0) {
      if (accelXPos != 0) {
        ui_set_stats_view_item();
        ui_set_view(VIEW_GIFTS);
        sid = 2;
        goto do_play_sound;
      }
      sid = 1;
      goto do_play_sound;
    }
    sid = 2;
    goto do_play_sound;
  }

  if (drv_button_is_triggered(0x02) != 0) {
    if (accelXPos != 0) {
      ui_set_stats_view_item();
      ui_set_view(VIEW_GIFTS);
    } else {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
    }
    sid = 0;
    goto do_play_sound;
  }
  return;

do_play_sound:
  drv_sound_play(sid);
}

// ROM: 0x9116  73.5%
void ui_handle_gifts(void) {
  uint8_t sid;
  if (drv_button_is_triggered(0x04) != 0) {
    if (ui_stats_prev_index(accelXPos) != 0) {
      if (gCurSubstateA != 0) {
        gCurSubstateY = 0;
        ui_stats_cycle_index(gCurSubstateA);
        ui_set_view(VIEW_POKE_ITEMS);
        sid = 2;
        goto do_play_sound;
      }
      sid = 1;
      goto do_play_sound;
    }
    sid = 2;
    goto do_play_sound;
  }

  if (drv_button_is_triggered(0x08) != 0) {
    if (ui_stats_next_index(accelXPos) != 0) {
      sid = 1;
      goto do_play_sound;
    }
    sid = 2;
    goto do_play_sound;
  }

  if (drv_button_is_triggered(0x02) != 0) {
    ui_reset_substate();
    ui_set_view(VIEW_HOME);
    sid = 0;
    goto do_play_sound;
  }
  return;

do_play_sound:
  drv_sound_play(sid);
}

// ROM: 0x8d88  69.7%
#pragma option speed=loop=1  /* pragma:auto */
void ui_render_pokemon_stats(void) {
  uint32_t romBase = 0x100280;
  void *buf180;
  uint32_t addr;
  uint16_t i;
  uint8_t x, y;

  sys_init_heap();
  buf180 = sbrk(0x180);

  drv_eeprom_read_block((uint16_t)(romBase + 0xB90), buf180, 0x140);
  drv_lcd_blit(0x50, 0x10, buf180, 0x50, 0x10);

  switch (gCurSubstateY) {
  case 0:
    gfx_draw_own_pokemon_small(0x3C, 0x18);
    gfx_draw_own_pokemon_name(0x00, 0x30, 7);
    break;
  case 1:
  case 2:
  case 3: {
    drv_eeprom_read_block(EEPROM_POKEMON_SLOTS, buf180, 0x30);
    addr = 0xCE8C + (uint32_t)(gCurSubstateY - 1) * 0x10;
    drv_eeprom_read_block((uint16_t)addr, (uint8_t *)buf180 + 0x30, 0x10);

    for (i = 0; i < 3; i++) {
      uint16_t currentId = ((uint16_t *)buf180)[0x30 / 2 + i * 8];
      if (*(uint16_t *)((uint8_t *)buf180 + 0x30) == currentId) {
        gfx_draw_route_pokemon(0x3C, 0x18, (uint8_t)i);
        gfx_draw_route_pokemon_name(0x00, 0x30, i, 0x07);
        break;
      }
    }
    break;
  }
  case 4:
    addr = romBase + 0xBA80 + (uint32_t)(animTick & 1) * 0xC0;
    drv_eeprom_read_block((uint16_t)addr, buf180, 0x180);
    drv_lcd_blit(0x20, 0x18, buf180, 0x3C, 0x18);
    gfx_draw_event_pokemon_info(0x00, 0x30, 7);
    break;
  case 5:
    drv_eeprom_read_block((uint16_t)(romBase + 0x1750), buf180, 0xC0);
    drv_lcd_blit(0x20, 0x18, buf180, 0x3C, 0x18);
    gfx_draw_text_box(0x30, 0x11, 0x0F, 0x00);
    break;
  case 6:
  case 7:
  case 8: {
    void *itemBuf;
    gfx_draw_treasure_chest_icon(0x3C, 0x18);
    addr = 0xCEBC + (uint32_t)(gCurSubstateY - 6) * 4;
    drv_eeprom_read_block((uint16_t)addr, buf180, 4);

    itemBuf = sbrk(0x14);
    drv_eeprom_read_block(EEPROM_SUBY_LOOKUP_TABLE, itemBuf, 0x14);

    for (i = 0; i < 10; i++) {
      if (*(uint16_t *)buf180 == ((uint16_t *)itemBuf)[i]) {
        gfx_draw_item_name(0x00, 0x30, i, 0x0F);
        break;
      }
    }
    break;
  }
  case 9:
    drv_eeprom_read_block((uint16_t)(romBase + 0x1690), buf180, 0xC0);
    drv_lcd_blit(0x20, 0x18, buf180, 0x3C, 0x18);
    gfx_draw_event_item_name(0x00, 0x30, 0, 0x0F);
    break;
  }

  addr = romBase + 0x278 + (uint32_t)((animTick & 1) + 3) * 0x10;
  drv_eeprom_read_block((uint16_t)addr, buf180, 0x10);

  x = (uint8_t)((gCurSubstateY % 5) * 8 + 0x10);
  if (gCurSubstateY == 0)
    x -= 8;
  y = (uint8_t)((gCurSubstateY / 5) * 0x10 + 0x10);

  drv_lcd_blit(8, 8, buf180, x, y);

  drv_eeprom_read_block((uint16_t)(romBase + 0x358), buf180, 0x40);
  drv_lcd_blit(8, 16, (uint8_t *)buf180 + 0x20, 0x00, 0x30);

  if (accelXPos != 0) {
    drv_lcd_blit(0x58, 0x00, buf180, 8, 16);
  }

  drv_eeprom_read_block((uint16_t)(romBase + 0x1E0), buf180, 0x10);
  if (gCurSubstateA & 1) {
    drv_lcd_blit(0x18, 0x18, buf180, 8, 8);
  }

  for (i = 0; i < 3; i++) {
    if (gCurSubstateA & (2 << i)) {
      drv_lcd_blit(0x18, (uint8_t)(i * 8 + 0x18), buf180, 8, 8);
    }
  }

  drv_eeprom_read_block((uint16_t)(romBase + 0x208), buf180, 0x10);
  for (i = 0; i < 3; i++) {
    if (gCurSubstateA & (0x40 << i)) {
      drv_lcd_blit(0x28, (uint8_t)(i * 8 + 0x18), buf180, 8, 8);
    }
  }

  if (gCurSubstateA & 0x10) {
    drv_eeprom_read_block((uint16_t)(romBase + 0x1F0), buf180, 0x10);
    drv_lcd_blit(0x18, 0x30, buf180, 8, 8);
  }

  if (gCurSubstateA & 0x200) {
    drv_eeprom_read_block((uint16_t)(romBase + 0x218), buf180, 0x10);
    drv_lcd_blit(0x28, 0x30, buf180, 8, 8);
  }

  if (gCurSubstateA & 0x20) {
    drv_eeprom_read_block((uint16_t)(romBase + 0x228), buf180, 0x10);
    drv_lcd_blit(0x28, 0x10, buf180, 8, 8);
  }

  gfx_draw_battery_low(0x58, 0);
}

// ROM: 0x918c  82.3%
void ui_render_items_stats(void) {
  void (*blit)(uint8_t, uint8_t, void *, uint8_t, uint8_t);
  void (*eread)(uint16_t, void *, uint16_t);
  void *buf;
  void *namebuf;
  uint16_t base;
  uint16_t anim;
  uint8_t sel;
  uint8_t col_coord, row_coord;
  int i;
  uint16_t item_id[2];

  blit = drv_lcd_blit;
  eread = drv_eeprom_read_block;
  sys_init_heap();
  buf = sbrk(0x140);
  base = 0x0280;

  eread(0x0338 + base, buf, 0x20);
  blit(0, 0, buf, 8, 16);

  eread(0x0B90 + base, buf, 0x140);
  blit(0x08, 0x00, buf, 0x50, 0x10);

  eread(0x1810 + base, buf, 0xC0);
  blit(0x3C, 0x18, buf, 0x20, 0x18);

  anim = (uint16_t)((uint16_t)(animTick & 1) + 3) * 0x10;
  eread(0x0278 + base + anim, buf, 0x10);

  sel = gCurSubstateY;
  col_coord = (uint8_t)((sel % 5) * 8 + 0x10);
  row_coord = (uint8_t)((sel / 5) * 0x10 + 0x10);
  blit(col_coord, row_coord, buf, 8, 8);

  base += 0x0208;
  eread(base, buf, 0x10);

  for (i = 0; i < 5; i++) {
    if (accelXPos & (1 << i)) {
      blit((uint8_t)(i * 8 + 0x10), 0x18, buf, 8, 8);
    }
  }

  for (i = 0; i < 5; i++) {
    if (accelXPos & (0x20 << i)) {
      blit((uint8_t)(i * 8 + 0x10), 0x28, buf, 8, 8);
    }
  }

  eread(0xCEC8 + (uint16_t)((uint16_t)gCurSubstateY << 2), (void *)item_id, 4);
  namebuf = sbrk(0x14);
  eread(0x8F8C, namebuf, 0x14);
  for (i = 0; i < 10; i++) {
    if (item_id[0] == ((uint16_t *)namebuf)[i]) {
      gfx_draw_item_name(0x00, 0x30, i, 0x0F);
      break;
    }
  }

  gfx_draw_battery_low(0x58, 0);
}

// ROM: 0x3b94  92.1%
void ui_handle_caught_stats_navigation(void) {
  if (drv_button_is_triggered(4)) {
    if (gCurSubstateZ == 0) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
      drv_sound_play(1);
      return;
    }
    gCurSubstateZ--;
    drv_sound_play(2);
  }

  if (drv_button_is_triggered(8)) {
    if (gCurSubstateZ == 2) {
      drv_sound_play(1);
      return;
    }
    gCurSubstateZ++;
    drv_sound_play(2);
  }

  if (drv_button_is_triggered(2)) {
    if (gCurSubstateA == 0) {
      game_log_poke_interaction();
    } else {
      game_log_item_interaction();
    }
    ui_reset_substate();
    ui_set_view(VIEW_HOME);
    drv_sound_play(0);
  }
}

// ROM: 0x3c0a  79.3%
void ui_render_caught_poke_stats(void) {
  uint16_t *ptr;
  uint8_t i;
  uint16_t addr = 0x8f52;
  uint16_t len = 0x30;

  sys_init_heap();
  ptr = sbrk(0x40);
  drv_eeprom_read_block(addr, ptr, len);
  drv_eeprom_read_block(EEPROM_LOG_CONTEXT + (gCurSubstateZ * 0x10), (uint8_t *)ptr + 0x30,
                        0x10);

  for (i = 0; i < 3; i++) {
    if (*(uint16_t *)((uint8_t *)ptr + 0x30) == ptr[i * 8]) {
      gfx_draw_route_pokemon_name(0, 0x30, i, 0x07);
      break;
    }
  }
}

// ROM: 0x3c76  79.4%
void ui_render_dowsed_item_stats(void) {
  uint32_t val;
  uint16_t *ptr;
  uint8_t i;

  sys_init_heap();
  ptr = sbrk(0x14);
  drv_eeprom_read_block(EEPROM_LOG_ITEMS + ((int8_t)gCurSubstateZ * 4), &val, 4);
  drv_eeprom_read_block(EEPROM_SUBY_LOOKUP_TABLE, ptr, 0x14);

  for (i = 0; i < 10; i++) {
    if (*(uint16_t *)&val == ptr[i]) {
      gfx_draw_item_name(0, 0x30, i, 0x0F);
      break;
    }
  }
}

// ROM: 0x3cd8  78.5%
void ui_render_inventory_stats_view(void) {
  void *ptr;
  uint16_t off;
  void (*drawFunc)(uint8_t, uint8_t, void *, uint8_t, uint8_t) =
      (void (*)(uint8_t, uint8_t, void *, uint8_t, uint8_t))drv_lcd_blit;

  sys_init_heap();
  ptr = sbrk(0x180);
  off = 0x280;

  drv_eeprom_read_block(off + 0x378, ptr, 0x20);
  drawFunc(0, 0, ptr, 8, 0x10);

  drv_eeprom_read_block(0x8b30, ptr, 0x140);
  drawFunc(8, 0, ptr, 0x50, 0x10);

  drv_eeprom_read_block(off + 0x2a8, ptr, 0x20);
  drawFunc((uint8_t)(0x18 + (gCurSubstateZ * 0x14)), 0x18,
           (uint8_t *)ptr + ((animTick & 1) * 0x10), 8, 8);

  if (gCurSubstateA == 0) {
    drv_eeprom_read_block(off + 0x1e0, ptr, 0x10);
  } else {
    drv_eeprom_read_block(off + 0x208, ptr, 0x10);
  }

  drawFunc(0x18, 0x20, ptr, 8, 8);
  drawFunc(0x2C, 0x20, ptr, 8, 8);
  drawFunc(0x40, 0x20, ptr, 8, 8);

  if (gCurSubstateZ <= 2) {
    if (gCurSubstateA == 0) {
      ui_render_caught_poke_stats();
    } else {
      ui_render_dowsed_item_stats();
    }
    gfx_draw_battery_low(0, 0);
  }
}

// ROM: 0x8bd2  67.8%
uint8_t ui_stats_next_index(uint16_t mask) {
  uint8_t y = gCurSubstateY;
  uint8_t count = 0;

  if (y == 9)
    return 1;
  y++;

  for (;;) {
    if (mask & (1 << y)) {
      gCurSubstateY = y;
      return 0;
    }
    if (y == 9)
      break;
    y++;
    count++;
    if (count >= 10)
      break;
  }
  return 1;
}

// ROM: 0x8c0e  74.5%  saves: e6,r5
#pragma option speed=register  /* pragma:auto */
uint8_t ui_stats_find_index(uint16_t mask) {
  uint8_t count;
  uint8_t y = (uint8_t)((gCurSubstateY + 1) % 10);
  gCurSubstateY = y;

  for (count = 0; count < 10; count++) {
    if (mask & (1 << gCurSubstateY)) {
      return 1;
    }
    y = (uint8_t)((gCurSubstateY + 1) % 10);
    gCurSubstateY = y;
  }
  return 0;
}

// ROM: 0x8c62  46.3%
uint8_t ui_stats_prev_index(uint16_t mask) {
  int8_t y;
  if (gCurSubstateY == 0)
    return 1;
  y = (int8_t)(gCurSubstateY - 1);

  for (;;) {
    if (mask & (1 << (uint8_t)y)) {
      gCurSubstateY = (uint8_t)y;
      return 0;
    }
    if (y == 0)
      break;
    y--;
  }
  return 1;
}

// ROM: 0x8ca4  50.9%
void ui_stats_cycle_index(uint16_t mask) {
  uint8_t y;
  do {
    y = (uint8_t)((gCurSubstateY + 9) % 10);
    gCurSubstateY = y;
  } while (!(mask & (1 << y)));
}

// ROM: 0x8cf4  96.2%
void ui_stats_reset_cursor(void) {
  gCurSubstateY = 9;
  ui_stats_find_index(*(uint16_t *)&gCurSubstateA);
}

// Reason: ROM emits no register-save prologue and trashes r3/r4/r5 freely
//   (a "leaf-like" convention seemingly used only here in the codebase).
//   ch38 emits the standard `push.l er6; push.l er5; push.w r4; push.w r3`
//   on entry plus matching pops on exit, and consequently allocates buf_8c8c
//   to ER3 and buf_8cbc to ER5 (high half) instead of r3/r4 — this propagates
//   throughout the body as a register-naming mismatch on every memory access.
//   The shift-by-loop idiom (`2 << i`, `0x40 << i`, `1 << i`) and the bset
//   sequences DO match ROM; structural body is correct. Stuck until we find a
//   way to suppress the prologue or force the unusual register allocation.
// Class: cannot-fix-without-compiler-change (calling-convention helper mismatch)
// ROM: 0x8aca  12.0%
void ui_load_inventory_mask(uint16_t *status_mask_ptr) {
  uint16_t *buf_8c8c;
  uint16_t *buf_8cbc;
  uint8_t i;

  sys_init_heap();
  buf_8c8c = (uint16_t *)sbrk(0x30);
  buf_8cbc = (uint16_t *)sbrk(0x34);

  for (i = 0; i < 2; i++) {
    status_mask_ptr[i] = 0;
  }

  if (walker_status_flags_BIT.walking) {
    status_mask_ptr[0] |= 0x01;
  }

  drv_eeprom_read_block(EEPROM_LOG_CONTEXT, buf_8c8c, 0x30);
  drv_eeprom_read_block(EEPROM_LOG_ITEMS, buf_8cbc, 0x34);

  for (i = 0; i < 3; i++) {
    if (buf_8c8c[(uint16_t)i << 3] != 0) {
      status_mask_ptr[0] |= (2 << i);
    }
  }

  for (i = 0; i < 3; i++) {
    if (buf_8cbc[(uint16_t)i << 1] != 0) {
      status_mask_ptr[0] |= (0x40 << i);
    }
  }

  for (i = 0; i < 10; i++) {
    if (buf_8cbc[6 + ((uint16_t)i << 1)] != 0) {
      status_mask_ptr[1] |= (1 << i);
    }
  }

  i = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
  if (i & 0x20)
    status_mask_ptr[0] |= 0x10;
  if (i & 0x10)
    status_mask_ptr[0] |= 0x20;
  if (i & 0x40)
    status_mask_ptr[0] |= 0x02;
}
