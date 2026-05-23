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
extern volatile uint8_t animTick;            /* 0xF7AC */
extern volatile uint8_t irResultCode;            /* 0xF7AD IR result (?) */
extern volatile uint8_t accelSampleCount;    /* 0xF7AE */
extern volatile uint8_t activityTimer;       /* 0xF7AF */
extern volatile uint8_t stepTimer;           /* 0xF7B0 */
extern volatile uint8_t currentlyActiveView; /* 0xF7B1 — see enum view_id */

/* View IDs dispatched by ui_dispatch_event / ui_dispatch_draw.
 * Walk-anim views are named from the walker's perspective: ARRIVAL when a
 * pokemon comes from the DS to begin a walk, DEPARTURE when it returns to
 * the DS at walk end (matches the ui_draw_{arrival,departure}_* helpers). */
enum view_id {
    VIEW_HOME                  = 0x00,
    VIEW_MAIN_MENU             = 0x01,
    VIEW_DOWSING               = 0x02,
    VIEW_POKERADAR             = 0x03,
    VIEW_BATTLE                = 0x04,
    /* 0x05 unused */
    VIEW_RADAR_FAILURE         = 0x06,
    VIEW_CAUGHT_STATS          = 0x07,
    VIEW_TRAINER_CARD          = 0x08,
    VIEW_SETTINGS              = 0x09,
    VIEW_POKE_ITEMS            = 0x0A,
    VIEW_GIFTS                 = 0x0B,
    VIEW_BORED_GIFT            = 0x0C,
    VIEW_PEER_PLAY             = 0x0D,
    VIEW_STEP_HISTORY          = 0x0E,
    VIEW_WALK_ARRIVAL_ANIM     = 0x0F,
    VIEW_WALK_DEPARTURE_ANIM   = 0x10,
    VIEW_EVENT_REWARD_ANIM     = 0x11,
    VIEW_DEBUG                 = 0x16,
    VIEW_ACCEL_DEBUG           = 0x17,
    VIEW_TEXT                  = 0x18
};
extern volatile uint8_t stepBatchSize;       /* 0xF7B2 */
extern volatile uint8_t subStepCount;            /* 0xF7B3 sub_step_count (?) */
extern volatile uint8_t batchAccumulator;            /* 0xF7B4 batch_accumulator (?) */

/* --- Button Input --- */
extern volatile uint8_t buttonInputRaw;
#define buttonInputRaw_BIT (((volatile button_input_t *)&buttonInputRaw)->BIT)
extern volatile uint8_t prevButtonInputRaw;
extern volatile uint8_t buttonTrigger;
extern volatile uint8_t buttonHoldDuration;

/* --- Pedometer & Activity --- */
/* Persisted save block at 0xF780..F797 (see struct session_save in globals.c).
 * Each member is exposed as a macro so existing call sites referring to the
 * field by its top-level name continue to compile and so &name remains a
 * valid pointer to the underlying byte. */
struct session_save {
    volatile uint32_t totalSteps;
    volatile uint32_t RamCache_STEP_COUNT_maybe;
    volatile uint32_t rtcTime;
    volatile uint16_t dayCounter;
    volatile uint16_t watts;
    volatile uint16_t sessionTicksElapsed;   /* ++ per tick in game_pedometer_init_counters,
                                                reset at session start; saturates at 0xFFFF */
    volatile uint8_t  stepWattCounter;
    volatile uint8_t  peerSlotIndex;       /* used only as 1-byte 0..22 ring index */
    volatile uint8_t  _peer_slot_tail[3];  /* original alloc was uint32; high bytes always 0 */
    volatile uint8_t  settingsByte;
};
extern struct session_save g_save;
#define totalSteps                  (g_save.totalSteps)
#define RamCache_STEP_COUNT_maybe   (g_save.RamCache_STEP_COUNT_maybe)
#define rtcTime                     (g_save.rtcTime)
#define dayCounter                  (g_save.dayCounter)
#define watts                       (g_save.watts)
#define sessionTicksElapsed         (g_save.sessionTicksElapsed)
#define stepWattCounter             (g_save.stepWattCounter)
#define peerSlotIndex               (g_save.peerSlotIndex)

extern volatile uint32_t sessionSteps;
extern volatile uint16_t recentSessionSteps; /* daily_step_cap (?) */
extern volatile uint16_t idleSeconds;
extern volatile uint8_t rtcSec;
extern volatile uint8_t rtcMin;
extern volatile uint8_t rtcHour;
extern volatile uint8_t pedTaskFlags;
#define pedTaskFlags_BIT (((volatile ped_task_flags_t *)&pedTaskFlags)->BIT)
extern volatile uint8_t scheduledNotifyHour;
extern volatile uint8_t lcdShadeBase;
extern volatile uint8_t menu_cursor;

