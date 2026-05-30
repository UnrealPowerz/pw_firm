#include "all_headers.h"

// ROM: 0x30a6  47.9%
#pragma option noregexpansion /* pragma:auto */
void ui_render_battle(void) {
  uint8_t *sprite_buf, *e2_buf, *e3_buf;
  uint16_t addr;
  uint8_t i, flip;
  void (*readEeprom)(uint16_t, void *, uint16_t) = drv_eeprom_read_block;
  void (*drawToScreen)(uint8_t, uint8_t, void *, uint8_t, uint8_t) =
      drv_lcd_blit;

  sys_init_heap();
  sprite_buf = (uint8_t *)sbrk(0x300);
  e3_buf = (uint8_t *)sbrk(0x18);
  e2_buf = (uint8_t *)sbrk(0x08);

  /* Background drawing */
  addr = (uint16_t)(animTick & 1) * 0xC0 + 0x91BE;
  readEeprom(addr, sprite_buf, 0xC0);
  drawToScreen((uint8_t)accelYPos, 8, sprite_buf, 0x20, 0x18);

  /* Digit drawing loop A */
  readEeprom(0x280 + 0x1DB0, sprite_buf, 0x10);
  for (i = 0; i < gCurSubstateA; i++) {
    drawToScreen((uint8_t)(0x38 + (i << 3)), 0, sprite_buf, 8, 8);
  }

  /* Digit drawing loop D (conditional) */
  if (DAT_f7d8 & 0x01) {
    for (i = 0; i < DAT_f7d1; i++) {
      drawToScreen((uint8_t)(0x08 + (i << 3)), 0x18, sprite_buf, 8, 8);
    }
  }

  /* Pokemon sprite drawing */
  if (gCurSubstateZ <= 10) {
    uint8_t s_y = gCurSubstateY;
    if (s_y < 4) {
      /* Wild Pokemon pattern */
      readEeprom(0x8F52 + (uint16_t)(s_y - 1) * 16, sprite_buf, 0x10);
      flip = sprite_buf[0x0E] & 0x01;
      addr = 0x9A7E + (uint16_t)(s_y - 1) * 0x180 +
             (uint16_t)(animTick & 1) * 0xC0;
    } else {
      /* Peer Pokemon pattern */
      readEeprom(0xBF08, sprite_buf, 0x10);
      flip = sprite_buf[0x0E] & 0x01;
      addr = 0xBF7C + (uint16_t)(animTick & 1) * 0xC0;
    }

    readEeprom(addr, sprite_buf, 0x180);
    if (!flip) {
      gfx_flip_horiz(0x20, 0x18, sprite_buf);
    }

    if (gCurSubstateZ == 10) {
      uint8_t x_offset = 0x2C - (accelXPos << 2);
      uint16_t val = fftTwiddleTable[x_offset];
      uint8_t y_screen = 0x14 - (uint8_t)(val >> 8);

      readEeprom(0x280 + 0x1E0, e3_buf, 0x10);
      readEeprom(0x280 + 0x200, e2_buf, 0x08);

      drawToScreen((uint8_t)x_offset, y_screen, e3_buf, 8, 8);
      gfx_alpha_blend(sprite_buf, 0x20, 0x18, e2_buf, e3_buf,
                      x_offset - DAT_f7d5, y_screen, 0x08);
      drawToScreen((uint8_t)DAT_f7d5, 0, sprite_buf, 0x20, 0x18);
    } else {
      gfx_draw_sprite_simple((uint8_t)DAT_f7d5, 0, 0x20, 0x18, sprite_buf);
    }
  }

  if (gCurSubstateZ <= 0x11) {
    switch (gCurSubstateZ) {
    case 0:
      if (accelXPos < 3) {
        uint8_t hh = (uint8_t)((3 - accelXPos) * 8);
        gfx_fill_rect(0, 0, 0x60, hh, 3);
        gfx_fill_rect(0, (uint8_t)(0x40 - hh), 0x60, hh, 3);
      }
      break;
    case 1:
      if (accelXPos >= dowsing_item_pos) {
        if (!drv_sound_is_playing()) {
          if (gCurSubstateY < 4) {
            gfx_draw_special_poke_name(0, 0x20, 5);
          } else {
            gfx_draw_route_pokemon_name(0x00, 0x20,
                                        (gCurSubstateY + 0xFF) & 0xFF, 0x05);
          }
          gfx_draw_text_box(0x30, 0x1F, 0x0E, 0x01);
          goto switch_default;
        }
      }
      break;
    case 2:
      readEeprom(0x1DD0 + (animTick & 1) * 0xC0, sprite_buf, 0x300);
      drawToScreen(0x00, 0x20, sprite_buf, 0x60, 0x20);
      goto switch_default;
    case 3: {
      uint8_t move_type = (DAT_f7d8 >> 3) & 0x03;
      if (move_type == 0) {
        if (accelXPos == 4) {
          drv_sound_play(0x0B);
          readEeprom(0x1BF0 + (animTick & 1) * 0xC0, sprite_buf, 0x1E0);
          drawToScreen(0x28, 0x00, sprite_buf, 0x10, 0x20);
        } else if (accelXPos < 4) {
          gfx_draw_own_pokemon_name(0x00, 0x20, 5);
          goto switch_default;
        }
      } else if (move_type == 1) {
        if (accelXPos == 4)
          drv_sound_play(0x0C);
        if (accelXPos < 4) {
          if (gCurSubstateY < 4) {
            gfx_draw_special_poke_name(0, 0x20, 5);
          } else {
            gfx_draw_route_pokemon_name(0x00, 0x20,
                                        (gCurSubstateY + 0xFF) & 0xFF, 0x05);
          }
          gfx_draw_text_box(0x30, 0x24, 0x0E, 0x00);
          goto switch_default;
        }
      } else if (move_type == 2) {
        if (accelXPos == 4) {
          drv_sound_play(0x0D);
          readEeprom(0x1C70 + (animTick & 1) * 0xC0, sprite_buf, 0x1E0);
          drawToScreen(0x28, 0x00, sprite_buf, 0x10, 0x20);
        } else if (accelXPos < 4) {
          gfx_draw_text_box(0x20, 0x25, 0x0D, 0x00);
          gfx_draw_text_box(0x30, 0x26, 0x0E, 0x00);
          goto switch_default;
        }
      }
      break;
    }
    case 4: {
      uint8_t m_type = (DAT_f7d8 >> 1) & 0x03;
      if (m_type == 0) {
        if (accelXPos == 4) {
          drv_sound_play(0x0B);
          readEeprom(0x1BF0 + (animTick & 1) * 0xC0, sprite_buf, 0x1E0);
          drawToScreen(0x28, 0x00, sprite_buf, 0x10, 0x20);
        } else if (accelXPos < 4) {
          if (gCurSubstateY < 4) {
            gfx_draw_special_poke_name(0, 0x20, 5);
          } else {
            gfx_draw_route_pokemon_name(0x00, 0x20,
                                        (gCurSubstateY + 0xFF) & 0xFF, 0x05);
          }
          gfx_draw_text_box(0x30, 0x23, 0x0E, 0x00);
          goto switch_default;
        }
      } else if (m_type == 1) {
        if (accelXPos == 4)
          drv_sound_play(0x0C);
        if (accelXPos < 4) {
          gfx_draw_own_pokemon_name(0, 0x20, 5);
          gfx_draw_text_box(0x30, 0x24, 0x0E, 0x00);
          goto switch_default;
        }
      }
      break;
    }
    case 5:
      if (gCurSubstateY < 4) {
        gfx_draw_special_poke_name(0, 0x20, 5);
      } else {
        gfx_draw_route_pokemon_name(0x00, 0x20, (gCurSubstateY + 0xFF) & 0xFF,
                                    0x05);
      }
      gfx_draw_text_box(0x30, 0x22, 0x0E, 0x00);
      goto switch_default;
    case 6:
      gfx_draw_value_with_icon(2, 0x20, 0x0D, (uint16_t)accelZPos);
      gfx_draw_text_box(0x30, 0x2A, 0x0E, 0x01);
      goto switch_default;
    case 7:
      if (accelXPos > 3) {
        if (gCurSubstateY < 4) {
          gfx_draw_special_poke_name(0, 0x20, 5);
        } else {
          gfx_draw_route_pokemon_name(0x00, 0x20, (gCurSubstateY + 0xFF) & 0xFF,
                                      0x05);
        }
        gfx_draw_text_box(0x30, 0x21, 0x0E, 0x01);
        goto switch_default;
      }
      break;
    case 8:
      gfx_draw_text_box(0x30, 0x29, 0x0F, 0x00);
      goto switch_default;
    case 9:
      gfx_draw_text_box(0x30, 0x28, 0x0F, 0x00);
      goto switch_default;
    case 10:
      gfx_draw_text_box(0x30, 0x27, 0x0F, 0x00);
      goto switch_default;
    case 11:
      readEeprom((uint16_t)&gfx_draw_present_icon + 0x280, sprite_buf, 0xC0);
      drawToScreen(0x08, 0x00, sprite_buf, 0x20, 0x18);
      break;

    case 12:
    case 14:
      readEeprom(0x460, sprite_buf, 0x10);
      drawToScreen(20, 0x0C, sprite_buf, 8, 8);
      break;

    case 13: {
      uint8_t bx = animTick & 3;
      uint8_t dx;
      if (bx == 0)
        dx = 0x13;
      else if (bx == 2)
        dx = 0x15;
      else
        dx = 0x14;
      readEeprom(0x460, sprite_buf, 0x10);
      drawToScreen(dx, 0x0C, sprite_buf, 8, 8);
      break;
    }

    case 15:
      readEeprom(0x460, sprite_buf, 0x10);
      drawToScreen(20, 0x0C, sprite_buf, 8, 8);
      readEeprom(0x2040, sprite_buf, 0x10);
      drawToScreen((uint8_t)(12 - accelXPos),
                   (uint8_t)(10 - 2 * accelXPos), sprite_buf, 8, 8);
      drawToScreen((uint8_t)(28 + accelXPos),
                   (uint8_t)(12 - 2 * accelXPos), sprite_buf, 8, 8);
      break;

    case 16:
      readEeprom(0x460, sprite_buf, 0x10);
      drawToScreen(20, 0x0C, sprite_buf, 8, 8);
      if (gCurSubstateY >= 4) {
        gfx_draw_route_pokemon_name(0x00, 0x20, (uint8_t)(gCurSubstateY - 1),
                                    0x05);
      } else {
        gfx_draw_special_poke_name(0x00, 0x20, 5);
      }
      gfx_draw_text_box(0x30, 0x20, 0x0E, 0x01);
      break;

    case 17:
      readEeprom((uint16_t)&gfx_draw_present_icon + 0x280, sprite_buf, 0xC0);
      drawToScreen(0x08, 0x00, sprite_buf, 0x20, 0x18);
      break;
    }
  switch_default:
    gfx_draw_battery_low(0, 0);
    accelXPos++;
    if (accelXPos > dowsing_item_pos) {
      accelXPos = dowsing_item_pos;
    }
  }
}

