#ifndef GLOBALS_H
#define GLOBALS_H

#include "types.h"

/* --- System Status Flags --- */
extern volatile uint8_t statusFlags;
#define statusFlags_BIT (((volatile status_flags_t *)&statusFlags)->BIT)
extern uint8_t wakeupFlagMaybe[3];
extern volatile uint8_t status_flags_f7f1;   /* DAT_f7f1 */
extern volatile uint16_t walker_status_flags; /* 0xF7B6 (.s 2 bytes) */
#define walker_status_flags_BIT (((volatile walker_status_t *)&walker_status_flags)->BIT)

/* --- LCD & UI / Timers --- */
extern volatile uint8_t DAT_f7ab;            /* 0xF7AB */
extern volatile uint8_t DAT_f7ac;            /* 0xF7AC */
extern volatile uint8_t DAT_f7ad;            /* 0xF7AD IR result (?) */
extern volatile uint8_t accelSampleCount;    /* 0xF7AE */
extern volatile uint8_t activityTimer;       /* 0xF7AF */
extern volatile uint8_t stepTimer;           /* 0xF7B0 */
extern volatile uint8_t currentlyActiveView; /* 0xF7B1 */
extern volatile uint8_t stepBatchSize;       /* 0xF7B2 */
extern volatile uint8_t DAT_f7b3;            /* 0xF7B3 sub_step_count (?) */
extern volatile uint8_t DAT_f7b4;            /* 0xF7B4 batch_accumulator (?) */

/* --- Button Input --- */
extern volatile uint8_t buttonInputRaw;
#define buttonInputRaw_BIT (((volatile button_input_t *)&buttonInputRaw)->BIT)
extern volatile uint8_t prevButtonInputRaw;
extern volatile uint8_t buttonTrigger;
extern volatile uint8_t buttonHoldDuration;

/* --- Pedometer & Activity --- */
extern volatile uint32_t totalSteps;                /* DAT_f780 */
extern volatile uint32_t RamCache_STEP_COUNT_maybe; /* DAT_f784 */
extern volatile uint32_t DAT_f788;
extern volatile uint16_t DAT_f78c;
extern volatile uint16_t watts; /* DAT_f78e */
extern volatile uint16_t DAT_f790;
extern volatile uint32_t sessionSteps;
extern volatile uint16_t watts_redundant; /* DAT_f7c8 (?) */
extern volatile uint8_t stepWattCounter;
extern volatile uint32_t DAT_f793;  /* 0xF793 (.s 4 bytes) */
extern volatile uint16_t DAT_f7a0; /* daily_step_cap (?) */
extern volatile uint16_t DAT_f7a2;
extern volatile uint8_t DAT_f7a4;
extern volatile uint8_t DAT_f7a5;
extern volatile uint8_t DAT_f7a6;
extern volatile uint8_t DAT_f7a7;
#define DAT_f7a7_BIT (((volatile ped_task_flags_t *)&DAT_f7a7)->BIT)
extern volatile uint8_t DAT_f7a8;
extern volatile uint8_t DAT_f7a9;
extern volatile uint8_t menu_cursor;

/* --- Substate Management & Sensor Data --- */
extern volatile uint8_t gCurSubstateY; /* 0xF7CD */
extern volatile uint8_t gCurSubstateZ; /* 0xF7CE */
extern volatile uint8_t gCurSubstateA; /* 0xF7CF (?) */
extern volatile uint8_t DAT_f7d1;      /* 0xF7D1 */
extern volatile uint8_t accelXPos; /* 0xF7D2 */
extern volatile uint8_t dowsing_item_pos; /* 0xF7D3 (?) */
extern volatile uint8_t accelYPos;       /* 0xF7D4 */
extern volatile uint8_t DAT_f7d5;         /* 0xF7D5 */
extern volatile uint16_t accelZPos;       /* 0xF7D6 */
extern volatile uint8_t DAT_f7d8;         /* 0xF7D8 */
extern volatile uint8_t DAT_f7d8_1;       /* 0xF7D9 */
extern volatile uint16_t DAT_f7da;        /* 0xF7DA */
extern volatile uint16_t DAT_f7dc;        /* 0xF7DC */
extern volatile uint16_t DAT_f7de;        /* 0xF7DE */

extern uint8_t ACCEL_SAMPLES_X[8];   /* 0xF826 (only 8 bytes named; accelXSamples is a 64-byte view) */
extern uint8_t ACCEL_SAMPLES_Y[32];  /* 0xF866 */
extern uint8_t ACCEL_SAMPLES_Z[3];   /* 0xF8A6 (only 3 bytes named; accelZSamples is a 64-byte view) */

/* Aliased multi-symbol views. fft_results is an int16_t window onto the same
 * region whose first uint32_t slot is DAT_f7e6. accelXSamples/Y/Z are
 * int8_t windows that span the next ~64 bytes from each ACCEL_SAMPLES_*. */
#define fft_results    ((volatile int16_t *)&DAT_f7e6)
#define accelXSamples  ((volatile int8_t *)ACCEL_SAMPLES_X)
#define accelYSamples  ((volatile int8_t *)ACCEL_SAMPLES_Y)
#define accelZSamples  ((volatile int8_t *)ACCEL_SAMPLES_Z)
#define L_F886         DAT_f886

