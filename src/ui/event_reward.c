#include "all_headers.h"

/*
 * VIEW_EVENT_REWARD_ANIM — animation played after an IR-received event reward.
 *
 * Reuses two phases from walk_arrival.c (ball-drop and cloud); the only
 * event-specific phases are the sparkles and the final reward-info banner.
 * The kind of reward shown in REWARD_INFO is selected by g.gCurSubstateA
 * (event-pokemon name / special route / event item / special map / stamp).
 *
 * The `uint16_t stackVar` local in ui_handle_event_reward_anim is an
 * intentional stack-frame placeholder.
 */

/* Sub-animation index in g.gCurSubstateY for VIEW_EVENT_REWARD_ANIM. */
enum event_reward_phase {
    REWARD_BALL_DROP = 0,
    REWARD_CLOUD     = 1,
    REWARD_INFO      = 3,
    REWARD_SPARKLES  = 4
};

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
    uint16_t packed = ((const uint16_t *)ANIM_SPARKLES_XY)[g.gCurSubstateZ];
    drv_lcd_blit((uint8_t)packed, (uint8_t)(packed >> 8), ptr, 8, 8);
  }

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  g.gCurSubstateZ++;
  if (g.gCurSubstateZ > 2) {
    g.gCurSubstateY = 1;
    g.gCurSubstateZ = 0;
  }
}

// ROM: 0x3fc6  50.6%  saves: r2,r3,r4,r5,er6
#pragma option case=ifthen  /* pragma:auto */
void ui_render_event_reward_info(void) {
  void *ptr;
  uint16_t off = 0x280;

  sys_init_heap();
  ptr = sbrk(0x180);

  if (g.gCurSubstateA <= 7) {
    switch (g.gCurSubstateA) {
    case 0:
      drv_eeprom_read_block(0xBA80 + ((g.animTick & 1) * 0xC0), ptr, 0x180);
      drv_lcd_blit(0x20, 0x08, ptr, 0x20, 0x18);
      gfx_draw_event_pokemon_name(0, 0x20, 5);
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
      if (g.gCurSubstateA == 4)      case_off = 0x238;
      else if (g.gCurSubstateA == 5) case_off = 0x238 + 0x10;
      else if (g.gCurSubstateA == 6) case_off = 0x258;
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
    uint8_t z = g.gCurSubstateZ;
    if (z < 0x10) {
      g.gCurSubstateZ = z + 1;
    }
  }
}

/* Reason: do NOT bit-field-ize g.settingsByte bit 0 reads here.
 * The expression `(uint8_t)(g.settingsByte & 1)` is passed as a
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
  uint8_t y = g.gCurSubstateY;
  if (y == REWARD_BALL_DROP) {
    if (g.gCurSubstateZ > 4) {
      g.gCurSubstateY = REWARD_SPARKLES;
      g.gCurSubstateZ = 0;
      drv_sound_play(SND_ANIM_CUE);
    }
    return;
  }
  if (y == REWARD_CLOUD) {
    if (g.gCurSubstateZ != 0) {
      g.gCurSubstateY = REWARD_INFO;
      g.gCurSubstateZ = 0;
      drv_sound_play(SND_ANIM_CUE);
    }
    return;
  }
  if (y != REWARD_INFO)
    return;
  if (drv_sound_is_playing())
    return;
  if (g.gCurSubstateZ <= 8)
    return;

  if (g.gCurSubstateA == 0) {
    void *ptr1;
    void *ptr3;
    sys_init_heap();
    ptr1 = sbrk(0xBE);
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, ptr1, 0xBE);
    sbrk(0x10);
    drv_eeprom_read_block(0xBA44, ptr1,
                          0x10); /* reusing ptr1 to save register */
    stackVar = ((g.settingsByte & 1)) << 8;
    ptr3 = sbrk(0x88);
    game_log_interaction(ptr1, ptr3, 0x1D,
                          (uint8_t)((g.settingsByte & 1)), 0, 4);
  } else if (g.gCurSubstateA == 2) {
    void *ptr1;
    uint16_t *ptr2;
    void *ptr3;
    sys_init_heap();
    ptr1 = sbrk(0xBE);
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, ptr1, 0xBE);
    ptr2 = (uint16_t *)sbrk(0x188);
    drv_eeprom_read_block(EEPROM_WILD_POKE, ptr2, 0x188);
    stackVar = ((g.settingsByte & 1)) << 8;
    ptr3 = sbrk(0x88);
    game_log_interaction(ptr1, ptr3, 0x1C,
                          (uint8_t)((g.settingsByte & 1)), ptr2[3], 0);
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
  uint8_t y = g.gCurSubstateY;
  if (y == REWARD_BALL_DROP) goto phase_ball_drop;
  if (y == REWARD_SPARKLES)  goto phase_sparkles;
  if (y == REWARD_CLOUD)     goto phase_cloud;
  if (y != REWARD_INFO)      goto done;
  goto phase_info;
phase_ball_drop: ui_draw_ball_drop_anim(); goto done;
phase_sparkles:  ui_draw_ball_sparkles_anim(); goto done;
phase_cloud:     ui_draw_arrival_cloud_anim(); goto done;
phase_info:      ui_render_event_reward_info();
done: gfx_draw_battery_low(0, 0);
}
