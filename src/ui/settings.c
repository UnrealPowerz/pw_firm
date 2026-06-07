#include "all_headers.h"

/*
 * Settings view (VIEW_SETTINGS).
 *
 * Three-page state machine driven by g.viewstate.Y.BYTE:
 *   SETTINGS_MENU    = 0   main settings list (g.sound_volume / shade rows)
 *   SETTINGS_VOLUME  = 1   g.sound_volume adjustment sub-page
 *   SETTINGS_SHADE   = 2   contrast/shade adjustment sub-page
 *
 * On the main page, g.viewstate.Z is the within-page cursor: 0 = g.sound_volume row,
 * 1 = shade row. Pressing R from the main page enters the highlighted row
 * (g.viewstate.Y.BYTE = g.viewstate.Z + 1). Pressing R from a sub-page commits
 * the new setting and exits to home.
 */

/* Settings dispatcher pages (g.viewstate.Y.BYTE in ui_handle_settings). */
enum settings_page {
    SETTINGS_MENU   = 0,
    SETTINGS_VOLUME = 1,
    SETTINGS_SHADE  = 2
};

// ROM: 0x6c88  100.0%
void ui_reset_settings_substate(void) {
  g.viewstate.Y.BYTE = 0;
  g.viewstate.Z = 0;
}

// ROM: 0x6c94  92.5%
void ui_handle_settings_main_page(void) {
  if (drv_button_is_triggered(BTN_R)) {
    if (g.viewstate.Z == 0) {
      drv_sound_play(SND_BACK);
      ui_clear_substate_y();
      ui_set_view(VIEW_MAIN_MENU);
      return;
    } else {
      g.viewstate.Z = 0;
      drv_sound_play(SND_CURSOR);
    }
  }
  if (drv_button_is_triggered(BTN_L)) {
    if (g.viewstate.Z != 1) {
      g.viewstate.Z = 1;
      drv_sound_play(SND_CURSOR);
    }
  }
}

// ROM: 0x6ce2  65.3%
void ui_handle_settings_volume(void) {
  uint8_t vol;
  uint16_t val;

  if (drv_button_is_triggered(BTN_R)) {
    vol = (g.save_settings.BYTE >> 1) & 0x03;
    val = (uint16_t)((int16_t)(int8_t)vol + 2);
    val = (uint16_t)((int16_t)val % 3);
    g.save_settings.BYTE = (uint8_t)((g.save_settings.BYTE & ~(0x03 << 1)) |
                                      (((uint8_t)val & 0x03) << 1));
    vol = (g.save_settings.BYTE >> 1) & 0x03;
    drv_sound_set_volume(vol);
    drv_sound_play(SND_CURSOR);
  }
  if (drv_button_is_triggered(BTN_L)) {
    vol = (g.save_settings.BYTE >> 1) & 0x03;
    val = (uint16_t)((int16_t)(int8_t)vol + 1);
    val = (uint16_t)((int16_t)val % 3);
    g.save_settings.BYTE = (uint8_t)((g.save_settings.BYTE & ~(0x03 << 1)) |
                                      (((uint8_t)val & 0x03) << 1));
    vol = (g.save_settings.BYTE >> 1) & 0x03;
    drv_sound_set_volume(vol);
    drv_sound_play(SND_CURSOR);
  }
}

// ROM: 0x6d6c  70.6%
void ui_handle_settings_shade(void) {
  uint8_t shade;

  if (drv_button_is_triggered(BTN_R)) {
    if ((g.save_settings.BYTE & 0x78) != 0) {
      shade = ((g.save_settings.BYTE >> 3) & 0x0F) - 1;
      g.save_settings.BYTE = (uint8_t)((g.save_settings.BYTE & ~(0x0F << 3)) |
                                        ((shade & 0x0F) << 3));
      drv_sound_play(SND_CURSOR);
    }
    shade = (g.save_settings.BYTE >> 3) & 0x0F;
    drv_lcd_set_contrast(shade);
  }
  if (drv_button_is_triggered(BTN_L)) {
    if ((g.save_settings.BYTE & 0x78) < 0x48) {
      shade = ((g.save_settings.BYTE >> 3) & 0x0F) + 1;
      g.save_settings.BYTE = (uint8_t)((g.save_settings.BYTE & ~(0x0F << 3)) |
                                        ((shade & 0x0F) << 3));
      drv_sound_play(SND_CURSOR);
    }
    shade = (g.save_settings.BYTE >> 3) & 0x0F;
    drv_lcd_set_contrast(shade);
  }
}

