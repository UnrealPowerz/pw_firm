#include "all_headers.h"

#pragma section P

// ROM: 0x02c4  84.8%
__entry(vect = 0) void PowerON_Reset(void) {
  uint16_t i;
  uint8_t cnt;

  set_imask_ccr((uint8_t)1);
  _INITSCT();
  HardwareSetup();
  set_imask_ccr((uint8_t)0);

  sys_init_heap();
  drv_accel_init();

  if (TCSRWD1_BIT.WRST) {
    cnt = drv_eeprom_read_u8(EEPROM_BOOT_COUNTER);
    cnt++;
    drv_eeprom_write_u8(EEPROM_BOOT_COUNTER, cnt);
  }

  /* totalSteps is volatile uint32_t, cast to uint8_t* for zeroing */
  for (i = 0; i < 0x3E; i++) {
    ((uint8_t *)&totalSteps)[i] = 0;
  }

  scheduledNotifyHour = 0;
  statusFlags_BIT.lcd_dirty = 1;
  walker_status_flags = (walker_status_flags & 0xE7) | 0x10;

  activityTimer = 0x3C;
  stepTimer = 0x5A;
  idleSeconds = 0xE10;

  game_reset_pedometer_flags();
  sys_factory_test();
  sys_wdt_unlock();
  sys_sync_eeprom_on_startup();
  sys_wdt_init();

  do {
    sys_wdt_kick();
  } while (drv_adc_check_low_battery(0x13) == 0);

  drv_timerw_init();

  {
    uint8_t vol = (RamCache_settingsByte >> 1) & 0x3;
    drv_sound_set_volume(vol);
  }

  drv_lcd_init();
  drv_rtc_load();

  {
    uint32_t stack_val;
    drv_eeprom_read_block(0x0153, &stack_val, 4);
    sys_seed_rng(stack_val);
  }

  drv_ir_init_output_pins();
  drv_buttons_init_irqs();
  drv_accel_init();

  sys_set_handler(sys_main_loop_low_power);
  ui_reset_substate();
  currentlyActiveView = VIEW_HOME;
  drv_rtc_init_timer_b();

  while (1) {
    set_ccr(0x00);
    currentEventLoopFunc();
  }
}
