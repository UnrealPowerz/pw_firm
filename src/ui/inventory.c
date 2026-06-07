#include "all_headers.h"

/*
 * Inventory browser — VIEW_POKE_ITEMS and VIEW_GIFTS.
 *
 * Entered from the main menu's "Pokemon & Items" entry. Shows a 5x2 grid
 * of slots covering own + caught + event pokemon and gift/dowsed items.
 *
 *   VIEW_POKE_ITEMS    - inventory grid: own / caught / event pokemon + items.
 *     Handler:  ui_handle_inventory_pokemon
 *     Render:   ui_render_inventory_pokemon
 *     Cursor:   g.viewstate.Y.BYTE = 0..9 (see enum inventory_slot below)
 *
 *   VIEW_GIFTS         - dowsed-items list.
 *     Handler:  ui_handle_inventory_items
 *     Render:   ui_render_inventory_items
 *     Cursor:   g.viewstate.Y.BYTE = 0..9 (gift slot)
 *
 * The storage-full discard picker (VIEW_DISCARD_PICKER) lives in
 * src/ui/discard_picker.c.
 *
 * "Presence mask" — when the inventory views are entered from the main menu,
 *   ui_load_inventory_mask builds a 2-word bitmask of what's available to
 *   show. The two words are stashed in (g.viewstate.A:uint16) and
 *   accel_xPosition_word:uint16, then read back by the render functions.
 *
 *   mask[0]  (held in *(uint16_t *)&g.viewstate.A, the pokemon/event word):
 *     bit 0  (0x001) - walker is currently walking
 *     bit 1  (0x002) - caught pokemon slot 0 present, OR step-hist 0x40 set
 *     bit 2  (0x004) - caught pokemon slot 1 present
 *     bit 3  (0x008) - caught pokemon slot 2 present
 *     bit 4  (0x010) - step-hist 0x20 (event pokemon received)
 *     bit 5  (0x020) - step-hist 0x10 (special map present)
 *     bit 6  (0x040) - gift item slot 0 present
 *     bit 7  (0x080) - gift item slot 1 present
 *     bit 8  (0x100) - gift item slot 2 present
 *     bit 9  (0x200) - referenced by render (special-map overlay) but never
 *                      set by ui_load_inventory_mask in our current decomp;
 *                      kept for ROM-match
 *
 *   mask[1]  (held in accel_xPosition_word, the dowsed-items word):
 *     bits 0..9       - dowsed item entries 0..9 present
 *
 * Cursor helpers (ui_inventory_cursor_next / _prev / _find_present /
 * _back_wrap / _reset) advance g.viewstate.Y.BYTE through the 10-bit presence
 * mask, skipping empty slots.
 */

/* Inventory grid slot indices for g.viewstate.Y. */
enum inventory_slot {
    INV_SLOT_OWN_POKE       = 0,   /* own pokemon */
    INV_SLOT_CAUGHT_POKE_1  = 1,
    INV_SLOT_CAUGHT_POKE_2  = 2,
    INV_SLOT_CAUGHT_POKE_3  = 3,
    INV_SLOT_EVENT_POKE     = 4,
    INV_SLOT_SPECIAL_MAP    = 5,
    INV_SLOT_GIFT_ITEM_1    = 6,
    INV_SLOT_GIFT_ITEM_2    = 7,
    INV_SLOT_GIFT_ITEM_3    = 8,
    INV_SLOT_EVENT_ITEM     = 9
};

// ROM: 0x9108  98.8%
void ui_inventory_jump_to_items(void) {
  g.viewstate.Y.BYTE = INV_SLOT_EVENT_ITEM;
  ui_inventory_cursor_find_present(accel_xPosition_word);
}

