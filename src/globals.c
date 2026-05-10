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

volatile uint32_t totalSteps;                 /* 0xF780 (.s split as 1+3) */
volatile uint32_t RamCache_STEP_COUNT_maybe;  /* 0xF784 */
volatile uint32_t DAT_f788;                   /* 0xF788 */
volatile uint16_t DAT_f78c;                   /* 0xF78C */
volatile uint16_t watts;                      /* 0xF78E */
volatile uint16_t DAT_f790;                   /* 0xF790 */
volatile uint8_t  stepWattCounter;            /* 0xF792 */
volatile uint32_t DAT_f793;                   /* 0xF793 (4 bytes) */
volatile uint8_t  RamCache_settingsByte;      /* 0xF797 */
volatile uint8_t  buttonInputRaw;             /* 0xF798 */
volatile uint8_t  prevButtonInputRaw;         /* 0xF799 */
volatile uint8_t  buttonTrigger;              /* 0xF79A */
volatile uint8_t  buttonHoldDuration;         /* 0xF79B */
volatile uint32_t sessionSteps;               /* 0xF79C */
volatile uint16_t DAT_f7a0;                   /* 0xF7A0 */
volatile uint16_t DAT_f7a2;                   /* 0xF7A2 */
volatile uint8_t  DAT_f7a4;                   /* 0xF7A4 */
volatile uint8_t  DAT_f7a5;                   /* 0xF7A5 */
volatile uint8_t  DAT_f7a6;                   /* 0xF7A6 */
volatile uint8_t  DAT_f7a7;                   /* 0xF7A7 */
volatile uint8_t  DAT_f7a8;                   /* 0xF7A8 */
volatile uint8_t  DAT_f7a9;                   /* 0xF7A9 */
volatile uint8_t  menu_cursor;                /* 0xF7AA */
volatile uint8_t  DAT_f7ab;                   /* 0xF7AB */
volatile uint8_t  DAT_f7ac;                   /* 0xF7AC */
volatile uint8_t  DAT_f7ad;                   /* 0xF7AD */
volatile uint8_t  accelSampleCount;           /* 0xF7AE */
volatile uint8_t  activityTimer;              /* 0xF7AF */
volatile uint8_t  stepTimer;                  /* 0xF7B0 */
volatile uint8_t  currentlyActiveView;        /* 0xF7B1 */
volatile uint8_t  stepBatchSize;              /* 0xF7B2 */
volatile uint8_t  DAT_f7b3;                   /* 0xF7B3 */
volatile uint8_t  DAT_f7b4;                   /* 0xF7B4 */
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
volatile uint8_t  DAT_f7d1;                   /* 0xF7D1 */
volatile uint8_t  accelXPos;                  /* 0xF7D2 (.s 1 byte; header types uint16_t -- wrong size) */
volatile uint8_t  dowsing_item_pos;           /* 0xF7D3 */
volatile uint8_t  accelYPos;                  /* 0xF7D4 (.s 1 byte; header types uint16_t) */
volatile uint8_t  DAT_f7d5;                   /* 0xF7D5 */
volatile uint16_t accelZPos;                  /* 0xF7D6 */
volatile uint8_t  DAT_f7d8;                   /* 0xF7D8 */
volatile uint8_t  DAT_f7d8_1;                 /* 0xF7D9 */
volatile uint16_t DAT_f7da;                   /* 0xF7DA */
volatile uint16_t DAT_f7dc;                   /* 0xF7DC */
volatile uint16_t DAT_f7de;                   /* 0xF7DE */
volatile uint16_t currentEventLoopFunc;       /* 0xF7E0 */
volatile uint16_t DAT_f7e2;                   /* 0xF7E2 */
volatile uint16_t DAT_f7e4;                   /* 0xF7E4 (.s 2 bytes) */

/* 0xF7E6..0xF7F5: DAT_f7e6 is the first slot of a 0x68-byte buffer that
 * spans through the following symbols. Code accesses it as both a
 * uint32_t (the first word) and as a 104-byte buffer via cast. The
 * subsequent symbols (DAT_f7ea, DAT_f7ee, DAT_f7f0, DAT_f7f2) overlap
 * within that buffer. fft_results is an int16_t[32] view of the same. */
volatile uint32_t DAT_f7e6;                   /* 0xF7E6 (4 bytes) */
volatile uint32_t DAT_f7ea;                   /* 0xF7EA */
volatile uint16_t DAT_f7ee;                   /* 0xF7EE */
volatile uint16_t DAT_f7f0;                   /* 0xF7F0 */
volatile uint32_t DAT_f7f2;                   /* 0xF7F2 (.s allocates 52 bytes -- macro views the rest) */
uint8_t           _pad_f7f6[52 - 4];          /* fills out DAT_f7f2's full extent (0xF7F6..0xF825) */

