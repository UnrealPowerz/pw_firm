#include "all_headers.h"

/*
 * VIEW_WALK_ARRIVAL_ANIM — animation played when the walker pokemon arrives
 * (returns from a session). Sub-animation index in g.ui_substateY drives the
 * phase dispatch.
 *
 * Also hosts two shared drawers — ui_draw_ball_drop_anim and
 * ui_draw_arrival_cloud_anim — that VIEW_EVENT_REWARD_ANIM also calls.
 * Kept here because walk-arrival is their primary user; event_reward.c
 * borrows them through normal external linkage.
 *
 * The `uint16_t dummy` / `uint16_t uninitializedE0` locals in several drawers
 * are intentional stack-frame placeholders that the ROM allocates but never
 * reads; removing them shifts ch38's stack layout and tanks the match.
 */

/* Sub-animation index in g.ui_substateY for VIEW_WALK_ARRIVAL_ANIM. */
enum walk_arrival_phase {
    ARRIVAL_BALL_DROP = 0,
    ARRIVAL_CLOUD     = 1,
    ARRIVAL_POKE      = 2,
    ARRIVAL_SUCCESS   = 3
};

// ROM: 0x3dbc  83.3%  saves: r2,r5,r6 -> sys_epilogue_r2_r5_r6
void ui_draw_ball_drop_anim(void) {
  void *ptr;
  uint16_t offset;
  uint16_t e0_dummy = 0;

  sys_init_heap();
  ptr = sbrk(0x180);

  if (g.ui_activeView != VIEW_EVENT_REWARD_ANIM) {
    offset = 0x2480;
  } else {
    offset = 0x460;
  }

  drv_eeprom_read_block(offset, ptr, 0x10);
  drv_lcd_blit(
      0x2c, ANIM_BALL_DROP_Y[g.ui_substateZ],
      (void *)ptr, 8, 8);

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  g.ui_substateZ++;
}

// ROM: 0x3ece  74.7%  saves: r2,r5,r6 -> sys_epilogue_r2_r5_r6
void ui_draw_arrival_cloud_anim(void) {
  void *ptr;
  uint16_t dummy;

  sys_init_heap();
  ptr = sbrk(0x180);

  drv_eeprom_read_block(0x1f70, ptr, 0xc0);
  drv_lcd_blit(0x20, 0x10, ptr, 0x20, 0x18);

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  {
    uint8_t z = g.ui_substateZ;
    if (z != 0) {
      g.ui_substateZ = z + 1;
    }
  }
}

// ROM: 0x3f32  76.0%  saves: r2,er3,r5 -> sys_epilogue_6
void ui_draw_arrival_poke_anim(void) {
  uint16_t dummy;
  gfx_draw_home_pokemon(0x10, 8);

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  g.ui_substateZ++;
}

// ROM: 0x3f72  73.8%  saves: r2,r3,r4
void ui_render_arrival_success(void) {
  uint16_t uninitializedE0;
  gfx_draw_own_pokemon_small(0x20, 4);
  gfx_draw_own_pokemon_name(0, 0x20, 5);
  gfx_draw_text_box(0x30, TEXT_PEER_HAS_ARRIVED, TEXT_BOX_NO_LINES, TEXT_BOX_STATIC);

  {
    uint8_t z = g.ui_substateZ;
    if (z < 0x10) {
      g.ui_substateZ = z + 1;
    }
  }

  if (drv_sound_is_playing() == 0) {
    if (g.ui_substateZ > 8) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
    }
  }
}

// ROM: 0x40f8  95.8%
void ui_handle_walk_arrival_anim(void) {
  uint8_t z;
  uint8_t y;
  z = g.ui_substateZ;
  y = g.ui_substateY;
  if (y == ARRIVAL_BALL_DROP) goto phase_ball_drop;
  if (y == ARRIVAL_CLOUD)     goto phase_cloud;
  if (y != ARRIVAL_POKE)      goto done;
  goto phase_poke;
phase_ball_drop:
  /* Ball-drop runs 5 frames (z 0..4), then advance to cloud. */
  if (z > 4) {
    g.ui_substateY = ARRIVAL_CLOUD;
    g.ui_substateZ = 0;
  }
  goto done;
phase_cloud:
  /* Cloud renderer self-increments z (so it stays at 0 here until
     externally bumped); when it has ticked, advance to poke + cue. */
  if (z == 0) goto done;
  g.ui_substateY = ARRIVAL_POKE;
  g.ui_substateZ = 0;
  goto play;
phase_poke:
  if (z <= 8) goto done;
  g.ui_substateZ = 0;
  g.ui_substateY = ARRIVAL_SUCCESS;
play:
  drv_sound_play(SND_ANIM_CUE);
done:;
}

// ROM: 0x4148  96.4%
void ui_render_walk_arrival_anim(void) {
  uint8_t y = g.ui_substateY;
  if (y == ARRIVAL_BALL_DROP) goto phase_ball_drop;
  if (y == ARRIVAL_CLOUD)     goto phase_cloud;
  if (y == ARRIVAL_POKE)      goto phase_poke;
  if (y != ARRIVAL_SUCCESS)   goto done;
  goto phase_success;
phase_ball_drop: ui_draw_ball_drop_anim(); goto done;
phase_cloud:     ui_draw_arrival_cloud_anim(); goto done;
phase_poke:      ui_draw_arrival_poke_anim(); goto done;
phase_success:   ui_render_arrival_success();
done: gfx_draw_battery_low(0, 0);
}
