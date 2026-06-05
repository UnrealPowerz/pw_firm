#include "all_headers.h"

/*
 * Pedometer — step detection, step accounting, g.save_watts accrual.
 *
 * Function clusters:
 *
 *   --- Accel-sample → step-rate pipeline ---
 *     game_detect_activity            Sum of 3-axis sample deltas — has motion?
 *     game_check_pedometer_activity   Wake from low-power if g.ped_stepTimer expired.
 *     game_process_accel_data         Main pipeline (FFT 3 axes + scan peaks +
 *                                     interpolate + commit step batch + accel-
 *                                     debug instrumentation).
 *     game_detect_steps_fft           Scan the FFT output for a step-rate peak
 *                                     bin; reject if dominated by noise.
 *     game_pedometer_interpolate_batch
 *                                     Parabolic interpolation around the peak
 *                                     bin to recover a sub-bin step rate.
 *
 *   --- Step / g.save_watts accounting ---
 *     game_pedometer_set_total        Setter with 9,999,999 cap.
 *     game_pedometer_increment_step   +1 step + 1/20-watt + save commit; also
 *                                     logs the step interaction when walking.
 *     game_pedometer_tick_counters    Batched +1 (called many times per tick
 *                                     based on g.ped_batchSize).
 *     game_add_watts                  Clamped += into g.save_watts.
 *     game_reset_step_data            Full reset (optionally also totals).
 *     game_rotate_step_history        Daily roll-over: shift the 6-day history
 *                                     and start a fresh g.session_steps slot.
 *
 *   --- Pedometer task dispatch (g.ped_taskFlags) ---
 *     game_pedometer_tick_session     Per-second g.save_sessionTicksElapsed bump
 *                                     (g.ped_taskFlags bit 0 dispatches to here).
 *     game_dispatch_pedometer_task    Pop bits off g.ped_taskFlags and run the
 *                                     corresponding task.
 *     game_reset_pedometer_flags      Clear the step-detection accumulators.
 *
 *   --- Step-gated unlocks ---
 *     game_check_step_unlock          Reads a uint16 step threshold from a
 *                                     caller-supplied buffer + offsets;
 *                                     returns 1 if NOT YET reached (slot is
 *                                     still locked). Used by the minigame
 *                                     encounter roller for tiered unlocks.
 *
 * Several functions carry "Reason / cannot-fix" markers describing the
 * specific ch38 vs ROM codegen mismatches; edit those with care.
 */

// ROM: 0xb124  99.4%
void game_reset_step_data(uint8_t a) {
  if (a != 0) {
    g.save_totalSteps = 0;
    g.save_dayCounter = 0;
    g.save_rtcTime = 0xD2B0B80;
    g.save_walkStepCount = 0;
  }
  g.save_sessionTicksElapsed = 0;
  g.save_watts = 0;
  g.save_stepWattCounter = 0;
  g.save_settings = (g.save_settings & 0xA4) | 0x24;
  g.save_peerSlotIndex = 0;
  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (uint8_t *)&g.save_totalSteps, 0x18);
}

// ROM: 0x9328  77.8%
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

// ROM: 0xa1a8  50.2%  saves: r6,r5
uint8_t game_detect_activity(void) {
  uint16_t total;
  uint16_t prev;
  volatile uint16_t p_copy;

  prev = g.ped_sampleCount;
  prev += 0x3F;
  prev &= 0x3F;
  p_copy = prev;

  if (((int16_t)accelXSamples[g.ped_sampleCount] -
       (int16_t)accelXSamples[prev]) >= 0) {
    total = (uint16_t)((int16_t)accelXSamples[g.ped_sampleCount] -
                       (int16_t)accelXSamples[p_copy]);
  } else {
    total = (uint16_t)((uint16_t)(-((int16_t)accelXSamples[g.ped_sampleCount])) +
                       (uint16_t)accelXSamples[p_copy]);
  }

  if (((int16_t)accelYSamples[g.ped_sampleCount] -
       (int16_t)accelYSamples[prev]) >= 0) {
    total += (uint16_t)((int16_t)accelYSamples[g.ped_sampleCount] -
                        (int16_t)accelYSamples[p_copy]);
  } else {
    total +=
        (uint16_t)((uint16_t)(-((int16_t)accelYSamples[g.ped_sampleCount])) +
                   (uint16_t)accelYSamples[p_copy]);
  }

  if (((int16_t)accelZSamples[g.ped_sampleCount] -
       (int16_t)accelZSamples[prev]) >= 0) {
    total += (uint16_t)((int16_t)accelZSamples[g.ped_sampleCount] -
                        (int16_t)accelZSamples[p_copy]);
  } else {
    total +=
        (uint16_t)((uint16_t)(-((int16_t)accelZSamples[g.ped_sampleCount])) +
                   (uint16_t)accelZSamples[p_copy]);
  }

  if (total > 30) {
    return 1;
  }
  return 0;
}

