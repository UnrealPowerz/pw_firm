#include "all_headers.h"

// ROM: 0x6954  88.4%
void sys_init_io_ports(uint16_t r0, uint16_t r1) {
  if (r0 != r1) {
    return;
  }
  if (!(walker_status_flags & 0x02)) {
    drv_lcd_clear_pages(0x40);
    ui_render_happy_walker(0);
    drv_lcd_flip();
    drv_lcd_clear_pages(0x40);
    ui_render_happy_walker(1);
  } else {
    drv_lcd_clear_pages(0x40);
    ui_draw_ir_icon(0);
    gfx_draw_battery_low(0, 0x58);
    drv_lcd_flip();
    drv_lcd_clear_pages(0x40);
    ui_draw_ir_icon(1);
    gfx_draw_battery_low(0, 0x58);
  }
  drv_lcd_flip();
  set_ccr(0x80);
  drv_ir_send_discovery();
  sys_set_handler(ir_comm_loop);
}

// ROM: 0x7882  87.2%
void sys_main_loop_low_power(void) {
  IENR2 |= 0x04;
  sys_enter_sleep(1);
  IENR2 &= ~0x04;
  IENR1 &= ~0x80;
  drv_accel_sample();
  IENR1 |= 0x80;
  drv_button_read();

  if (!(walker_status_flags & 0x18)) {
    game_dispatch_pedometer_task();
    if (game_detect_activity()) {
      sys_power_save_low_power();
    } else if (statusFlags & 0x08) {
      sys_power_save_low_power();
    }
  } else if ((walker_status_flags & 0x18) == 0x10) {
    if (currentlyActiveView == 0) {
      game_check_periodic_events();
      ui_dispatch_event();
      if (currentEventLoopFunc == (uint16_t)(uintptr_t)ir_comm_loop) {
        goto end;
      }
    }
  }

  if (walker_status_flags & 0x02) {
    if (accelSampleCount == 0x3F) {
      game_process_accel_data();
    }
  }

  if (statusFlags & 0x01) {
    if ((walker_status_flags & 0x18) == 0x10) {
      drv_lcd_clear_pages(0x40);
      ui_dispatch_draw();
      drv_lcd_flip();
      DAT_f7ac++;
    }
    statusFlags &= ~0x01;
  } else {
    game_dispatch_pedometer_task();
    if ((walker_status_flags & 0x18) == 0x10) {
      if (activityTimer == 0) {
        drv_lcd_power_save();
        walker_status_flags = (walker_status_flags & 0xE7) | 0x08;
        wakeupFlagMaybe = 0;
        buttonHoldDuration = 0;
      }
    } else {
      set_ccr(0x80);
      drv_adc_check_battery();
      set_ccr(0);
      game_check_pedometer_activity();
    }
  }

  game_pedometer_tick_counters();
  if (drv_sound_is_playing()) {
    sys_set_handler(sys_main_loop_active);
    IENR2 &= ~0x04;
    drv_timerw_enable();
  }

end:
  accelSampleCount = (accelSampleCount + 1) & 0x3F;
}

// ROM: 0x7998  93.5%
void sys_main_loop_active(void) {
  SYSCR1 = 0x27;
  SYSCR2 = 0xE0;
  statusFlags |= 0x10;
  if (GRA != 0) {
    sleep();
  }
  sys_wdt_kick();
  drv_button_read();
  ui_dispatch_event();

  if (statusFlags & 0x01) {
    if ((walker_status_flags & 0x18) == 0x10) {
      drv_lcd_clear_pages(0x40);
      ui_dispatch_draw();
      drv_lcd_flip();
      DAT_f7ac++;
    }
    statusFlags &= ~0x01;
  }

  if (!drv_sound_is_playing()) {
    drv_timerw_disable();
    sys_set_handler(sys_main_loop_low_power);
    accelSampleCount = 0;
  }
}