/* --- Substate Management & Sensor Data --- */
extern volatile uint8_t gCurSubstateY; /* 0xF7CD */
extern volatile uint8_t gCurSubstateZ; /* 0xF7CE */
extern volatile uint8_t gCurSubstateA; /* 0xF7CF (?) */
/* gCurSubstateY -- bits 0/1 used as flags via bset/bclr/bst in ROM. */
#define gCurSubstateY_BIT (((volatile byte_bits_t *)&gCurSubstateY)->BIT)

/* DAT_f7d8 -- bit 0 used as a flag in battle.c via bset/bclr in ROM. */
#define DAT_f7d8_BIT (((volatile byte_bits_t *)&DAT_f7d8)->BIT)

/* irPacketReceivedFlag -- bit 0 used; ROM emits bset/bclr. */
#define irPacketReceivedFlag_BIT (((volatile byte_bits_t *)&irPacketReceivedFlag)->BIT)

extern volatile uint8_t DAT_f7d1;      /* 0xF7D1 -- multi-purpose: standby
   state bits 0/1/2 (sys_enter_standby/sys_update_standby_state) AND a
   small counter / position byte in battle/dowsing/radar/social/pedometer.
   ROM uses `bset/bclr/bnot @rN` for the bit operations -- access via
   DAT_f7d1_BIT.bN to match. */
#define DAT_f7d1_BIT (((volatile byte_bits_t *)&DAT_f7d1)->BIT)
extern volatile uint8_t accelXPos; /* 0xF7D2 */
extern volatile uint8_t dowsing_item_pos; /* 0xF7D3 (?) */
extern volatile uint8_t accelYPos;       /* 0xF7D4 */
extern volatile uint8_t DAT_f7d5;         /* 0xF7D5 */
extern volatile uint16_t accelZPos;       /* 0xF7D6 */
extern volatile uint8_t DAT_f7d8;         /* 0xF7D8 */
extern volatile uint8_t DAT_f7d8_1;       /* 0xF7D9 */
extern volatile uint16_t axisStepThresholdLo;        /* 0xF7DA */
extern volatile uint16_t axisStepThresholdHi;        /* 0xF7DC */
extern volatile uint16_t axisIdleThreshold;        /* 0xF7DE */

/* 0xF866..0xF955: 240-byte multi-purpose scratch region (sibling of g_scratch).
 * Memory is reused across mutually-exclusive subsystems:
 *
 *   - Accel-Y samples:   int8_t[64] from offset 0   (covers ACCEL_SAMPLES_Y + at_f886/88e/896/897)
 *   - Accel-Z samples:   int8_t[64] from offset 0x40 (overlaps IR session-key state)
 *   - IR session state:  nextSessionKey/sessionKey/handshake/retry counters/xfer (offsets 0x50..0x67)
 *   - IR packet buffer:  136 bytes from offset 0x68  (cmd/subtype/crc/session/payload)
 *   - Step detection state when IR idle: stepDetectAccum, pendingStepDetect, isNotWalking
 *     overlap with the IR packet payload tail.
 *
 * Field names use offset-based `at_NN` for slots whose semantic role across
 * views isn't single. Per-view names are exposed as macros below. */
