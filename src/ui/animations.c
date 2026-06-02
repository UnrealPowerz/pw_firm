#include "all_headers.h"

// ROM: 0x3dbc  83.3%  saves: r2,r5,r6 -> sys_epilogue_r2_r5_r6
void ui_draw_ball_drop_anim(void) {
  void *ptr;
  uint16_t offset;
  uint16_t e0_dummy = 0;

  sys_init_heap();
  ptr = sbrk(0x180);

  if (currentlyActiveView != VIEW_EVENT_REWARD_ANIM) {
    offset = 0x2480;
  } else {
    offset = 0x460;
  }

  drv_eeprom_read_block(offset, ptr, 0x10);
  drv_lcd_blit(
      0x2c, ballDropAnimYTable[gCurSubstateZ],
      (void *)ptr, 8, 8);

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  gCurSubstateZ++;
}

// ROM: 0x3e34  75.0%  saves: r2,r5,r6 -> sys_epilogue_r2_r5_r6
void ui_draw_ball_sparkles_anim(void) {
  void *ptr;
  uint16_t dummy;

  sys_init_heap();
  ptr = sbrk(0x180);

  drv_eeprom_read_block(0x460, ptr, 0x10);
  drv_lcd_blit(0x2c, 0x10, ptr, 8, 8);

  drv_eeprom_read_block(0x2040, ptr, 0x10);
  {
    uint16_t packed = ((const uint16_t *)sparklesAnimXYTable)[gCurSubstateZ];
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

// ROM: 0x3ece  74.7%  saves: r2,r5,r6 -> sys_epilogue_r2_r5_r6
void ui_draw_arrival_cloud_anim(void) {
  void *ptr;
  uint16_t dummy;

  sys_init_heap();
  ptr = sbrk(0x180);

  drv_eeprom_read_block(0x1f70, ptr, 0xc0);
  drv_lcd_blit(0x20, 0x10, ptr, 0x20, 0x18);

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  {
    uint8_t z = gCurSubstateZ;
    if (z != 0) {
      gCurSubstateZ = z + 1;
    }
  }
}

// ROM: 0x3f32  76.0%  saves: r2,er3,r5 -> sys_epilogue_6
void ui_draw_arrival_poke_anim(void) {
  uint16_t dummy;
  gfx_draw_home_pokemon(0x10, 8);

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  gCurSubstateZ++;
}

// ROM: 0x3f72  73.8%  saves: r2,r3,r4
void ui_render_arrival_success(void) {
  uint16_t uninitializedE0;
  gfx_draw_own_pokemon_small(0x20, 4);
  gfx_draw_own_pokemon_name(0, 0x20, 5);
  gfx_draw_text_box(0x30, TEXT_PEER_HAS_ARRIVED, TEXT_BOX_NO_LINES, TEXT_BOX_STATIC);

  {
    uint8_t z = gCurSubstateZ;
    if (z < 0x10) {
      gCurSubstateZ = z + 1;
    }
  }

  if (drv_sound_is_playing() == 0) {
    if (gCurSubstateZ > 8) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
    }
  }
}

// ROM: 0x3fc6  50.6%  saves: r2,r3,r4,r5,er6
#pragma option case=ifthen  /* pragma:auto */
void ui_render_arrival_reward_info(void) {
  void *ptr;
  uint16_t off = 0x280;

  sys_init_heap();
  ptr = sbrk(0x180);

  if (gCurSubstateA <= 7) {
    switch (gCurSubstateA) {
    case 0:
      drv_eeprom_read_block(0xBA80 + ((animTick & 1) * 0xC0), ptr, 0x180);
      drv_lcd_blit(0x20, 0x08, ptr, 0x20, 0x18);
      gfx_draw_event_pokemon_info(0, 0x20, 5);
      break;
    case 1:
      drv_eeprom_read_block(off + 0x1750, ptr, 0xC0);
      drv_lcd_blit(0x20, 0x04, ptr, 0x20, 0x18);
      gfx_draw_text_box(0x20, TEXT_SPECIAL_ROUTE, TEXT_BOX_NO_SHADOW, TEXT_BOX_STATIC);
      break;
    case 2:
      gfx_draw_treasure_chest_icon(0x20, 0x04);
      gfx_draw_event_item_name(0, 0x20, 0, 0x0D);
      break;
    case 3:
      drv_eeprom_read_block(off + 0x1750, ptr, 0xC0);
      drv_lcd_blit(0x20, 0x04, ptr, 0x20, 0x18);
      gfx_draw_text_box(0x20, TEXT_SPECIAL_MAP, TEXT_BOX_NO_SHADOW, TEXT_BOX_STATIC);
      break;
    case 4:
    case 5:
    case 6:
    case 7: {
      uint16_t case_off;
      if (gCurSubstateA == 4)      case_off = 0x238;
      else if (gCurSubstateA == 5) case_off = 0x238 + 0x10;
      else if (gCurSubstateA == 6) case_off = 0x258;
      else                         case_off = 0x268;
      drv_eeprom_read_block(off + case_off, ptr, 0x10);
      drv_lcd_blit(0x2C, 0x10, ptr, 0x08, 0x08);
      gfx_draw_text_box(0x20, TEXT_STAMP, TEXT_BOX_NO_SHADOW, TEXT_BOX_STATIC);
      break;
    }
    }
  }

  gfx_draw_text_box(0x30, TEXT_RECEIVED, TEXT_BOX_NO_LINES, TEXT_BOX_STATIC);
  {
    uint8_t z = gCurSubstateZ;
    if (z < 0x10) {
      gCurSubstateZ = z + 1;
    }
  }
}

// ROM: 0x40f8  95.8%
void ui_handle_walk_arrival_anim(void) {
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
  drv_sound_play(SND_ANIM_CUE);
done:;
}

// ROM: 0x4148  96.4%
void ui_render_walk_arrival_anim(void) {
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
// ROM: 0x4178  66.1%
void ui_handle_event_reward_anim(void) {
  uint16_t stackVar;
  uint8_t y = gCurSubstateY;
  if (y == 0) {
    if (gCurSubstateZ > 4) {
      gCurSubstateY = 4;
      gCurSubstateZ = 0;
      drv_sound_play(SND_ANIM_CUE);
    }
    return;
  }
  if (y == 1) {
    if (gCurSubstateZ != 0) {
      gCurSubstateY = 3;
      gCurSubstateZ = 0;
      drv_sound_play(SND_ANIM_CUE);
    }
    return;
  }
  if (y != 3)
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
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, ptr1, 0xBE);
    sbrk(0x10);
    drv_eeprom_read_block(0xBA44, ptr1,
                          0x10); /* reusing ptr1 to save register */
    stackVar = ((RamCache_settingsByte & 1)) << 8;
    ptr3 = sbrk(0x88);
    game_log_interaction(ptr1, ptr3, 0x1D,
                          (uint8_t)((RamCache_settingsByte & 1)), 0, 4);
  } else if (gCurSubstateA == 2) {
    void *ptr1;
    uint16_t *ptr2;
    void *ptr3;
    sys_init_heap();
    ptr1 = sbrk(0xBE);
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, ptr1, 0xBE);
    ptr2 = (uint16_t *)sbrk(0x188);
    drv_eeprom_read_block(EEPROM_WILD_POKE, ptr2, 0x188);
    stackVar = ((RamCache_settingsByte & 1)) << 8;
    ptr3 = sbrk(0x88);
    game_log_interaction(ptr1, ptr3, 0x1C,
                          (uint8_t)((RamCache_settingsByte & 1)), ptr2[3], 0);
  } else {
    ui_reset_substate();
    ui_set_view(VIEW_HOME);
    return;
  }
  ui_reset_substate();
  ui_set_view(VIEW_HOME);
}

