#include "all_headers.h"

// ROM: 0x728a  85.4%
void ui_render_step_history_graph(void) {
  uint8_t *buf;
  uint8_t day;

  sys_init_heap();
  buf = sbrk(0x180);

  drv_eeprom_read_block(0x2350, buf, 0x100);
  drv_lcd_blit(0x20, 0x20, buf, 0x20, 0x10);

  day = DAT_f7ad - 1;
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
      if (gCurSubstateY == 0) {
        drv_sound_play(1);
        ui_clear_substate_y();
        ui_set_view(1);
        return;
      }
      gCurSubstateY--;
      drv_sound_play(2);
    }
    if (drv_button_is_triggered(0x08) != 0) {
      if (gCurSubstateY < 7) {
        gCurSubstateY++;
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
  ui_set_view(0);
}

// ROM: 0xb48c  0.0%  saves: er3,er4,er5,er6
void ui_render_trainer_card_time(void) {
  /* Register usage hints:
   * R3: @drv_eeprom_read_block
   * R4: Constant H'280
   * R5: @drv_lcd_blit   E5: Constant H'40
   * R6: buf                   E6: Constant H'140
   */
  void (*ep_read)(uint16_t, void *, uint16_t);
  uint16_t base_addr;
  void (*draw_img)(void *, uint16_t, uint16_t, uint8_t, uint8_t);
  uint16_t sz_small;
  uint16_t sz_large;
  uint8_t *buf;

  ep_read = drv_eeprom_read_block;
  sz_small = 0x40;
  sz_large = 0x140;
  base_addr = 0x280;
  draw_img = (void (*)(void *, uint16_t, uint16_t, uint8_t,
                       uint8_t))drv_lcd_blit;

  sys_init_heap();
  buf = (uint8_t *)sbrk(sz_large);

  /* Page 1: Labels */
  ep_read(0xA50 + base_addr, buf, sz_large);
  draw_img(buf, 0x50, 0x10, 8, 0);

  /* Page 2: Step count label */
  ep_read(0xF90 + base_addr, buf, sz_small);
  draw_img(buf, 0x10, 0x10, 0, 0x10);

  /* Page 3: Step count value */
  ep_read(0xFD0 + base_addr, buf, sz_large);
  draw_img(buf, 0x50, 0x10, 0x10, 0x10);

  /* Page 4: Watts label */
  ep_read(0x1110 + base_addr, buf, sz_small);
  draw_img(buf, 0x10, 0x10, 0, 0x20);

  /* Gender */
  if ((RamCache_settingsByte & 1)) {
    ep_read(0xC8FC, buf, sz_large);
  } else {
    ep_read(0x907E, buf, sz_large);
  }
  draw_img(buf, 0x50, 0x10, 0x10, 0x20);

  /* Arrows */
  ep_read(0x338 + base_addr, buf, 0xC0);
  draw_img(buf + sz_small, 8, 0x10, 0, 0);
  draw_img(buf + 0x20, 8, 0x10, 0x58, 0);

  /* Time label */
  ep_read(0x11F0 + base_addr, buf, 0x80);
  draw_img(buf, 0x20, 0x10, 0, 0x30);

  /* Digits */
  ep_read(base_addr, buf, sz_large);

  /* Time values */
  {
    uint8_t hr = DAT_f7a6;
    draw_img(buf + ((((uint16_t)hr >> 4) & 7) * 0x20), 8, 0x10, 0x20, 0x30);
    draw_img(buf + ((uint16_t)(hr & 0xF) * 0x20), 8, 0x10, 0x28, 0x30);

    hr = DAT_f7a5; /* mn */
    draw_img(buf + ((((uint16_t)hr >> 4) & 7) * 0x20), 8, 0x10, 0x38, 0x30);
    draw_img(buf + ((uint16_t)(hr & 0xF) * 0x20), 8, 0x10, 0x40, 0x30);

    hr = DAT_f7a4; /* sc */
    draw_img(buf + ((((uint16_t)hr >> 4) & 7) * 0x20), 8, 0x10, 0x50, 0x30);
    draw_img(buf + ((uint16_t)(hr & 0xF) * 0x20), 8, 0x10, 0x58, 0x30);
  }

  /* Colons */
  ep_read(base_addr + sz_large, buf, 0x20);
  draw_img(buf, 8, 0x10, 0x30, 0x30);
  draw_img(buf, 8, 0x10, 0x48, 0x30);
}

// ROM: 0xb682  8.0%  saves: er2,r3,r4,er5,er6
#pragma option speed=register  /* pragma:auto */
void ui_render_daily_step_history(void) {
  void (*ep_read)(uint16_t, void *, uint16_t);
  void (*draw_img)(void *, uint8_t, uint8_t, uint8_t, uint8_t);
  uint16_t base;
  uint8_t *buf;
  uint32_t step_data;
  uint16_t addr;
  uint16_t idx;

  ep_read = drv_eeprom_read_block;
  draw_img =
      (void (*)(void *, uint8_t, uint8_t, uint8_t, uint8_t))drv_lcd_blit;
  base = 0x280;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);

  addr = 0x338 + base;
  ep_read(addr, buf, 0xC0);
  draw_img(buf, 8, 0x10, 0, 0);

  if (gCurSubstateY < 7) {
    draw_img(buf + 0x20, 8, 0x10, 0x58, 0);
  }

  addr = 0x1270 + base;
  ep_read(addr, buf, 0xA0);
  draw_img(buf, 0x28, 0x10, 0x28, 0);

  addr = 0x1310 + base;
  ep_read(addr, buf, 0x100);
  draw_img(buf, 0x40, 0x10, 0, 0x20);

  addr = 0x1150 + base;
  ep_read(addr, buf, 0xA0);
  draw_img(buf, 0x28, 0x10, 0x38, 0x10);
  draw_img(buf, 0x28, 0x10, 0x38, 0x30);

  addr = 0x160 + base;
  ep_read(addr, buf, 0x20);
  draw_img(buf, 8, 0x10, 0x18, 0);

  ep_read(base, buf, 0x140);

  idx = (uint16_t)gCurSubstateY * 0x20;
  draw_img(buf + idx, 8, 0x10, 0x20, 0);

  {
    uint16_t day_idx = (uint16_t)gCurSubstateY - 1;
    uint16_t day_addr = 0xCEF0 + day_idx * 4;
    ep_read(day_addr, &step_data, 4);
    gfx_draw_numeric_value(0x30, 0x10, (uint32_t)step_data, 0);
  }

  gfx_draw_numeric_value(0x58, 0x20, (uint32_t)DAT_f78c, 0);
  gfx_draw_numeric_value(0x30, 0x30, totalSteps, 0);
}