// ROM: 0x2938  96.9%
void game_start_battle(void) {
  gCurSubstateZ = 0;
  gCurSubstateA = 4;
  DAT_f7d1 = 4;
  accelXPos = 0;
  dowsing_item_pos = 6;
  accelYPos = 0x38;
  DAT_f7d5 = 0xE0;
  DAT_f7d8 &= 0x1E;
  DAT_f7d8_1 = 0;
  drv_sound_play(10);
}

// ROM: 0x2972  62.5%
void game_battle_process_turn(void) {
  uint32_t rnd;
  uint8_t r;
  uint8_t move_type;
  uint8_t index;

  if (drv_sound_is_playing())
    return;

  rnd = sys_get_rng();
  r = (uint8_t)((rnd >> 3) % 100);
  move_type = (uint8_t)((DAT_f7d8 >> 5) & 0x07);
  index = (uint8_t)(move_type * 3);

  if (r < battleMoveOutcomeWeights[index + 2]) {
    DAT_f7d8 = (DAT_f7d8 & 0xE7) | 0x10;
  } else if (r < battleMoveOutcomeWeights[index + 2] + battleMoveOutcomeWeights[index + 1]) {
    DAT_f7d8 = (DAT_f7d8 & 0xE7) | 0x08;
  } else {
    DAT_f7d8 = (DAT_f7d8 & 0xE7);
  }

  if (drv_button_is_triggered(0x04) != 0) {
    uint8_t m;
    DAT_f7d8 &= 0xF9;
    m = (uint8_t)((DAT_f7d8 >> 3) & 3);
    if (m != 3) {
      gCurSubstateZ = 3;
      accelXPos = 0;
      dowsing_item_pos = 8;
      return;
    }
  }

  if (drv_button_is_triggered(0x08) != 0) {
    uint8_t m;
    DAT_f7d8 = (uint8_t)((DAT_f7d8 & 0xF9) | 0x02);
    m = (uint8_t)((DAT_f7d8 >> 3) & 3);
    if (m == 2) {
      drv_sound_play(0x0E);
      gCurSubstateZ = 7;
      accelXPos = 0;
      dowsing_item_pos = 0x0A;
      return;
    } else if (m == 1) {
      drv_sound_play(0);
      gCurSubstateZ = 8;
      accelXPos = 0;
      dowsing_item_pos = 6;
      return;
    } else if (m == 0) {
      gCurSubstateZ = 4;
      accelXPos = 0;
      dowsing_item_pos = 8;
      return;
    }
  }

  if (drv_button_is_triggered(0x02) != 0) {
    drv_sound_play(0x0F);
    gCurSubstateZ = 10;
    accelXPos = 0;
    dowsing_item_pos = 6;
  }
}