// ROM: 0x42a0  96.4%
void ui_render_event_reward_anim(void) {
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

/* Note: jumped from 1.6% to 75.5% after we settled on default -regparam=2
 * + -cmncode (the ROM uses helper-call style for short functions like
 * this one, and ch38's $sp_regsv$3 + sys_epilogue tail-jump aligns well
 * with what compare_bin sees).  No further action recommended. */
// ROM: 0x42d0  76.0%  saves: r2,er3,r5 -> sys_epilogue_6
void ui_draw_poke_departure_anim(void) {
  uint16_t dummy;
  gfx_draw_home_pokemon(0x10, 8);

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  gCurSubstateZ++;
}

// ROM: 0x4310  77.9%  saves: r2,r5,r6 -> sys_epilogue_r2_r5_r6
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

// ROM: 0x4372  81.3%  saves: r2,r5,r6 -> sys_epilogue_r2_r5_r6
void ui_draw_cloud_anim(void) {
  void *ptr;
  uint16_t uninitializedE0;

  sys_init_heap();
  ptr = sbrk(0x180);

  if (gCurSubstateZ <= 4) {
    drv_eeprom_read_block(0x2480, ptr, 0x10);
    drv_lcd_blit(
        0x2c, cloudAnimYTable[gCurSubstateZ],
        (void *)ptr, 8, 8);
  }

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  gCurSubstateZ++;
}

// ROM: 0x43e4  86.0%  saves: r2
void ui_render_departure_success(void) {
  uint16_t uninitializedE0;

  sys_init_heap();
  sbrk(0x180);

  gfx_draw_own_pokemon_name(0, 0x20, 5);
  gfx_draw_text_box(0x30, TEXT_PEER_HAS_LEFT, TEXT_BOX_NO_LINES, TEXT_BOX_STATIC);

  {
    uint8_t z = gCurSubstateZ;
    if (z < 0x10) {
      gCurSubstateZ = z + 1;
    }
  }

  if (drv_sound_is_playing() == 0) {
    if (gCurSubstateZ > 8) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
    }
  }
}

// ROM: 0x4434  78.5%  saves: r2,r5,r6 -> sys_epilogue_r2_r5_r6
void ui_render_operation_completed(void) {
  void *ptr;
  uint16_t uninitializedE0;

  sys_init_heap();
  ptr = sbrk(0x100);

  drv_eeprom_read_block(0x2350, ptr, 0x100);
  drv_lcd_blit(0x20, 0x10, ptr, 0x20, 0x20);
  gfx_draw_text_box(0x30, TEXT_COMPLETED, TEXT_BOX_FULL, TEXT_BOX_STATIC);

  {
    uint8_t z = gCurSubstateZ;
    if (z < 0x10) {
      gCurSubstateZ = z + 1;
    }
  }

  if (drv_sound_is_playing() == 0) {
    if (gCurSubstateZ > 8) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
    }
  }
}

// ROM: 0x449e  92.6%
void ui_handle_walk_departure_anim(void) {
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
  drv_sound_play(SND_ANIM_CUE);
done:
  ;
}

// ROM: 0x44f4  94.2%
void ui_render_walk_departure_anim(void) {
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
