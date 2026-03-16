#include "all_headers.h"

// ROM: 0x693a  61.7%
void sys_set_handler(void (*func)(void)) {
  DAT_f7e2 = currentEventLoopFunc;
  currentEventLoopFunc = (uint16_t)(uintptr_t)func;
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

// ROM: 0x7348  87.4%
void ui_dispatch_event(void) {
  drv_lcd_strobe_res();
  sys_get_rng();
  switch (currentlyActiveView) {
  case 0:
    ui_handle_home();
    break;
  case 0x0C:
    ui_handle_bored_gift();
    break;
  case 0x0D:
    ui_handle_peer_play();
    break;
  case 1:
    ui_handle_main_menu();
    break;
  case 2:
    ui_handle_dowsing();
    break;
  case 3:
    ui_handle_pokeradar();
    break;
  case 4:
    ui_handle_battle();
    break;
  case 6:
    ui_handle_radar_failure();
    break;
  case 7:
    ui_handle_caught_stats_navigation();
    break;
  case 8:
    ui_handle_trainer_stats();
    break;
  case 0x0A:
    ui_handle_poke_items();
    break;
  case 9:
    ui_handle_settings();
    break;
  case 0x0B:
    ui_handle_gifts();
    break;
  case 0x0E:
    if (walker_status_flags & 0x02) {
      if (drv_button_is_triggered(0x0E)) {
        ui_reset_substate();
        ui_set_view(0);
        drv_sound_play(0);
      }
    }
    break;
  case 0x0F:
    ui_handle_return_from_walk();
    break;
  case 0x10:
    ui_handle_start_walk();
    break;
  case 0x11:
    ui_handle_event_reward();
    break;
  case 0x16:
    ui_handle_debug_input();
    break;
  case 0x17:
    sys_noop();
    break;
  default:
    break;
  }
  DAT_f7ab++;
  drv_lcd_cs_high();
}

// ROM: 0x7406  89.6%
void ui_dispatch_draw(void) {
  drv_lcd_strobe_res();
  switch (currentlyActiveView) {
  case 0:
    if (!(walker_status_flags & 0x02)) {
      ui_render_empty_eeprom();
    } else {
      ui_render_home_route();
      ui_render_home_bar();
    }
    break;
  case 0x0C:
    ui_render_social_feelings();
    break;
  case 0x0D:
    ui_render_peer_play();
    break;
  case 1:
    ui_render_main_menu();
    break;
  case 2:
    ui_render_dowsing();
    break;
  case 3:
    ui_render_pokeradar();
    break;
  case 4:
    ui_render_battle();
    break;
  case 6:
    game_render_step_counter();
    break;
  case 7:
    ui_render_inventory_stats_view();
    break;
  case 8:
    ui_render_trainer_card();
    break;
  case 0x0A:
    ui_render_pokemon_stats();
    break;
  case 9:
    ui_render_settings();
    break;
  case 0x0B:
    ui_render_items_stats();
    break;
  case 0x0E:
    if (!(walker_status_flags & 0x02)) {
      ui_render_sad_walker();
    } else {
      ui_render_step_history_graph();
    }
    break;
  case 0x0F:
    ui_render_joined_walk_anim();
    break;
  case 0x10:
    ui_render_start_walk_anim();
    break;
  case 0x11:
    ui_render_return_from_walk_anim();
    break;
  case 0x16:
    ui_render_debug();
    break;
  case 0x17:
    ui_render_accel_debug();
    break;
  case 0x18:
    gfx_draw_string_simple();
    break;
  default:
    break;
  }
  drv_lcd_cs_high();
}

// ROM: 0x974e  98.3%
void ui_clear_substate_y(void) { gCurSubstateY = 0; }
