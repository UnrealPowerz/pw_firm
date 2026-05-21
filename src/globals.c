#include "all_headers.h"

/* RAM globals (section B, 0xF780..0xFF7F).
 *
 * Allocations are listed in the SAME ORDER as the original src/globals.s
 * so ch38 lays them out at the same byte offsets. Each declaration's
 * primary type is chosen to consume exactly as many bytes as the .s
 * `.RES.B` reservation -- not necessarily what callers semantically want.
 *
 * Aliased / overlapping symbols (where one C name views a region that
 * spans multiple physical allocations) are exposed via macros in
 * include/globals.h, not as second C definitions. */

/* 0xF780..0xF797: 24-byte EEPROM-backed save block. Struct definition
 * (and per-field macros) live in include/globals.h; this is the single
 * allocation. */
struct session_save g_save;
volatile uint8_t  buttonInputRaw;             /* 0xF798 */
volatile uint8_t  prevButtonInputRaw;         /* 0xF799 */
volatile uint8_t  buttonTrigger;              /* 0xF79A */
volatile uint8_t  buttonHoldDuration;         /* 0xF79B */
volatile uint32_t sessionSteps;               /* 0xF79C */
volatile uint16_t recentSessionSteps;                   /* 0xF7A0 */
volatile uint16_t idleSeconds;                   /* 0xF7A2 */
volatile uint8_t  rtcSec;                   /* 0xF7A4 */
volatile uint8_t  rtcMin;                   /* 0xF7A5 */
volatile uint8_t  rtcHour;                   /* 0xF7A6 */
volatile uint8_t  pedTaskFlags;                   /* 0xF7A7 */
volatile uint8_t  scheduledNotifyHour;                   /* 0xF7A8 */
volatile uint8_t  lcdShadeBase;                   /* 0xF7A9 */
volatile uint8_t  menu_cursor;                /* 0xF7AA */
volatile uint8_t  DAT_f7ab;                   /* 0xF7AB -- only ever incremented (in
                                                  ui_dispatch.c default-case branch and
                                                  switchD_7364__caseD_5 in ROM); never
                                                  read by any code we've decompiled OR
                                                  by main.mar. Vestigial counter. */
volatile uint8_t  animTick;                   /* 0xF7AC */
volatile uint8_t  irResultCode;                   /* 0xF7AD */
volatile uint8_t  accelSampleCount;           /* 0xF7AE */
volatile uint8_t  activityTimer;              /* 0xF7AF */
volatile uint8_t  stepTimer;                  /* 0xF7B0 */
volatile uint8_t  currentlyActiveView;        /* 0xF7B1 */
volatile uint8_t  stepBatchSize;              /* 0xF7B2 */
volatile uint8_t  subStepCount;                   /* 0xF7B3 */
volatile uint8_t  batchAccumulator;                   /* 0xF7B4 */
volatile uint8_t  statusFlags;                /* 0xF7B5 */
volatile uint16_t walker_status_flags;        /* 0xF7B6 (.s allocates 2 bytes) */
volatile uint16_t lastCommandTime;            /* 0xF7B8 */
volatile uint8_t  commandPos;                 /* 0xF7BA */
uint8_t           wakeupFlagMaybe[3];         /* 0xF7BB..F7BD (volatile via macro) */
volatile uint16_t heapPointer;                /* 0xF7BE */
volatile uint32_t nextRandom;                 /* 0xF7C0 */
uint8_t          *soundData;                  /* 0xF7C4 (2 bytes -- pointer in -cpu=300HN) */
uint16_t          volume;                     /* 0xF7C6 (.s 2 bytes; header types as uint8_t) */
uint16_t          noteDuration;               /* 0xF7C8 */
uint16_t          isSeparateNote;             /* 0xF7CA */
uint16_t          soundHeader;                /* 0xF7CC (.s 2 bytes; header types as uint8_t) */
volatile uint8_t  gCurSubstateY;              /* 0xF7CE */
volatile uint8_t  gCurSubstateZ;              /* 0xF7CF */
volatile uint8_t  gCurSubstateA;              /* 0xF7D0 */
volatile uint8_t  DAT_f7d1;                   /* 0xF7D1 -- shared multi-purpose
                                                  substate byte. Roles per subsystem:
                                                  - sys_enter/update_standby_state:
                                                    bitfield (bits 0/1/2 -- see _BIT macro)
                                                  - menu_main: bit-checks for standby
                                                  - battle: turn counter (init 4, --)
                                                  - dowsing: position cycle (mod 6)
                                                  - radar: state counter (++/--)
                                                  - social: music note ID (0x2C..0x30)
                                                  - pedometer: vs DAT_f7d8_1 (counter cmp)
                                                  - debug: drawn as ASCII digit
                                                  No single name captures all uses; left
                                                  as DAT_f7d1. _BIT macro in globals.h
                                                  for the standby bit operations. */