// ROM: 0x8d02  77.9%
void ui_handle_inventory_pokemon(void) {
  uint8_t sid;
  if (drv_button_is_triggered(BTN_R) != 0) {
    if (ui_inventory_cursor_prev(*(volatile uint16_t *)&g.viewstate.A) != 0) {
      /* Already at the first present slot — exit back to the main menu. */
      ui_clear_substate_y();
      ui_set_view(VIEW_MAIN_MENU);
      sid = SND_BACK;
      goto do_play_sound;
    }
    sid = SND_CURSOR;
    goto do_play_sound;
  }

  if (drv_button_is_triggered(BTN_L) != 0) {
    if (ui_inventory_cursor_next(*(volatile uint16_t *)&g.viewstate.A) != 0) {
      /* Walked off the end of pokemon — fall through to gifts list if any. */
      if (accel_xPosition_word != 0) {
        ui_inventory_jump_to_items();
        ui_set_view(VIEW_GIFTS);
        sid = SND_CURSOR;
        goto do_play_sound;
      }
      sid = SND_BACK;
      goto do_play_sound;
    }
    sid = SND_CURSOR;
    goto do_play_sound;
  }

  if (drv_button_is_triggered(BTN_M) != 0) {
    if (accel_xPosition_word != 0) {
      ui_inventory_jump_to_items();
      ui_set_view(VIEW_GIFTS);
    } else {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
    }
    sid = SND_CONFIRM;
    goto do_play_sound;
  }
  return;

do_play_sound:
  drv_sound_play(sid);
}

// ROM: 0x9116  81.5%
void ui_handle_inventory_items(void) {
  uint8_t sid;
  if (drv_button_is_triggered(BTN_R) != 0) {
    if (ui_inventory_cursor_prev(accel_xPosition_word) != 0) {
      /* At first gift — flip back into the pokemon view if any present,
         otherwise just play the boundary beep. */
      if (g.viewstate.A != 0) {
        g.viewstate.Y.BYTE = 0;
        ui_inventory_cursor_back_wrap(*(volatile uint16_t *)&g.viewstate.A);
        ui_set_view(VIEW_POKE_ITEMS);
        sid = SND_CURSOR;
        goto do_play_sound;
      }
      sid = SND_BACK;
      goto do_play_sound;
    }
    sid = SND_CURSOR;
    goto do_play_sound;
  }

  if (drv_button_is_triggered(BTN_L) != 0) {
    if (ui_inventory_cursor_next(accel_xPosition_word) != 0) {
      sid = SND_BACK;
      goto do_play_sound;
    }
    sid = SND_CURSOR;
    goto do_play_sound;
  }

  if (drv_button_is_triggered(BTN_M) != 0) {
    ui_reset_substate();
    ui_set_view(VIEW_HOME);
    sid = SND_CONFIRM;
    goto do_play_sound;
  }
  return;

do_play_sound:
  drv_sound_play(sid);
}

