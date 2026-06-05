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

/* --- Persistent save block (24 B; mirrored to EEPROM via save_write_reliable) --- */
/* 0xF780 */ volatile uint32_t save_totalSteps;
/* 0xF784 */ volatile uint32_t save_walkStepCount;  /* walk step count */
/* 0xF788 */ volatile uint32_t save_rtcTime;
/* 0xF78C */ volatile uint16_t save_dayCounter;
/* 0xF78E */ volatile uint16_t save_watts;
/* 0xF790 */ volatile uint16_t save_sessionTicksElapsed;
/* 0xF792 */ volatile uint8_t  save_stepWattCounter;
/* 0xF793 */ volatile uint8_t  save_peerSlotIndex;
/* 0xF794 */ volatile uint8_t  _peer_slot_tail[3];     /* original alloc was uint32 */
/* 0xF797 */ volatile uint8_t  save_settings;           /* aka RamCache_settingsByte */

/* --- Button input (4 B) --- */
/* 0xF798 */ volatile uint8_t  btn_inputRaw;          /* bit-field via _BIT */
/* 0xF799 */ volatile uint8_t  btn_inputPrevious;
/* 0xF79A */ volatile uint8_t  btn_trigger;
/* 0xF79B */ volatile uint8_t  btn_holdDuration;

/* --- Activity / timing --- */
/* 0xF79C */ volatile uint32_t session_steps;
/* 0xF7A0 */ volatile uint16_t session_recentSteps;
/* 0xF7A2 */ volatile uint16_t session_idleSeconds;
/* 0xF7A4 */ volatile uint8_t  rtc_seconds;
/* 0xF7A5 */ volatile uint8_t  rtc_minutes;
/* 0xF7A6 */ volatile uint8_t  rtc_hours;
/* 0xF7A7 */ volatile uint8_t  ped_taskFlags;           /* bit-field via _BIT */
/* 0xF7A8 */ volatile uint8_t  notif_scheduledHour;
/* 0xF7A9 */ volatile uint8_t  lcd_shadeBase;
/* 0xF7AA */ volatile uint8_t  ui_menuCursor;
/* 0xF7AB */ volatile uint8_t  ui_dispatchTickCounter;               /* write-only dispatch tick */
/* 0xF7AC */ volatile uint8_t  ui_animationTick;
/* 0xF7AD */ volatile uint8_t  ir_resultCode;
/* 0xF7AE */ volatile uint8_t  ped_sampleCount;
/* 0xF7AF */ volatile uint8_t  ped_activityTimer;
/* 0xF7B0 */ volatile uint8_t  ped_stepTimer;
/* 0xF7B1 */ volatile uint8_t  ui_activeView;
/* 0xF7B2 */ volatile uint8_t  ped_batchSize;
/* 0xF7B3 */ volatile uint8_t  ped_subStepCount;
/* 0xF7B4 */ volatile uint8_t  ped_batchAccumulator;

/* --- System status flags --- */
/* 0xF7B5 */ volatile uint8_t  sys_statusFlags;            /* bit-field via _BIT */
/* 0xF7B6 */ volatile uint8_t  sys_walkerFlags;    /* bit-field via _BIT */
/* 0xF7B7 */ volatile uint8_t  _pad_f7b7;

/* --- Command / wake / heap / RNG --- */
/* 0xF7B8 */ volatile uint16_t ir_lastCommandTime;
/* 0xF7BA */ volatile uint8_t  ir_commandPos;
/* 0xF7BB */ volatile uint8_t  sys_wakeFlag[3];     /* [0]=wake flag; rest pad */
/* 0xF7BE */ volatile uint16_t sys_heapPointer;            /* sbrk break pointer */
/* 0xF7C0 */ volatile uint32_t rng_state;

/* --- Sound engine state --- */
/* 0xF7C4 */ volatile uint8_t *sound_dataPointer;
/* 0xF7C6 */ volatile uint8_t  sound_volume;                 /* 0..3 */
/* 0xF7C7 */ volatile uint8_t  _pad_f7c7;
/* 0xF7C8 */ volatile uint16_t sound_noteDuration;
/* 0xF7CA */ volatile uint16_t sound_isSeparateNote;
/* 0xF7CC */ volatile uint8_t  sound_header;
/* 0xF7CD */ volatile uint8_t  _pad_f7cd;

/* --- View substate + accel positions --- */
/* --- View-state region (0xF7CE..D9, 12 bytes) ---
 * 3-byte universal substate triple + 9 bytes of view-multiplexed state.
 * Definition in globals.h above struct b_ram_section. */
/* 0xF7CE */ struct viewstate viewstate;
/* 0xF7DA */ volatile uint16_t ped_axisStepThresholdLo;
/* 0xF7DC */ volatile uint16_t ped_axisStepThresholdHi;
/* 0xF7DE */ volatile uint16_t ped_axisIdleThreshold;

/* --- Event loop / LCD state --- */
/* 0xF7E0 */ volatile event_loop_func_t sys_tickHandler;
/* 0xF7E2 */ volatile event_loop_func_t sys_savedTickHandler;
/* 0xF7E4 */ volatile uint8_t  lcd_pageOffset;
/* 0xF7E5 */ volatile uint8_t  _pad_f7e5;

/* --- Multi-purpose overlay #1 (g_scratch, 128 B) --- */
/* 0xF7E6 */ volatile uint8_t  scratch1[0x80];

/* --- Multi-purpose overlay #2 (g_scratch2, 240 B) --- */
/* 0xF866 */ volatile uint8_t  scratch2[0xF0];

/* --- EEPROM page scratch + sbrk heap region (1434 B) --- */
/* 0xF956 */ volatile uint8_t  eepromPageScratch[0x59A];   /* through 0xFEEF */
