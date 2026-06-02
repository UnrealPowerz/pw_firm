#include "all_headers.h"

/*
 * Battle minigame.
 *
 * The player tries to catch (or chase off) a wild or peer-sourced pokemon.
 * Each turn the player picks one of three actions; the AI rolls its own
 * outcome from a weighted table, animations play, eventually the encounter
 * ends with caught / fled / lost.
 *
 * Player input mapping (game_battle_process_turn, called from PICK_MOVE):
 *   BTN_M = Attack       -> ATTACK_ANIM
 *   BTN_L = Defend/Flee  -> branches on DAT_f7d8 bits 3-4:
 *                            m==0 -> COUNTER_ANIM
 *                            m==1 -> STARE_DOWN
 *                            m==2 -> FLED  (only path that leaves)
 *   BTN_R = Throw Ball   -> THROW_BALL
 *
 * State enum: see `enum battle_state` below. Transitions documented per-case.
 *
 * DAT_f7d8 packed flags (named where understood):
 *   bit 0    : started_fight (set in render case 1)
 *   bits 1-2 : counter sub-mode  (read via (DAT_f7d8 >> 1) & 3, written by
 *              BTN_L path)
 *   bits 3-4 : attack outcome    (read via (DAT_f7d8 >> 3) & 3)
 *   bits 5-7 : move-class index  (selects 3-entry row in
 *              battleMoveOutcomeWeights)
 *
 * Globals repurposed for battle (all named for their non-battle use):
 *   gCurSubstateA      = wiggle/turn counter
 *   gCurSubstateY      = pokemon kind (1..3 wild route slot, 4 peer)
 *   accelXPos          = per-state animation tick (counts up to dowsing_item_pos)
 *   accelYPos          = pokemon sprite y position (animation frame)
 *   accelZPos          = watts paid on loss (state 5)
 *   DAT_f7d1           = HP/wiggle bar segments left
 *   DAT_f7d5           = pokemon sprite x position / ball x position
 *   DAT_f7d8           = packed flags byte (see above)
 *   DAT_f7d8_1         = wiggle-success counter (3 caps capture)
 *   dowsing_item_pos   = state-dwell length (frames to wait before advancing)
 */

enum battle_state {
    BS_INTRO          = 0,   /* vertical-shutter scroll-in                       */
    BS_APPEARED       = 1,   /* "<pokemon> appeared!" banner; button -> PICK_MOVE */
    BS_PICK_MOVE      = 2,   /* waits on player input (game_battle_process_turn) */
    BS_ATTACK_ANIM    = 3,   /* hit / evade / crit by (DAT_f7d8>>3)&3            */
    BS_COUNTER_ANIM   = 4,   /* counter; loops to PICK_MOVE or chains ATTACK_ANIM */
    BS_DEFEATED       = 5,   /* "<pokemon> was too strong" -> LOST               */
    BS_LOST           = 6,   /* "Lost!" + watts forfeit; button -> home          */
    BS_FLED           = 7,   /* "fled..." screen; button -> home (SND_FAIL)      */
    BS_STARE_DOWN     = 8,   /* "Stare down!" -> PICK_MOVE                       */
    BS_ALMOST_HAD_IT  = 9,   /* "Almost had it!" -> FLED                         */
    BS_THROW_BALL     = 10,  /* "Threw a Poke Ball." -> BALL_FLY                 */
    BS_BALL_FLY       = 11,  /* ball flight phase                                */
    BS_BALL_LAND      = 12,  /* ball-on-ground frame -> WIGGLE_ROLL              */
    BS_WIGGLE_ROLL    = 13,  /* capture roll: pass -> WIGGLE_CHECK, fail -> BALL_MISS */
    BS_WIGGLE_CHECK   = 14,  /* 3rd success -> CAUGHT_FANFARE, else -> WIGGLE_ROLL */
    BS_CAUGHT_FANFARE = 15,  /* fanfare -> CAUGHT_TEXT                           */
    BS_CAUGHT_TEXT    = 16,  /* "<pokemon> was caught!"; button -> finish + home */
    BS_BALL_MISS      = 17   /* ball miss recovery -> ALMOST_HAD_IT              */
};