// ROM: 0xb7ee  68.7%  saves: er2,r3,r4,er5,r6
#pragma option speed=register  /* pragma:auto */
void ui_render_step_goal_reached(void) {
  uint8_t *buf;
  uint8_t i;
  uint16_t base;

  base = 0x280;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);
  drv_eeprom_read_block(base, buf, 0x140);

  for (i = 0; i < 7; i++) {
    uint16_t x = (uint16_t)i * 8;
    drv_lcd_blit(8, 8, buf + 0x120, (uint8_t)x, 8);
  }

  drv_eeprom_read_block(0x1150 + base, buf, 0xA0);
  drv_lcd_blit(0x28, 0x10, buf, 0x38, 8);

  drv_eeprom_read_block(0x2210 + base, buf, 0xA0);
  drv_lcd_blit(0x28, 0x10, buf, 0x38, 0x28);

  gfx_draw_numeric_value(0x30, 0x28, RamCache_STEP_COUNT_maybe, 0);
  gfx_draw_text_box(0x18, 0x43, 0x0F, 0x01);
}

// ROM: 0xb8a6  84.2%  saves: r2,r6
void ui_render_step_goal_reward(void) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xC0);
  drv_eeprom_read_block(0x1910, buf, 0xC0);
  drv_lcd_blit(0x20, 0x18, buf, 0x10, 4);
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
