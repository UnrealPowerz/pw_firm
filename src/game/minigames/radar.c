#include "all_headers.h"

/*
 * Pokeradar minigame.
 *
 * Four grass patches are arranged in a 2x2 grid. One patch (rolled at init)
 * hides the encounter; the player gets a limited time per round to navigate
 * the cursor (BTN_M/L) over the patches and commit (BTN_R) on what they
 * think is the right one. Successful guesses progress toward an encounter;
 * the timer running out or any wrong commit ends the game in failure.
 *
 * State machine (g.ui_substateZ):
 *   RADAR_SEARCH         (0) - cursor navigation; runs ui_handle_radar_grass_menu
 *   RADAR_FADE_TO_BATTLE (1) - shutter fade-in before game_start_battle
 *   RADAR_REWARD         (2) - item reward "received!" screen; press -> home
 *   RADAR_LOCK_ANIM      (3) - "Found something!" lock animation; chains back to
 *                              SEARCH for next round, or to FADE_TO_BATTLE if
 *                              enough rounds completed
 *
 * Globals repurposed for radar:
 *   g.ui_substateA      = cursor position over the 2x2 patch grid (0..3)
 *   g.ui_substateY      = encounter pokemon kind (1..3 wild, 4 peer)
 *   g.accel_xPosition          = total rounds required before battle (rolled at init)
 *   g.accel_yPosition          = sub-tick before each timer decrement
 *   accel_zPosition_byte        = lock-animation timer for RADAR_LOCK_ANIM
 *   g.DAT_f7d1           = rounds completed so far
 *   g.DAT_f7d5           = time-remaining countdown (SEARCH) / fade-frame (FADE)
 *   g.dowsing_item_pos   = secret patch index (0..3), re-rolled each round
 */

enum radar_state {
    RADAR_SEARCH         = 0,
    RADAR_FADE_TO_BATTLE = 1,
    RADAR_REWARD         = 2,   /* dead: never set in ROM either; case kept in
                                   render+handle to match the original switch */
    RADAR_LOCK_ANIM      = 3
};