// ROM: 0xa2f6  83.8%
void game_check_pedometer_activity(void) {
  /* When g.ped_stepTimer hits 0 (no steps for a while), exit low-power sleep to
     resume scanning. g.ped_stepTimer is reset to 30 in game_process_accel_data
     whenever a non-zero step batch is committed. */
  if (g.ped_stepTimer == 0) {
    sys_wake_from_low_power();
  }
}

// ROM: 0xa32e  90.0%
void game_pedometer_set_total(uint32_t val) {
  g.save_totalSteps;
  if (val >= 9999999) {
    val = 9999999;
  }
  g.save_totalSteps = val;
}

/* Reason: do NOT bit-field-ize g.ped_taskFlags.
 * Tested converting `(g.ped_taskFlags & 0x0N)` to `ped_taskFlags_BIT.<name>` and the
 * function regressed by -12.8% (67.9% -> 55.1%).  The ROM tests these
 * three bits with `btst #N, r0l; beq` (the original C used `& mask` in
 * if-conditions), not with `bld; bcc`.  Flat-mask form is what matches
 * here, even though for g.sys_statusFlags the bit-field form is the one that
 * matches.  Lesson: always check `bld` vs `btst` count for the global in
 * main.mar before sweeping it to bit-field form.
 * Class: do-not-bit-field */
// ROM: 0xa34a  69.0%  saves: er2,r3,r5,er6
void game_dispatch_pedometer_task(void) {
  if (!sys_statusFlags_BIT.pedometer_paused) {
    if ((g.ped_taskFlags & 0x01)) {
      game_pedometer_tick_session();
    }
    if ((g.ped_taskFlags & 0x02)) {
      game_pedometer_increment_step();
    }
    if ((g.ped_taskFlags & 0x04)) {
      game_rotate_step_history();
    }
    g.ped_taskFlags &= 0xF8;
  }
}

// ROM: 0xa396  99.3%
void game_pedometer_tick_session(void) {
  /* Increment with overflow guard — g.save_sessionTicksElapsed saturates at 0xFFFF
     rather than wrapping to 0. Driven by g.ped_taskFlags bit 0 (per-second). */
  if (g.save_sessionTicksElapsed + 1 != 0) {
    g.save_sessionTicksElapsed++;
  }
}

// ROM: 0xa3aa  78.0%
void game_pedometer_increment_step(void) {
  sys_statusFlags_BIT.battery_check_request = 1;

  if (g.save_totalSteps < 9999999 && g.save_walkStepCount < 9999999) {
    g.save_walkStepCount++;
  }

  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (uint8_t *)&g.save_totalSteps, 0x18);

  if ((sys_walkerFlags_BIT.walking) != 0) {
    void *buf;
    void *extra_buf;
    uint16_t val;

    sys_init_heap();
    buf = sbrk(0xBE);
    drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, buf, 0xBE);

    val = 0;
    if (((g.save_settings & 1)) != 0) {
      val = 1;
    }

    extra_buf = sbrk(0x88);
    game_log_interaction(buf, extra_buf, 0x1B, (uint8_t)val, 0, 0);
  }

  g.session_recentSteps = 0;
  if (g.rtc_hours == g.notif_scheduledHour) {
    g.ped_taskFlags |= 0x04;
  }
}

// ROM: 0xa45e  62.9%
void game_rotate_step_history(void) {
  void *buf;
  uint8_t i;
  uint8_t j;

  {
    uint16_t d = g.save_dayCounter;
    if (d < 9999) {
      g.save_dayCounter = d + 1;
    }
  }

  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (uint8_t *)&g.save_totalSteps, 0x18);

  sys_init_heap();
  buf = sbrk(0x1C);
  drv_eeprom_read_block(EEPROM_LOG_POKE_STATS, buf, 0x1C);

  for (i = 0; i < 6; i++) {
    ((uint32_t *)((uint8_t *)buf + 24))[-(int)i] =
        ((uint32_t *)((uint8_t *)buf + 20))[-(int)i];
  }

  *(uint32_t *)buf = g.session_steps;
  drv_eeprom_write_block(EEPROM_LOG_POKE_STATS, buf, 0x1C);

  g.session_steps = 0;

  for (j = 10; j != 0; j--) {
    drv_eeprom_fill((uint16_t)((j * 0x224) + 0xDC08), 0x0028, 0xFF);
  }
}

