#include "all_headers.h"

// ROM: 0x3dbc  77.6%
void ui_draw_ball_drop_anim(void) {
  void *ptr;
  uint16_t offset;
  uint16_t e0_dummy = 0;

  sys_init_heap();
  ptr = sbrk(0x180);

  if (currentlyActiveView != 0x11) {
    offset = 0x2480;
  } else {
    offset = 0x460;
  }

  drv_eeprom_read_block(offset, ptr, 0x10);
  drv_lcd_blit(
      0x2c, (uint8_t)(*((uint8_t *)(uintptr_t)(0xbd70 + gCurSubstateZ))),
      (void *)ptr, 8, 8);

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  gCurSubstateZ++;
}

// ROM: 0x3e34  71.5%
void ui_draw_ball_sparkles_anim(void) {
  void *ptr;
  uint16_t dummy;

  sys_init_heap();
  ptr = sbrk(0x180);

  drv_eeprom_read_block(0x460, ptr, 0x10);
  drv_lcd_blit(0x2c, 0x10, ptr, 8, 8);

  drv_eeprom_read_block(0x2040, ptr, 0x10);
  {
    uint16_t packed = *((uint16_t *)(0xbd76 + (gCurSubstateZ << 1)));
    drv_lcd_blit((uint8_t)packed, (uint8_t)(packed >> 8), ptr, 8, 8);
  }

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  gCurSubstateZ++;
  if (gCurSubstateZ > 2) {
    gCurSubstateY = 1;
    gCurSubstateZ = 0;
  }
}

// ROM: 0x3ece  70.1%
void ui_draw_arrival_cloud_anim(void) {
  void *ptr;
  uint16_t dummy;

  sys_init_heap();
  ptr = sbrk(0x180);

  drv_eeprom_read_block(0x1f70, ptr, 0xc0);
  drv_lcd_blit(0x20, 0x10, ptr, 0x20, 0x18);

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  if (gCurSubstateZ != 0) {
    gCurSubstateZ++;
  }
}

// ROM: 0x3f32  1.6%
void ui_draw_arrival_poke_anim(void) {
  uint16_t dummy;
  gfx_draw_home_pokemon(0x10, 8);

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  gCurSubstateZ++;
}

// ROM: 0x3f72  69.7%
void ui_render_arrival_success(void) {
  uint16_t uninitializedE0;
  gfx_draw_own_pokemon_small(0x20, 4);
  gfx_draw_own_pokemon_name(0x20, 5, 0);
  gfx_draw_text_box(0x30, 0x0E, 0x0D, 0x00);

  if (gCurSubstateZ < 0x10) {
    gCurSubstateZ++;
  }

  if (drv_sound_is_playing() == 0) {
    if (gCurSubstateZ > 8) {
      ui_reset_substate();
      ui_set_view(0);
    }
  }
}

// ROM: 0x3fc6  40.8%
#pragma option case=ifthen  /* pragma:auto */
void ui_render_arrival_reward_info(void) {
  void *ptr;
  uint16_t off;
  uint16_t uninitializedE0;

  sys_init_heap();
  ptr = sbrk(0x180);
  off = 0x280;

  if (gCurSubstateA <= 7) {
    switch (gCurSubstateA) {
    case 0:
      drv_eeprom_read_block(0xba80 + ((DAT_f7ac & 1) * 0xc0), ptr, 0x180);
      drv_lcd_blit(0x20, 8, ptr, 0x20, 0x18);
      break;
    case 1:
    case 2:
      if (gCurSubstateA == 1) {
        off += 0x1270;
      } else {
        off += 0x14f0;
      }
      drv_eeprom_read_block(off, ptr, 0x280);
      drv_lcd_blit((uint8_t)((DAT_f7ac & 1) * 0x80), 8, ptr, 0x20, 0x28);
      gfx_draw_event_item_name(0x20, 0x13, 0x0D, 0);
      break;
    case 3:
      drv_eeprom_read_block(off + 0x1750, ptr, 0xc0);
      drv_lcd_blit(0x20, 4, ptr, 0x20, 0x18);
      gfx_draw_event_pokemon_info(0x20, 0x11, 0);
      gfx_draw_treasure_chest_icon(0x20, 0x20);
      break;
    case 4:
    case 5:
    case 6:
    case 7:
      if (gCurSubstateA == 4)
        off += 0x238;
      else if (gCurSubstateA == 5)
        off += 0x238 + 0x10;
      else if (gCurSubstateA == 6)
        off += 0x258;
      else if (gCurSubstateA == 7)
        off += 0x268;
      drv_eeprom_read_block(off, ptr, 0x10);
      drv_lcd_blit(0x2c, 0x10, ptr, 8, 8);
      gfx_draw_text_box(0x20, 0x0D, 0x12, 0x00);
      break;
    }
  } else {
    gfx_draw_text_box(0x30, 0x0E, 0x0F, 0x00);
    if (gCurSubstateZ < 0x10) {
      gCurSubstateZ++;
    }
  }
}

