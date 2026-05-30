#include "all_headers.h"

// ROM: 0xb124  96.9%
void game_reset_step_data(uint8_t a) {
  if (a != 0) {
    totalSteps = 0;
    dayCounter = 0;
    rtcTime = 0xD2B0B80;
    RamCache_STEP_COUNT_maybe = 0;
  }
  sessionTicksElapsed = 0;
  watts = 0;
  stepWattCounter = 0;
  RamCache_settingsByte = (RamCache_settingsByte & 0xA4) | 0x24;
  peerSlotIndex = 0;
  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (uint8_t *)&totalSteps, 0x18);
}

// ROM: 0x9328  75.6%
void game_reset_pedometer_flags(void) {
  DAT_f8ee = 0;
  pendingStepDetect = 0;
  stepDetectAccum = 0;
  isNotWalking = 0;
}

// Reason: ROM saves r2/e4 (words) + er6 (long) = 8 bytes via mixed
//   `push.w r2; push.w e4; push.l er6`; ch38 saves er6/er5/r4 = 10 bytes
//   via `push.l er6; push.l er5; push.w r4`. Different register selection
//   for the same data. ROM also does `mov.l @r6, er4` (32-bit load of two
//   adjacent uint16_ts together); ch38 splits into two `mov.w` loads. C
//   would need explicit `*(uint32_t *)ptr` to coax 32-bit load but the
//   following body uses both halves separately, so the compiler is right
//   to split. Body logic (interpolation between consecutive samples)
//   matches.
// Class: cannot-fix-without-compiler-change (mixed push.w/push.l + paired
//   16-bit load fusion)
// ROM: 0x9342  32.6%  saves: r2,e4,er6
uint32_t game_pedometer_interpolate_batch(uint8_t flags, uint16_t arg2) {
  uint32_t n;
  uint16_t d;
  uint16_t *ptr = (uint16_t *)(uint32_t)arg2;
  uint16_t v5, v6, v4;

  if (flags == 0) {
    uint16_t e4 = ptr[0];
    uint16_t r4 = ptr[1];
    d = e4 + r4;
    v5 = (uint16_t)(flags + 5) << 9;
    n = (uint32_t)v5 * e4;
    v6 = (uint16_t)(flags + 6) << 9;
    n += (uint32_t)v6 * r4;
  } else if (flags == 9) {
    uint16_t e4 = ptr[8];
    uint16_t e6 = ptr[9];
    d = e4 + e6;
    n = (uint32_t)e4 * 0x1A00 + (uint32_t)e6 * 0x1C00;
  } else {
    uint16_t lr4 = ptr[flags - 1];
    uint16_t le4 = ptr[flags + 1];
    uint16_t lr3 = ptr[flags];
    d = lr4 + le4 + lr3;

    v4 = (uint16_t)(flags + 4) << 9;
    n = (uint32_t)v4 * lr4;
    v6 = (uint16_t)(flags + 6) << 9;
    n += (uint32_t)v6 * le4;
    v5 = (uint16_t)(flags + 5) << 9;
    n += (uint32_t)v5 * lr3;
  }

  return n / (uint32_t)d;
}

// ROM: 0xa12c  63.2%
void game_render_step_counter(void) {
  uint16_t r6;
  uint8_t *ram_ptr;

  sys_init_heap();
  ram_ptr = sbrk(0xC0);
  drv_eeprom_read_block(0x1CB0, ram_ptr, 0xC0);

  for (r6 = 0; r6 < 4; r6++) {
    drv_lcd_blit(radarYCoordTable[r6], (uint8_t)((r6 & 1) * 0x18),
                 ram_ptr, 0x20, 0x18);
  }

  gfx_draw_text_box(0x30, 0x1E, 0x0F, 0x01);
  gfx_draw_battery_low(0, 0);
}

