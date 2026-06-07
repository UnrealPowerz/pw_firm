#include "all_headers.h"

/*
 * Trainer Card view.
 *
 * Page navigation (g.viewstate.Z + g.viewstate.Y.BYTE):
 *
 *   TC_PAGE_DEFAULT (z=0):
 *     y=0 ............... clock face          (ui_render_trainer_card_time)
 *     y=1..7 ............ daily step history  (ui_render_daily_step_history)
 *     BTN_R up    -> y--, or exit to MAIN_MENU if y==0
 *     BTN_L down  -> y++ (capped at 7)
 *     BTN_M       -> if g.save_totalSteps >= 10_000_000 jump to TC_PAGE_GOAL_REACHED,
 *                    else exit to home
 *
 *   TC_PAGE_GOAL_REACHED (z=1):
 *     "Good job!" 10-million-step milestone banner (ui_render_step_goal_reached).
 *     BTN_M       -> if reward-claimed bit at EEPROM 0xCE88 b0 unset, set it
 *                    and advance to TC_PAGE_REWARD; else exit to home.
 *
 *   TC_PAGE_REWARD (z=2):
 *     "Reward / received!" banner (ui_render_step_goal_reward).
 *     BTN_M       -> exit to home.
 */

enum trainer_card_page {
    TC_PAGE_DEFAULT      = 0,
    TC_PAGE_GOAL_REACHED = 1,
    TC_PAGE_REWARD       = 2
};

// ROM: 0xb3c0  100.0%
void ui_reset_trainer_card_state(void) {
  g.viewstate.Z = 0;
  g.viewstate.Y.BYTE = 0;
}

#define STEP_GOAL 0x98967F   /* 9,999,999 — one step short of 10M, the cutoff
                                for awarding the milestone reward */

// ROM: 0xb3cc  75.2%
void ui_handle_trainer_card(void) {
  uint16_t reward_flag_addr;
  uint8_t reward_flags;

  /* TC_PAGE_DEFAULT: navigate the 8 sub-views with BTN_R (up) / BTN_L (down). */
  if (g.viewstate.Z == TC_PAGE_DEFAULT) {
    if (drv_button_is_triggered(BTN_R) != 0) {
      uint8_t cursor = g.viewstate.Y.BYTE;
      if (cursor == 0) {
        /* Already on clock — leave the trainer card entirely. */
        drv_sound_play(SND_BACK);
        ui_clear_substate_y();
        ui_set_view(VIEW_MAIN_MENU);
        return;
      }
      g.viewstate.Y.BYTE = cursor - 1;
      drv_sound_play(SND_CURSOR);
    }
    if (drv_button_is_triggered(BTN_L) != 0) {
      uint8_t cursor = g.viewstate.Y.BYTE;
      if (cursor < 7) {
        g.viewstate.Y.BYTE = cursor + 1;
        drv_sound_play(SND_CURSOR);
      }
    }
  }

  /* BTN_M advances through the milestone sequence (or exits if at the end). */
  if (drv_button_is_triggered(BTN_M) != 0) {
    if (g.viewstate.Z == TC_PAGE_DEFAULT) {
      if (g.save_totalSteps >= STEP_GOAL) {
        g.viewstate.Z = TC_PAGE_GOAL_REACHED;
        drv_sound_play(SND_CONFIRM);
        return;
      }
      goto exit_to_home;
    }
    if (g.viewstate.Z == TC_PAGE_GOAL_REACHED) {
      reward_flag_addr = 0xCE84 + 4;
      reward_flags = drv_eeprom_read_u8(reward_flag_addr);
      if (!(reward_flags & 0x01)) {
        reward_flags |= 0x01;
        drv_eeprom_write_u8(reward_flag_addr, reward_flags);
        g.viewstate.Z = TC_PAGE_REWARD;
        drv_sound_play(SND_FANFARE);
        return;
      }
      goto exit_to_home;
    }
    if (g.viewstate.Z == TC_PAGE_REWARD) {
      goto exit_to_home;
    }
  }
  return;

exit_to_home:
  drv_sound_play(SND_CONFIRM);
  ui_reset_substate();
  ui_set_view(VIEW_HOME);
}