// ROM: 0x40f8  94.5%
void ui_handle_return_from_walk(void) {
  uint8_t z;
  uint8_t y;
  z = gCurSubstateZ;
  y = gCurSubstateY;
  if (y == 0) goto case0;
  if (y == 1) goto case1;
  if (y != 2) goto done;
  goto case2;
case0:
  if (z > 4) {
    gCurSubstateY = 1;
    gCurSubstateZ = 0;
  }
  goto done;
case1:
  if (z == 0) goto done;
  gCurSubstateY = 2;
  gCurSubstateZ = 0;
  goto play;
case2:
  if (z <= 8) goto done;
  gCurSubstateZ = 0;
  gCurSubstateY = 3;
play:
  drv_sound_play(6);
done:;
}

// ROM: 0x4148  96.1%
void ui_render_joined_walk_anim(void) {
  uint8_t y = gCurSubstateY;
  if (y == 0) goto case0;
  if (y == 1) goto case1;
  if (y == 2) goto case2;
  if (y != 3) goto done;
  goto case3;
case0: ui_draw_ball_drop_anim(); goto done;
case1: ui_draw_arrival_cloud_anim(); goto done;
case2: ui_draw_arrival_poke_anim(); goto done;
case3: ui_render_arrival_success();
done: gfx_draw_battery_low(0, 0);
}

/* Reason: do NOT bit-field-ize RamCache_settingsByte bit 0 reads here.
 * The expression `(uint8_t)(RamCache_settingsByte & 1)` is passed as a
 * function argument; ch38 already compiles it to MOV+SUB+BLD+BST (the
 * byte-widening produces the bit-store-to-byte sequence the ROM also
 * uses).  Switching to RamCache_settingsByte_BIT.mute adds a redundant
 * widen and regressed this function by -3.2%.  See note in include/types.h
 * about the multi-bit fields of settings_byte_t -- those are tabled and
 * may require shift-based access to match.
 * Class: do-not-bit-field */
// ROM: 0x4178  60.9%
void ui_handle_event_reward(void) {
  uint16_t stackVar;
  if (gCurSubstateY == 0) {
    if (gCurSubstateZ > 4) {
      gCurSubstateY = 4;
      gCurSubstateZ = 0;
      drv_sound_play(6);
    }
    return;
  }
  if (gCurSubstateY == 1) {
    if (gCurSubstateZ != 0) {
      gCurSubstateY = 3;
      gCurSubstateZ = 0;
      drv_sound_play(6);
    }
    return;
  }
  if (gCurSubstateY != 3)
    return;
  if (drv_sound_is_playing())
    return;
  if (gCurSubstateZ <= 8)
    return;

  if (gCurSubstateA == 0) {
    void *ptr1;
    void *ptr3;
    sys_init_heap();
    ptr1 = sbrk(0xBE);
    drv_eeprom_read_block(0x8F00, ptr1, 0xBE);
    sbrk(0x10);
    drv_eeprom_read_block(0xBA44, ptr1,
                          0x10); /* reusing ptr1 to save register */
    stackVar = ((RamCache_settingsByte & 1)) << 8;
    ptr3 = sbrk(0x88);
    game_log_interaction(ptr3, ptr1, 0x1D,
                          (uint8_t)((RamCache_settingsByte & 1)), 4);
  } else if (gCurSubstateA == 2) {
    void *ptr1;
    uint16_t *ptr2;
    void *ptr3;
    sys_init_heap();
    ptr1 = sbrk(0xBE);
    drv_eeprom_read_block(0x8F00, ptr1, 0xBE);
    ptr2 = (uint16_t *)sbrk(0x188);
    drv_eeprom_read_block(0xBD40, ptr2, 0x188);
    stackVar = ((RamCache_settingsByte & 1)) << 8;
    ptr3 = sbrk(0x88);
    game_log_interaction(ptr3, ptr1, 0x1C,
                          (uint8_t)((RamCache_settingsByte & 1)), ptr2[3]);
  } else {
    ui_reset_substate();
    ui_set_view(0);
    return;
  }
  ui_reset_substate();
  ui_set_view(0);
}

// ROM: 0x42a0  96.1%
void ui_render_return_from_walk_anim(void) {
  uint8_t y = gCurSubstateY;
  if (y == 0) goto case0;
  if (y == 4) goto case4;
  if (y == 1) goto case1;
  if (y != 3) goto done;
  goto case3;
case0: ui_draw_ball_drop_anim(); goto done;
case4: ui_draw_ball_sparkles_anim(); goto done;
case1: ui_draw_arrival_cloud_anim(); goto done;
case3: ui_render_arrival_reward_info();
done: gfx_draw_battery_low(0, 0);
}