// ROM: 0xa1a8  54.4%  saves: r6,r5
uint8_t game_detect_activity(void) {
  uint16_t total;
  uint16_t prev;
  volatile uint16_t p_copy;

  prev = accelSampleCount;
  prev += 0x3F;
  prev &= 0x3F;
  p_copy = prev;

  if (((int16_t)accelXSamples[accelSampleCount] -
       (int16_t)accelXSamples[prev]) >= 0) {
    total = (uint16_t)((int16_t)accelXSamples[accelSampleCount] -
                       (int16_t)accelXSamples[p_copy]);
  } else {
    total = (uint16_t)((uint16_t)(-((int16_t)accelXSamples[accelSampleCount])) +
                       (uint16_t)accelXSamples[p_copy]);
  }

  if (((int16_t)accelYSamples[accelSampleCount] -
       (int16_t)accelYSamples[prev]) >= 0) {
    total += (uint16_t)((int16_t)accelYSamples[accelSampleCount] -
                        (int16_t)accelYSamples[p_copy]);
  } else {
    total +=
        (uint16_t)((uint16_t)(-((int16_t)accelYSamples[accelSampleCount])) +
                   (uint16_t)accelYSamples[p_copy]);
  }

  if (((int16_t)accelZSamples[accelSampleCount] -
       (int16_t)accelZSamples[prev]) >= 0) {
    total += (uint16_t)((int16_t)accelZSamples[accelSampleCount] -
                        (int16_t)accelZSamples[p_copy]);
  } else {
    total +=
        (uint16_t)((uint16_t)(-((int16_t)accelZSamples[accelSampleCount])) +
                   (uint16_t)accelZSamples[p_copy]);
  }

  if (total > 30) {
    return 1;
  }
  return 0;
}

// ROM: 0xa2f6  82.5%
void game_check_pedometer_activity(void) {
  if (stepTimer == 0) {
    sys_wake_from_low_power();
  }
}

// ROM: 0xa32e  88.3%
void game_pedometer_set_total(uint32_t val) {
  totalSteps;
  if (val >= 9999999) {
    val = 9999999;
  }
  totalSteps = val;
}

/* Reason: do NOT bit-field-ize pedTaskFlags.
 * Tested converting `(pedTaskFlags & 0x0N)` to `pedTaskFlags_BIT.<name>` and the
 * function regressed by -12.8% (67.9% -> 55.1%).  The ROM tests these
 * three bits with `btst #N, r0l; beq` (the original C used `& mask` in
 * if-conditions), not with `bld; bcc`.  Flat-mask form is what matches
 * here, even though for statusFlags the bit-field form is the one that
 * matches.  Lesson: always check `bld` vs `btst` count for the global in
 * main.mar before sweeping it to bit-field form.
 * Class: do-not-bit-field */
// ROM: 0xa34a  67.9%  saves: er2,r3,r5,er6
void game_dispatch_pedometer_task(void) {
  if (!statusFlags_BIT.pedometer_paused) {
    if ((pedTaskFlags & 0x01)) {
      game_pedometer_init_counters();
    }
    if ((pedTaskFlags & 0x02)) {
      game_pedometer_increment_step();
    }
    if ((pedTaskFlags & 0x04)) {
      game_rotate_step_history();
    }
    pedTaskFlags &= 0xF8;
  }
}

// ROM: 0xa396  97.1%
void game_pedometer_init_counters(void) {
  if (sessionTicksElapsed + 1 != 0) {
    sessionTicksElapsed++;
  }
}

// ROM: 0xa3aa  78.6%
void game_pedometer_increment_step(void) {
  statusFlags_BIT.battery_check_request = 1;

  if (totalSteps < 9999999 && RamCache_STEP_COUNT_maybe < 9999999) {
    RamCache_STEP_COUNT_maybe++;
  }

  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (uint8_t *)&totalSteps, 0x18);

  if ((walker_status_flags_BIT.walking) != 0) {
    void *buf;
    void *extra_buf;
    uint16_t val;

    sys_init_heap();
    buf = sbrk(0xBE);
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, buf, 0xBE);

    val = 0;
    if (((RamCache_settingsByte & 1)) != 0) {
      val = 1;
    }

    extra_buf = sbrk(0x88);
    game_log_interaction(buf, extra_buf, 0x1B, (uint8_t)val, 0, 0);
  }

  recentSessionSteps = 0;
  if (rtcHour == scheduledNotifyHour) {
    pedTaskFlags |= 0x04;
  }
}

