#include "all_headers.h"

/*
 * UI dispatcher — the per-tick event/render fanout from g.currentlyActiveView
 * to the handler/render pair for each VIEW_* in include/globals.h.
 *
 * The main loop calls:
 *   ui_dispatch_event() once per input tick   — runs the active view's input
 *                                               handler and finalizes the SPI
 *                                               chip-select for the LCD bus.
 *   ui_dispatch_draw()  once per frame        — runs the active view's
 *                                               renderer, then finalizes SPI.
 *
 * Helpers:
 *   ui_set_view()           writes g.currentlyActiveView (one-liner).
 *   ui_reset_substate()     zeroes the substate cursors before a view switch.
 *   ui_clear_substate_y()   zeroes only g.gCurSubstateY (one-liner).
 *   sys_set_handler()       swaps the foreground event-loop function pointer,
 *                           saving the prior one for restoration. Used when
 *                           entering / leaving the IR app's low-power loop.
 */

// ROM: 0x693a  61.7%  saves: r6
void sys_set_handler(event_loop_func_t func) {
  g.sys_savedTickHandler = g.sys_tickHandler;
  g.sys_tickHandler = func;
}

// ROM: 0x69b8  100.0%
void ui_set_view(uint8_t viewId) { g.currentlyActiveView = viewId; }

// ROM: 0x6a1c  77.0%
void ui_reset_substate(void) {
  g.gCurSubstateY = 0;
  g.gCurSubstateZ = 0;
  g.gCurSubstateA = 0x20;       /* initial animation tick / dwell countdown */
  g.DAT_f7d1 &= ~0x07;          /* clear the 3 low flag bits (b0/b1/b2) used by
                                 home + battle; preserve the upper byte state */
}

// ROM: 0x7348  86.1%  saves: er2,er3,er4,er5,er6
void ui_dispatch_event(void) {
  /* Tick the RNG once per input frame so that even views without user-driven
     randomness still advance the state. Return value is intentionally ignored. */
  sys_get_rng();
  switch (g.currentlyActiveView) {
  case VIEW_HOME:
    ui_handle_home();
    break;
  case VIEW_BORED_GIFT:
    ui_handle_bored_gift();
    break;
  case VIEW_PEER_PLAY:
    ui_handle_peer_play();
    break;
  case VIEW_MAIN_MENU:
    ui_handle_main_menu();
    break;
  case VIEW_DOWSING:
    ui_handle_dowsing();
    break;
  case VIEW_POKERADAR:
    ui_handle_pokeradar();
    break;
  case VIEW_BATTLE:
    ui_handle_battle();
    break;
  case VIEW_RADAR_FAILURE:
    ui_handle_radar_failure();
    break;
  case VIEW_DISCARD_PICKER:
    ui_handle_discard_picker();
    break;
  case VIEW_TRAINER_CARD:
    ui_handle_trainer_card();
    break;
  case VIEW_POKE_ITEMS:
    ui_handle_inventory_pokemon();
    break;
  case VIEW_SETTINGS:
    ui_handle_settings();
    break;
  case VIEW_GIFTS:
    ui_handle_inventory_items();
    break;
  case VIEW_STEP_HISTORY:
    if (sys_walkerFlags_BIT.session_active) {
      if (drv_button_is_triggered(BTN_ANY)) {
        ui_reset_substate();
        ui_set_view(VIEW_HOME);
        drv_sound_play(SND_CONFIRM);
      }
    }
    break;
  case VIEW_WALK_ARRIVAL_ANIM:
    ui_handle_walk_arrival_anim();
    break;
  case VIEW_WALK_DEPARTURE_ANIM:
    ui_handle_walk_departure_anim();
    break;
  case VIEW_EVENT_REWARD_ANIM:
    ui_handle_event_reward_anim();
    break;
  case VIEW_FACTORY_TEST:
    ui_handle_factory_test();
    break;
  case VIEW_ACCEL_DEBUG:
    /* The accel-debug view's render fn does the work; no per-tick input. */
    sys_noop();
    break;
  default:
    break;
  }
  g.DAT_f7ab++;                   /* coarse activity / debounce tick counter */
  /* Wait for the last LCD SPI transfer to drain, then drop chip-select. */
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// ROM: 0x7406  84.4%  saves: er2,er3,er4,er5,er6
void ui_dispatch_draw(void) {
  switch (g.currentlyActiveView) {
  case VIEW_HOME:
    if (!(sys_walkerFlags_BIT.session_active)) {
      ui_render_empty_eeprom();
    } else {
      ui_render_home_route();
      ui_render_home_bar();
    }
    break;
  case VIEW_BORED_GIFT:
    ui_render_bored_gift();
    break;
  case VIEW_PEER_PLAY:
    ui_render_peer_play();
    break;
  case VIEW_MAIN_MENU:
    ui_render_main_menu();
    break;
  case VIEW_DOWSING:
    ui_render_dowsing();
    break;
  case VIEW_POKERADAR:
    ui_render_pokeradar();
    break;
  case VIEW_BATTLE:
    ui_render_battle();
    break;
  case VIEW_RADAR_FAILURE:
    ui_render_radar_failure();
    break;
  case VIEW_DISCARD_PICKER:
    ui_render_discard_picker();
    break;
  case VIEW_TRAINER_CARD:
    ui_render_trainer_card();
    break;
  case VIEW_POKE_ITEMS:
    ui_render_inventory_pokemon();
    break;
  case VIEW_SETTINGS:
    ui_render_settings();
    break;
  case VIEW_GIFTS:
    ui_render_inventory_items();
    break;
  case VIEW_STEP_HISTORY:
    if (!(sys_walkerFlags_BIT.session_active)) {
      ui_render_sad_walker();
    } else {
      ui_render_step_history();
    }
    break;
  case VIEW_WALK_ARRIVAL_ANIM:
    ui_render_walk_arrival_anim();
    break;
  case VIEW_WALK_DEPARTURE_ANIM:
    ui_render_walk_departure_anim();
    break;
  case VIEW_EVENT_REWARD_ANIM:
    ui_render_event_reward_anim();
    break;
  case VIEW_FACTORY_TEST:
    ui_render_factory_test();
    break;
  case VIEW_ACCEL_DEBUG:
    ui_render_accel_debug();
    break;
  case VIEW_TEXT:
    /* Diagnostic view — renders a fixed string from EEPROM 0xBF93. */
    gfx_draw_string_simple();
    break;
  default:
    break;
  }
  /* Wait for the last LCD SPI transfer to drain, then drop chip-select. */
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// ROM: 0x974e  100.0%
void ui_clear_substate_y(void) { g.gCurSubstateY = 0; }
