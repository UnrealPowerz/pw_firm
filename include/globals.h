#ifndef GLOBALS_H
#define GLOBALS_H

#include "types.h"

/* --- System Status Flags --- */
extern volatile uint8_t statusFlags;
#define statusFlags_BIT (((volatile status_flags_t *)&statusFlags)->BIT)
extern volatile uint8_t wakeupFlagMaybe;
extern volatile uint8_t status_flags_f7f1;   /* DAT_f7f1 */
extern volatile uint8_t walker_status_flags; /* DAT_f7ef (?) */
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
extern volatile uint8_t DAT_f793;  /* DAT_f793 */
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
extern volatile uint16_t
    accelXPos; /* 0xF7D2 (Combined with dowsing_item_pos) */
extern volatile uint8_t dowsing_item_pos; /* 0xF7D3 (?) */
extern volatile uint16_t accelYPos;       /* 0xF7D4 (Combined with DAT_f7d5) */
extern volatile uint8_t DAT_f7d5;         /* 0xF7D5 */
extern volatile uint16_t accelZPos;       /* 0xF7D6 */
extern volatile uint8_t DAT_f7d8;         /* 0xF7D8 */
extern volatile uint8_t DAT_f7d8_1;       /* 0xF7D9 */
extern volatile uint16_t DAT_f7da;        /* 0xF7DA */
extern volatile uint16_t DAT_f7dc;        /* 0xF7DC */
extern volatile uint16_t DAT_f7de;        /* 0xF7DE */

extern uint8_t ACCEL_SAMPLES_X[32];
extern uint8_t ACCEL_SAMPLES_Y[32];
extern uint8_t ACCEL_SAMPLES_Z[32];
extern volatile int16_t fft_results[32];  // at 0xF7E6
extern volatile int8_t accelXSamples[64]; // at 0xF826
extern volatile int8_t accelYSamples[64]; // at 0xF866
extern volatile int8_t accelZSamples[64]; // at 0xF8A6

/* --- Control Flow --- */
extern volatile uint16_t currentEventLoopFunc;
extern volatile uint16_t DAT_f7e2;
extern volatile uint16_t gSleepModeEventLoopFunc;

/* --- RNG & Memory --- */
extern volatile uint32_t nextRandom;
extern volatile uint16_t heapPointer;

/* --- LCD & EEPROM --- */
extern volatile uint8_t DAT_f7e4; /* lcd_page_offset (?) */
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
extern volatile uint16_t DAT_f8c3;
extern volatile uint8_t DAT_f8be;
extern volatile uint8_t DAT_f8bf;
extern volatile uint8_t DAT_f8c2;
extern volatile uint32_t DAT_f8b6;
extern volatile uint32_t DAT_f8ba;
extern volatile uint8_t DAT_f8c1;
extern volatile uint8_t DAT_f840;
extern volatile uint8_t DAT_f841;
extern volatile uint8_t DAT_f842;
extern volatile uint8_t DAT_f844;
extern volatile uint8_t DAT_f088;
extern volatile uint8_t TX_PACKET_payload;
extern uint8_t DAT_f7e6[0x68];
extern uint8_t
    DAT_f84e[0x68]; /* Used as sound buffer overlaps with ACCEL_SAMPLES_X */
extern volatile uint32_t DAT_f846;
extern volatile uint32_t DAT_f7ea;
extern volatile uint16_t DAT_f7ee;
extern volatile uint16_t DAT_f7f0;
extern volatile uint32_t DAT_f7f2;
extern volatile uint8_t DAT_f82e[0x12];
extern volatile uint8_t DAT_f843;
extern volatile uint16_t DAT_f856;
extern volatile uint8_t DAT_f896;
extern volatile uint16_t DAT_f956;
extern uint8_t DAT_f580[];
extern volatile uint16_t DAT_f886;
extern volatile uint8_t DAT_f88e;
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
extern uint8_t soundHeader;
extern uint8_t volume;
extern uint16_t noteDuration;
extern uint16_t isSeparateNote;

#endif /* GLOBALS_H */