// The 0x280 EEPROM base is held in a `volatile uint16_t base` so ch38 keeps it
// in a register and computes each address via add (like ROM's `add.w r4`),
// instead of inlining the full immediate at every call site. This recovered
// the function from 0.0% — the prior "cannot-fix" verdict was wrong.
// ROM: 0xb48c  54.4%  saves: er3,er4,er5,er6
void ui_render_trainer_card_time(void) {
  uint8_t *buf;
  uint8_t time_part;
  volatile uint16_t base = EEP_SPRITE_BASE;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);

  /* "TRAINER CARD" heading bar + per-screen labels. */
  drv_eeprom_read_block(EEP_MENU_HDG_TRAINER + base, buf, 0x140);
  drv_lcd_blit(8, 0, buf, 0x50, 0x10);

  drv_eeprom_read_block(EEP_TRAINER_PERSON_ICON + base, buf, 0x40);
  drv_lcd_blit(0, 0x10, buf, 0x10, 0x10);

  drv_eeprom_read_block(EEP_TRAINER_NAME_IMG + base, buf, 0x140);
  drv_lcd_blit(0x10, 0x10, buf, 0x50, 0x10);

  drv_eeprom_read_block(EEP_TRAINER_ROUTE_SMALL + base, buf, 0x40);
  drv_lcd_blit(0, 0x20, buf, 0x10, 0x10);

  /* Date label — different art for co-op vs solo (special route vs normal). */
  if (g.save_settings.BYTE & 1) {
    drv_eeprom_read_block(EEP_ABS_SPECIAL_ROUTE_HOME, buf, 0x140);
  } else {
    drv_eeprom_read_block(EEP_ABS_HOME_ROUTE_NAME, buf, 0x140);
  }
  drv_lcd_blit(0x10, 0x20, buf, 0x50, 0x10);

  /* Reads the left/right/return arrow strip; blit `+0x40` picks the return
   * symbol (left gutter), `+0x20` picks the right arrow (right gutter). */
  drv_eeprom_read_block(EEP_MENU_ARROW_LEFT + base, buf, 0xC0);
  drv_lcd_blit(0, 0, buf + 0x40, 8, 0x10);
  drv_lcd_blit(0x58, 0, buf + 0x20, 8, 0x10);

  drv_eeprom_read_block(EEP_LABEL_TIME + base, buf, 0x80);
  drv_lcd_blit(0, 0x30, buf, 0x20, 0x10);

  /* Digit sheet — 16 glyphs of 0x20 bytes each at the sprite-base anchor. */
  drv_eeprom_read_block(base, buf, 0x140);

  time_part = g.rtc_hours;
  drv_lcd_blit(0x20, 0x30, buf + ((((uint16_t)time_part >> 4) & 7) * 0x20), 8, 0x10);
  drv_lcd_blit(0x28, 0x30, buf + ((uint16_t)(time_part & 0xF) * 0x20), 8, 0x10);

  time_part = g.rtc_minutes;
  drv_lcd_blit(0x38, 0x30, buf + ((((uint16_t)time_part >> 4) & 7) * 0x20), 8, 0x10);
  drv_lcd_blit(0x40, 0x30, buf + ((uint16_t)(time_part & 0xF) * 0x20), 8, 0x10);

  time_part = g.rtc_seconds;
  drv_lcd_blit(0x50, 0x30, buf + ((((uint16_t)time_part >> 4) & 7) * 0x20), 8, 0x10);
  drv_lcd_blit(0x58, 0x30, buf + ((uint16_t)(time_part & 0xF) * 0x20), 8, 0x10);

  /* Colon glyph (10th entry in the digit sheet). */
  drv_eeprom_read_block(EEP_SPRITE_BASE + EEP_DIGITS_COLON, buf, 0x20);
  drv_lcd_blit(0x30, 0x30, buf, 8, 0x10);
  drv_lcd_blit(0x48, 0x30, buf, 8, 0x10);
}

