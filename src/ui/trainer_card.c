#include "all_headers.h"

// ROM: 0x728a  85.4%
void ui_render_step_history_graph(void) {
  uint8_t *buf;
  uint8_t day;

  sys_init_heap();
  buf = sbrk(0x180);

  drv_eeprom_read_block(0x2350, buf, 0x100);
  drv_lcd_blit(0x20, 0x10, buf, 0x20, 0x20);

  day = irResultCode - 1;
  if (day > 7) {
    gfx_draw_battery_low(0x58, 0);
    return;
  }

  switch (day) {
  case 0:
    gfx_draw_text_box(0x30, 1, 0x0F, 0x01);
    break;
  case 1:
    gfx_draw_text_box(0x30, 4, 0x0F, 0x01);
    break;
  case 2:
    gfx_draw_text_box(0x20, 2, 0x0D, 0x00);
    gfx_draw_text_box(0x30, 3, 0x0E, 0x01);
    break;
  case 4:
    gfx_draw_text_box(0x20, 9, 0x0D, 0x00);
    gfx_draw_text_box(0x30, 0x0A, 0x0E, 0x01);
    break;
  case 5:
    gfx_draw_text_box(0x20, 7, 0x0D, 0x00);
    gfx_draw_text_box(0x30, 8, 0x0E, 0x01);
    break;
  case 6:
    gfx_draw_text_box(0x20, 0x0B, 0x0D, 0x00);
    gfx_draw_text_box(0x30, 0x0C, 0x0E, 0x01);
    break;
  case 3:
    gfx_draw_text_box(0x30, 0x15, 0x0F, 0x01);
    break;
  }

  gfx_draw_battery_low(0x58, 0);
}

// ROM: 0xb3c0  97.5%
void ui_reset_trainer_card_state(void) {
  gCurSubstateZ = 0;
  gCurSubstateY = 0;
}

// ROM: 0xb3cc  73.8%
void ui_handle_trainer_stats(void) {
  uint16_t addr;
  uint8_t flag;

  if (gCurSubstateZ == 0) {
    if (drv_button_is_triggered(0x04) != 0) {
      uint8_t y = gCurSubstateY;
      if (y == 0) {
        drv_sound_play(1);
        ui_clear_substate_y();
        ui_set_view(VIEW_MAIN_MENU);
        return;
      }
      gCurSubstateY = y - 1;
      drv_sound_play(2);
    }
    if (drv_button_is_triggered(0x08) != 0) {
      uint8_t y = gCurSubstateY;
      if (y < 7) {
        gCurSubstateY = y + 1;
        drv_sound_play(2);
      }
    }
  }

  if (drv_button_is_triggered(0x02) != 0) {
    if (gCurSubstateZ == 0) {
      if (totalSteps >= 0x98967F) {
        gCurSubstateZ = 1;
        drv_sound_play(0);
        return;
      }
      goto go_back;
    }
    if (gCurSubstateZ == 1) {
      addr = 0xCE84 + 4;
      flag = drv_eeprom_read_u8(addr);
      if (!(flag & 0x01)) {
        flag |= 0x01;
        drv_eeprom_write_u8(addr, flag);
        gCurSubstateZ = 2;
        drv_sound_play(7);
        return;
      }
      goto go_back;
    }
    if (gCurSubstateZ == 2) {
      goto go_back;
    }
  }
  return;

go_back:
  drv_sound_play(0);
  ui_reset_substate();
  ui_set_view(VIEW_HOME);
}

// The 0x280 EEPROM base is held in a `volatile uint16_t base` so ch38 keeps it
// in a register and computes each address via add (like ROM's `add.w r4`),
// instead of inlining the full immediate at every call site. This recovered
// the function from 0.0% — the prior "cannot-fix" verdict was wrong.
// ROM: 0xb48c  54.3%  saves: er3,er4,er5,er6
void ui_render_trainer_card_time(void) {
  uint8_t *buf;
  uint8_t hr;
  volatile uint16_t base = 0x280;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);

  drv_eeprom_read_block(0xA50 + base, buf, 0x140);
  drv_lcd_blit(8, 0, buf, 0x50, 0x10);

  drv_eeprom_read_block(0xF90 + base, buf, 0x40);
  drv_lcd_blit(0, 0x10, buf, 0x10, 0x10);

  drv_eeprom_read_block(0xFD0 + base, buf, 0x140);
  drv_lcd_blit(0x10, 0x10, buf, 0x50, 0x10);

  drv_eeprom_read_block(0x1110 + base, buf, 0x40);
  drv_lcd_blit(0, 0x20, buf, 0x10, 0x10);

  if (RamCache_settingsByte & 1) {
    drv_eeprom_read_block(0xC8FC, buf, 0x140);
  } else {
    drv_eeprom_read_block(0x907E, buf, 0x140);
  }
  drv_lcd_blit(0x10, 0x20, buf, 0x50, 0x10);

  drv_eeprom_read_block(0x338 + base, buf, 0xC0);
  drv_lcd_blit(0, 0, buf + 0x40, 8, 0x10);
  drv_lcd_blit(0x58, 0, buf + 0x20, 8, 0x10);

  drv_eeprom_read_block(0x11F0 + base, buf, 0x80);
  drv_lcd_blit(0, 0x30, buf, 0x20, 0x10);

  drv_eeprom_read_block(base, buf, 0x140);

  hr = rtcHour;
  drv_lcd_blit(0x20, 0x30, buf + ((((uint16_t)hr >> 4) & 7) * 0x20), 8, 0x10);
  drv_lcd_blit(0x28, 0x30, buf + ((uint16_t)(hr & 0xF) * 0x20), 8, 0x10);

  hr = rtcMin;
  drv_lcd_blit(0x38, 0x30, buf + ((((uint16_t)hr >> 4) & 7) * 0x20), 8, 0x10);
  drv_lcd_blit(0x40, 0x30, buf + ((uint16_t)(hr & 0xF) * 0x20), 8, 0x10);

  hr = rtcSec;
  drv_lcd_blit(0x50, 0x30, buf + ((((uint16_t)hr >> 4) & 7) * 0x20), 8, 0x10);
  drv_lcd_blit(0x58, 0x30, buf + ((uint16_t)(hr & 0xF) * 0x20), 8, 0x10);

  drv_eeprom_read_block(0x280 + 0x140, buf, 0x20);
  drv_lcd_blit(0x30, 0x30, buf, 8, 0x10);
  drv_lcd_blit(0x48, 0x30, buf, 8, 0x10);
}