// ROM: 0x6dfc  78.8%
void ui_handle_settings(void) {
  switch (g.viewstate.Y.BYTE) {
  case SETTINGS_MENU:   ui_handle_settings_main_page(); break;
  case SETTINGS_VOLUME: ui_handle_settings_volume();    break;
  case SETTINGS_SHADE:  ui_handle_settings_shade();     break;
  }
  if (drv_button_is_triggered(BTN_M)) {
    if (g.viewstate.Y.BYTE == SETTINGS_MENU) {
      /* R on the main settings menu enters the highlighted row. The within-
         page cursor (g.viewstate.Z) is 0 for g.sound_volume and 1 for shade; +1 maps
         to SETTINGS_VOLUME / SETTINGS_SHADE. */
      drv_sound_play(SND_CONFIRM);
      g.viewstate.Y.BYTE = g.viewstate.Z + 1;
    } else {
      /* R inside a sub-page commits the new setting and exits to home. */
      drv_sound_play(SND_CONFIRM);
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&g.save_totalSteps, 0x18);
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
  drv_eeprom_read_block(SPR_OFF(menu_hdg_settings), buf, 0x140);
  drv_lcd_blit(8, 0, buf, 0x50, 0x10);

  /* "Sound" row label. */
  drv_eeprom_read_block(SPR_OFF(label_sound), buf, 0xA0);
  drv_lcd_blit(8, 0x10, buf, 0x28, 0x10);

  /* "Shade" row label. */
  drv_eeprom_read_block(SPR_OFF(label_shade), buf, 0xA0);
  drv_lcd_blit(0x38, 0x10, buf, 0x28, 0x10);

  /* Arrows sheet — used below to pick the cursor variant for each subview. */
  drv_eeprom_read_block(SPR_OFF(arrows_8x8), buf, 0xC0);

  /* Within-page cursor x: 0 = g.sound_volume row, 0x30 = shade row. */
  cursor_x = g.viewstate.Z * 0x30;

  switch (g.viewstate.Y.BYTE) {
  case SETTINGS_MENU:
    animOff = ((g.ui_animationTick & 0x01) + 9) * 0x10;
    drv_lcd_blit(cursor_x, 0x14, buf + animOff, 8, 8);
    break;
  case SETTINGS_VOLUME:
    drv_lcd_blit(cursor_x, 0x14, buf + 0xB0, 8, 8);

    volVal = (g.save_settings.BYTE >> 1) & 0x03;
    animOff = ((g.ui_animationTick & 0x01) + 9) * 0x10;
    drv_lcd_blit((uint8_t)(volVal * 0x20), 0x2C, buf + animOff, 8, 8);

    /* Speaker icons sheet: none/low/high in sequence (3 * 0x60 = 0x120). */
    drv_eeprom_read_block(SPR_OFF(speaker_none), buf, 0x120);
    drv_lcd_blit(0x08, 0x28, buf, 0x18, 0x10);
    drv_lcd_blit(0x28, 0x28, buf + 0x60, 0x18, 0x10);
    drv_lcd_blit(0x48, 0x28, buf + 0xC0, 0x18, 0x10);
    break;
  case SETTINGS_SHADE:
    shVal = (g.save_settings.BYTE >> 3) & 0x0F;
    shadeOff = shVal * 8 + 8;
    animOff = ((g.ui_animationTick & 0x01) + 3) * 0x10;

    drv_lcd_blit((uint8_t)shadeOff, 0x20, buf + animOff, 8, 8);
    drv_lcd_blit((uint8_t)(g.viewstate.Z * 0x30), 0x14, buf + 0xB0, 8, 8);

    drv_eeprom_read_block(SPR_OFF(contrast_bar), buf, 0x20);
    for (i = 0; i < 0x0A; i++) {
      uint8_t xpos;
      xpos = (uint8_t)(i * 8) + 8;
      drv_lcd_blit(xpos, 0x28, buf, 8, 0x10);
    }
    break;
  }

  /* Return-symbol chevron in the left gutter. */
  drv_eeprom_read_block(SPR_OFF(menu_return_symbol), buf, 0x20);
  drv_lcd_blit(0, 0, buf, 8, 0x10);
  gfx_draw_battery_low(0x58, 0);
}
