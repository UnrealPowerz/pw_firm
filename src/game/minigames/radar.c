#include "all_headers.h"

// Reason: ROM uses `mov.l #0x1000280, er5` (ER-packs 0x100 size + 0x280 base
//   into one 32-bit immediate, then uses e5/r5 halves separately). ch38 emits
//   `mov.w #256, e6` + inline 0x280 immediates at each use site. ROM also
//   uses `mulxu.w` for `(animTick+9)*0x10`; ch38 uses 4 SHLL.W (×2×2×2×2).
//   ROM has no prologue; ch38 emits `$sp_regsv$3`.
// Class: cannot-fix-without-compiler-change (ER-packing + multiplication
//   strength reduction differs + sp_regsv$3)
// ROM: 0x9f44  50.6%
#pragma option speed=shift  /* pragma:auto */
void ui_render_pokeradar(void) {
  void *buf;
  uint8_t a;
  uint8_t i;

  sys_init_heap();
  buf = sbrk(0x100);

  drv_eeprom_read_block(0x278 + 0x280 + (((animTick & 1) + 9) * 0x10), buf,
                        0x10);

  a = gCurSubstateA;
  drv_lcd_blit(8, 8, buf, (a & 1) * 0x18 + 8, radarYCoordTable[a] - 8);

  drv_eeprom_read_block(0x1A30 + 0x280, buf, 0xC0);

  for (i = 0; i < 4; i++) {
    drv_lcd_blit(0x20, 0x18, buf, (i & 1) * 0x18, radarYCoordTable[i]);
  }

  if (accelZPos != 0) {
    drv_eeprom_read_block(0x1AF0 + 0x280, buf, 0x100);
    drv_lcd_blit(0x10, 0x10, (uint8_t *)buf + 0xC0,
                       (dowsing_item_pos & 1) * 0x18,
                       radarYCoordTable[dowsing_item_pos] + 0x10);

    if (gCurSubstateZ == 3) {
      gfx_draw_text_box(0x30, 0x1D, 0x0F, 0x00);
    } else if (gCurSubstateZ == 1) {
      gfx_fill_rect(0, (uint8_t)(DAT_f7d5 * 8), 0x60, 8, 3);
      gfx_fill_rect(0, (uint8_t)(0x40 - DAT_f7d5 * 8), 0x60, 8, 3);
      DAT_f7d5++;
    } else if (gCurSubstateZ == 2) {
      gfx_draw_value_with_icon(2, 0x20, 0x0D, gCurSubstateY);
      gfx_draw_text_box(0x30, 0x0F, 0x0E, 0x01);
    }
  } else {
    gfx_draw_text_box(0x30, 0x1C, 0x0F, 0x00);
    if (accelYPos == 0) {
      drv_eeprom_read_block(0x1AF0 + 0x280, buf, 0x100);
      drv_lcd_blit(0x10, 0x10, (uint8_t *)buf + radarFrameMultiplier[DAT_f7d1] * 0x40,
                         (dowsing_item_pos & 1) * 0x18,
                         radarYCoordTable[dowsing_item_pos] + 0x10);
    }
  }

  gfx_draw_battery_low(0, 0);
}

// ROM: 0x9dce  88.8%
void ui_handle_radar_grass_menu(void) {
  if (drv_button_is_triggered(0x04) != 0) {
    gCurSubstateA = (gCurSubstateA + 3) & 3;
    drv_sound_play(2);
  }
  if (drv_button_is_triggered(0x08) != 0) {
    gCurSubstateA = (gCurSubstateA + 1) & 3;
    drv_sound_play(2);
  }
  if (drv_button_is_triggered(0x02) != 0) {
    if (DAT_f7d5 != 0) {
      if (gCurSubstateA == dowsing_item_pos) {
        drv_sound_play(3);
        gCurSubstateZ = 3;
        accelZPos = 0x10;
        return;
      }
      if (DAT_f7d1 == 0) {
        drv_sound_play(4);
        return;
      }
    }
  } else {
    if (accelYPos != 0) {
      accelYPos--;
      return;
    }
    if (DAT_f7d5 != 0) {
      DAT_f7d5--;
    }
    if (DAT_f7d5 != 0) {
      return;
    }
  }

  drv_sound_play(0x0E);
  ui_set_view(VIEW_RADAR_FAILURE);
}

// ROM: 0x9e72  63.1%
void ui_handle_pokeradar(void) {
  uint32_t r;
  if (drv_sound_is_playing())
    return;

  if (gCurSubstateZ == 0) {
    ui_handle_radar_grass_menu();
    return;
  } else if (gCurSubstateZ == 1) {
    if (DAT_f7d5 > 4) {
      game_start_battle();
      ui_set_view(VIEW_BATTLE);
    }
    return;
  } else if (gCurSubstateZ == 2) {
    if (drv_button_is_triggered(0x0E) == 0)
      return;
    drv_sound_play(0);
    ui_reset_substate();
    ui_set_view(VIEW_HOME);
    return;
  } else if (gCurSubstateZ != 3) {
    return;
  }

  if (accelZPos == 0)
    return;
  accelZPos--;
  if (accelZPos != 0)
    return;

  if ((int16_t)DAT_f7d1 < (int16_t)((uint16_t)accelXPos - 1)) {
    gCurSubstateZ = 1;
    accelZPos = 1;
    DAT_f7d5 = 0;
    return;
  }

  gCurSubstateZ = 0;
  r = sys_get_rng() >> 2;
  accelYPos = (uint8_t)((uint16_t)r / radarStateYDivisor[DAT_f7d1] + 0x10);
  DAT_f7d1++;
  DAT_f7d5 = radarStateXTable[DAT_f7d1];
  dowsing_item_pos = (uint8_t)((sys_get_rng() << 3) & 3);
}

// ROM: 0xa10a  97.5%
void ui_handle_radar_failure(void) {
  if (drv_button_is_triggered(0x0E) != 0) {
    gCurSubstateA = 0;
    drv_sound_play(4);
    ui_reset_substate();
    ui_set_view(VIEW_HOME);
  }
}

// ROM: 0x9d92  85.5%
void game_pokeradar_init(void) {
  uint8_t *ram_ptr;
  game_generate_encounter_dowsing();
  gCurSubstateZ = 0;
  gCurSubstateA = 0;
  DAT_f7d1 = 0;
  accelYPos = 5;
  ram_ptr = (uint8_t *)0; /* base RAM pointer for reading cached EEPROM */
  DAT_f7d5 = ram_ptr[0xBF1A];
  dowsing_item_pos = ((sys_get_rng() << 3) >> 8) &
                     3; /* Matches `shll.w r0` x 3 and taking high byte */
  accelZPos = 0;
}
