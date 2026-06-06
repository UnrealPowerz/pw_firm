/* B_RAM struct members — included inside `struct b_ram_section { ... }` in
 * globals.h. The full struct instance is `g` (declared in src/globals.c)
 * and anchored at 0xF780 by the linker.
 *
 * Types here match how the original macros viewed each RAM byte (uint8_t /
 * uint16_t / uint32_t / pointer / function-pointer). Where main.mar's
 * `.RES.B N` width is larger than the C-side access width, an explicit
 * `_pad_<addr>` field absorbs the difference so the byte offsets still
 * match main.mar exactly. Verified at byte level by
 * scripts/compare_data_layout.py.
 */

/* --- Persistent save block (24 B at 0xF780..0xF797) ---
 * Mirrored to EEPROM_SAVE_BLOCK / EEPROM_SAVE_BLOCK_BACKUP via
 * save_write_reliable; restored at boot in resetprg.c. Treated as one
 * contiguous block by the I/O path (see save.c). */
/* 0xF780 */ volatile uint32_t save_totalSteps;                /* lifetime step count; never reset */
/* 0xF784 */ volatile uint32_t save_walkStepCount;             /* steps taken during current walk session (zeroed at walk start) */
/* 0xF788 */ volatile uint32_t save_rtcTime;                   /* persisted RTC value; slaved from peer during IR sync if non-zero */
/* 0xF78C */ volatile uint16_t save_dayCounter;                /* increments at midnight; drives daily-reset logic */
/* 0xF78E */ volatile uint16_t save_watts;                     /* current g.save_watts balance (currency for items / battle stakes) */
/* 0xF790 */ volatile uint16_t save_sessionTicksElapsed;       /* ticks within current session (~minutes-scale) */
/* 0xF792 */ volatile uint8_t  save_stepWattCounter;           /* fractional g.save_watts accumulator (1 g.save_watt per N steps) */
/* 0xF793 */ volatile uint8_t  save_peerSlotIndex;             /* rotating index into the peer-log EEPROM ring */
/* 0xF794 */ volatile uint8_t  _peer_slot_tail[3];             /* main.mar reserved a uint32 here; tail unused */
/* 0xF797 */ volatile settings_byte_t save_settings;           /* packed user settings (mute, sound_volume, contrast); see settings_byte_t in types.h */

/* --- Button input (4 B; see drivers/buttons.c) --- */
/* 0xF798 */ volatile button_input_t btn_inputRaw;             /* current-frame button mask (set by drv_button_read from PDRB) */
/* 0xF799 */ volatile uint8_t  btn_inputPrevious;              /* previous frame's btn_inputRaw.BYTE — feeds edge detection */
/* 0xF79A */ volatile uint8_t  btn_trigger;                    /* (raw ^ prev) & raw — set of buttons that just became pressed this frame */
/* 0xF79B */ volatile uint8_t  btn_holdDuration;               /* ticks the right-button has been held; >=8 triggers deep sleep */

/* --- Activity / timing --- */
/* 0xF79C */ volatile uint32_t session_steps;                  /* steps accumulated since session start (drives reward calc) */
/* 0xF7A0 */ volatile uint16_t session_recentSteps;            /* recent-window step count (10-min-ish) used by reward tier */
/* 0xF7A2 */ volatile uint16_t session_idleSeconds;            /* seconds since last input or step; gates auto-sleep */
/* 0xF7A4 */ volatile uint8_t  rtc_seconds;                    /* live RTC seconds (read from peripheral each tick) */
/* 0xF7A5 */ volatile uint8_t  rtc_minutes;
/* 0xF7A6 */ volatile uint8_t  rtc_hours;
/* 0xF7A7 */ volatile ped_task_flags_t ped_taskFlags;          /* pedometer task dispatch flags (rotate, step, init); see types.h */
/* 0xF7A8 */ volatile uint8_t  notif_scheduledHour;            /* hour at which the daily reminder fires */
/* 0xF7A9 */ volatile uint8_t  lcd_shadeBase;                  /* base shade level applied across the framebuffer (fade dimmer) */
/* 0xF7AA */ volatile uint8_t  ui_menuCursor;                  /* main-menu cursor index (MENU_* enum in menu_consts.h) */
/* 0xF7AB */ volatile uint8_t  ui_dispatchTickCounter;         /* per-frame tick counter for dispatch (write-only) */
/* 0xF7AC */ volatile uint8_t  ui_animationTick;               /* free-running animation tick (used as phase by renderers) */
/* 0xF7AD */ volatile uint8_t  ir_resultCode;                  /* result of last IR session: 0=success, 1=user-cancel, 6=peer rejected, etc. */
/* 0xF7AE */ volatile uint8_t  accel_sampleCount;              /* circular index into the 64-byte accel sample buffers */
/* 0xF7AF */ volatile uint8_t  sys_activityTimer;              /* counts down from 0x5A on input; idle when 0 */
/* 0xF7B0 */ volatile uint8_t  ped_stepTimer;                  /* countdown between step-detect attempts */
/* 0xF7B1 */ volatile uint8_t  ui_activeView;                  /* current view ID (VIEW_* enum in globals.h) */
/* 0xF7B2 */ volatile uint8_t  ped_batchSize;                  /* number of samples per FFT batch */
/* 0xF7B3 */ volatile uint8_t  ped_subStepCount;               /* sub-tick step count (rolls into session_steps) */
/* 0xF7B4 */ volatile uint8_t  ped_batchAccumulator;           /* batch-level accumulator before commit */