union pw_scratch2 {
    uint8_t bytes[0xF0];
    struct {
        uint8_t  accel_y[32];        /* +0x00 ACCEL_SAMPLES_Y / accelYSamples[0..31] */
        uint8_t  at_f886[8];         /* +0x20 = accelYSamples[32..39] (also L_F886) */
        uint8_t  at_f88e[8];         /* +0x28 = accelYSamples[40..47] */
        uint8_t  at_f896;            /* +0x30 = accelYSamples[48] */
        uint8_t  at_f897[15];        /* +0x31 = accelYSamples[49..63] */
        uint8_t  accel_z[3];         /* +0x40 ACCEL_SAMPLES_Z / accelZSamples[0..2] */
        uint8_t  at_f8a9[13];        /* +0x43 = accelZSamples[3..15] */
        volatile uint32_t next_session_key;     /* +0x50 */
        volatile uint32_t session_key;          /* +0x54 */
        volatile uint8_t  ir_handshake_step;    /* +0x58 */
        volatile uint8_t  ir_timeout_retry[2];  /* +0x59 (uint16 at odd offset; access via cast) */
        volatile uint8_t  at_f8c1;              /* +0x5B */
        volatile uint8_t  ir_crc_retry_count;   /* +0x5C */
        volatile uint8_t  ir_packet_received_flag; /* +0x5D */
        volatile uint8_t  requested_pkmn_action;/* +0x5E REQUESTED_POKEMON_ACTION_TYPE */
        volatile uint8_t  ir_session_phase;     /* +0x5F */
        volatile uint16_t ir_xfer_remaining;    /* +0x60 */
        volatile uint16_t ir_xfer_src;          /* +0x62 */
        volatile uint16_t ir_xfer_dst;          /* +0x64 */
        volatile uint8_t  ir_xfer_chunk_count;  /* +0x66 */
        volatile uint8_t  rdr_data_byte;        /* +0x67 */
        volatile uint8_t  cmd;                  /* +0x68 commandType */
        volatile uint8_t  cmd_subtype;          /* +0x69 commandSubtype */
        volatile uint8_t  cmd_crc_lo;           /* +0x6A commandCrcLo */
        volatile uint8_t  cmd_crc_hi;           /* +0x6B commandCrcHi */
        volatile uint32_t cmd_session_token;    /* +0x6C commandSessionToken */
        volatile uint8_t  payload[0x80];        /* +0x70 IR payload (also overlaps step state when idle) */
    } as_struct;
};
extern union pw_scratch2 g_scratch2;

/* Backward-compatible names */
#define ACCEL_SAMPLES_Y                (g_scratch2.as_struct.accel_y)
#define accelYSamples                  ((volatile int8_t *)g_scratch2.as_struct.accel_y)
#define DAT_f886                       (g_scratch2.as_struct.at_f886)
#define DAT_f88e                       (g_scratch2.as_struct.at_f88e)
#define DAT_f896                       (g_scratch2.as_struct.at_f896)
#define DAT_f897                       (g_scratch2.as_struct.at_f897)
#define ACCEL_SAMPLES_Z                (g_scratch2.as_struct.accel_z)
#define accelZSamples                  ((volatile int8_t *)g_scratch2.as_struct.accel_z)
#define DAT_f8a9                       (g_scratch2.as_struct.at_f8a9)
#define nextSessionKey                 (g_scratch2.as_struct.next_session_key)
#define sessionKey                     (g_scratch2.as_struct.session_key)
#define irHandshakeStep                (g_scratch2.as_struct.ir_handshake_step)
#define irTimeoutRetryCount            (*(volatile uint16_t *)g_scratch2.as_struct.ir_timeout_retry)
#define DAT_f8c1                       (g_scratch2.as_struct.at_f8c1)
#define irCrcRetryCount                (g_scratch2.as_struct.ir_crc_retry_count)
#define irPacketReceivedFlag           (g_scratch2.as_struct.ir_packet_received_flag)
#define REQUESTED_POKEMON_ACTION_TYPE  (g_scratch2.as_struct.requested_pkmn_action)
#define irSessionPhase                 (g_scratch2.as_struct.ir_session_phase)
#define irXferRemaining                (g_scratch2.as_struct.ir_xfer_remaining)
#define irXferSrc                      (g_scratch2.as_struct.ir_xfer_src)
#define irXferDst                      (g_scratch2.as_struct.ir_xfer_dst)
#define irXferChunkCount               (g_scratch2.as_struct.ir_xfer_chunk_count)
#define rdr_data                       (g_scratch2.as_struct.rdr_data_byte)
#define commandType                    (g_scratch2.as_struct.cmd)
#define commandSubtype                 (g_scratch2.as_struct.cmd_subtype)
#define commandCrcLo                   (g_scratch2.as_struct.cmd_crc_lo)
#define commandCrcHi                   (g_scratch2.as_struct.cmd_crc_hi)
#define commandSessionToken            (g_scratch2.as_struct.cmd_session_token)
#define irPacketPayload                (g_scratch2.as_struct.payload[0])
#define DAT_f8d7                       (g_scratch2.as_struct.payload[1])
#define DAT_f8d8                       (&g_scratch2.as_struct.payload[2])
#define stepDetectAccum                (*(volatile uint32_t *)&g_scratch2.as_struct.payload[0x10])
#define pendingStepDetect              (*(volatile uint32_t *)&g_scratch2.as_struct.payload[0x14])
#define DAT_f8ee                       (g_scratch2.as_struct.payload[0x18])
#define isNotWalking                   (g_scratch2.as_struct.payload[0x19])
#define L_F886                         DAT_f886