// ROM: 0x2c32  79.0%
uint8_t game_battle_check_capture_success(void) {
  uint32_t r = sys_get_rng();
  uint8_t mod = (uint8_t)((r >> 3) % 100);

  if (DAT_f7d1 == 0)
    return 0;
  if (captureSuccessProbs[DAT_f7d1 - 1] > mod)
    return 1;
  return 0;
}

// ROM: 0x2a96  73.4%  saves: er5,e6
void game_battle_handle_finish(void) {
  uint8_t sub_y = (uint8_t)(gCurSubstateY - 1);

  if (sub_y > 3)
    return;

  if (sub_y < 3) {
    uint8_t sub_result;
    void *buffer_30;
    void *battle_context;
    void *poke_buf;

    sys_init_heap();
    buffer_30 = sbrk(0x30);
    drv_eeprom_read_block(EEPROM_LOG_CONTEXT, buffer_30, 0x30);

    sub_result = save_find_empty_reward_slot(buffer_30);
    if (sub_result >= 3) {
      gCurSubstateA = 0;
      game_start_peer_session();
      ui_set_view(VIEW_CAUGHT_STATS);
      return;
    }

    drv_eeprom_read_block(EEPROM_POKEMON_SLOTS + (uint16_t)sub_y * 16,
                          (uint8_t *)buffer_30 + (uint16_t)sub_result * 16, 16);
    drv_eeprom_write_block(EEPROM_LOG_CONTEXT, buffer_30, 0x30);

    battle_context = sbrk(0xBE);
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, battle_context, 0xBE);

    poke_buf = sbrk(0x88);
    game_log_interaction(battle_context, poke_buf, 0x0D, 0, 0, (uint8_t)gCurSubstateY);
  } else {
    void *buffer_68;
    void *m_context;
    void *poke_buf;
    uint8_t val, val2;

    sys_init_heap();
    buffer_68 = sbrk(0x68);
    val = drv_eeprom_read_u8(EEPROM_EEP_STR);

    if (gfx_xor_rect_ram(buffer_68, val) != 0)
      return;

    val2 = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
    if (val2 & 0x20)
      return;

    m_context = sbrk(0x170);

    drv_eeprom_read_block(0xBF08, m_context, 0x10);
    drv_eeprom_write_block(0xBA44, m_context, 0x10);

    drv_eeprom_read_block(0xBF18, m_context, 0x2C);
    drv_eeprom_write_block(0xBA54, m_context, 0x2C);

    drv_eeprom_read_block(0xBF7C, m_context, 0x170);
    drv_eeprom_write_block(0xBA80, m_context, 0x170);

    drv_eeprom_read_block(0xC6FC, m_context, 0x140);
    drv_eeprom_write_block(0xBC00, m_context, 0x140);

    drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, val2 | 0x20);
    save_set_event_bit(buffer_68, val);

    m_context = sbrk(0xBE);
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, m_context, 0xBE);

    poke_buf = (void *)sbrk(0x88);
    game_log_interaction(m_context, poke_buf, 0x0E, 0x01, 0, (uint8_t)gCurSubstateY);
  }
}