/* Reason: callee-save ABI mismatch.
 * ROM saves r2/er3/r5 via push.* instructions and tail-calls a shared
 * sys_epilogue_6 to pop them.  Our ch38 build saves er3-er6 via the
 * runtime helpers $sp_regsv$3 / $spregld2$3.  Different register sets and
 * different shared-helper names mean the prologue/epilogue bytes can never
 * align.  The body (gfx_draw_home_pokemon + 2 gfx_fill_rect + increment)
 * is byte-identical, but it is sandwiched between mismatched frames so
 * the per-instruction match score stays in single digits.
 * Class: cannot-fix-without-compiler-change */
// ROM: 0x42d0  1.6%
void ui_draw_poke_departure_anim(void) {
  uint16_t dummy;
  gfx_draw_home_pokemon(0x10, 8);

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  gCurSubstateZ++;
}

// ROM: 0x4310  74.2%
void ui_draw_cloud_departure_anim(void) {
  void *ptr;
  uint16_t dummy;

  sys_init_heap();
  ptr = sbrk(0x180);

  drv_eeprom_read_block(0x1f70, ptr, 0xc0);
  drv_lcd_blit(0x20, 0x10, ptr, 0x20, 0x18);

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  gCurSubstateY = 0;
  gCurSubstateZ = 0;
}

// ROM: 0x4372  75.4%
void ui_draw_cloud_anim(void) {
  void *ptr;
  uint16_t uninitializedE0;

  sys_init_heap();
  ptr = sbrk(0x180);

  if (gCurSubstateZ <= 4) {
    drv_eeprom_read_block(0x2480, ptr, 0x10);
    drv_lcd_blit(
        0x2c, (uint8_t)(*((uint8_t *)(uintptr_t)(0xbd7c + gCurSubstateZ))),
        (void *)ptr, 8, 8);
  }

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  gCurSubstateZ++;
}

// ROM: 0x43e4  81.5%
void ui_render_departure_success(void) {
  uint16_t uninitializedE0;

  sys_init_heap();
  sbrk(0x180);

  gfx_draw_own_pokemon_name(0x20, 5, 0);
  gfx_draw_text_box(0x30, 0x0E, 0x0E, 0x00);

  if (gCurSubstateZ < 0x10) {
    gCurSubstateZ++;
  }

  if (drv_sound_is_playing() == 0) {
    if (gCurSubstateZ > 8) {
      ui_reset_substate();
      ui_set_view(0);
    }
  }
}

// ROM: 0x4434  77.3%
void ui_render_operation_completed(void) {
  void *ptr;
  uint16_t uninitializedE0;

  sys_init_heap();
  ptr = sbrk(0x100);

  drv_eeprom_read_block(0x2350, ptr, 0x100);
  drv_lcd_blit(0x20, 0x10, ptr, 0x20, 0x20);
  gfx_draw_text_box(0x30, 0x0F, 0x10, 0x00);

  if (gCurSubstateZ < 0x10) {
    gCurSubstateZ++;
  }

  if (drv_sound_is_playing() == 0) {
    if (gCurSubstateZ > 8) {
      ui_reset_substate();
      ui_set_view(0);
    }
  }
}

// ROM: 0x449e  91.5%
void ui_handle_start_walk(void) {
  uint8_t z;
  uint8_t y;
  z = gCurSubstateZ;
  y = gCurSubstateY;
  if (y == 5) goto case5;
  if (y == 2) goto case2;
  if (y == 0) goto case0;
  if (y != 6) goto done;
  goto case6;
case5:
  gCurSubstateZ = 0;
  gCurSubstateY = 2;
  goto sound;
case2:
  if (z <= 8) goto done;
  gCurSubstateZ = 0;
  gCurSubstateY = 1;
  goto done;
case0:
  if (z < 9) goto done;
  y = 3;
  goto shared;
case6:
  y = 7;
shared:
  gCurSubstateY = y;
  gCurSubstateZ = 0;
sound:
  drv_sound_play(6);
done:
  ;
}

// ROM: 0x44f4  94.0%
void ui_render_start_walk_anim(void) {
  uint8_t y = gCurSubstateY;
  if (y == 2) goto case2;
  if (y == 1) goto case1;
  if (y == 0) goto case0;
  if (y == 3) goto case3;
  if (y == 6) goto case67;
  if (y != 7) goto done;
  goto case67;
case2: ui_draw_poke_departure_anim(); goto done;
case1: ui_draw_cloud_departure_anim(); goto done;
case0: ui_draw_cloud_anim(); goto done;
case3: ui_render_departure_success(); goto done;
case67: ui_render_operation_completed();
done: gfx_draw_battery_low(0, 0);
}
