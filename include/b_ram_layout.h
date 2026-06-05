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
/* 0xF780 */ volatile uint32_t totalSteps;
/* 0xF784 */ volatile uint32_t RamCache_STEP_COUNT_maybe;  /* walk step count */
/* 0xF788 */ volatile uint32_t rtcTime;
/* 0xF78C */ volatile uint16_t dayCounter;
/* 0xF78E */ volatile uint16_t watts;
/* 0xF790 */ volatile uint16_t sessionTicksElapsed;
/* 0xF792 */ volatile uint8_t  stepWattCounter;
/* 0xF793 */ volatile uint8_t  peerSlotIndex;
/* 0xF794 */ volatile uint8_t  _peer_slot_tail[3];     /* original alloc was uint32 */
/* 0xF797 */ volatile uint8_t  settingsByte;           /* aka RamCache_settingsByte */

/* --- Button input (4 B) --- */
/* 0xF798 */ volatile uint8_t  btn_inputRaw;          /* bit-field via _BIT */
/* 0xF799 */ volatile uint8_t  btn_inputPrevious;
/* 0xF79A */ volatile uint8_t  btn_trigger;
/* 0xF79B */ volatile uint8_t  btn_holdDuration;

/* --- Activity / timing --- */
/* 0xF79C */ volatile uint32_t sessionSteps;
/* 0xF7A0 */ volatile uint16_t recentSessionSteps;
/* 0xF7A2 */ volatile uint16_t idleSeconds;
/* 0xF7A4 */ volatile uint8_t  rtcSec;
/* 0xF7A5 */ volatile uint8_t  rtcMin;
/* 0xF7A6 */ volatile uint8_t  rtcHour;
/* 0xF7A7 */ volatile uint8_t  pedTaskFlags;           /* bit-field via _BIT */
/* 0xF7A8 */ volatile uint8_t  scheduledNotifyHour;
/* 0xF7A9 */ volatile uint8_t  lcdShadeBase;
/* 0xF7AA */ volatile uint8_t  menu_cursor;
/* 0xF7AB */ volatile uint8_t  DAT_f7ab;               /* write-only dispatch tick */
/* 0xF7AC */ volatile uint8_t  animTick;
/* 0xF7AD */ volatile uint8_t  irResultCode;
/* 0xF7AE */ volatile uint8_t  accelSampleCount;
/* 0xF7AF */ volatile uint8_t  activityTimer;
/* 0xF7B0 */ volatile uint8_t  stepTimer;
/* 0xF7B1 */ volatile uint8_t  currentlyActiveView;
/* 0xF7B2 */ volatile uint8_t  stepBatchSize;
/* 0xF7B3 */ volatile uint8_t  subStepCount;
/* 0xF7B4 */ volatile uint8_t  batchAccumulator;

/* --- System status flags --- */
/* 0xF7B5 */ volatile uint8_t  statusFlags;            /* bit-field via _BIT */
/* 0xF7B6 */ volatile uint8_t  walker_status_flags;    /* bit-field via _BIT */
/* 0xF7B7 */ volatile uint8_t  _pad_f7b7;

/* --- Command / wake / heap / RNG --- */
/* 0xF7B8 */ volatile uint16_t lastCommandTime;
/* 0xF7BA */ volatile uint8_t  commandPos;
/* 0xF7BB */ volatile uint8_t  wakeupFlagMaybe[3];     /* [0]=wake flag; rest pad */
/* 0xF7BE */ volatile uint16_t heapPointer;            /* sbrk break pointer */
/* 0xF7C0 */ volatile uint32_t nextRandom;

/* --- Sound engine state --- */
/* 0xF7C4 */ volatile uint8_t *soundData;
/* 0xF7C6 */ volatile uint8_t  volume;                 /* 0..3 */
/* 0xF7C7 */ volatile uint8_t  _pad_f7c7;
/* 0xF7C8 */ volatile uint16_t noteDuration;
/* 0xF7CA */ volatile uint16_t isSeparateNote;
/* 0xF7CC */ volatile uint8_t  soundHeader;
/* 0xF7CD */ volatile uint8_t  _pad_f7cd;

/* --- View substate + accel positions --- */
/* 0xF7CE */ volatile uint8_t  gCurSubstateY;
/* 0xF7CF */ volatile uint8_t  gCurSubstateZ;
/* 0xF7D0 */ volatile uint8_t  gCurSubstateA;
/* 0xF7D1 */ volatile uint8_t  DAT_f7d1;               /* MULTI-PURPOSE */
/* 0xF7D2 */ volatile uint8_t  accelXPos;              /* accelPos_X = uint16 view */
/* 0xF7D3 */ volatile uint8_t  dowsing_item_pos;
/* 0xF7D4 */ volatile uint8_t  accelYPos;              /* accelPos_Y = uint16 view */
/* 0xF7D5 */ volatile uint8_t  DAT_f7d5;               /* MULTI-PURPOSE */
/* 0xF7D6 */ volatile uint16_t accelZPos;              /* accelZPos_b = uint8 view */
/* 0xF7D8 */ volatile uint8_t  DAT_f7d8;               /* MULTI-PURPOSE */
/* 0xF7D9 */ volatile uint8_t  DAT_f7d8_1;
/* 0xF7DA */ volatile uint16_t axisStepThresholdLo;
/* 0xF7DC */ volatile uint16_t axisStepThresholdHi;
/* 0xF7DE */ volatile uint16_t axisIdleThreshold;

/* --- Event loop / LCD state --- */
/* 0xF7E0 */ volatile event_loop_func_t currentEventLoopFunc;
/* 0xF7E2 */ volatile event_loop_func_t savedEventLoopFunc;
/* 0xF7E4 */ volatile uint8_t  lcdPageOffset;
/* 0xF7E5 */ volatile uint8_t  _pad_f7e5;

/* --- Multi-purpose overlay #1 (g_scratch, 128 B) --- */
/* 0xF7E6 */ volatile uint8_t  scratch1[0x80];

/* --- Multi-purpose overlay #2 (g_scratch2, 240 B) --- */
/* 0xF866 */ volatile uint8_t  scratch2[0xF0];

/* --- EEPROM page scratch + sbrk heap region (1434 B) --- */
/* 0xF956 */ volatile uint8_t  eepromPageScratch[0x59A];   /* through 0xFEEF */
