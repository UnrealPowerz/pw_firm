#include "all_headers.h"

// ROM: 0x6954  83.2%
void sys_init_io_ports(event_loop_func_t a, event_loop_func_t b) {
  if (a != b) {
    return;
  }
  /* Connect setup disables interrupts, so the sound-timer ISR can't run
   * during ir_comm_loop. If a beep was still playing when we got here, its
   * buffer (ACCEL_SAMPLES_X) will be overwritten by IR payload data, and
   * when we exit connect and interrupts come back, drv_sound_update will
   * try to play that garbage as notes — producing a continuous screech.
   * Stop any in-progress sound up front. */
  soundData = NULL;
  if (!(walker_status_flags_BIT.session_active)) {
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

// ROM: 0x7882  95.8%
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
    } else if (statusFlags_BIT.button_event) {
      sys_power_save_low_power();
    }
  } else if ((walker_status_flags & 0x18) == 0x10) {
    game_check_periodic_events();
    ui_dispatch_event();
    if (currentEventLoopFunc == ir_comm_loop) {
      goto end;
    }
  }

  if (walker_status_flags_BIT.session_active) {
    if (accelSampleCount == 0x3F) {
      game_process_accel_data();
    }
  }

  if (statusFlags_BIT.tick) {
    if ((walker_status_flags & 0x18) == 0x10) {
      drv_lcd_clear_pages(0x40);
      ui_dispatch_draw();
      drv_lcd_flip();
      animTick++;
    }
    statusFlags_BIT.tick = 0;
  } else {
    game_dispatch_pedometer_task();
    if ((walker_status_flags & 0x18) == 0x10) {
      if (activityTimer == 0) {
        drv_lcd_power_save();
        walker_status_flags = (walker_status_flags & 0xE7) | 0x08;
        wakeupFlagMaybe[0] = 0;
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

// ROM: 0x7998  97.5%
void sys_main_loop_active(void) {
  SYSCR1 = 0x27;
  SYSCR2 = 0xE0;
  statusFlags_BIT.lcd_dirty = 1;
  if (GRA != 0) {
    sleep();
  }
  sys_wdt_kick();
  drv_button_read();
  ui_dispatch_event();

  if (statusFlags_BIT.tick) {
    if ((walker_status_flags & 0x18) == 0x10) {
      drv_lcd_clear_pages(0x40);
      ui_dispatch_draw();
      drv_lcd_flip();
      animTick++;
    }
    statusFlags_BIT.tick = 0;
  }

  if (!drv_sound_is_playing()) {
    drv_timerw_disable();
    sys_set_handler(sys_main_loop_low_power);
    accelSampleCount = 0;
  }
}