/* 0xF7E6..0xF865: 128-byte multi-purpose scratch region. The same bytes are
 * reused across three subsystems (mutually exclusive in time):
 *
 *   - IR/peer transfer buffer: 0x68 bytes from offset 0; first 16 are typed
 *     fields (DAT_f7e6/ea/ee/f0/f2), the rest is the IR payload.
 *   - FFT magnitude bins:       int16_t[32] sharing offsets 0..0x3F.
 *   - X-axis accel samples:     int8_t[64] starting at offset 0x40 (overlaps
 *     ACCEL_SAMPLES_X + DAT_f82e + DAT_f840..f846 + trainerRecBuf + trainerRecBuf_loc).
 *
 * Sound-buffer playback also reuses ACCEL_SAMPLES_X (see drivers/sound.c).
 *
 * The struct member names are offset-based (`at_NN`) because higher-level
 * semantics differ per view; per-view names are exposed as macros below. */
union pw_scratch {
    uint8_t bytes[0x80];
    int16_t fft[32];
    struct {
        uint32_t at_00;       /* +00  DAT_f7e6 (uint32 view of first slot) */
        uint32_t at_04;       /* +04  DAT_f7ea */
        uint16_t at_08;       /* +08  DAT_f7ee */
        uint16_t at_0a;       /* +0A  DAT_f7f0 */
        uint32_t at_0c;       /* +0C  DAT_f7f2 (.s allocates 52 bytes here; rest is at_10) */
        uint8_t  at_10[0x30]; /* +10..3F  IR payload tail */
        uint8_t  at_40[8];    /* +40  ACCEL_SAMPLES_X */
        uint8_t  at_48[18];   /* +48  DAT_f82e */
        uint8_t  at_5a;       /* +5A  DAT_f840 */
        uint8_t  at_5b;       /* +5B  DAT_f841 */
        uint8_t  at_5c;       /* +5C  DAT_f842 */
        uint8_t  at_5d;       /* +5D  DAT_f843 */
        uint16_t at_5e;       /* +5E  DAT_f844 */
        uint32_t at_60;       /* +60  peerRcvdRtcTime (uint32 prefix; .s allocates 8 bytes total) */
        uint8_t  at_64[4];    /* +64  peerRcvdRtcTime unused tail */
        uint8_t  at_68[8];    /* +68  trainerRecBuf */
        uint16_t at_70;       /* +70  trainerRecBuf_loc (uint16 prefix; .s allocates 16 bytes total) */
        uint8_t  at_72[14];   /* +72..7F  trainerRecBuf_loc tail */
    } as_struct;
};
extern union pw_scratch g_scratch;

/* IR transfer buffer view (used during IR sync). */
#define DAT_f7e6        (g_scratch.bytes)              /* uint8_t[128] -- decays to (uint8_t *) */
#define DAT_f7ea        (g_scratch.as_struct.at_04)
#define DAT_f7ee        (g_scratch.as_struct.at_08)
#define DAT_f7f0        (g_scratch.as_struct.at_0a)
#define DAT_f7f2        (g_scratch.as_struct.at_0c)

/* FFT view (used during step detection). */
#define fft_results     (g_scratch.fft)

/* Accel X view + the named globals it overlaps. ACCEL_SAMPLES_X is the
 * 8-byte "window" name; accelXSamples is a 64-byte int8 view extending
 * 0x40 bytes from the same start. */
#define ACCEL_SAMPLES_X (g_scratch.as_struct.at_40)
#define accelXSamples   ((volatile int8_t *)g_scratch.as_struct.at_40)
#define DAT_f82e        (g_scratch.as_struct.at_48)
#define DAT_f840        (g_scratch.as_struct.at_5a)
#define DAT_f841        (g_scratch.as_struct.at_5b)
#define DAT_f842        (g_scratch.as_struct.at_5c)
#define DAT_f843        (g_scratch.as_struct.at_5d)
#define DAT_f844        (g_scratch.as_struct.at_5e)
/* peerRcvdRtcTime: uint32 at offset +0x60 of the IR transfer buffer
 * (g_scratch). When a peer-sync packet is received, this field holds the
 * peer's RTC time; ir_parse_rx_packet uses it to slave our rtcTime if
 * non-zero. (Same memory is the at_60 8-byte slot of g_scratch.) */
#define peerRcvdRtcTime (g_scratch.as_struct.at_60)
/* trainerRecBuf: 8-byte field that's the START of a 0x68-byte trainer-record
 * load buffer used by session.c (game_start_walk) and ir_protocol.c (peer
 * sync paths). The full 0x68 bytes overflow g_scratch into the next BSS
 * region -- known latent issue documented at the union site. */
