#include "all_headers.h"

/*
 * VIEW_WALK_DEPARTURE_ANIM — animation played when the walker pokemon leaves
 * (session ends or wipe-on-clear). Self-contained; doesn't share sub-anims
 * with the other animation views.
 *
 * Two distinct entry phases:
 *   DEPARTURE_START      (5) — normal session-end, plays the poke-leaves
 *                              sequence (POKE -> CLOUD_AFTER -> CLOUD ->
 *                              SUCCESS)
 *   DEPARTURE_DONE_PRE   (6) — wipe / clear-stats path, jumps straight to
 *                              the "Completed!" final banner (DONE)
 *
 * Several drawers carry intentional `uint16_t dummy` / `uint16_t
 * uninitializedE0` stack-frame placeholders.
 */

/* Sub-animation index in gCurSubstateY for VIEW_WALK_DEPARTURE_ANIM. */
enum walk_departure_phase {
    DEPARTURE_CLOUD        = 0,   /* cloud rising                            */
    DEPARTURE_CLOUD_AFTER  = 1,   /* cloud-clear frame                       */
    DEPARTURE_POKE         = 2,   /* poke disappears                         */
    DEPARTURE_SUCCESS      = 3,   /* "<poke> has left." banner               */
    DEPARTURE_START        = 5,   /* entry: fires sound -> POKE              */
    DEPARTURE_DONE_PRE     = 6,   /* entry: fires sound -> DONE              */
    DEPARTURE_DONE         = 7    /* "Completed!" banner                     */
};

/* Note: jumped from 1.6% to 75.5% after we settled on default -regparam=2
 * + -cmncode (the ROM uses helper-call style for short functions like
 * this one, and ch38's $sp_regsv$3 + sys_epilogue tail-jump aligns well
 * with what compare_bin sees).  No further action recommended. */
// ROM: 0x42d0  76.0%  saves: r2,er3,r5 -> sys_epilogue_6
void ui_draw_poke_departure_anim(void) {
  uint16_t dummy;
  gfx_draw_home_pokemon(0x10, 8);

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  gCurSubstateZ++;
}

// ROM: 0x4310  77.9%  saves: r2,r5,r6 -> sys_epilogue_r2_r5_r6
void ui_draw_departure_cloud_anim(void) {
  void *ptr;
  uint16_t dummy;

  sys_init_heap();
  ptr = sbrk(0x180);

  drv_eeprom_read_block(0x1f70, ptr, 0xc0);
  drv_lcd_blit(0x20, 0x10, ptr, 0x20, 0x18);

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  gCurSubstateY = 0;
  gCurSubstateZ = 0;
}

// ROM: 0x4372  81.3%  saves: r2,r5,r6 -> sys_epilogue_r2_r5_r6
void ui_draw_cloud_rise_anim(void) {
  void *ptr;
  uint16_t uninitializedE0;

  sys_init_heap();
  ptr = sbrk(0x180);

  if (gCurSubstateZ <= 4) {
    drv_eeprom_read_block(0x2480, ptr, 0x10);
    drv_lcd_blit(
        0x2c, ANIM_CLOUD_Y[gCurSubstateZ],
        (void *)ptr, 8, 8);
  }

  gfx_fill_rect(0, 0, 0x60, 8, 3);
  gfx_fill_rect(0, 0x38, 0x60, 8, 3);

  gCurSubstateZ++;
}

// ROM: 0x43e4  86.0%  saves: r2
void ui_render_departure_success(void) {
  uint16_t uninitializedE0;

  sys_init_heap();
  sbrk(0x180);

  gfx_draw_own_pokemon_name(0, 0x20, 5);
  gfx_draw_text_box(0x30, TEXT_PEER_HAS_LEFT, TEXT_BOX_NO_LINES, TEXT_BOX_STATIC);

  {
    uint8_t z = gCurSubstateZ;
    if (z < 0x10) {
      gCurSubstateZ = z + 1;
    }
  }

  if (drv_sound_is_playing() == 0) {
    if (gCurSubstateZ > 8) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
    }
  }
}

// ROM: 0x4434  78.5%  saves: r2,r5,r6 -> sys_epilogue_r2_r5_r6
void ui_render_operation_completed(void) {
  void *ptr;
  uint16_t uninitializedE0;

  sys_init_heap();
  ptr = sbrk(0x100);

  drv_eeprom_read_block(0x2350, ptr, 0x100);
  drv_lcd_blit(0x20, 0x10, ptr, 0x20, 0x20);
  gfx_draw_text_box(0x30, TEXT_COMPLETED, TEXT_BOX_FULL, TEXT_BOX_STATIC);

  {
    uint8_t z = gCurSubstateZ;
    if (z < 0x10) {
      gCurSubstateZ = z + 1;
    }
  }

  if (drv_sound_is_playing() == 0) {
    if (gCurSubstateZ > 8) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
    }
  }
}

// ROM: 0x449e  92.6%
void ui_handle_walk_departure_anim(void) {
  uint8_t z;
  uint8_t y;
  z = gCurSubstateZ;
  y = gCurSubstateY;
  if (y == DEPARTURE_START)    goto phase_start;
  if (y == DEPARTURE_POKE)     goto phase_poke;
  if (y == DEPARTURE_CLOUD)    goto phase_cloud;
  if (y != DEPARTURE_DONE_PRE) goto done;
  goto phase_done_pre;
phase_start:
  /* Entry — kick the poke-departure anim with sound. */
  gCurSubstateZ = 0;
  gCurSubstateY = DEPARTURE_POKE;
  goto sound;
phase_poke:
  if (z <= 8) goto done;
  gCurSubstateZ = 0;
  gCurSubstateY = DEPARTURE_CLOUD_AFTER;
  goto done;
phase_cloud:
  if (z < 9) goto done;
  y = DEPARTURE_SUCCESS;
  goto shared_advance;
phase_done_pre:
  y = DEPARTURE_DONE;
shared_advance:
  gCurSubstateY = y;
  gCurSubstateZ = 0;
sound:
  drv_sound_play(SND_ANIM_CUE);
done:
  ;
}

// ROM: 0x44f4  94.2%
void ui_render_walk_departure_anim(void) {
  uint8_t y = gCurSubstateY;
  if (y == DEPARTURE_POKE)        goto phase_poke;
  if (y == DEPARTURE_CLOUD_AFTER) goto phase_cloud_after;
  if (y == DEPARTURE_CLOUD)       goto phase_cloud;
  if (y == DEPARTURE_SUCCESS)     goto phase_success;
  if (y == DEPARTURE_DONE_PRE)    goto phase_done;
  if (y != DEPARTURE_DONE)        goto done;
  goto phase_done;
phase_poke:        ui_draw_poke_departure_anim(); goto done;
phase_cloud_after: ui_draw_departure_cloud_anim(); goto done;
phase_cloud:       ui_draw_cloud_rise_anim(); goto done;
phase_success:     ui_render_departure_success(); goto done;
phase_done:        ui_render_operation_completed();
done: gfx_draw_battery_low(0, 0);
}
