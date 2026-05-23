#include "all_headers.h"

// ROM: 0x693a  51.7%  saves: r6
void sys_set_handler(event_loop_func_t func) {
  savedEventLoopFunc = currentEventLoopFunc;
  currentEventLoopFunc = func;
}

// ROM: 0x69b8  97.5%
void ui_set_view(uint8_t viewId) { currentlyActiveView = viewId; }

// ROM: 0x6a1c  75.5%
void ui_reset_substate(void) {
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 0x20;
  DAT_f7d1 &= ~0x07;
}

// ROM: 0x7348  85.7%  saves: er2,er3,er4,er5,er6
void ui_dispatch_event(void) {
  sys_get_rng();
  switch (currentlyActiveView) {
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
  case VIEW_CAUGHT_STATS:
    ui_handle_caught_stats_navigation();
    break;
  case VIEW_TRAINER_CARD:
    ui_handle_trainer_stats();
    break;
  case VIEW_POKE_ITEMS:
    ui_handle_poke_items();
    break;
  case VIEW_SETTINGS:
    ui_handle_settings();
    break;
  case VIEW_GIFTS:
    ui_handle_gifts();
    break;
  case VIEW_STEP_HISTORY:
    if (walker_status_flags_BIT.session_active) {
      if (drv_button_is_triggered(0x0E)) {
        ui_reset_substate();
        ui_set_view(VIEW_HOME);
        drv_sound_play(0);
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
  case VIEW_DEBUG:
    ui_handle_debug_input();
    break;
  case VIEW_ACCEL_DEBUG:
    sys_noop();
    break;
  default:
    break;
  }
  DAT_f7ab++;
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// ROM: 0x7406  84.2%  saves: er2,er3,er4,er5,er6
void ui_dispatch_draw(void) {
  switch (currentlyActiveView) {
  case VIEW_HOME:
    if (!(walker_status_flags_BIT.session_active)) {
      ui_render_empty_eeprom();
    } else {
      ui_render_home_route();
      ui_render_home_bar();
    }
    break;
  case VIEW_BORED_GIFT:
    ui_render_social_feelings();
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
    game_render_step_counter();
    break;
  case VIEW_CAUGHT_STATS:
    ui_render_inventory_stats_view();
    break;
  case VIEW_TRAINER_CARD:
    ui_render_trainer_card();
    break;
  case VIEW_POKE_ITEMS:
    ui_render_pokemon_stats();
    break;
  case VIEW_SETTINGS:
    ui_render_settings();
    break;
  case VIEW_GIFTS:
    ui_render_items_stats();
    break;
  case VIEW_STEP_HISTORY:
    if (!(walker_status_flags_BIT.session_active)) {
      ui_render_sad_walker();
    } else {
      ui_render_step_history_graph();
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
  case VIEW_DEBUG:
    ui_render_debug();
    break;
  case VIEW_ACCEL_DEBUG:
    ui_render_accel_debug();
    break;
  case VIEW_TEXT:
    gfx_draw_string_simple();
    break;
  default:
    break;
  }
  while (!SSSR_BIT.TEND)
    ;
  PDR1 |= 0x01;
}

// ROM: 0x974e  98.3%
void ui_clear_substate_y(void) { gCurSubstateY = 0; }