// ROM: 0xa45e  59.1%
void game_rotate_step_history(void) {
  void *buf;
  uint8_t i;
  uint8_t j;

  {
    uint16_t d = dayCounter;
    if (d < 9999) {
      dayCounter = d + 1;
    }
  }

  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (uint8_t *)&totalSteps, 0x18);

  sys_init_heap();
  buf = sbrk(0x1C);
  drv_eeprom_read_block(EEPROM_LOG_POKE_STATS, buf, 0x1C);

  for (i = 0; i < 6; i++) {
    ((uint32_t *)((uint8_t *)buf + 24))[-(int)i] =
        ((uint32_t *)((uint8_t *)buf + 20))[-(int)i];
  }

  *(uint32_t *)buf = sessionSteps;
  drv_eeprom_write_block(EEPROM_LOG_POKE_STATS, buf, 0x1C);

  sessionSteps = 0;

  for (j = 10; j != 0; j--) {
    drv_eeprom_fill((uint16_t)((j * 0x224) + 0xDC08), 0x0028, 0xFF);
  }
}

// ROM: 0x9698  73.1%  saves: er6
uint32_t game_detect_steps_fft(volatile int16_t *fft_res) {
  uint16_t peakVal;
  uint16_t maxVal;
  uint8_t peakBin;
  uint8_t i;
  volatile int16_t *p;
  uint16_t val;
  uint16_t *binBase;

  maxVal = 0;
  p = fft_res + 1;
  i = 0;
  do {
    val = (uint16_t)*p++;
    if (val > maxVal)
      maxVal = val;
  } while (++i < 29);

  peakVal = 0;
  peakBin = 0xFF;
  binBase = (uint16_t *)((uint8_t *)fft_res + 10);
  i = 0;
  do {
    uint8_t bin = fftBinTable[i];
    uint16_t *pVal = binBase + bin;
    val = *pVal;
    if (val >= 512) {
      if ((uint16_t)(peakVal * 3) < (uint16_t)(val * 2)) {
        peakVal = val;
        peakBin = bin;
      }
    }
  } while (++i < 10);

  if (peakBin != 0xFF) {
    if (isNotWalking) {
      if (maxVal <= (uint16_t)(peakVal * 4 / 3)) {
        goto success;
      }
    } else {
      if (maxVal <= (uint16_t)(peakVal << 1)) {
        goto success;
      }
    }
  }

  DAT_f8ee = 0;
  pendingStepDetect = 0;
  isNotWalking = 1;
  return 0;

success:
  isNotWalking = 0;
  return (uint32_t)game_pedometer_interpolate_batch(
      peakBin, (uint16_t)(uint32_t)binBase);
}