// ROM: 0x30a6  70.1%
#pragma option noregexpansion /* pragma:auto */
void ui_render_battle(void) {
  uint8_t *sprite_buf, *frame_buf, *mask_buf;
  uint16_t addr;
  uint8_t i, flip;
  /* These function-pointer aliases are load-bearing: ROM caches the addresses
     in registers and reuses them at every call site. Removing them tanks the
     match score. Names match the underlying drivers. */
  void (*read_eeprom)(uint16_t, void *, uint16_t) = drv_eeprom_read_block;
  void (*blit)(uint8_t, uint8_t, void *, uint8_t, uint8_t) = drv_lcd_blit;

  sys_init_heap();
  sprite_buf = (uint8_t *)sbrk(0x300);
  mask_buf   = (uint8_t *)sbrk(0x18);
  frame_buf  = (uint8_t *)sbrk(0x08);

  /* Own-pokemon back sprite (top of screen) — frame from animTick. */
  addr = (uint16_t)(animTick & 1) * 0xC0 + 0x91BE;
  read_eeprom(addr, sprite_buf, 0xC0);
  blit((uint8_t)accelYPos, 8, sprite_buf, 0x20, 0x18);

  /* HP/turn pips for player side (top row). */
  read_eeprom(0x280 + 0x1DB0, sprite_buf, 0x10);
  for (i = 0; i < gCurSubstateA; i++) {
    blit((uint8_t)(0x38 + (i << 3)), 0, sprite_buf, 8, 8);
  }

  /* HP/wiggle pips for wild side (bottom row). Only drawn once the
     "started_fight" bit is set (case 1 advances past intro). */
  if (DAT_f7d8 & 0x01) {
    for (i = 0; i < DAT_f7d1; i++) {
      blit((uint8_t)(0x08 + (i << 3)), 0x18, sprite_buf, 8, 8);
    }
  }

  /* Wild-pokemon sprite. Suppressed for ball-fly / wiggle / caught states
     (>= BS_BALL_FLY), where the ball overlay takes over. */
  if (gCurSubstateZ <= BS_THROW_BALL) {
    uint8_t kind = gCurSubstateY;
    if (kind < 4) {
      /* Route encounter (kinds 1..3): index into 16-byte metadata table at
         0x8F52 to read the flip flag, then load 0x180-byte sprite. */
      read_eeprom(0x8F52 + (uint16_t)(kind - 1) * 16, sprite_buf, 0x10);
      flip = sprite_buf[0x0E] & 0x01;
      addr = 0x9A7E + (uint16_t)(kind - 1) * 0x180 +
             (uint16_t)(animTick & 1) * 0xC0;
    } else {
      /* Peer-sourced encounter (kind == 4): metadata at EEPROM 0xBF08,
         sprite at 0xBF7C. */
      read_eeprom(0xBF08, sprite_buf, 0x10);
      flip = sprite_buf[0x0E] & 0x01;
      addr = 0xBF7C + (uint16_t)(animTick & 1) * 0xC0;
    }

    read_eeprom(addr, sprite_buf, 0x180);
    if (!flip) {
      gfx_flip_horiz(0x20, 0x18, sprite_buf);
    }

    if (gCurSubstateZ == BS_THROW_BALL) {
      /* "Threw a Poke Ball" — alpha-blend the ball over the pokemon as it
         arcs in. fftTwiddleTable doubles as a sine table for the arc. */
      uint8_t ball_x = 0x2C - (accelXPos << 2);
      const int16_t *sine = (const int16_t *)fftTwiddleTable;
      int16_t s = sine[(uint16_t)accelXPos << 2];
      uint8_t ball_y = (uint8_t)(0x14 - (uint8_t)((uint16_t)(s << 1) >> 8));

      read_eeprom(0x280 + 0x1E0, mask_buf, 0x10);
      read_eeprom(0x280 + 0x200, frame_buf, 0x08);

      blit((uint8_t)ball_x, ball_y, mask_buf, 8, 8);
      gfx_alpha_blend(sprite_buf, 0x20, 0x18, mask_buf, frame_buf,
                      ball_x - DAT_f7d5, ball_y, 0x08);
      blit((uint8_t)DAT_f7d5, 0, sprite_buf, 0x20, 0x18);
    } else {
      gfx_draw_sprite_simple((uint8_t)DAT_f7d5, 0, 0x18, 0x20, sprite_buf);
    }
  }

  if (gCurSubstateZ <= BS_BALL_MISS) {
    switch (gCurSubstateZ) {
    case BS_INTRO:
      /* Vertical shutter close-in: black bars shrink from top and bottom. */
      if (accelXPos < 3) {
        uint8_t bar_h = (uint8_t)((3 - accelXPos) * 8);
        gfx_fill_rect(0, 0, 0x60, bar_h, 3);
        gfx_fill_rect(0, (uint8_t)(0x40 - bar_h), 0x60, bar_h, 3);
      }
      break;
    case BS_APPEARED:
      if (accelXPos >= dowsing_item_pos) {
        if (!drv_sound_is_playing()) {
          if (gCurSubstateY < 4) {
            gfx_draw_route_pokemon_name(0x00, 0x20,
                                        (uint8_t)(gCurSubstateY - 1), 0x05);
          } else {
            gfx_draw_special_poke_name(0, 0x20, 5);
          }
          gfx_draw_text_box(0x30, TEXT_APPEARED, TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
          goto switch_default;
        }
      }
      break;
    case BS_PICK_MOVE:
      /* "?" menu sprite — player picks their next action. */
      read_eeprom(0x280 + 0x1DD0, sprite_buf, 0x300);
      blit(0x00, 0x20, sprite_buf, 0x60, 0x20);
      goto switch_default;
    case BS_ATTACK_ANIM: {
      /* Player-attack animation. Outcome is in DAT_f7d8 bits 3-4:
         0 = hit, 1 = enemy evaded, 2 = critical hit. */
      uint8_t outcome = (DAT_f7d8 >> 3) & 0x03;
      if (outcome == 0) {
        if (accelXPos == 4) {
          drv_sound_play(SND_ATTACK_HIT);
          read_eeprom(0x280 + 0x1BF0, sprite_buf, 0x80);
          blit(0x28, 0x00, sprite_buf, 0x10, 0x20);
        }
        if (accelXPos >= 4) {
          gfx_draw_own_pokemon_name(0x00, 0x20, 5);
          gfx_draw_text_box(0x30, TEXT_ATTACKED, TEXT_BOX_NO_LINES, TEXT_BOX_STATIC);
          goto switch_default;
        }
      } else if (outcome == 1) {
        if (accelXPos == 4)
          drv_sound_play(SND_ATTACK_MISS);
        if (accelXPos >= 4) {
          if (gCurSubstateY < 4) {
            gfx_draw_route_pokemon_name(0x00, 0x20,
                                        (uint8_t)(gCurSubstateY - 1), 0x05);
          } else {
            gfx_draw_special_poke_name(0, 0x20, 5);
          }
          gfx_draw_text_box(0x30, TEXT_EVADED, TEXT_BOX_NO_LINES, TEXT_BOX_STATIC);
          goto switch_default;
        }
      } else if (outcome == 2) {
        if (accelXPos == 4) {
          drv_sound_play(SND_CRIT_HIT);
          read_eeprom(0x280 + 0x1C70, sprite_buf, 0x80);
          blit(0x28, 0x00, sprite_buf, 0x10, 0x20);
        }
        if (accelXPos >= 4) {
          gfx_draw_text_box(0x20, TEXT_CRITICAL_HIT, TEXT_BOX_NO_SHADOW, TEXT_BOX_STATIC);
          gfx_draw_text_box(0x30, TEXT_BLANK, TEXT_BOX_NO_LINES, TEXT_BOX_STATIC);
          goto switch_default;
        }
      }
      break;
    }
    case BS_COUNTER_ANIM: {
      /* Counter-attack animation. Sub-mode in DAT_f7d8 bits 1-2:
         0 = enemy hit you, 1 = you evaded. */
      uint8_t sub = (DAT_f7d8 >> 1) & 0x03;
      if (sub == 0) {
        if (accelXPos == 4) {
          drv_sound_play(SND_ATTACK_HIT);
          read_eeprom(0x280 + 0x1BF0, sprite_buf, 0x80);
          blit(0x28, 0x00, sprite_buf, 0x10, 0x20);
        }
        if (accelXPos >= 4) {
          if (gCurSubstateY < 4) {
            gfx_draw_route_pokemon_name(0x00, 0x20,
                                        (uint8_t)(gCurSubstateY - 1), 0x05);
          } else {
            gfx_draw_special_poke_name(0, 0x20, 5);
          }
          gfx_draw_text_box(0x30, TEXT_ATTACKED, TEXT_BOX_NO_LINES, TEXT_BOX_STATIC);
          goto switch_default;
        }
      } else if (sub == 1) {
        if (accelXPos == 4)
          drv_sound_play(SND_ATTACK_MISS);
        if (accelXPos >= 4) {
          gfx_draw_own_pokemon_name(0, 0x20, 5);
          gfx_draw_text_box(0x30, TEXT_EVADED, TEXT_BOX_NO_LINES, TEXT_BOX_STATIC);
          goto switch_default;
        }
      }
      break;
    }
    case BS_DEFEATED:
      if (gCurSubstateY < 4) {
        gfx_draw_route_pokemon_name(0x00, 0x20,
                                    (uint8_t)(gCurSubstateY - 1), 0x05);
      } else {
        gfx_draw_special_poke_name(0, 0x20, 5);
      }
      gfx_draw_text_box(0x30, TEXT_WAS_TOO_STRONG, TEXT_BOX_NO_LINES, TEXT_BOX_STATIC);
      goto switch_default;
    case BS_LOST:
      gfx_draw_value_with_icon(2, 0x20, 0x0D, (uint16_t)accelZPos);
      gfx_draw_text_box(0x30, TEXT_LOST, TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
      goto switch_default;
    case BS_FLED:
      if (accelXPos > 3) {
        if (gCurSubstateY < 4) {
          gfx_draw_route_pokemon_name(0x00, 0x20,
                                      (uint8_t)(gCurSubstateY - 1), 0x05);
        } else {
          gfx_draw_special_poke_name(0, 0x20, 5);
        }
        gfx_draw_text_box(0x30, TEXT_FLED, TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
        goto switch_default;
      }
      break;
    case BS_STARE_DOWN:
      gfx_draw_text_box(0x30, TEXT_STARE_DOWN, TEXT_BOX_FULL, TEXT_BOX_STATIC);
      goto switch_default;
    case BS_ALMOST_HAD_IT:
      gfx_draw_text_box(0x30, TEXT_ALMOST_HAD_IT, TEXT_BOX_FULL, TEXT_BOX_STATIC);
      goto switch_default;
    case BS_THROW_BALL:
      gfx_draw_text_box(0x30, TEXT_THREW_POKEBALL, TEXT_BOX_FULL, TEXT_BOX_STATIC);
      goto switch_default;
    case BS_BALL_FLY:
      /* Ball-in-flight sprite. */
      read_eeprom(0x280 + 0x1CF0, sprite_buf, 0xC0);
      blit(0x08, 0x00, sprite_buf, 0x20, 0x18);
      break;

    case BS_BALL_LAND:
    case BS_WIGGLE_CHECK:
      /* Ball at rest, centred — wiggle frames cycle this. */
      read_eeprom(0x460, sprite_buf, 0x10);
      blit(20, 0x0C, sprite_buf, 8, 8);
      break;

    case BS_WIGGLE_ROLL: {
      /* Ball wiggle: 4-frame animation drifts the ball x by ±1. */
      uint8_t phase = animTick & 3;
      uint8_t wiggle_x;
      if (phase == 0)
        wiggle_x = 0x13;
      else if (phase == 2)
        wiggle_x = 0x15;
      else
        wiggle_x = 0x14;
      read_eeprom(0x460, sprite_buf, 0x10);
      blit(wiggle_x, 0x0C, sprite_buf, 8, 8);
      break;
    }

    case BS_CAUGHT_FANFARE:
      /* Caught fanfare: ball + two diverging confetti sparks. */
      read_eeprom(0x460, sprite_buf, 0x10);
      blit(20, 0x0C, sprite_buf, 8, 8);
      read_eeprom(0x2040, sprite_buf, 0x10);
      blit((uint8_t)(12 - accelXPos),
           (uint8_t)(10 - 2 * accelXPos), sprite_buf, 8, 8);
      blit((uint8_t)(28 + accelXPos),
           (uint8_t)(12 - 2 * accelXPos), sprite_buf, 8, 8);
      break;

    case BS_CAUGHT_TEXT:
      read_eeprom(0x460, sprite_buf, 0x10);
      blit(20, 0x0C, sprite_buf, 8, 8);
      if (gCurSubstateY < 4) {
        gfx_draw_route_pokemon_name(0x00, 0x20, (uint8_t)(gCurSubstateY - 1),
                                    0x05);
      } else {
        gfx_draw_special_poke_name(0x00, 0x20, 5);
      }
      gfx_draw_text_box(0x30, TEXT_WAS_CAUGHT, TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
      break;

    case BS_BALL_MISS:
      /* Ball miss — recovery frame (same ball-flight sprite). */
      read_eeprom(0x280 + 0x1CF0, sprite_buf, 0xC0);
      blit(0x08, 0x00, sprite_buf, 0x20, 0x18);
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

// ROM: 0x2938  99.7%
void game_start_battle(void) {
  gCurSubstateZ = BS_INTRO;
  gCurSubstateA = 4;          /* 4 player HP/turn pips */
  DAT_f7d1 = 4;               /* 4 wild HP/wiggle pips */
  accelXPos = 0;
  dowsing_item_pos = 6;       /* state-0 intro dwell length */
  accelYPos = 0x38;
  DAT_f7d5 = 0xE0;
  DAT_f7d8 &= 0x1E;           /* preserve bits 1-4, clear move-class + flag */
  DAT_f7d8_1 = 0;             /* wiggle-success counter */
  drv_sound_play(SND_BATTLE_START);
}

// ROM: 0x2972  59.2%
void game_battle_process_turn(void) {
  uint32_t rnd;
  uint8_t rnd_pct;
  uint8_t move_class;
  uint8_t weights_idx;

  if (drv_sound_is_playing())
    return;

  /* Roll attack outcome (hit / evade / crit) from per-move-class weights.
     The three-entry row is laid out as [crit_weight, evade_weight, hit_weight]
     and chosen by cumulative compare. */
  rnd = sys_get_rng();
  rnd_pct = (uint8_t)((rnd >> 3) % 100);
  move_class = (uint8_t)((DAT_f7d8 >> 5) & 0x07);
  weights_idx = (uint8_t)(move_class * 3);

  if (rnd_pct < battleMoveOutcomeWeights[weights_idx + 2]) {
    DAT_f7d8 = (DAT_f7d8 & 0xE7) | 0x10;            /* outcome = crit (2) */
  } else if (rnd_pct < battleMoveOutcomeWeights[weights_idx + 2] +
                       battleMoveOutcomeWeights[weights_idx + 1]) {
    DAT_f7d8 = (DAT_f7d8 & 0xE7) | 0x08;            /* outcome = evade (1) */
  } else {
    DAT_f7d8 = (DAT_f7d8 & 0xE7);                   /* outcome = hit (0) */
  }

  /* Attack (middle button). */
  if (drv_button_is_triggered(BTN_M) != 0) {
    uint8_t outcome;
    DAT_f7d8 &= 0xF9;
    outcome = (uint8_t)((DAT_f7d8 >> 3) & 3);
    if (outcome != 3) {
      gCurSubstateZ = BS_ATTACK_ANIM;
      accelXPos = 0;
      dowsing_item_pos = 8;
      return;
    }
  }

  /* Defend / Flee (left button). The sub-mode (DAT_f7d8 bits 3-4) picks
     between counter-anim, stare-down, and the only flee path. */
  if (drv_button_is_triggered(BTN_L) != 0) {
    uint8_t sub_mode;
    DAT_f7d8 = (uint8_t)((DAT_f7d8 & 0xF9) | 0x02);
    sub_mode = (uint8_t)((DAT_f7d8 >> 3) & 3);
    if (sub_mode == 2) {
      drv_sound_play(SND_FLED);
      gCurSubstateZ = BS_FLED;
      accelXPos = 0;
      dowsing_item_pos = 0x0A;
      return;
    } else if (sub_mode == 1) {
      drv_sound_play(SND_CONFIRM);
      gCurSubstateZ = BS_STARE_DOWN;
      accelXPos = 0;
      dowsing_item_pos = 6;
      return;
    } else if (sub_mode == 0) {
      gCurSubstateZ = BS_COUNTER_ANIM;
      accelXPos = 0;
      dowsing_item_pos = 8;
      return;
    }
  }

  /* Throw ball (right button). */
  if (drv_button_is_triggered(BTN_R) != 0) {
    drv_sound_play(SND_BALL_THROW);
    gCurSubstateZ = BS_THROW_BALL;
    accelXPos = 0;
    dowsing_item_pos = 6;
  }
}

// ROM: 0x2c32  84.5%
uint8_t game_battle_check_capture_success(void) {
  uint32_t rnd = sys_get_rng();
  uint8_t rnd_pct = (uint8_t)((rnd >> 3) % 100);

  if (DAT_f7d1 == 0)
    return 0;
  if (captureSuccessProbs[DAT_f7d1 - 1] > rnd_pct)
    return 1;
  return 0;
}

// ROM: 0x2a96  84.9%  saves: er5,e6
void game_battle_handle_finish(void) {
  uint8_t kind_idx = (uint8_t)(gCurSubstateY - 1);

  if (kind_idx > 3)
    return;

  if (kind_idx < 3) {
    /* Route-encounter catch: write into the next free pokemon slot in
       the 0x30-byte log block, then log the interaction. */
    uint8_t reward_slot;
    void *log_block;
    void *trainer_buf;
    void *gift_buf;

    sys_init_heap();
    log_block = sbrk(0x30);
    drv_eeprom_read_block(EEPROM_LOG_CONTEXT, log_block, 0x30);

    reward_slot = save_find_empty_reward_slot(log_block);
    if (reward_slot >= 3) {
      /* Bag full — bail out to caught-stats peer session. */
      gCurSubstateA = 0;
      game_start_peer_session();
      ui_set_view(VIEW_CAUGHT_STATS);
      return;
    }

    drv_eeprom_read_block(EEPROM_POKEMON_SLOTS + (uint16_t)kind_idx * 16,
                          (uint8_t *)log_block + (uint16_t)reward_slot * 16, 16);
    drv_eeprom_write_block(EEPROM_LOG_CONTEXT, log_block, 0x30);

    trainer_buf = sbrk(0xBE);
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);

    gift_buf = sbrk(0x88);
    game_log_interaction(trainer_buf, gift_buf, 0x0D, 0, 0, (uint8_t)gCurSubstateY);
  } else {
    /* Peer-pokemon catch: copy the peer-event blocks from the staging area
       (0xBF08+) into their permanent homes (0xBA44+), mark the event
       received, then log the interaction. */
    void *scratch;
    void *block_buf;
    void *trainer_buf;
    void *gift_buf;
    uint8_t event_seed, hist_flags;

    sys_init_heap();
    scratch = sbrk(0x68);
    event_seed = drv_eeprom_read_u8(EEPROM_EEP_STR);

    if (gfx_xor_rect_ram(scratch, event_seed) != 0)
      return;

    hist_flags = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
    if (hist_flags & 0x20)
      return;

    block_buf = sbrk(0x170);

    drv_eeprom_read_block(0xBF08, block_buf, 0x10);
    drv_eeprom_write_block(0xBA44, block_buf, 0x10);

    drv_eeprom_read_block(0xBF18, block_buf, 0x2C);
    drv_eeprom_write_block(0xBA54, block_buf, 0x2C);

    drv_eeprom_read_block(0xBF7C, block_buf, 0x170);
    drv_eeprom_write_block(0xBA80, block_buf, 0x170);

    drv_eeprom_read_block(0xC6FC, block_buf, 0x140);
    drv_eeprom_write_block(0xBC00, block_buf, 0x140);

    drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, hist_flags | 0x20);
    save_set_event_bit(scratch, event_seed);

    trainer_buf = sbrk(0xBE);
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);

    gift_buf = (void *)sbrk(0x88);
    game_log_interaction(trainer_buf, gift_buf, 0x0E, 0x01, 0, (uint8_t)gCurSubstateY);
  }
}

// Reason: ROM hoists `mov.l #0x880000, er5` (ER-packing of constants 0x88
//   and 0) plus `mov.w #0xBE, e6` at entry, and pre-loads accelXPos/
//   dowsing_item_pos into r6l/r6h before the dispatch. ch38 inlines all of
//   these at their use sites. ROM has no prologue; ch38 emits `$sp_regsv$3`.
//   Body switch-jump-table structure matches.
// Class: cannot-fix-without-compiler-change (ER-packing + no-prologue + constant
//   hoisting). Same Tier-3 cluster as ui_render_battle / gfx_blit_to_buffer.
// ROM: 0x2c62  54.1%
void ui_handle_battle(void) {
  uint8_t state = (uint8_t)gCurSubstateZ;
  uint8_t tick = accelXPos;
  uint8_t dwell = dowsing_item_pos;

  if (state == BS_PICK_MOVE) {
    /* State 2 input handling is its own function. */
    game_battle_process_turn();
    return;
  }

  if (state > BS_BALL_MISS)
    return;

  switch (state) {
  case BS_INTRO:
    if (tick < dwell)
      return;
    gCurSubstateZ = BS_APPEARED;
    accelXPos = 0;
    dowsing_item_pos = 3;
    break;

  case BS_APPEARED:
    DAT_f7d5 = battleAnimP1XFrames[tick];
    if (accelXPos < dowsing_item_pos)
      return;
    DAT_f7d8_BIT.b0 = 1;        /* started_fight flag */
    if (drv_sound_is_playing())
      return;
    if (drv_button_is_triggered(BTN_ANY)) {
      gCurSubstateZ = BS_PICK_MOVE;
    }
    break;

  case BS_ATTACK_ANIM: {
    /* Player-attack resolution. Outcome (bits 3-4) chains into either a
       follow-up state-2 with a fresh move-class, or a counter at state 4,
       or — if HP runs out — to the fled screen. */
    uint8_t outcome;
    uint8_t hp;
    accelYPos = battleAnimP3YFrames[(uint16_t)tick * 2];
    DAT_f7d5 = battleAnimP3XFrames[(uint16_t)tick * 2];
    if (drv_sound_is_playing())
      return;
    if (tick < dwell)
      return;
    accelYPos = 0x38;
    DAT_f7d5 = 8;
    hp = DAT_f7d1;
    outcome = (uint8_t)((DAT_f7d8 >> 3) & 0x03);
    if (outcome == 0) {
      uint8_t next_class;
      if (hp <= 1)
        goto hp_empty;
      DAT_f7d1 = hp - 1;
      next_class = (uint8_t)((DAT_f7d8 >> 1) & 0x03);
      if (next_class == 0)
        goto enter_counter;
      if (next_class != 1)
        return;
      gCurSubstateZ = BS_PICK_MOVE;
      DAT_f7d8 = (uint8_t)((DAT_f7d8 & 0x1F) | 0x20);  /* move-class = 1 */
      return;
    } else if (outcome == 1) {
      goto enter_counter;
    } else if (outcome == 2) {
      if (hp <= 2)
        goto hp_empty;
      DAT_f7d1 = hp - 2;
      goto enter_counter;
    }
    return;
  hp_empty:
    DAT_f7d1 = 0;
    drv_sound_play(SND_FLED);
    gCurSubstateZ = BS_FLED;
    accelXPos = 0;
    dowsing_item_pos = 0x0A;
    break;
  enter_counter:
    gCurSubstateZ = BS_COUNTER_ANIM;
    accelXPos = 0;
    dowsing_item_pos = 8;
  } break;

  case BS_COUNTER_ANIM: {
    /* Counter-attack: sub bits 1-2 pick between enemy-hit and player-evade.
       Sub == 0 decrements gCurSubstateA (player turn counter); when it hits
       zero the player has lost. */
    uint8_t sub;
    accelYPos = battleAnimP4YFrames[(uint16_t)tick * 2];
    DAT_f7d5 = battleAnimP4XFrames[(uint16_t)tick * 2];
    if (drv_sound_is_playing())
      return;
    if (tick < dwell)
      return;
    accelYPos = 0x38;
    DAT_f7d5 = 8;
    sub = (uint8_t)((DAT_f7d8 >> 1) & 0x03);
    if (sub == 0) {
      uint8_t next_class;
      uint8_t new_a = (uint8_t)(gCurSubstateA - 1);
      gCurSubstateA = new_a;
      if (new_a == 0) {
        gCurSubstateZ = BS_DEFEATED;
        accelXPos = 0;
        dowsing_item_pos = 6;
        return;
      }
      /* Rotate to next move-class encoded in bits 3-4. */
      next_class = (uint8_t)((DAT_f7d8 >> 3) & 0x03);
      if (next_class == 0) {
        DAT_f7d8 = (uint8_t)((DAT_f7d8 & 0x1F) | 0x20);
      } else if (next_class == 1) {
        DAT_f7d8 = (uint8_t)((DAT_f7d8 & 0x1F) | 0x60);
      } else if (next_class == 2) {
        DAT_f7d8 = (uint8_t)((DAT_f7d8 & 0x1F) | 0x40);
      }
      gCurSubstateZ = BS_PICK_MOVE;
      return;
    }
    if (sub == 1) {
      gCurSubstateZ = BS_ATTACK_ANIM;
      accelXPos = 0;
      dowsing_item_pos = 8;
      return;
    }
    return;
  } break;

  case BS_DEFEATED:
    /* "was too strong" — record the loss and forfeit up to 10 watts. */
    if (tick < dwell)
      return;
    {
      void *trainer_buf;
      void *log_buf;
      sys_init_heap();
      trainer_buf = sbrk(0xBE);
      drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);
      if (gCurSubstateY < 4) {
        log_buf = sbrk(0x10);
        game_log_interaction(log_buf, trainer_buf, 0x10, 0x00, 0, (uint8_t)gCurSubstateY);
      } else {
        log_buf = sbrk(0x110);
        game_log_interaction(log_buf, trainer_buf, 0x10, 0x01, 0, (uint8_t)gCurSubstateY);
      }
      if (watts >= 10) {
        accelZPos = 10;
      } else {
        accelZPos = (uint8_t)watts;
      }
      watts -= accelZPos;
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
    }
    gCurSubstateZ = BS_LOST;
    accelXPos = 0;
    dowsing_item_pos = 8;
    break;

  case BS_LOST:
    if (drv_button_is_triggered(BTN_ANY)) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
    }
    break;

  case BS_FLED:
    /* "fled" screen — slide ball off, then wait for input to log + exit. */
    if (tick > 3) {
      DAT_f7d5 = 0xE0;
    } else {
      DAT_f7d5 = battleAnimP1XFrames[3 - tick];
    }
    if (drv_button_is_triggered(BTN_ANY)) {
      void *trainer_buf, *log_buf;
      sys_init_heap();
      trainer_buf = sbrk(0xBE);
      drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);
      if (gCurSubstateY < 4) {
        log_buf = sbrk(0x10);
        game_log_interaction(log_buf, trainer_buf, 0x0F, 0x00, 0, (uint8_t)gCurSubstateY);
      } else {
        log_buf = sbrk(0x110);
        game_log_interaction(log_buf, trainer_buf, 0x0F, 0x01, 0, (uint8_t)gCurSubstateY);
      }
      drv_sound_play(SND_FAIL);
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
    }
    break;

  case BS_STARE_DOWN:
    /* Stare down — bump move-class to "max" then drop back to state 2. */
    if (tick < dwell)
      return;
    gCurSubstateZ = BS_PICK_MOVE;
    DAT_f7d8 = (uint8_t)((DAT_f7d8 & 0x1F) | 0x80);
    break;

  case BS_ALMOST_HAD_IT:
    /* "Almost had it!" — leads into the fled screen with a beat of pause. */
    if (tick < dwell)
      return;
    drv_sound_play(SND_FLED);
    gCurSubstateZ = BS_FLED;
    accelXPos = 0;
    dowsing_item_pos = 0x0A;
    break;

  case BS_THROW_BALL:
    /* Ball thrown — clear started_fight bit, start ball-flight. */
    if (tick < dwell)
      return;
    DAT_f7d8 &= ~0x01;
    gCurSubstateZ = BS_BALL_FLY;
    accelXPos = 0;
    dowsing_item_pos = 2;
    break;

  case BS_BALL_FLY:
    if (tick < dwell)
      return;
    gCurSubstateZ = BS_BALL_LAND;
    accelXPos = 0;
    dowsing_item_pos = 3;
    break;

  case BS_BALL_LAND:
    if (tick < dwell)
      return;
    gCurSubstateZ = BS_WIGGLE_ROLL;
    accelXPos = 0;
    dowsing_item_pos = 4;
    break;

  case BS_WIGGLE_CHECK:
    /* Wiggle-success: third success in a row caps off as caught. */
    if (tick < dwell)
      return;
    if (DAT_f7d8_1 >= 3) {
      gCurSubstateZ = BS_CAUGHT_FANFARE;
      accelXPos = 0;
      dowsing_item_pos = 6;
      drv_sound_play(SND_FANFARE);
    } else {
      gCurSubstateZ = BS_WIGGLE_ROLL;
      accelXPos = 0;
      dowsing_item_pos = 4;
    }
    break;

  case BS_WIGGLE_ROLL:
    /* Roll a wiggle: pass -> 14 (counter++), fail -> 17 (miss recovery). */
    if (tick < dwell)
      return;
    if (game_battle_check_capture_success()) {
      DAT_f7d8_1++;
      gCurSubstateZ = BS_WIGGLE_CHECK;
      accelXPos = 0;
      dowsing_item_pos = 4;
    } else {
      gCurSubstateZ = BS_BALL_MISS;
      accelXPos = 0;
      dowsing_item_pos = 1;
    }
    break;

  case BS_CAUGHT_FANFARE:
    /* Second fanfare beat before showing "was caught!" text. */
    if (tick < dwell)
      return;
    gCurSubstateZ = BS_CAUGHT_TEXT;
    accelXPos = 0;
    dowsing_item_pos = 6;
    drv_sound_play(SND_FANFARE);
    break;

  case BS_CAUGHT_TEXT:
    /* "was caught!" — finish (log + write reward) on button press. */
    if (tick < dwell)
      return;
    if (!drv_button_is_triggered(BTN_ANY))
      return;
    game_battle_handle_finish();
    if (currentlyActiveView != VIEW_BATTLE)
      return;
    ui_reset_substate();
    ui_set_view(VIEW_HOME);
    break;

  case BS_BALL_MISS:
    /* Ball miss — fall through to "Almost had it!". */
    if (tick < dwell)
      return;
    gCurSubstateZ = BS_ALMOST_HAD_IT;
    accelXPos = 0;
    dowsing_item_pos = 6;
    break;
  }
}