// ROM: 0x9698  72.7%  saves: er6
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
    uint8_t bin = FFT_BINS[i];
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
// ROM: 0x945a  78.2%  saves: er2,er3,er4,er5,er6
void game_process_accel_data(void) {
  uint32_t steps;
  uint16_t i;
  uint8_t view, sub, limit;
  uint16_t threshold, tx, ty, tz;

  sys_statusFlags_BIT.sleeping = 0;

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

  view = g.ui_activeView;
  if (view == VIEW_ACCEL_DEBUG) {
    sub = g.ui_substateA;
    limit = g.DAT_f7d8;
    if (sub < limit) {
      if (steps != 0) {
        g.ui_substateA = sub + 1;
        threshold = g.ped_axisStepThresholdLo;
        tx = accel_xPosition_word;
        if (tx < threshold)
          g.ui_activeView = VIEW_TEXT;
        ty = accel_yPosition_word;
        if (ty < threshold)
          g.ui_activeView = VIEW_TEXT;
        tz = g.accel_zPosition;
        if (tz < threshold)
          g.ui_activeView = VIEW_TEXT;
        threshold = g.ped_axisStepThresholdHi;
        tx = accel_xPosition_word;
        if (tx > threshold)
          g.ui_activeView = VIEW_TEXT;
        ty = accel_yPosition_word;
        if (ty > threshold)
          g.ui_activeView = VIEW_TEXT;
        tz = g.accel_zPosition;
        if (tz > threshold)
          g.ui_activeView = VIEW_TEXT;
      }
    } else if (g.DAT_f7d1 < g.DAT_f7d8_1) {
      threshold = g.ped_axisIdleThreshold;
      if (accel_xPosition_word < threshold && accel_yPosition_word < threshold &&
          g.accel_zPosition < threshold) {
        g.DAT_f7d1++;
      }
    }
  }

  if (steps == 0) {
    pendingStepDetect = 0;
  } else {
    sys_statusFlags_BIT.sleeping = 1;

    if (pendingStepDetect != 0) {
      uint32_t accumulation = stepDetectAccum + pendingStepDetect;
      stepDetectAccum = accumulation;
      pendingStepDetect = 0;

      g.ped_batchSize = (uint8_t)(accumulation >> 9);
      stepDetectAccum = accumulation & 0x1FF;

      g.session_recentSteps += (uint16_t)g.ped_batchSize;
      if (g.session_recentSteps > 9999) {
        g.session_recentSteps = 9999;
      }

      g.session_steps += (uint32_t)g.ped_batchSize;
      if (g.session_steps > 99999) {
        g.session_steps = 99999;
      }

      game_pedometer_set_total(g.save_totalSteps + (uint32_t)g.ped_batchSize);

      g.save_stepWattCounter += g.ped_batchSize;
      if (g.save_stepWattCounter >= 20) {
        g.save_stepWattCounter -= 20;
        i = g.save_watts + 1;
        if (i > 9999) {
          i = 9999;
        }
        g.save_watts = i;
      }
    }

    {
      uint32_t accumulation = stepDetectAccum + steps;
      stepDetectAccum = accumulation;
      g.ped_batchSize = (uint8_t)(accumulation >> 9);
      stepDetectAccum = accumulation & 0x1FF;
    }

    if (g.ped_batchSize != 0) {
      g.ped_stepTimer = 30;
    }
    g.ped_subStepCount = 0;
    g.ped_batchAccumulator = 32;
  }
}

// Reads a little-endian uint16 "step threshold" from buf[a + b] and reports
// whether the player has NOT yet reached it (g.session_steps < threshold), i.e.
// the slot is still locked. ROM took the buffer pointer implicitly in r5 (a
// caller-saved register the callers leave set) and returned the comparison via
// the carry flag; both callers do `bcs <skip>`. Passing `buf` explicitly is
// the faithful, readable equivalent — it won't byte-match ROM (extra arg in a
// register) but is semantically exact.
// ROM: 0x4f50  19.3%
uint8_t game_check_step_unlock(uint16_t a, uint16_t b, const uint8_t *buf) {
  const uint8_t *p = buf + a + b;
  uint16_t threshold = (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
  return (uint8_t)(g.session_steps < (uint32_t)threshold);
}

// ROM: 0x24ac  91.6%
void game_pedometer_tick_counters(void) {
  if (g.ped_subStepCount == g.ped_batchSize) {
    return;
  }

  g.ped_batchAccumulator += g.ped_batchSize;
  if (g.ped_batchAccumulator <= 0x40) {
    return;
  }

  g.session_recentSteps++;
  if (g.session_recentSteps > 9999) {
    g.session_recentSteps = 9999;
  }

  g.session_steps++;
  if (g.session_steps > 99999) {
    g.session_steps = 99999;
  }

  game_pedometer_set_total(g.save_totalSteps + 1);

  g.save_stepWattCounter++;
  if (g.save_stepWattCounter >= 20) {
    uint16_t w;
    g.save_stepWattCounter -= 20;
    w = g.save_watts + 1;
    if (w > 9999) {
      w = 9999;
    }
    g.save_watts = w;
  }

  g.ped_subStepCount++;
  if (g.ped_subStepCount > g.ped_batchSize) {
    g.ped_subStepCount = g.ped_batchSize;
  }

  g.ped_batchAccumulator -= 0x40;
}

// ROM: 0x1f3e  91.1%
void game_add_watts(uint16_t amount) {
  g.save_watts += amount;
  if (g.save_watts > 9999) {
    g.save_watts = 9999;
  }
  save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&g.save_totalSteps, 0x18);
}