volatile uint8_t  accelXPos;                  /* 0xF7D2 (.s 1 byte; header types uint16_t -- wrong size) */
volatile uint8_t  dowsing_item_pos;           /* 0xF7D3 */
volatile uint8_t  accelYPos;                  /* 0xF7D4 (.s 1 byte; header types uint16_t) */
volatile uint8_t  DAT_f7d5;                   /* 0xF7D5 -- shared multi-purpose
                                                  scratch byte. Roles:
                                                  - battle: sprite X-coord (set from
                                                    battleAnimP{1,3,4}XFrames tables)
                                                  - radar: Y-coord-row (drawn as *8)
                                                  - dowsing: value-with-icon (count)
                                                  No single name fits; left as DAT_f7d5. */
volatile uint16_t accelZPos;                  /* 0xF7D6 */
volatile uint8_t  DAT_f7d8;                   /* 0xF7D8 -- start of 8-byte EEPROM
                                                  factory calibration block (loaded
                                                  via drv_eeprom_read_block(8, &DAT_f7d8, 8)
                                                  in debug.c). Multi-purpose:
                                                  - factory test: byte 0 = step limit
                                                  - battle: bit-field for move state
                                                    (bit 0 via _BIT; bits 1-2, 3-4,
                                                    5-7 are multi-bit sub-fields)
                                                  - dowsing: paired with DAT_f7d8_1 as
                                                    uint16 item ID
                                                  No single name fits. */
volatile uint8_t  DAT_f7d8_1;                 /* 0xF7D9 -- 2nd byte of the calib
                                                  block above; also used as battle
                                                  sub-counter (++/reset) and as
                                                  high byte of dowsing item ID. */
volatile uint16_t axisStepThresholdLo;                   /* 0xF7DA */
volatile uint16_t axisStepThresholdHi;                   /* 0xF7DC */
volatile uint16_t axisIdleThreshold;                   /* 0xF7DE */
event_loop_func_t currentEventLoopFunc;       /* 0xF7E0 */
event_loop_func_t savedEventLoopFunc;         /* 0xF7E2 -- snapshot before swap */
volatile uint16_t lcdPageOffset;                   /* 0xF7E4 (.s 2 bytes) */

/* 0xF7E6..0xF865: 128-byte multi-purpose scratch region.
 * Union definition + per-view macros are in include/globals.h. Replaces
 * what used to be 12 separate globals (DAT_f7e6/ea/ee/f0/f2,
 * ACCEL_SAMPLES_X, DAT_f82e, DAT_f840..846, trainerRecBuf, trainerRecBuf_loc) plus
 * their padding. All those names are now macros into g_scratch. */
union pw_scratch g_scratch;

/* 0xF866..0xF955: 240-byte multi-purpose scratch region (g_scratch2 union
 * defined in include/globals.h). All globals previously declared at
 * 0xF866..0xF8EF are now macros into this union; this is the single
 * allocation. */
union pw_scratch2 g_scratch2;
/* 0xF956 onward: multi-purpose EEPROM-page scratch buffer.
 *   - game_find_seen_peer:    40-byte compare buffer (vs trainer record).
 *   - ir_protocol case 0x00:  128-byte EEPROM page (LZSS decode + write).
 * The named extent here (14 bytes) consolidates three Ghidra splits at
 * 0xF956/0xF957/0xF963 — actual reads/writes overflow into the head of
 * DAT_f964 below (up to 128 bytes total). Harmless because the usage
 * windows are short and DAT_f964 isn't accessed concurrently. */
uint8_t           eepromPageScratch[14];      /* 0xF956..0xF963 (see comment above) */
uint8_t           DAT_f964[1420];             /* 0xF964..0xFEEF -- 1420 bytes of
                                                  unnamed buffer. ZERO references in
                                                  both our C source AND main.mar. Most
                                                  likely candidates: original heap area
                                                  (sbrk arena -- our static replacement
                                                  is smaller); or a buffer accessed via
                                                  a base pointer set by undecompiled
                                                  code. Leave as-is until we discover
                                                  the access path. */
uint8_t           DAT_fef0[136];              /* 0xFEF0..0xFF77 -- destination of
                                                  _INITSCT data copy at boot
                                                  (rom_data_start->ram_data_start);
                                                  SDK-managed, do NOT rename. */
uint8_t           initialStackPosition[8];    /* 0xFF78..0xFF7F */
