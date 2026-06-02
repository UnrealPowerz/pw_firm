#include "all_headers.h"

/*
 * Settings view (VIEW_SETTINGS).
 *
 * Three-page state machine driven by gCurSubstateY:
 *   SETTINGS_MENU    = 0   main settings list (volume / shade rows)
 *   SETTINGS_VOLUME  = 1   volume adjustment sub-page
 *   SETTINGS_SHADE   = 2   contrast/shade adjustment sub-page
 *
 * On the main page, gCurSubstateZ is the within-page cursor: 0 = volume row,
 * 1 = shade row. Pressing R from the main page enters the highlighted row
 * (gCurSubstateY = gCurSubstateZ + 1). Pressing R from a sub-page commits
 * the new setting and exits to home.
 */

/* Settings dispatcher pages (gCurSubstateY in ui_handle_settings). */
enum settings_page {
    SETTINGS_MENU   = 0,
    SETTINGS_VOLUME = 1,
    SETTINGS_SHADE  = 2
};

// ROM: 0x6c88  100.0%
void ui_reset_settings_substate(void) {
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
}

// ROM: 0x6c94  92.5%
void ui_handle_settings_main_page(void) {
  if (drv_button_is_triggered(BTN_M)) {
    if (gCurSubstateZ == 0) {
      drv_sound_play(SND_BACK);
      ui_clear_substate_y();
      ui_set_view(VIEW_MAIN_MENU);
      return;
    } else {
      gCurSubstateZ = 0;
      drv_sound_play(SND_CURSOR);
    }
  }
  if (drv_button_is_triggered(BTN_L)) {
    if (gCurSubstateZ != 1) {
      gCurSubstateZ = 1;
      drv_sound_play(SND_CURSOR);
    }
  }
}

// ROM: 0x6ce2  65.3%
void ui_handle_settings_volume(void) {
  uint8_t vol;
  uint16_t val;

  if (drv_button_is_triggered(BTN_M)) {
    vol = (RamCache_settingsByte >> 1) & 0x03;
    val = (uint16_t)((int16_t)(int8_t)vol + 2);
    val = (uint16_t)((int16_t)val % 3);
    RamCache_settingsByte = (uint8_t)((RamCache_settingsByte & ~(0x03 << 1)) |
                                      (((uint8_t)val & 0x03) << 1));
    vol = (RamCache_settingsByte >> 1) & 0x03;
    drv_sound_set_volume(vol);
    drv_sound_play(SND_CURSOR);
  }
  if (drv_button_is_triggered(BTN_L)) {
    vol = (RamCache_settingsByte >> 1) & 0x03;
    val = (uint16_t)((int16_t)(int8_t)vol + 1);
    val = (uint16_t)((int16_t)val % 3);
    RamCache_settingsByte = (uint8_t)((RamCache_settingsByte & ~(0x03 << 1)) |
                                      (((uint8_t)val & 0x03) << 1));
    vol = (RamCache_settingsByte >> 1) & 0x03;
    drv_sound_set_volume(vol);
    drv_sound_play(SND_CURSOR);
  }
}

// ROM: 0x6d6c  70.6%
void ui_handle_settings_shade(void) {
  uint8_t shade;

  if (drv_button_is_triggered(BTN_M)) {
    if ((RamCache_settingsByte & 0x78) != 0) {
      shade = ((RamCache_settingsByte >> 3) & 0x0F) - 1;
      RamCache_settingsByte = (uint8_t)((RamCache_settingsByte & ~(0x0F << 3)) |
                                        ((shade & 0x0F) << 3));
      drv_sound_play(SND_CURSOR);
    }
    shade = (RamCache_settingsByte >> 3) & 0x0F;
    drv_lcd_set_contrast(shade);
  }
  if (drv_button_is_triggered(BTN_L)) {
    if ((RamCache_settingsByte & 0x78) < 0x48) {
      shade = ((RamCache_settingsByte >> 3) & 0x0F) + 1;
      RamCache_settingsByte = (uint8_t)((RamCache_settingsByte & ~(0x0F << 3)) |
                                        ((shade & 0x0F) << 3));
      drv_sound_play(SND_CURSOR);
    }
    shade = (RamCache_settingsByte >> 3) & 0x0F;
    drv_lcd_set_contrast(shade);
  }
}

// ROM: 0x6dfc  78.8%
void ui_handle_settings(void) {
  if (gCurSubstateY == SETTINGS_MENU) {
    ui_handle_settings_main_page();
  } else if (gCurSubstateY == SETTINGS_VOLUME) {
    ui_handle_settings_volume();
  } else if (gCurSubstateY == SETTINGS_SHADE) {
    ui_handle_settings_shade();
  }
  if (drv_button_is_triggered(BTN_R)) {
    if (gCurSubstateY == SETTINGS_MENU) {
      /* R on the main settings menu enters the highlighted row. The within-
         page cursor (gCurSubstateZ) is 0 for volume and 1 for shade; +1 maps
         to SETTINGS_VOLUME / SETTINGS_SHADE. */
      drv_sound_play(SND_CONFIRM);
      gCurSubstateY = gCurSubstateZ + 1;
    } else {
      /* R inside a sub-page commits the new setting and exits to home. */
      drv_sound_play(SND_CONFIRM);
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
  uint8_t cursor_x;
  int i;
  uint8_t volVal;
  uint8_t shVal;
  uint8_t shadeOff;
  uint16_t animOff;

  sys_init_heap();
  buf = sbrk(0x140);

  /* "Settings" header. */
  drv_eeprom_read_block(0xF50, buf, 0x140);
  drv_lcd_blit(8, 0, buf, 0x50, 0x10);

  /* "Volume" row label. */
  drv_eeprom_read_block(0x1690, buf, 0xA0);
  drv_lcd_blit(8, 0x10, buf, 0x28, 0x10);

  /* "Shade" row label. */
  drv_eeprom_read_block(0x1730, buf, 0xA0);
  drv_lcd_blit(0x38, 0x10, buf, 0x28, 0x10);

  drv_eeprom_read_block(0x4F8, buf, 0xC0);

  /* Within-page cursor x: 0 = volume row, 0x30 = shade row. */
  cursor_x = gCurSubstateZ * 0x30;

  if (gCurSubstateY == SETTINGS_MENU) {
    animOff = ((animTick & 0x01) + 9) * 0x10;
    drv_lcd_blit(cursor_x, 0x14, buf + animOff, 8, 8);
  } else if (gCurSubstateY == SETTINGS_VOLUME) {
    drv_lcd_blit(cursor_x, 0x14, buf + 0xB0, 8, 8);

    volVal = (RamCache_settingsByte >> 1) & 0x03;
    animOff = ((animTick & 0x01) + 9) * 0x10;
    drv_lcd_blit((uint8_t)(volVal * 0x20), 0x2C, buf + animOff, 8, 8);

    drv_eeprom_read_block(0x17D0, buf, 0x120);
    drv_lcd_blit(0x08, 0x28, buf, 0x18, 0x10);
    drv_lcd_blit(0x28, 0x28, buf + 0x60, 0x18, 0x10);
    drv_lcd_blit(0x48, 0x28, buf + 0xC0, 0x18, 0x10);
  } else if (gCurSubstateY == SETTINGS_SHADE) {
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