#define trainerRecBuf   (g_scratch.as_struct.at_68)
/* trainerRecBuf_loc: uint16 at offset +8 from trainerRecBuf == trainer_record.loc
 * field of the loaded record (used in ROM as `mov.w @trainerRecBuf_loc` paired with
 * `mov.l @trainerRecBuf` to copy the id+loc pair to a packet header). */
#define trainerRecBuf_loc (g_scratch.as_struct.at_70)

#define accelYSamples  ((volatile int8_t *)ACCEL_SAMPLES_Y)
#define accelZSamples  ((volatile int8_t *)ACCEL_SAMPLES_Z)
#define L_F886         DAT_f886

/* --- Control Flow --- */
/* Per-tick handler invoked from the main loop; type defined in types.h. */
extern event_loop_func_t currentEventLoopFunc;  /* 0xF7E0 */
extern event_loop_func_t savedEventLoopFunc;    /* 0xF7E2 -- snapshot before swap */

/* --- RNG & Memory --- */
extern volatile uint32_t nextRandom;
extern volatile uint16_t heapPointer;

/* --- LCD & EEPROM --- */
extern volatile uint16_t lcdPageOffset; /* lcd_page_offset (?) -- 0xF7E4 (.s 2 bytes) */
#define RamCache_settingsByte       (g_save.settingsByte)
#define RamCache_settingsByte_BIT (((volatile settings_byte_t *)&RamCache_settingsByte)->BIT)

/* --- Step Processing & IR Comm: see g_scratch2 union above for the full
 * 240-byte scratch region; these symbols are macros. Only globals NOT
 * inside the union are still declared here. */
extern volatile uint8_t commandPos;
extern volatile uint16_t lastCommandTime;
/* Absolute hardware-address aliases (no allocation -- compiler generates
 * direct pointer literals). */
#define DAT_f088 (*(volatile uint8_t *)0xF088)
#define DAT_f580 ((uint8_t *)0xF580)
extern uint8_t eepromPageScratch[14];
extern volatile uint8_t ir_status;
extern const uint8_t musicNoteYTableA[];
extern const uint8_t musicNoteYTableB[];
extern const uint8_t musicNoteInitialState[];
extern const uint8_t mainMenuYCoords[]; /* Main menu Y-coordinate table */
extern const uint8_t IMG_POKEWALKER_IR_ACTIVE[];
extern const uint8_t PERIODTAB[];

extern const uint8_t IMG_POKEWALKER_LARGE[];
extern const uint8_t walkerFaceNeutral[];
extern const uint8_t walkerFaceHappy[];
extern const uint8_t walkerFaceSad[];
extern const uint8_t walkerEmptyExtraGlyph[];
extern const uint8_t DAT_bd40[];

extern const uint8_t radarYCoordTable[];
extern const uint8_t radarFrameMultiplier[];
extern const uint8_t fftBinTable[];
extern const uint8_t radarStateYDivisor[];
extern const uint8_t radarStateXTable[];
extern const uint8_t fftTwiddleTable[];
extern const uint8_t battleMoveOutcomeWeights[];
extern const uint8_t captureSuccessProbs[];
extern const uint8_t battleAnimP1XFrames[];
extern const uint8_t battleAnimP3YFrames[];
extern const uint8_t battleAnimP3XFrames[];
extern const uint8_t battleAnimP4YFrames[];
extern const uint8_t battleAnimP4XFrames[];
extern const uint8_t font3ByteGlyphs[];
extern const uint8_t ballDropAnimYTable[];
extern const uint8_t sparklesAnimXYTable[];
extern const uint8_t cloudAnimYTable[];
extern const uint8_t routeIconIndices[];
extern const uint8_t lcdInitFallbackSeq[];
extern const uint8_t menuItemCostTable[];
extern const uint8_t factoryTestSoundData[];
extern const uint8_t interactionRewardPtrTable[];
extern const uint8_t hexDigits[];
extern const uint8_t nintendoMagic[];
extern const char factoryStr_NG1[4];
extern const char factoryStr_EEP[4];
extern const char factoryStr_NG2[4];
extern const char factoryStr_NG3[4];
extern const char factoryStr_NG4[4];
extern const char factoryStr_V[2];
extern const char factoryStr_NG5[4];
extern const char factoryStr_OK[3];
extern const char factoryStr_NG6[4];

/* --- Sound Engine Globals (Fixed addresses) --- */
extern uint8_t *soundData;
extern uint16_t soundHeader;          /* 0xF7CC (.s 2 bytes) */
extern uint16_t volume;               /* 0xF7C6 (.s 2 bytes) */
extern uint16_t noteDuration;
extern uint16_t isSeparateNote;

#endif /* GLOBALS_H */