// Reason: ROM uses `mov.l #0x1000280, er5` (ER-packs 0x100 size + 0x280 base
//   into one 32-bit immediate, then uses e5/r5 halves separately). ch38 emits
//   `mov.w #256, e6` + inline 0x280 immediates at each use site. ROM also
//   uses `mulxu.w` for `(g.ui_animationTick+9)*0x10`; ch38 uses 4 SHLL.W (×2×2×2×2).
//   ROM has no prologue; ch38 emits `$sp_regsv$3`.
// Class: cannot-fix-without-compiler-change (ER-packing + multiplication
//   strength reduction differs + sp_regsv$3)
// ROM: 0x9f44  45.5%
void ui_render_pokeradar(void) {
  void *buf;
  uint8_t cursor;
  uint8_t i;
  volatile uint16_t base = 0x280;

  sys_init_heap();
  buf = sbrk(0x100);

  /* Animated grass-blade sprite that sits over the player's selected patch. */
  drv_eeprom_read_block(0x278 + base + (((g.ui_animationTick & 1) + 9) * 0x10), buf, 0x10);

  cursor = g.ui_substateA;
  drv_lcd_blit(RADAR_Y_COORDS[cursor] - 8, (cursor & 1) * 0x18 + 8, buf, 8, 8);

  /* The four grass patches. */
  drv_eeprom_read_block(0x1A30 + base, buf, 0xC0);
  for (i = 0; i < 4; i++) {
    drv_lcd_blit(RADAR_Y_COORDS[i], (i & 1) * 0x18, buf, 0x20, 0x18);
  }

  if (accel_zPosition_byte != 0) {
    /* Reveal phase — overlay the encounter icon on the secret patch. */
    drv_eeprom_read_block(0x1AF0 + base, buf, 0x100);
    drv_lcd_blit(RADAR_Y_COORDS[g.dowsing_item_pos] + 0x10,
                 (g.dowsing_item_pos & 1) * 0x18, (uint8_t *)buf + 0xC0,
                 0x10, 0x10);

    if (g.ui_substateZ == RADAR_LOCK_ANIM) {
      gfx_draw_text_box(0x30, TEXT_FOUND_SOMETHING_EX, TEXT_BOX_FULL, TEXT_BOX_STATIC);
    } else if (g.ui_substateZ == RADAR_FADE_TO_BATTLE) {
      /* Vertical shutter close-in before battle. */
      gfx_fill_rect(0, 0, 0x60, (uint8_t)(g.DAT_f7d5 * 8), 3);
      gfx_fill_rect(0, (uint8_t)(0x40 - g.DAT_f7d5 * 8), 0x60,
                    (uint8_t)(g.DAT_f7d5 * 8), 3);
      g.DAT_f7d5++;
    } else if (g.ui_substateZ == RADAR_REWARD) {
      gfx_draw_value_with_icon(2, 0x20, 0x0D, g.ui_substateY);
      gfx_draw_text_box(0x30, TEXT_RECEIVED, TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
    }
  } else {
    /* Search phase — prompt and (briefly) flash the next-patch hint. */
    gfx_draw_text_box(0x30, TEXT_FIND_A_POKEMON, TEXT_BOX_FULL, TEXT_BOX_STATIC);
    if (g.accel_yPosition == 0) {
      drv_eeprom_read_block(0x1AF0 + base, buf, 0x100);
      drv_lcd_blit(RADAR_Y_COORDS[g.dowsing_item_pos] + 0x10,
                   (g.dowsing_item_pos & 1) * 0x18,
                   (uint8_t *)buf + RADAR_FRAME_MULT[g.DAT_f7d1] * 0x40,
                   0x10, 0x10);
    }
  }

  gfx_draw_battery_low(0, 0);
}

// ROM: 0x9dce  91.6%
void ui_handle_radar_grass_menu(void) {
  /* Cursor moves: M = back (+3 mod 4 = -1), L = forward (+1 mod 4). */
  if (drv_button_is_triggered(BTN_M) != 0) {
    g.ui_substateA = (g.ui_substateA + 3) & 3;
    drv_sound_play(SND_CURSOR);
  }
  if (drv_button_is_triggered(BTN_L) != 0) {
    g.ui_substateA = (g.ui_substateA + 1) & 3;
    drv_sound_play(SND_CURSOR);
  }

  if (drv_button_is_triggered(BTN_R) != 0) {
    /* Commit on current patch — only valid while the round timer is alive. */
    if (g.DAT_f7d5 != 0) {
      if (g.ui_substateA == g.dowsing_item_pos) {
        /* Correct patch — start lock animation. */
        drv_sound_play(SND_RADAR_LOCK);
        g.ui_substateZ = RADAR_LOCK_ANIM;
        accel_zPosition_byte = 0x10;
        return;
      }
      if (g.DAT_f7d1 == 0) {
        /* First-round wrong guess: just beep, don't end the game. */
        drv_sound_play(SND_FAIL);
        return;
      }
    }
    /* Wrong commit on a later round, or timer already expired -> failure. */
  } else {
    /* No commit: count down the round timer in two stages (g.accel_yPosition drains
       first, then each g.accel_yPosition cycle ticks g.DAT_f7d5 down by one). */
    {
      uint8_t sub = g.accel_yPosition;
      if (sub != 0) {
        g.accel_yPosition = sub - 1;
        return;
      }
    }
    {
      uint8_t t = g.DAT_f7d5;
      if (t != 0) {
        g.DAT_f7d5 = t - 1;
      }
    }
    if (g.DAT_f7d5 != 0) {
      return;
    }
    /* Timer hit zero this tick -> failure. */
  }

  drv_sound_play(SND_FLED);
  ui_set_view(VIEW_RADAR_FAILURE);
}

// ROM: 0x9e72  66.6%
void ui_handle_pokeradar(void) {
  uint32_t r;
  uint8_t state;

  if (drv_sound_is_playing())
    return;

  state = g.ui_substateZ;
  if (state == RADAR_SEARCH) {
    ui_handle_radar_grass_menu();
    return;
  } else if (state == RADAR_FADE_TO_BATTLE) {
    /* g.DAT_f7d5 counts up in the render fn; when the shutter is closed enough,
       launch the actual battle. */
    if (g.DAT_f7d5 > 4) {
      game_start_battle();
      ui_set_view(VIEW_BATTLE);
    }
    return;
  } else if (state == RADAR_REWARD) {
    /* Item reward shown — press any button to return home. */
    if (drv_button_is_triggered(BTN_ANY) == 0)
      return;
    drv_sound_play(SND_CONFIRM);
    ui_reset_substate();
    ui_set_view(VIEW_HOME);
    return;
  } else if (state != RADAR_LOCK_ANIM) {
    return;
  }

  /* RADAR_LOCK_ANIM: hold for accel_zPosition_byte ticks while the reveal plays. */
  if (accel_zPosition_byte == 0)
    return;
  accel_zPosition_byte--;
  if (accel_zPosition_byte != 0)
    return;

  /* Animation done. If we've completed enough rounds, fade to battle;
     otherwise re-roll the secret patch and go back to searching. */
  if ((int16_t)g.DAT_f7d1 >= (int16_t)((uint16_t)g.accel_xPosition - 1)) {
    g.ui_substateZ = RADAR_FADE_TO_BATTLE;
    accel_zPosition_byte = 1;
    g.DAT_f7d5 = 0;
    return;
  }

  g.ui_substateZ = RADAR_SEARCH;
  r = sys_get_rng() >> 2;
  g.accel_yPosition = (uint8_t)((uint16_t)r % RADAR_STATE_Y_DIVISOR[g.DAT_f7d1] + 0x10);
  g.DAT_f7d1++;
  g.DAT_f7d5 = RADAR_STATE_X[g.DAT_f7d1];
  g.dowsing_item_pos = (uint8_t)((sys_get_rng() << 3) & 3);
}

// ROM: 0xa10a  97.9%
void ui_handle_radar_failure(void) {
  if (drv_button_is_triggered(BTN_ANY) != 0) {
    g.ui_substateA = 0;
    drv_sound_play(SND_FAIL);
    ui_reset_substate();
    ui_set_view(VIEW_HOME);
  }
}

// ROM: 0xa12c  75.2%
void ui_render_radar_failure(void) {
  uint16_t i;
  uint8_t *buf;

  /* Draw the 4 grass-patch sprites again (frozen, no animation overlay) +
     the "It got away..." text-box. */
  sys_init_heap();
  buf = sbrk(0xC0);
  drv_eeprom_read_block(0x1CB0, buf, 0xC0);

  for (i = 0; i < 4; i++) {
    drv_lcd_blit(RADAR_Y_COORDS[i], (uint8_t)((i & 1) * 0x18),
                 buf, 0x20, 0x18);
  }

  gfx_draw_text_box(0x30, TEXT_IT_GOT_AWAY, TEXT_BOX_FULL, TEXT_BOX_BLINK);
  gfx_draw_battery_low(0, 0);
}

/*
 * Pre-roll for the radar encounter. Picks what's behind the curtain and
 * how many rounds the player has to win to reach it:
 *
 *   g.ui_substateY = 4  : peer-event encounter (co-op mode + step gates passed)
 *   g.ui_substateY = 1..3: wild encounter, slot N+1 (solo path, step-gated tiers
 *                         in the trainer profile at offset 0x82)
 *   g.ui_substateY = 3, g.accel_xPosition low: fall-through small encounter
 *
 *   g.accel_xPosition: number of rounds the radar minigame should require before
 *              transitioning to battle (3..4 for peer/wild, 1..2 for the
 *              fall-through).
 */
// ROM: 0x9c48  79.2%
void game_roll_radar_encounter(void) {
  uint32_t steps_required;
  uint8_t *scratch;
  uint8_t slot;
  uint8_t rnd_pct;
  uint8_t *trainer_buf;

  g.ui_substateY = 0;
  if (((g.save_settings & 1)) != 0) {
    uint8_t peer_evt_seed = drv_eeprom_read_u8(EEPROM_EEP_STR);
    sys_init_heap();
    scratch = sbrk(0x68);
    if (save_check_event_bit(scratch, peer_evt_seed) == 0) {
      if ((drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS) & 0x20) == 0) {
        sys_init_heap();
        scratch = sbrk(4);
        drv_eeprom_read_block(0xBF44, scratch, 4);
        steps_required = ((uint32_t)scratch[1] << 16) |
                         ((uint32_t)scratch[2] << 8) | scratch[3];
        if (steps_required <= g.session_steps) {
          if ((sys_get_rng() % 100) < scratch[2]) {
            g.ui_substateY = 4;
            g.accel_xPosition = ((sys_get_rng() >> 3) & 1) + 3;
            return;
          }
        }
      }
    }
  }

  /* LAB_9cea: solo path — three step-gated encounter tiers. */
  sys_init_heap();
  scratch = sbrk(0x30);
  drv_eeprom_read_block(EEPROM_LOG_CONTEXT, scratch, 0x30);
  rnd_pct = (uint8_t)(sys_get_rng() % 100);

  sys_init_heap();
  trainer_buf = sbrk(0xBE);
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, trainer_buf, 0xBE);

  for (slot = 0; slot < 3; slot++) {
    if (!game_check_step_unlock((uint16_t)(slot * 2), 0x82, trainer_buf)) {
      if (trainer_buf[0x88 + slot] > rnd_pct) {
        g.ui_substateY = slot + 1;
        /* Codegen note: ROM emits the add as `-slot + (rng>>3 & 1) + 3`,
           hence the negation order here matters for the score. */
        g.accel_xPosition = (slot * -1) + ((sys_get_rng() >> 3) & 1) + 3;
        return;
      }
    }
  }

  g.ui_substateY = 3;
  g.accel_xPosition = ((sys_get_rng() >> 3) & 1) + 1;
}

// ROM: 0x9d92  88.0%
void game_pokeradar_init(void) {
  uint8_t *ram_base;
  game_roll_radar_encounter();         /* rolls g.ui_substateY + g.accel_xPosition */
  g.ui_substateZ = RADAR_SEARCH;
  g.ui_substateA = 0;                   /* cursor at patch 0 */
  g.DAT_f7d1 = 0;                        /* zero rounds completed */
  g.accel_yPosition = 5;                       /* initial sub-tick counter */
  /* g.DAT_f7d5 (round timer) seeded from EEPROM cache shadow at 0xBF1A. */
  ram_base = (uint8_t *)0;
  g.DAT_f7d5 = ram_base[0xBF1A];
  /* Initial secret patch from 2-bit rng slice. */
  g.dowsing_item_pos = ((sys_get_rng() << 3) >> 8) & 3;
  accel_zPosition_byte = 0;
}