/* --- Control Flow --- */
extern volatile uint16_t currentEventLoopFunc;
extern volatile uint16_t DAT_f7e2;
extern volatile uint16_t gSleepModeEventLoopFunc;

/* --- RNG & Memory --- */
extern volatile uint32_t nextRandom;
extern volatile uint16_t heapPointer;

/* --- LCD & EEPROM --- */
extern volatile uint16_t DAT_f7e4; /* lcd_page_offset (?) -- 0xF7E4 (.s 2 bytes) */
extern volatile uint8_t RamCache_settingsByte;
#define RamCache_settingsByte_BIT (((volatile settings_byte_t *)&RamCache_settingsByte)->BIT)

/* --- Step Processing Scratch --- */
extern volatile uint32_t DAT_f8e6;
extern volatile uint32_t DAT_f8ea;
extern volatile uint8_t DAT_f8ee;
extern volatile uint8_t isNotWalking;

/* --- IR Comm Variables --- */
extern volatile uint8_t commandPos;
extern volatile uint8_t commandType;
extern volatile uint8_t rdr_data;
extern volatile uint16_t lastCommandTime;
extern volatile uint8_t REQUESTED_POKEMON_ACTION_TYPE;
extern volatile uint16_t DAT_f8c6;
extern volatile uint16_t DAT_f8c8;
extern volatile uint16_t DAT_f8ca;
extern volatile uint8_t DAT_f8cc;
extern volatile uint8_t DAT_f8c5;
extern volatile uint8_t DAT_f8c3;
extern volatile uint8_t DAT_f8be;
extern volatile uint16_t DAT_f8bf;  /* 0xF8BF (.s 2 bytes) */
extern volatile uint8_t DAT_f8c2;
extern volatile uint32_t DAT_f8b6;
extern volatile uint32_t DAT_f8ba;
extern volatile uint8_t DAT_f8c1;
extern volatile uint8_t DAT_f840;
extern volatile uint8_t DAT_f841;
extern volatile uint8_t DAT_f842;
extern volatile uint16_t DAT_f844;  /* 0xF844 (.s 2 bytes) */
extern volatile uint8_t TX_PACKET_payload;
extern volatile uint32_t DAT_f7e6;             /* 0xF7E6 -- first slot of a 0x68-byte buffer */
extern uint8_t DAT_f84e[8];                    /* 0xF84E (8 named bytes; surrounding aliases see C globals) */
/* Absolute hardware-address aliases (no allocation -- compiler generates
 * direct pointer literals). */
#define DAT_f088 (*(volatile uint8_t *)0xF088)
#define DAT_f580 ((uint8_t *)0xF580)
extern volatile uint32_t DAT_f846;
extern volatile uint32_t DAT_f7ea;
extern volatile uint16_t DAT_f7ee;
extern volatile uint16_t DAT_f7f0;
extern volatile uint32_t DAT_f7f2;
extern volatile uint8_t DAT_f82e[0x12];
extern volatile uint8_t DAT_f843;
extern volatile uint16_t DAT_f856;
extern volatile uint8_t DAT_f896;
extern volatile uint8_t DAT_f956;
extern uint8_t DAT_f886[8];                    /* 0xF886 */
extern volatile uint8_t DAT_f88e[8];           /* 0xF88E (.s 8 bytes) */
extern volatile uint8_t ir_status;
extern const uint8_t L_BE70[];
extern const uint8_t L_BE71[];
extern const uint8_t DAT_be72[];
extern const uint8_t L_BF14[]; /* Main menu Y-coordinate table */
extern const uint8_t IMG_POKEWALKER_IR_ACTIVE[];
extern const uint8_t PERIODTAB[];

extern const uint8_t IMG_POKEWALKER_LARGE[];
extern const uint8_t DAT_bc74[];
extern const uint8_t DAT_bc94[];
extern const uint8_t DAT_bcb4[];
extern const uint8_t DAT_bcd4[];
extern const uint8_t DAT_bd40[];

extern const uint8_t L_BF25[];
extern const uint8_t L_BF21[];
extern const uint8_t fftBinTable[];
extern const uint8_t L_BF1E[];
extern const uint8_t L_BF1A[];
extern const uint8_t DAT_bdd0[];
extern const uint8_t DAT_bb36[];
extern const uint8_t L_BB45[];
extern const uint8_t DAT_bb0e[];
extern const uint8_t DAT_bb12[];
extern const uint8_t DAT_bb13[];
extern const uint8_t DAT_bb24[];
extern const uint8_t DAT_bb25[];
extern const uint8_t L_BCF4[];

/* --- Sound Engine Globals (Fixed addresses) --- */
extern uint8_t *soundData;
extern uint16_t soundHeader;          /* 0xF7CC (.s 2 bytes) */
extern uint16_t volume;               /* 0xF7C6 (.s 2 bytes) */
extern uint16_t noteDuration;
extern uint16_t isSeparateNote;

#endif /* GLOBALS_H */