/* --- System status flags (packed unions; see types.h) --- */
/* 0xF7B5 */ volatile status_flags_t sys_statusFlags;          /* tick, low_battery, button_event, eeprom_busy, sleeping, etc. */
/* 0xF7B6 */ volatile walker_status_t sys_walkerFlags;         /* mode (0x18 mask: active/low-power/deep-sleep), walking, session_active, input_pending */
/* 0xF7B7 */ volatile uint8_t  _pad_f7b7;

/* --- Command / wake / heap / RNG --- */
/* 0xF7B8 */ volatile uint16_t ir_lastCommandTime;             /* TCNT snapshot of last command — drives IR timeout */
/* 0xF7BA */ volatile uint8_t  ir_commandPos;                  /* cursor into the IR command stream */
/* 0xF7BB */ volatile uint8_t  sys_wakeFlag[3];                /* [0]=wake-from-deep-sleep flag; [1..2] alignment pad */
/* 0xF7BE */ volatile uint16_t sys_heapPointer;                /* sbrk break pointer; reset by sys_init_heap (sbrk.c) */
/* 0xF7C0 */ volatile uint32_t rng_state;                      /* LFSR; ticked once per input frame (ui_dispatch_event) */

/* --- Sound engine state (see drivers/sound.c) --- */
/* 0xF7C4 */ volatile uint8_t *sound_dataPointer;              /* current playback cursor into the period-table stream */
/* 0xF7C6 */ volatile uint8_t  sound_volume;                   /* 0..3 — independent of save_settings.sound_volume; per-cue runtime gain */
/* 0xF7C7 */ volatile uint8_t  _pad_f7c7;
/* 0xF7C8 */ volatile uint16_t sound_noteDuration;             /* ticks remaining for current note */
/* 0xF7CA */ volatile uint16_t sound_isSeparateNote;           /* nonzero -> emit a fresh gate edge before the next note */
/* 0xF7CC */ volatile uint8_t  sound_header;                   /* current cue's header byte (controls repeat / end) */
/* 0xF7CD */ volatile uint8_t  _pad_f7cd;

/* --- View-state region (0xF7CE..D9, 12 bytes) ---
 * 3-byte universal substate triple (Y/Z/A) + 9 bytes of view-multiplexed
 * state exposed as named per-subsystem substructs (battle/radar/dowsing/
 * peerPlay/factoryTest/accelDebug/homeStandby) — see struct viewstate in
 * globals.h above struct b_ram_section. */
/* 0xF7CE */ struct viewstate viewstate;

/* --- Pedometer thresholds (per-axis; loaded from EEPROM_ACCEL_CAL) --- */
/* 0xF7DA */ volatile uint16_t ped_axisStepThresholdLo;        /* low gate for accel position diff to count as a step */
/* 0xF7DC */ volatile uint16_t ped_axisStepThresholdHi;        /* high gate (above = motion artifact, ignored) */
/* 0xF7DE */ volatile uint16_t ped_axisIdleThreshold;          /* gate below which device is considered at rest */

/* --- Event loop / LCD state --- */
/* 0xF7E0 */ volatile event_loop_func_t sys_tickHandler;       /* per-tick handler (current view's handler) */
/* 0xF7E2 */ volatile event_loop_func_t sys_savedTickHandler;  /* saved handler restored on view switch back */
/* 0xF7E4 */ volatile uint8_t  lcd_pageOffset;                 /* current LCD page being written by the renderer */
/* 0xF7E5 */ volatile uint8_t  _pad_f7e5;

/* --- Multi-purpose overlay #1 (128 B at 0xF7E6..0xF865; pw_scratch union) ---
 * Time-multiplexed across IR-staged trainer record / FFT bins / X-accel
 * samples / peer RTC / second trainer record. See pw_scratch in globals.h. */
/* 0xF7E6 */ union pw_scratch  scratch1;

/* --- Multi-purpose overlay #2 (240 B at 0xF866..0xF955; pw_scratch2 union) ---
 * Y/Z accel samples / IR session state / IR command header / IR payload
 * (which also overlays step-detect state when IR is idle). See pw_scratch2
 * in globals.h. */
/* 0xF866 */ union pw_scratch2 scratch2;

/* --- EEPROM page scratch + sbrk heap region (1434 B at 0xF956..0xFEEF) ---
 * `eepromPageScratch` aliases into the sbrk heap area (heap base 0xF8F0,
 * cap 0x400) — it's not a separate buffer. Used by ir_protocol and
 * peer_play as a temp page-write buffer while the heap is idle. See
 * memory/b_ram_late_region.md. */
/* 0xF956 */ volatile uint8_t  eepromPageScratch[0x59A];       /* through 0xFEEF */