// ROM: 0x8d88  78.1%
void ui_render_inventory_pokemon(void) {
  uint32_t romBase = 0x100280;
  void *buf180;
  uint32_t addr;
  uint16_t i;
  uint8_t x, y;

  sys_init_heap();
  buf180 = sbrk(0x180);

  /* "Pokemon" header strip. */
  drv_eeprom_read_block((uint16_t)(romBase + 0xB90), buf180, 0x140);
  drv_lcd_blit(0x08, 0x00, buf180, 0x50, 0x10);

  /* Per-slot detail panel. */
  switch (g.viewstate.Y.BYTE) {
  case INV_SLOT_OWN_POKE:
    gfx_draw_own_pokemon_small(0x3C, 0x18);
    gfx_draw_own_pokemon_name(0x00, 0x30, 7);
    break;
  case INV_SLOT_CAUGHT_POKE_1:
  case INV_SLOT_CAUGHT_POKE_2:
  case INV_SLOT_CAUGHT_POKE_3: {
    /* Match the caught species ID at 0xCE8C+(slot-1)*0x10 against the route
       table at EEPROM_POKEMON_SLOTS, then draw the matching sprite + name. */
    drv_eeprom_read_block(EEPROM_POKEMON_SLOTS, buf180, 0x30);
    addr = 0xCE8C + (uint32_t)(g.viewstate.Y.BYTE - 1) * 0x10;
    drv_eeprom_read_block((uint16_t)addr, (uint8_t *)buf180 + 0x30, 0x10);

    for (i = 0; i < 3; i++) {
      uint16_t slot_id = ((uint16_t *)buf180)[i * 8];
      uint16_t caught_id = *(uint16_t *)((uint8_t *)buf180 + 0x30);
      if (slot_id == caught_id) {
        gfx_draw_route_pokemon(0x3C, 0x18, (uint8_t)i);
        gfx_draw_route_pokemon_name(0x00, 0x30, i, 0x07);
        break;
      }
    }
    break;
  }
  case INV_SLOT_EVENT_POKE:
    /* Peer-event pokemon — sprite cached at EEPROM 0xBA80, two-frame anim. */
    addr = romBase + 0xBA80 + (uint32_t)(g.ui_animationTick & 1) * 0xC0;
    drv_eeprom_read_block((uint16_t)addr, buf180, 0x180);
    drv_lcd_blit(0x3C, 0x18, buf180, 0x20, 0x18);
    gfx_draw_event_pokemon_name(0x00, 0x30, 7);
    break;
  case INV_SLOT_SPECIAL_MAP:
    drv_eeprom_read_block((uint16_t)(romBase + 0x1750), buf180, 0xC0);
    drv_lcd_blit(0x3C, 0x18, buf180, 0x20, 0x18);
    gfx_draw_text_box(0x30, TEXT_SPECIAL_MAP, TEXT_BOX_FULL, TEXT_BOX_STATIC);
    break;
  case INV_SLOT_GIFT_ITEM_1:
  case INV_SLOT_GIFT_ITEM_2:
  case INV_SLOT_GIFT_ITEM_3: {
    /* Match the gift item ID at 0xCEBC+(slot-6)*4 against the lookup table
       and draw its name. */
    void *itemBuf;
    gfx_draw_treasure_chest_icon(0x3C, 0x18);
    addr = 0xCEBC + (uint32_t)(g.viewstate.Y.BYTE - 6) * 4;
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
  case INV_SLOT_EVENT_ITEM:
    drv_eeprom_read_block((uint16_t)(romBase + 0x1690), buf180, 0xC0);
    drv_lcd_blit(0x3C, 0x18, buf180, 0x20, 0x18);
    gfx_draw_event_item_name(0x00, 0x30, 0, 0x0F);
    break;
  }

  /* Cursor sprite (blinking, 2-frame) over the current slot in the 5x2 grid. */
  addr = romBase + 0x278 + (uint32_t)((g.ui_animationTick & 1) + 3) * 0x10;
  drv_eeprom_read_block((uint16_t)addr, buf180, 0x10);

  x = (uint8_t)((g.viewstate.Y.BYTE % 5) * 8 + 0x10);
  if (g.viewstate.Y.BYTE == INV_SLOT_OWN_POKE)
    x -= 8;
  y = (uint8_t)((g.viewstate.Y.BYTE / 5) * 0x10 + 0x10);

  drv_lcd_blit(x, y, buf180, 8, 8);

  /* Status icons across the grid, driven by the mask in
     g.viewstate.A (lo word) + accel_xPosition_word (hi word). */
  drv_eeprom_read_block((uint16_t)(romBase + 0x358), buf180, 0x40);
  drv_lcd_blit(0, 0, (uint8_t *)buf180 + 0x20, 8, 0x10);

  {
  uint16_t maskA = *(volatile uint16_t *)&g.viewstate.A;
  uint16_t maskX = accel_xPosition_word;

  /* Right-side dowsed-items "available" tab. */
  if (maskX != 0) {
    drv_lcd_blit(0x58, 0x00, buf180, 8, 16);
  }

  drv_eeprom_read_block((uint16_t)(romBase + 0x1E0), buf180, 0x10);

  /* Walking indicator (own pokemon column). */
  if (maskA & 0x001) {
    drv_lcd_blit(0x08, 0x18, buf180, 8, 8);
  }

  /* Caught pokemon icons (slots 1..3 of the row). */
  for (i = 0; i < 3; i++) {
    if (maskA & (2 << i)) {
      drv_lcd_blit((uint8_t)(i * 8 + 0x18), 0x18, buf180, 8, 8);
    }
  }

  /* Gift item icons (bottom row, slots 1..3). */
  drv_eeprom_read_block((uint16_t)(romBase + 0x208), buf180, 0x10);
  for (i = 0; i < 3; i++) {
    if (maskA & (0x40 << i)) {
      drv_lcd_blit((uint8_t)(i * 8 + 0x18), 0x28, buf180, 8, 8);
    }
  }

  /* Event-pokemon icon. */
  if (maskA & 0x010) {
    drv_eeprom_read_block((uint16_t)(romBase + 0x1F0), buf180, 0x10);
    drv_lcd_blit(0x30, 0x18, buf180, 8, 8);
  }

  /* Bit 9 — render expects an overlay here but no load path sets it; kept
     to match ROM. */
  if (maskA & 0x200) {
    drv_eeprom_read_block((uint16_t)(romBase + 0x218), buf180, 0x10);
    drv_lcd_blit(0x30, 0x28, buf180, 8, 8);
  }

  /* Special-map icon. */
  if (maskA & 0x020) {
    drv_eeprom_read_block((uint16_t)(romBase + 0x228), buf180, 0x10);
    drv_lcd_blit(0x10, 0x28, buf180, 8, 8);
  }
  }

  gfx_draw_battery_low(0x58, 0);
}

// ROM: 0x918c  81.9%
void ui_render_inventory_items(void) {
  void (*blit)(uint8_t, uint8_t, void *, uint8_t, uint8_t);
  void (*eread)(uint16_t, void *, uint16_t);
  void *buf;
  void *namebuf;
  uint16_t anim;
  uint8_t cursor;
  uint8_t col_x, row_y;
  int i;
  uint16_t item_id[2];

  blit = drv_lcd_blit;
  eread = drv_eeprom_read_block;
  sys_init_heap();
  buf = sbrk(0x140);
  /* Left gutter chevron. */
  eread(SPR_OFF(menu_arrow_left), buf, 0x20);
  blit(0, 0, buf, 8, 16);

  /* "POKéMON & ITEMS" heading bar. */
  eread(SPR_OFF(menu_hdg_inventory), buf, 0x140);
  blit(0x08, 0x00, buf, 0x50, 0x10);

  /* Treasure-chest illustration in the right panel. */
  eread(SPR_OFF(present_large), buf, 0xC0);
  blit(0x3C, 0x18, buf, 0x20, 0x18);

  /* Cursor sprite over the current slot in the 5x2 grid. The +3-indexed
   * arrow variant (with tick toggle) gives the animated right-pointing
   * cursor used elsewhere too. */
  anim = (uint16_t)((uint16_t)(g.ui_animationTick & 1) + 3) * 0x10;
  eread(SPR_OFF(arrows_8x8) + anim, buf, 0x10);

  cursor = g.viewstate.Y.BYTE;
  col_x = (uint8_t)((cursor % 5) * 8 + 0x10);
  row_y = (uint8_t)((cursor / 5) * 0x10 + 0x10);
  blit(col_x, row_y, buf, 8, 8);

  /* Item-symbol glyph — filled-cell icon for each present item in the
   * dowsed-items grid (10 slots = 2 rows x 5). */
  eread(SPR_OFF(item_symbol), buf, sizeof(SPR.item_symbol));

  for (i = 0; i < 5; i++) {
    if (g.viewstate.v.bytes.at_d2 & (1 << i)) {
      blit((uint8_t)(i * 8 + 0x10), 0x18, buf, 8, 8);
    }
  }

  for (i = 0; i < 5; i++) {
    if (g.viewstate.v.bytes.at_d2 & (0x20 << i)) {
      blit((uint8_t)(i * 8 + 0x10), 0x28, buf, 8, 8);
    }
  }

  /* Lookup the item name for the currently-selected slot. */
  eread(0xCEC8 + (uint16_t)((uint16_t)g.viewstate.Y.BYTE << 2), (void *)item_id, 4);
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

// ROM: 0x8bd2  79.1%
uint8_t ui_inventory_cursor_next(uint16_t mask) {
  uint8_t y = g.viewstate.Y.BYTE;
  uint8_t tries;

  if (y == INV_SLOT_EVENT_ITEM)
    return 1;
  y++;

  for (tries = 0; tries < 10; tries++) {
    if (mask & (1 << y)) {
      g.viewstate.Y.BYTE = y;
      return 0;
    }
    if (y == INV_SLOT_EVENT_ITEM)
      return 1;
    y++;
  }
  return 1;
}

// ROM: 0x8c0e  74.8%  saves: e6,r5
uint8_t ui_inventory_cursor_find_present(uint16_t mask) {
  uint8_t tries;
  uint8_t y = (uint8_t)((g.viewstate.Y.BYTE + 1) % 10);
  g.viewstate.Y.BYTE = y;

  for (tries = 0; tries < 10; tries++) {
    if (mask & (1 << g.viewstate.Y.BYTE)) {
      return 1;
    }
    y = (uint8_t)((g.viewstate.Y.BYTE + 1) % 10);
    g.viewstate.Y.BYTE = y;
  }
  return 0;
}

// ROM: 0x8c62  90.6%
uint8_t ui_inventory_cursor_prev(uint16_t mask) {
  uint8_t tries;
  if (g.viewstate.Y.BYTE == 0)
    return 1;
  g.viewstate.Y.BYTE--;

  for (tries = 0; tries < 10; tries++) {
    if (mask & (1 << g.viewstate.Y.BYTE))
      return 0;
    if (g.viewstate.Y.BYTE == 0)
      return 1;
    g.viewstate.Y.BYTE--;
  }
  return 0;
}

// ROM: 0x8ca4  80.2%
void ui_inventory_cursor_back_wrap(uint16_t mask) {
  uint8_t tries;
  g.viewstate.Y.BYTE = (uint8_t)((g.viewstate.Y.BYTE + 9) % 10);
  for (tries = 0; tries < 10; tries++) {
    if (mask & (1 << g.viewstate.Y.BYTE))
      return;
    g.viewstate.Y.BYTE = (uint8_t)((g.viewstate.Y.BYTE + 9) % 10);
  }
}

// ROM: 0x8cf4  98.8%
void ui_inventory_cursor_reset(void) {
  g.viewstate.Y.BYTE = INV_SLOT_EVENT_ITEM;
  ui_inventory_cursor_find_present(*(uint16_t *)&g.viewstate.A);
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
// ROM: 0x8aca  89.2%
void ui_load_inventory_mask(uint16_t *status_mask_ptr) {
  uint16_t *poke_slots;
  uint16_t *item_slots;
  uint8_t i;
  uint8_t hist_flags;

  sys_init_heap();
  poke_slots = (uint16_t *)sbrk(0x30);
  item_slots = (uint16_t *)sbrk(0x34);

  for (i = 0; i < 2; i++) {
    status_mask_ptr[i] = 0;
  }

  if (g.sys_walkerFlags.BIT.walking) {
    status_mask_ptr[0] |= 0x01;       /* bit 0: walking */
  }

  drv_eeprom_read_block(EEPROM_LOG_CONTEXT, poke_slots, 0x30);
  drv_eeprom_read_block(EEPROM_LOG_ITEMS, item_slots, 0x34);

  /* mask[0] bits 1..3: caught-pokemon slots present. */
  for (i = 0; i < 3; i++) {
    if (poke_slots[(uint16_t)i << 3] != 0) {
      status_mask_ptr[0] |= (2 << i);
    }
  }

  /* mask[0] bits 6..8: gift-item slots present. */
  for (i = 0; i < 3; i++) {
    if (item_slots[(uint16_t)i << 1] != 0) {
      status_mask_ptr[0] |= (0x40 << i);
    }
  }

  /* mask[1] bits 0..9: dowsed item slots present. */
  for (i = 0; i < 10; i++) {
    if (item_slots[6 + ((uint16_t)i << 1)] != 0) {
      status_mask_ptr[1] |= (1 << i);
    }
  }

  /* Step-history flag bits remapped into the icon-grid bit layout. */
  hist_flags = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
  if (hist_flags & 0x20)
    status_mask_ptr[0] |= 0x10;     /* bit 4: event pokemon */
  if (hist_flags & 0x10)
    status_mask_ptr[0] |= 0x20;     /* bit 5: special map */
  if (hist_flags & 0x40)
    status_mask_ptr[0] |= 0x02;     /* bit 1 overlap (event item special-route) */
}