// Reason: ROM hoists `mov.l #0x880000, er5` (ER-packing of constants 0x88
//   and 0) plus `mov.w #0xBE, e6` at entry, and pre-loads accelXPos/
//   dowsing_item_pos into r6l/r6h before the dispatch. ch38 inlines all of
//   these at their use sites. ROM has no prologue; ch38 emits `$sp_regsv$3`.
//   Body switch-jump-table structure matches.
// Class: cannot-fix-without-compiler-change (ER-packing + no-prologue + constant
//   hoisting). Same Tier-3 cluster as ui_render_battle / gfx_blit_to_buffer.
// ROM: 0x2c62  50.2%
void ui_handle_battle(void) {
  uint8_t sub_z = (uint8_t)gCurSubstateZ;
  uint8_t x = accelXPos;
  uint8_t item_pos = dowsing_item_pos;

  if (sub_z == 2) {
    game_battle_process_turn();
    return;
  }

  if (sub_z > 0x11)
    return;

  switch (sub_z) {
  case 0:
    if (x < item_pos)
      return;
    gCurSubstateZ = 1;
    accelXPos = 0;
    dowsing_item_pos = 8;
    break;

  case 1:
    DAT_f7d5 = battleAnimP1XFrames[x];
    if (accelXPos < dowsing_item_pos)
      return;
    DAT_f7d8_BIT.b0 = 1;
    if (drv_sound_is_playing())
      return;
    if (drv_button_is_triggered(0x0E)) {
      gCurSubstateZ = 2;
    }
    break;

  case 3: {
    uint8_t move_type;
    accelYPos = battleAnimP3YFrames[x];
    DAT_f7d5 = battleAnimP3XFrames[x];
    if (drv_sound_is_playing())
      return;
    if (x < item_pos)
      return;
    accelYPos = 0x38;
    DAT_f7d5 = 8;
    move_type = (uint8_t)((DAT_f7d8 >> 3) & 0x03);
    if (move_type == 0) {
      uint8_t d1_val = DAT_f7d1;
      if (d1_val > 1) {
        uint8_t nm;
        DAT_f7d1 = d1_val - 1;
        nm = (uint8_t)((DAT_f7d8 >> 1) & 0x03);
        if (nm == 0) {
          gCurSubstateZ = 4;
        } else if (nm == 1) {
          gCurSubstateZ = 2;
          DAT_f7d8 = (uint8_t)((DAT_f7d8 & 0x1F) | 0x20);
          return;
        }
      } else {
        DAT_f7d1 = 0;
        drv_sound_play(14);
        gCurSubstateZ = 7;
        accelXPos = 0;
        dowsing_item_pos = 0x0A;
      }
    }
    gCurSubstateZ = 4;
    accelXPos = 0;
    dowsing_item_pos = 8;
  } break;

  case 4: {
    uint8_t m_type;
    accelYPos = battleAnimP4YFrames[x];
    DAT_f7d5 = battleAnimP4XFrames[x];
    if (drv_sound_is_playing())
      return;
    if (x < item_pos)
      return;
    accelYPos = 0x38;
    DAT_f7d5 = 8;
    m_type = (uint8_t)((DAT_f7d8 >> 1) & 0x03);
    if (m_type == 0) {
      uint8_t a_val = gCurSubstateA;
      if (a_val > 1) {
        uint8_t next_nm;
        gCurSubstateA = a_val - 1;
        next_nm = (uint8_t)((DAT_f7d8 >> 3) & 0x03);
        if (next_nm == 0) {
          DAT_f7d8 = (uint8_t)((DAT_f7d8 & 0x1F) | 0x20);
        } else if (next_nm == 1) {
          DAT_f7d8 = (uint8_t)((DAT_f7d8 & 0x1F) | 0x60);
        } else if (next_nm == 2) {
          DAT_f7d8 = (uint8_t)((DAT_f7d8 & 0x1F) | 0x40);
        }
      } else {
        gCurSubstateZ = 5;
        accelXPos = 0;
        dowsing_item_pos = 6;
        return;
      }
    }
    gCurSubstateZ = 2;
  } break;

  case 5:
    if (x < item_pos)
      return;
    {
      void *buf;
      void *ctx;
      sys_init_heap();
      buf = sbrk(0xBE);
      drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, buf, 0xBE);
      if (gCurSubstateY < 4) {
        ctx = sbrk(0x10);
        game_log_interaction(ctx, buf, 0x10, 0x00, 0, (uint8_t)gCurSubstateY);
      } else {
        ctx = sbrk(0x110);
        game_log_interaction(ctx, buf, 0x10, 0x01, 0, (uint8_t)gCurSubstateY);
      }
      if (watts >= 10) {
        accelZPos = 10;
      } else {
        accelZPos = (uint8_t)watts;
      }
      watts -= accelZPos;
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
    }
    gCurSubstateZ = 6;
    accelXPos = 0;
    dowsing_item_pos = 8;
    break;

  case 6:
    if (drv_button_is_triggered(0x0E)) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
    }
    break;

  case 7:
    if (x > 3) {
      DAT_f7d5 = 0xE0;
    } else {
      DAT_f7d5 = battleAnimP1XFrames[3 - x];
    }
    if (drv_button_is_triggered(0x0E)) {
      void *buf, *ctx;
      sys_init_heap();
      buf = sbrk(0xBE);
      drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, buf, 0xBE);
      if (gCurSubstateY < 4) {
        ctx = sbrk(0x10);
        game_log_interaction(ctx, buf, 0x0F, 0x00, 0, (uint8_t)gCurSubstateY);
      } else {
        ctx = sbrk(0x110);
        game_log_interaction(ctx, buf, 0x0F, 0x01, 0, (uint8_t)gCurSubstateY);
      }
      drv_sound_play(4);
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
    }
    break;

  case 8:
    if (x < item_pos)
      return;
    gCurSubstateZ = 2;
    DAT_f7d8 = (uint8_t)((DAT_f7d8 & 0x1F) | 0x80);
    break;

  case 9:
    if (x < item_pos)
      return;
    drv_sound_play(0x0E);
    gCurSubstateZ = 7;
    accelXPos = 0;
    dowsing_item_pos = 0x0A;
    break;

  case 10:
    if (x < item_pos)
      return;
    DAT_f7d8 &= ~0x01;
    gCurSubstateZ = 11;
    accelXPos = 0;
    dowsing_item_pos = 2;
    break;

  case 11:
    if (x < item_pos)
      return;
    gCurSubstateZ = 12;
    accelXPos = 0;
    dowsing_item_pos = 3;
    break;

  case 12:
    if (x < item_pos)
      return;
    gCurSubstateZ = 13;
    accelXPos = 0;
    dowsing_item_pos = 4;
    break;

  case 14:
    if (x < item_pos)
      return;
    if (DAT_f7d8_1 >= 3) {
      gCurSubstateZ = 15;
      accelXPos = 0;
      dowsing_item_pos = 6;
      drv_sound_play(7);
    } else {
      gCurSubstateZ = 13;
      accelXPos = 0;
      dowsing_item_pos = 4;
    }
    break;

  case 13:
    if (x < item_pos)
      return;
    if (game_battle_check_capture_success()) {
      DAT_f7d8_1++;
      gCurSubstateZ = 14;
      accelXPos = 0;
      dowsing_item_pos = 4;
    } else {
      gCurSubstateZ = 17;
      accelXPos = 0;
      dowsing_item_pos = 1;
    }
    break;

  case 15:
    if (x < item_pos)
      return;
    gCurSubstateZ = 16;
    accelXPos = 0;
    dowsing_item_pos = 6;
    drv_sound_play(7);
    break;

  case 16:
    if (x < item_pos)
      return;
    if (!drv_button_is_triggered(0x0E))
      return;
    game_battle_handle_finish();
    if (currentlyActiveView != VIEW_BATTLE)
      return;
    ui_reset_substate();
    ui_set_view(VIEW_HOME);
    break;

  case 17:
    if (x < item_pos)
      return;
    gCurSubstateZ = 9;
    accelXPos = 0;
    dowsing_item_pos = 6;
    break;
  }
}