// 0x280 EEPROM base held in `volatile uint16_t base` to force register reuse
// (see ui_render_trainer_card_time). Lifted 47.8% → 75.2%.
// ROM: 0xb682  75.2%  saves: er2,r3,r4,er5,er6
void ui_render_daily_step_history(void) {
  uint8_t *buf;
  uint32_t step_data;
  uint16_t day_addr;
  uint16_t day_idx;
  volatile uint16_t base = 0x280;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);

  drv_eeprom_read_block(0x338 + base, buf, 0xC0);
  drv_lcd_blit(0, 0, buf, 8, 0x10);

  if (gCurSubstateY < 7) {
    drv_lcd_blit(0x58, 0, buf + 0x20, 8, 0x10);
  }

  drv_eeprom_read_block(0x1270 + base, buf, 0xA0);
  drv_lcd_blit(0x28, 0, buf, 0x28, 0x10);

  drv_eeprom_read_block(0x1310 + base, buf, 0x100);
  drv_lcd_blit(0, 0x20, buf, 0x40, 0x10);

  drv_eeprom_read_block(0x1150 + base, buf, 0xA0);
  drv_lcd_blit(0x38, 0x10, buf, 0x28, 0x10);
  drv_lcd_blit(0x38, 0x30, buf, 0x28, 0x10);

  drv_eeprom_read_block(0x160 + base, buf, 0x20);
  drv_lcd_blit(0x18, 0, buf, 8, 0x10);

  drv_eeprom_read_block(base, buf, 0x140);
  drv_lcd_blit(0x20, 0, buf + (uint16_t)gCurSubstateY * 0x20, 8, 0x10);

  day_idx = (uint16_t)gCurSubstateY - 1;
  day_addr = 0xCEF0 + day_idx * 4;
  drv_eeprom_read_block(day_addr, &step_data, 4);
  gfx_draw_numeric_value(0x30, 0x10, (uint32_t)step_data, 0);

  gfx_draw_numeric_value(0x58, 0x20, (uint32_t)dayCounter, 0);
  gfx_draw_numeric_value(0x30, 0x30, totalSteps, 0);
}

// ROM: 0xb7ee  64.4%  saves: er2,r3,r4,er5,r6
void ui_render_step_goal_reached(void) {
  uint8_t *buf;
  uint8_t i;
  volatile uint16_t base;

  base = 0x280;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);
  drv_eeprom_read_block(base, buf, 0x140);

  for (i = 0; i < 7; i++) {
    uint16_t x = (uint16_t)i * 8;
    drv_lcd_blit((uint8_t)x, 8, buf + 0x120, 8, 0x10);
  }

  drv_eeprom_read_block(0x1150 + base, buf, 0xA0);
  drv_lcd_blit(0x38, 0x08, buf, 0x28, 0x10);

  drv_eeprom_read_block(0x2210 + base, buf, 0xA0);
  drv_lcd_blit(0x38, 0x28, buf, 0x28, 0x10);

  gfx_draw_numeric_value(0x30, 0x28, RamCache_STEP_COUNT_maybe, 0);
  gfx_draw_text_box(0x18, 0x43, 0x0F, 0x01);
}

// ROM: 0xb8a6  84.2%  saves: r2,r6
void ui_render_step_goal_reward(void) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xC0);
  drv_eeprom_read_block(0x1910, buf, 0xC0);
  drv_lcd_blit(0x20, 0x04, buf, 0x20, 0x18);
  gfx_draw_text_box(0x20, 0x42, 0x0D, 0x00);
  gfx_draw_text_box(0x30, 0x0F, 0x0E, 0x01);
}

// ROM: 0xb8f2  95.8%
void ui_render_trainer_card(void) {
  uint8_t z;

  z = gCurSubstateZ;
  if (z == 0)
    goto state0;
  if (z == 1)
    goto state1;
  if (z != 2)
    goto draw_battery;
  goto state2;

state0:
  if (gCurSubstateY)
    goto subY_nonzero;
  ui_render_trainer_card_time();
  goto draw_battery;
subY_nonzero:
  ui_render_daily_step_history();
  goto draw_battery;

state1:
  ui_render_step_goal_reached();
  goto draw_battery;

state2:
  ui_render_step_goal_reward();

draw_battery:
  gfx_draw_battery_low(0x58, 0);
}
