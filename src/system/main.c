#include "all_headers.h"

/*
 * Main event-loop entry points.
 *
 *   sys_main_loop_low_power    Default idle loop body. Sleeps the CPU,
 *                              wakes on accel/RTC IRQ, samples accel +
 *                              buttons, ticks the pedometer task queue,
 *                              renders if a tick fired. Spins up the
 *                              active loop if sound starts playing.
 *
 *   sys_main_loop_active       Sound-playing loop body. Stays awake through
 *                              sleep() while sound queue drains. Drops back
 *                              to the low-power loop when sound completes.
 *
 * The IR-session kickoff (sys_begin_ir_session) lives in
 * src/engine/ir_protocol.c alongside the rest of the IR machinery.
 */

// ROM: 0x7882  95.8%
void sys_main_loop_low_power(void) {
  IENR2 |= 0x04;
  sys_enter_sleep(1);
  IENR2 &= ~0x04;
  IENR1 &= ~0x80;
  drv_accel_sample();
  IENR1 |= 0x80;
  drv_button_read();

  if (!(g.sys_walkerFlags & WALKER_MODE_MASK)) {
    game_dispatch_pedometer_task();
    if (game_detect_activity()) {
      sys_enter_low_power();
    } else if (sys_statusFlags_BIT.button_event) {
      sys_enter_low_power();
    }
  } else if ((g.sys_walkerFlags & WALKER_MODE_MASK) == WALKER_MODE_DEEP_SLEEP) {
    game_check_periodic_events();
    ui_dispatch_event();
    if (g.sys_tickHandler == ir_comm_loop) {
      goto end;
    }
  }

  if (sys_walkerFlags_BIT.session_active) {
    if (g.accel_sampleCount == 0x3F) {
      game_process_accel_data();
    }
  }

  if (sys_statusFlags_BIT.tick) {
    if ((g.sys_walkerFlags & WALKER_MODE_MASK) == WALKER_MODE_DEEP_SLEEP) {
      drv_lcd_clear_pages(0x40);
      ui_dispatch_draw();
      drv_lcd_flip();
      g.ui_animationTick++;
    }
    sys_statusFlags_BIT.tick = 0;
  } else {
    game_dispatch_pedometer_task();
    if ((g.sys_walkerFlags & WALKER_MODE_MASK) == WALKER_MODE_DEEP_SLEEP) {
      if (g.sys_activityTimer == 0) {
        drv_lcd_power_save();
        g.sys_walkerFlags = (g.sys_walkerFlags & 0xE7) | WALKER_MODE_LOW_POWER;
        g.sys_wakeFlag[0] = 0;
        g.btn_holdDuration = 0;
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
  g.accel_sampleCount = (g.accel_sampleCount + 1) & 0x3F;
}

// ROM: 0x7998  97.5%
void sys_main_loop_active(void) {
  SYSCR1 = 0x27;
  SYSCR2 = 0xE0;
  sys_statusFlags_BIT.lcd_dirty = 1;
  if (GRA != 0) {
    sleep();
  }
  sys_wdt_kick();
  drv_button_read();
  ui_dispatch_event();

  if (sys_statusFlags_BIT.tick) {
    if ((g.sys_walkerFlags & WALKER_MODE_MASK) == WALKER_MODE_DEEP_SLEEP) {
      drv_lcd_clear_pages(0x40);
      ui_dispatch_draw();
      drv_lcd_flip();
      g.ui_animationTick++;
    }
    sys_statusFlags_BIT.tick = 0;
  }

  if (!drv_sound_is_playing()) {
    drv_timerw_disable();
    sys_set_handler(sys_main_loop_low_power);
    g.accel_sampleCount = 0;
  }
}