/* 0xF826..0xF865: ACCEL_SAMPLES_X is the named slot; accelXSamples is a
 * 64-byte view that overlaps the following DAT_f82e..DAT_f846 slots. */
uint8_t           ACCEL_SAMPLES_X[8];         /* 0xF826 */
volatile uint8_t  DAT_f82e[18];               /* 0xF82E */
volatile uint8_t  DAT_f840;                   /* 0xF840 */
volatile uint8_t  DAT_f841;                   /* 0xF841 */
volatile uint8_t  DAT_f842;                   /* 0xF842 */
volatile uint8_t  DAT_f843;                   /* 0xF843 */
volatile uint16_t DAT_f844;                   /* 0xF844 */
volatile uint32_t DAT_f846;                   /* 0xF846 */
uint8_t           DAT_f84e[8];                /* 0xF84E (.s 8 bytes; header types as uint8_t[0x68]) */
volatile uint16_t DAT_f856;                   /* 0xF856 (.s 16 bytes -- header types as uint16_t) */
uint8_t           _pad_f858[14];              /* fills out the 16 bytes from 0xF856 */

/* 0xF866..0xF8A5: ACCEL_SAMPLES_Y is the named slot; accelYSamples is a
 * 64-byte view spanning ACCEL_SAMPLES_Y + L_F886/DAT_f886 + DAT_f88e + DAT_f896 + DAT_f897. */
uint8_t           ACCEL_SAMPLES_Y[32];        /* 0xF866 */
uint8_t           DAT_f886[8];                /* 0xF886 (also aliased as L_F886) */
volatile uint8_t  DAT_f88e[8];                /* 0xF88E */
volatile uint8_t  DAT_f896;                   /* 0xF896 */
volatile uint8_t  DAT_f897[15];               /* 0xF897..F8A5 */

/* 0xF8A6..0xF8E5: ACCEL_SAMPLES_Z is the named slot; accelZSamples is
 * a 64-byte view spanning the rest. */
uint8_t           ACCEL_SAMPLES_Z[3];         /* 0xF8A6 */
volatile uint8_t  DAT_f8a9[13];               /* 0xF8A9..F8B5 */
volatile uint32_t DAT_f8b6;                   /* 0xF8B6 */
volatile uint32_t DAT_f8ba;                   /* 0xF8BA */
volatile uint8_t  DAT_f8be;                   /* 0xF8BE */
volatile uint16_t DAT_f8bf;                   /* 0xF8BF (.s 2 bytes; header types as uint8_t) */
volatile uint8_t  DAT_f8c1;                   /* 0xF8C1 */
volatile uint8_t  DAT_f8c2;                   /* 0xF8C2 */
volatile uint8_t  DAT_f8c3;                   /* 0xF8C3 (.s 1 byte) */
volatile uint8_t  REQUESTED_POKEMON_ACTION_TYPE; /* 0xF8C4 */
volatile uint8_t  DAT_f8c5;                   /* 0xF8C5 */
volatile uint16_t DAT_f8c6;                   /* 0xF8C6 */
volatile uint16_t DAT_f8c8;                   /* 0xF8C8 */
volatile uint16_t DAT_f8ca;                   /* 0xF8CA */
volatile uint8_t  DAT_f8cc;                   /* 0xF8CC */
volatile uint8_t  rdr_data;                   /* 0xF8CD */
volatile uint8_t  commandType;                /* 0xF8CE */
volatile uint8_t  DAT_f8cf;                   /* 0xF8CF */
volatile uint8_t  DAT_f8d0;                   /* 0xF8D0 */
volatile uint8_t  DAT_f8d1;                   /* 0xF8D1 */
volatile uint32_t DAT_f8d2;                   /* 0xF8D2 */
volatile uint8_t  TX_PACKET_payload;          /* 0xF8D6 */
volatile uint8_t  DAT_f8d7;                   /* 0xF8D7 */
volatile uint8_t  DAT_f8d8[14];               /* 0xF8D8..F8E5 */
volatile uint32_t DAT_f8e6;                   /* 0xF8E6 */
volatile uint32_t DAT_f8ea;                   /* 0xF8EA */
volatile uint8_t  DAT_f8ee;                   /* 0xF8EE */
volatile uint8_t  isNotWalking;               /* 0xF8EF (.s allocates 103 bytes; rest is padding) */
uint8_t           _pad_f8f0[102];             /* fills out isNotWalking's 103-byte run */
volatile uint8_t  DAT_f956;                   /* 0xF956 (.s 1 byte) */
uint8_t           _pad_f957[12];              /* DAT_f957: 12 bytes of slack (unnamed in C usage) */
volatile uint8_t  DAT_f963;                   /* 0xF963 */
uint8_t           DAT_f964[1420];             /* 0xF964..0xFEEF */
uint8_t           DAT_fef0[136];              /* 0xFEF0..0xFF77 */
uint8_t           initialStackPosition[8];    /* 0xFF78..0xFF7F */