// Reason: ROM saves er2/er3/er4/er5/er6 separately (5 push.l = 20 bytes); ch38
//   saves only er6/er5 (8 bytes). Different register-usage pattern means
//   stack-arg/local offsets diverge. ROM also unrolls the initial zero-fill
//   loop by 2 (`mov.w r0, @(addr, r6); inc.w #2, r6; mov.w r0, @(addr, r6);
//   inc.w #2, r6`); ch38 emits one write + one increment per iteration.
//   Body's accel-FFT pipeline, threshold checks, and step counting match.
// Class: cannot-fix-without-compiler-change (callee-save set + loop unroll)
// ROM: 0x945a  59.4%  saves: er2,er3,er4,er5,er6
void game_process_accel_data(void) {
  uint32_t steps;
  uint16_t i;
  uint8_t view, sub, limit;
  uint16_t threshold, tx, ty, tz;

  statusFlags_BIT.sleeping = 0;

  i = 0;
  do {
    fft_results[i] = 0;
    fft_results[i + 1] = 0;
    i += 2;
  } while (i < 32);

  drv_accel_fft(ACCEL_SAMPLES_X);
  drv_accel_fft(ACCEL_SAMPLES_Y);
  drv_accel_fft(ACCEL_SAMPLES_Z);

  steps = game_detect_steps_fft(fft_results);

  view = currentlyActiveView;
  if (view == VIEW_ACCEL_DEBUG) {
    sub = gCurSubstateA;
    limit = DAT_f7d8;
    if (sub < limit) {
      if (steps != 0) {
        gCurSubstateA = sub + 1;
        threshold = axisStepThresholdLo;
        tx = accelPos_X;
        if (tx < threshold)
          currentlyActiveView = VIEW_TEXT;
        ty = accelPos_Y;
        if (ty < threshold)
          currentlyActiveView = VIEW_TEXT;
        tz = accelZPos;
        if (tz < threshold)
          currentlyActiveView = VIEW_TEXT;
        threshold = axisStepThresholdHi;
        tx = accelPos_X;
        if (tx > threshold)
          currentlyActiveView = VIEW_TEXT;
        ty = accelPos_Y;
        if (ty > threshold)
          currentlyActiveView = VIEW_TEXT;
        tz = accelZPos;
        if (tz > threshold)
          currentlyActiveView = VIEW_TEXT;
      }
    } else if (DAT_f7d1 < DAT_f7d8_1) {
      threshold = axisIdleThreshold;
      if (accelPos_X < threshold && accelPos_Y < threshold &&
          accelZPos < threshold) {
        DAT_f7d1++;
      }
    }
  }

  if (steps == 0) {
    pendingStepDetect = 0;
  } else {
    statusFlags_BIT.sleeping = 1;

    if (pendingStepDetect != 0) {
      uint32_t accumulation = stepDetectAccum + pendingStepDetect;
      stepDetectAccum = accumulation;
      pendingStepDetect = 0;

      stepBatchSize = (uint8_t)(accumulation >> 9);
      stepDetectAccum = accumulation & 0x1FF;

      recentSessionSteps += (uint16_t)stepBatchSize;
      if (recentSessionSteps > 9999) {
        recentSessionSteps = 9999;
      }

      sessionSteps += (uint32_t)stepBatchSize;
      if (sessionSteps > 99999) {
        sessionSteps = 99999;
      }

      game_pedometer_set_total(totalSteps + (uint32_t)stepBatchSize);

      stepWattCounter += stepBatchSize;
      if (stepWattCounter >= 20) {
        stepWattCounter -= 20;
        i = watts + 1;
        if (i > 9999) {
          i = 9999;
        }
        watts = i;
      }
    }

    {
      uint32_t accumulation = stepDetectAccum + steps;
      stepDetectAccum = accumulation;
      stepBatchSize = (uint8_t)(accumulation >> 9);
      stepDetectAccum = accumulation & 0x1FF;
    }

    if (stepBatchSize != 0) {
      stepTimer = 30;
    }
    subStepCount = 0;
    batchAccumulator = 32;
  }
}

// Reads a little-endian uint16 "step threshold" from buf[a + b] and reports
// whether the player has NOT yet reached it (sessionSteps < threshold), i.e.
// the slot is still locked. ROM took the buffer pointer implicitly in r5 (a
// caller-saved register the callers leave set) and returned the comparison via
// the carry flag; both callers do `bcs <skip>`. Passing `buf` explicitly is
// the faithful, readable equivalent — it won't byte-match ROM (extra arg in a
// register) but is semantically exact.
// ROM: 0x4f50
uint8_t game_check_step_unlock(uint16_t a, uint16_t b, const uint8_t *buf) {
  const uint8_t *p = buf + a + b;
  uint16_t threshold = (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
  return (uint8_t)(sessionSteps < (uint32_t)threshold);
}

// ROM: 0x24ac  89.7%
void game_pedometer_tick_counters(void) {
  if (subStepCount == stepBatchSize) {
    return;
  }

  batchAccumulator += stepBatchSize;
  if (batchAccumulator <= 0x40) {
    return;
  }

  recentSessionSteps++;
  if (recentSessionSteps > 9999) {
    recentSessionSteps = 9999;
  }

  sessionSteps++;
  if (sessionSteps > 99999) {
    sessionSteps = 99999;
  }

  game_pedometer_set_total(totalSteps + 1);

  stepWattCounter++;
  if (stepWattCounter >= 20) {
    uint16_t w;
    stepWattCounter -= 20;
    w = watts + 1;
    if (w > 9999) {
      w = 9999;
    }
    watts = w;
  }

  subStepCount++;
  if (subStepCount > stepBatchSize) {
    subStepCount = stepBatchSize;
  }

  batchAccumulator -= 0x40;
}

// ROM: 0x1f3e  89.6%
void game_add_watts(uint16_t amount) {
  watts += amount;
  if (watts > 9999) {
    watts = 9999;
  }
  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
}