// 0x280 EEPROM base held in `volatile uint16_t base` to force register reuse
// (see ui_render_trainer_card_time). Lifted 47.8% → 75.2%.
// ROM: 0xb682  75.3%  saves: er2,r3,r4,er5,er6
void ui_render_daily_step_history(void) {
  uint8_t *buf;
  uint32_t day_steps;
  uint16_t day_addr;
  uint16_t days_ago;
  volatile uint16_t base = EEP_SPRITE_BASE;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);

  /* Left gutter chevron (always); right chevron only for y < 7 (not at end).
   * Loads the left-arrow + right-arrow + return strip; first 8x16 glyph is
   * the left arrow, +0x20 picks the right arrow. */
  drv_eeprom_read_block(EEP_MENU_ARROW_LEFT + base, buf, 0xC0);
  drv_lcd_blit(0, 0, buf, 8, 0x10);
  if (g.viewstate.Y.BYTE < 7) {
    drv_lcd_blit(0x58, 0, buf + 0x20, 8, 0x10);
  }

  /* "Days" header label. */
  drv_eeprom_read_block(EEP_LABEL_DAYS + base, buf, 0xA0);
  drv_lcd_blit(0x28, 0, buf, 0x28, 0x10);

  /* "Total days:" bottom label. */
  drv_eeprom_read_block(EEP_LABEL_TOTAL_DAYS + base, buf, 0x100);
  drv_lcd_blit(0, 0x20, buf, 0x40, 0x10);

  /* "Steps" label, drawn twice (middle and bottom rows). */
  drv_eeprom_read_block(EEP_LABEL_STEPS + base, buf, 0xA0);
  drv_lcd_blit(0x38, 0x10, buf, 0x28, 0x10);
  drv_lcd_blit(0x38, 0x30, buf, 0x28, 0x10);

  /* "-" glyph from the digit sheet (entry 11: '0'..'9' + ':' + '-'). */
  drv_eeprom_read_block(EEP_DIGITS + 0x160 + base, buf, 0x20);
  drv_lcd_blit(0x18, 0, buf, 8, 0x10);

  /* Day digit (picked from the 16-glyph sheet by g.viewstate.Y.BYTE). */
  drv_eeprom_read_block(base, buf, 0x140);
  drv_lcd_blit(0x20, 0, buf + (uint16_t)g.viewstate.Y.BYTE * 0x20, 8, 0x10);

  /* Step count for "today − g.viewstate.Y.BYTE". Each day uses 4 bytes at 0xCEF0. */
  days_ago = (uint16_t)g.viewstate.Y.BYTE - 1;
  day_addr = 0xCEF0 + days_ago * 4;
  drv_eeprom_read_block(day_addr, &day_steps, 4);
  gfx_draw_numeric_value(0x30, 0x10, (uint32_t)day_steps, 0);

  gfx_draw_numeric_value(0x58, 0x20, (uint32_t)g.save_dayCounter, 0);
  gfx_draw_numeric_value(0x30, 0x30, g.save_totalSteps, 0);
}

// ROM: 0xb7ee  72.8%  saves: er2,r3,r4,er5,r6
void ui_render_step_goal_reached(void) {
  uint8_t *buf;
  uint8_t i;
  volatile uint16_t base;

  base = EEP_SPRITE_BASE;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0x140);
  drv_eeprom_read_block(base, buf, 0x140);

  /* Row of 7 "1" digits across the top — the literal "10000000". */
  for (i = 0; i < 7; i++) {
    uint16_t x = (uint16_t)i * 8;
    drv_lcd_blit((uint8_t)x, 8, buf + 0x120, 8, 0x10);
  }

  drv_eeprom_read_block(EEP_LABEL_STEPS + base, buf, 0xA0);
  drv_lcd_blit(0x38, 0x08, buf, 0x28, 0x10);

  drv_eeprom_read_block(EEP_HOURS_FRAME + base, buf, 0xA0);
  drv_lcd_blit(0x38, 0x28, buf, 0x28, 0x10);

  gfx_draw_numeric_value(0x30, 0x28, g.save_walkStepCount, 0);
  gfx_draw_text_box(0x18, TEXT_GOOD_JOB, TEXT_BOX_FULL, TEXT_BOX_BLINK);
}

// ROM: 0xb8a6  84.3%  saves: r2,r6
void ui_render_step_goal_reward(void) {
  uint8_t *buf;

  sys_init_heap();
  buf = (uint8_t *)sbrk(0xC0);
  /* Gift/reward sprite. */
  drv_eeprom_read_block(EEP_ABS_TREASURE_CHEST, buf, 0xC0);
  drv_lcd_blit(0x20, 0x04, buf, 0x20, 0x18);
  gfx_draw_text_box(0x20, TEXT_REWARD, TEXT_BOX_NO_SHADOW, TEXT_BOX_STATIC);
  gfx_draw_text_box(0x30, TEXT_RECEIVED, TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
}

// ROM: 0xb8f2  96.4%
void ui_render_trainer_card(void) {
  uint8_t page = g.viewstate.Z;

  if (page == TC_PAGE_DEFAULT)
    goto page_default;
  if (page == TC_PAGE_GOAL_REACHED)
    goto page_goal_reached;
  if (page != TC_PAGE_REWARD)
    goto draw_battery;
  goto page_reward;

page_default:
  if (g.viewstate.Y.BYTE)
    goto day_history;
  ui_render_trainer_card_time();
  goto draw_battery;
day_history:
  ui_render_daily_step_history();
  goto draw_battery;

page_goal_reached:
  ui_render_step_goal_reached();
  goto draw_battery;

page_reward:
  ui_render_step_goal_reward();

draw_battery:
  gfx_draw_battery_low(0x58, 0);
}
