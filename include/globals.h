#ifndef GLOBALS_H
#define GLOBALS_H

#include "types.h"

/* ============================================================
 * RAM data layout (B_RAM section 0xF780..0xFEEF)
 *
 * `struct b_ram_section` mirrors main.mar's B_RAM symbol-by-symbol.
 * The single instance `g` is declared in src/globals.c and anchored
 * at 0xF780 by the linker (see -start=B_RAM/F780 in Makefile).
 *
 * Defined ABOVE the volatile-pointer macros that follow so member
 * identifiers don't get macro-expanded inside the struct definition.
 * Members are currently storage-only — every existing macro below
 * still drives RAM access; per-member migration to `g.<name>` is a
 * later step that requires deleting the corresponding macros first.
 *
 * Verify with `scripts/compare_data_layout.py --section B_RAM`.
 * ============================================================ */
/* View-state region (0xF7CE..0xF7D9, 12 bytes).
 *
 * Y/Z/A are the universal per-tick substate triple every view uses. The
 * remaining 9 bytes are time-multiplexed: each view re-uses them for its
 * own state (accel position when sampling, dowsing slot index, battle HP,
 * radar round, peer reward tier, inventory bitmask, etc.).
 *
 * Until each view's interpretation is researched, accesses go through the
 * `bytes` view (raw byte at ROM offset). Per-subsystem typed views can be
 * added to the union as we audit each consumer. The `union` deliberately
 * uses *named* inner members (not anonymous) for ch38 6.2.2 C89 compat. */
struct viewstate {
    volatile uint8_t  Y;        /* 0xF7CE */
    volatile uint8_t  Z;        /* 0xF7CF */
    volatile uint8_t  A;        /* 0xF7D0 */
    union {
        struct {
            volatile uint8_t  at_d1;   /* 0xF7D1 */
            volatile uint8_t  at_d2;   /* 0xF7D2 */
            volatile uint8_t  at_d3;   /* 0xF7D3 */
            volatile uint8_t  at_d4;   /* 0xF7D4 */
            volatile uint8_t  at_d5;   /* 0xF7D5 */
            volatile uint8_t  at_d6;   /* 0xF7D6 */
            volatile uint8_t  at_d7;   /* 0xF7D7 */
            volatile uint8_t  at_d8;   /* 0xF7D8 */
            volatile uint8_t  at_d9;   /* 0xF7D9 */
        } bytes;
        /* Dowsing minigame's interpretation of these 9 bytes.
         * Documented in dowsing.c's header comment. */
        struct {
            volatile uint8_t cursor;            /* 0xF7D1 = at_d1 — current cursor (0..5) */
            volatile uint8_t attemptsRemaining; /* 0xF7D2 = at_d2 — counts down each guess */
            volatile uint8_t hiddenSlot;        /* 0xF7D3 = at_d3 — secret item slot (0..5) */
            volatile uint8_t markedWrongSlot;   /* 0xF7D4 = at_d4 — last wrong-guess slot */
            volatile uint8_t wattReward;        /* 0xF7D5 = at_d5 — nonzero -> show g.save_watts */
            volatile uint8_t saveSlot;          /* 0xF7D6 = at_d6 — index for awarded-item store */
            volatile uint8_t _at_d7;            /* 0xF7D7 — unused by dowsing */
            volatile uint8_t awardedItemHi;     /* 0xF7D8 = at_d8 — uint16 item-id hi byte */
            volatile uint8_t awardedItemLo;     /* 0xF7D9 = at_d9 — uint16 item-id lo byte */
        } dowsing;
        /* Add more per-subsystem views here as each is audited. ch38 pads
         * uint16 members to even offsets within a struct; since this
         * union sits at an odd offset within viewstate (Y/Z/A take 3
         * bytes), any uint16 member here would land 1 byte off. Keep
         * fields uint8 and expose wider views via cast macros below. */
    } v;
};

struct b_ram_section {
#include "b_ram_layout.h"
};

extern volatile struct b_ram_section g;

/* --- System Status Flags --- */
#define sys_statusFlags_BIT (((volatile status_flags_t *)&g.sys_statusFlags)->BIT)
extern volatile uint8_t status_flags_f7f1;   /* DAT_f7f1 */
#define sys_walkerFlags_BIT (((volatile walker_status_t *)&g.sys_walkerFlags)->BIT)

/* --- LCD & UI / Timers --- */

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
    VIEW_DISCARD_PICKER          = 0x07,
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
    VIEW_FACTORY_TEST                 = 0x16,
    VIEW_ACCEL_DEBUG           = 0x17,
    VIEW_TEXT                  = 0x18
};

/* --- Button Input --- */
#define btn_inputRaw_BIT (((volatile button_input_t *)&g.btn_inputRaw)->BIT)

/* --- Pedometer & Activity --- */
/* Persisted save block at 0xF780..F797 (see struct session_save in globals.c).
 * Each member is exposed as a macro so existing call sites referring to the
 * field by its top-level name continue to compile and so &name remains a
 * valid pointer to the underlying byte. */

#define ped_taskFlags_BIT (((volatile ped_task_flags_t *)&g.ped_taskFlags)->BIT)

/* --- Substate Management & Sensor Data --- */
/* g.viewstate.Y -- bits 0/1 used as flags via bset/bclr/bst in ROM. */
#define ui_substateY_BIT (((volatile byte_bits_t *)&g.viewstate.Y)->BIT)

/* g.viewstate.v.bytes.at_d8 -- bit 0 used as a flag in battle.c via bset/bclr in ROM. */
#define DAT_f7d8_BIT (((volatile byte_bits_t *)&g.viewstate.v.bytes.at_d8)->BIT)

/* ir_packetReceivedFlag -- bit 0 used; ROM emits bset/bclr. */
#define ir_packetReceivedFlag_BIT (((volatile byte_bits_t *)&g_scratch2.s.ir_packetReceivedFlag)->BIT)

#define DAT_f7d1_BIT (((volatile byte_bits_t *)&g.viewstate.v.bytes.at_d1)->BIT)
/* 16-bit "accel physics" view: the accel driver and game_process_accel_data
 * read these positions as uint16 (mov.w) while game/UI code uses the byte form
 * above for slot indices. */
#define accel_xPosition_word  (*(volatile uint16_t *)&g.viewstate.v.bytes.at_d2)
#define accel_yPosition_word  (*(volatile uint16_t *)&g.viewstate.v.bytes.at_d4)
#define accel_zPosition_word  (*(volatile uint16_t *)&g.viewstate.v.bytes.at_d6)
/* 0xF7D6 is accessed as a BYTE (mov.b) in most game-logic contexts (dowsing
 * slot index, radar countdown, etc.); only the accel-physics accumulator in
 * drv_accel_sample treats it as the high byte of a uint16. This alias is the
 * byte view. */
#define accel_zPosition_byte (*(volatile uint8_t  *)&g.viewstate.v.bytes.at_d6)
/* 0xF7D8 is also accessed as a 16-bit word in dowsing (item ID) — disassembly
 * shows mov.w @g.viewstate.v.bytes.at_d8 + drv_eeprom_write_block size 2. The byte alias above
 * is used by battle.c and pedometer.c for flag/limit bytes. */
#define DAT_f7d8_w  (*(volatile uint16_t *)&g.viewstate.v.dowsing.awardedItemHi)

/* 0xF866..0xF955: 240-byte multi-purpose scratch region (sibling of g_scratch).
 * Memory is reused across mutually-exclusive subsystems:
 *
 *   - Accel-Y samples:   int8_t[64] from offset 0   (covers accel_samplesYArr + at_f886/88e/896/897)
 *   - Accel-Z samples:   int8_t[64] from offset 0x40 (overlaps IR session-key state)
 *   - IR session state:  ir_sessionKeyNext/ir_sessionKey/handshake/retry counters/xfer (offsets 0x50..0x67)
 *   - IR packet buffer:  136 bytes from offset 0x68  (cmd/subtype/crc/session/payload)
 *   - Step detection state when IR idle: ped_stepDetectAccumulator, ped_pendingStepDetect, ped_isNotWalking
 *     overlap with the IR packet payload tail.
 *
 * Field names use offset-based `at_NN` for slots whose semantic role across
 * views isn't single. Per-view names are exposed as macros below. */
union pw_scratch2 {
    uint8_t bytes[0xF0];
    struct {
        uint8_t  accel_y[32];        /* +0x00 accel_samplesYArr / accel_samplesY[0..31] */
        uint8_t  at_f886[8];         /* +0x20 = accel_samplesY[32..39] */
        uint8_t  at_f88e[8];         /* +0x28 = accel_samplesY[40..47] */
        uint8_t  at_f896;            /* +0x30 = accel_samplesY[48] */
        uint8_t  at_f897[15];        /* +0x31 = accel_samplesY[49..63] */
        uint8_t  accel_z[3];         /* +0x40 accel_samplesZArr / accel_samplesZ[0..2] */
        uint8_t  at_f8a9[13];        /* +0x43 = accel_samplesZ[3..15] */
        volatile uint32_t ir_sessionKeyNext;       /* +0x50 */
        volatile uint32_t ir_sessionKey;           /* +0x54 */
        volatile uint8_t  ir_handshakeStep;        /* +0x58 */
        volatile uint8_t  ir_timeoutRetryCount;    /* +0x59 (ROM uses byte access only) */
        volatile uint8_t  _pad_at_5a;              /* +0x5A */
        volatile uint8_t  at_f8c1;                 /* +0x5B */
        volatile uint8_t  ir_crcRetryCount;        /* +0x5C */
        volatile uint8_t  ir_packetReceivedFlag;   /* +0x5D */
        volatile uint8_t  ir_requestedPokemonAction; /* +0x5E */
        volatile uint8_t  ir_sessionPhase;         /* +0x5F */
        volatile uint16_t ir_xferRemaining;        /* +0x60 */
        volatile uint16_t ir_xferSrc;              /* +0x62 */
        volatile uint16_t ir_xferDst;              /* +0x64 */
        volatile uint8_t  ir_xferChunkCount;       /* +0x66 */
        volatile uint8_t  ir_rdrData;              /* +0x67 */
        volatile uint8_t  ir_commandType;          /* +0x68 */
        volatile uint8_t  ir_commandSubtype;       /* +0x69 (unused) */
        volatile uint8_t  ir_commandCrcLo;         /* +0x6A */
        volatile uint8_t  ir_commandCrcHi;         /* +0x6B */
        volatile uint32_t ir_commandSessionToken;  /* +0x6C */
        volatile uint8_t  payload[0x80];           /* +0x70 IR payload (also overlaps step state when idle) */
    } s;
};
#define g_scratch2 (*(union pw_scratch2 *)0xF866u)

/* Typed views that don't have a direct struct member equivalent. The struct
 * fields (g_scratch2.s.X) are used directly at call sites for everything else. */
#define accel_samplesY            ((volatile int8_t *)g_scratch2.s.accel_y)
#define accel_samplesZ            ((volatile int8_t *)g_scratch2.s.accel_z)
#define DAT_f886                  (g_scratch2.s.at_f886)
#define DAT_f88e                  (g_scratch2.s.at_f88e)
#define DAT_f896                  (g_scratch2.s.at_f896)
#define DAT_f8c1                  (g_scratch2.s.at_f8c1)
#define ir_payload                (g_scratch2.s.payload[0])
#define ped_stepDetectAccumulator (*(volatile uint32_t *)&g_scratch2.s.payload[0x10])
#define ped_pendingStepDetect     (*(volatile uint32_t *)&g_scratch2.s.payload[0x14])
#define DAT_f8ee                  (g_scratch2.s.payload[0x18])
#define ped_isNotWalking          (g_scratch2.s.payload[0x19])

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
        uint32_t at_00;              /* +00  DAT_f7e6 uint32 view of first slot */
        uint32_t at_04;              /* +04  DAT_f7ea */
        uint16_t at_08;              /* +08  DAT_f7ee */
        uint16_t at_0a;              /* +0A  DAT_f7f0 */
        uint32_t at_0c;              /* +0C  DAT_f7f2 (.s allocates 52 bytes here; rest is at_10) */
        uint8_t  at_10[0x30];        /* +10..3F  IR payload tail */
        uint8_t  accel_samplesXArr[8]; /* +40 */
        uint8_t  at_48[18];          /* +48  DAT_f82e */
        uint8_t  at_5a;              /* +5A  DAT_f840 */
        uint8_t  at_5b;              /* +5B  DAT_f841 */
        uint8_t  at_5c;              /* +5C  DAT_f842 */
        uint8_t  at_5d;              /* +5D  DAT_f843 */
        uint16_t at_5e;              /* +5E  DAT_f844 */
        uint32_t peerRcvdRtcTime;    /* +60 .s allocates 8 bytes total */
        uint8_t  at_64[4];           /* +64 peerRcvdRtcTime unused tail */
        uint8_t  trainerRecBuf[8];   /* +68 start of 0x68-byte buffer (overflows union) */
        uint16_t trainerRecBuf_loc;  /* +70 .s allocates 16 bytes total */
        uint8_t  at_72[14];          /* +72..7F trainerRecBuf_loc tail */
    } s;
};
#define g_scratch (*(union pw_scratch *)0xF7E6u)

/* Typed view aliases (no plain struct equivalent). */
#define DAT_f7e6        (g_scratch.bytes)              /* uint8_t[128] -- decays to (uint8_t *) */
#define DAT_f7ea        (g_scratch.s.at_04)
#define DAT_f7ee        (g_scratch.s.at_08)
#define DAT_f7f0        (g_scratch.s.at_0a)
#define DAT_f7f2        (g_scratch.s.at_0c)
#define fft_results     (g_scratch.fft)
#define accelXSamples   ((volatile int8_t *)g_scratch.s.accel_samplesXArr)
#define DAT_f82e        (g_scratch.s.at_48)
#define DAT_f840        (g_scratch.s.at_5a)
#define DAT_f841        (g_scratch.s.at_5b)
#define DAT_f842        (g_scratch.s.at_5c)
#define DAT_f843        (g_scratch.s.at_5d)
#define DAT_f844        (g_scratch.s.at_5e)


/* --- Control Flow --- */
/* Per-tick handler invoked from the main loop; type defined in types.h. */

/* --- RNG & Memory --- */

/* --- LCD & EEPROM --- */
#define _pad_f7e5 (*(volatile uint8_t *)0xF7E5u)
#define save_settings_BIT (((volatile settings_byte_t *)&g.save_settings)->BIT)

/* --- Step Processing & IR Comm: see g_scratch2 union above for the full
 * 240-byte scratch region; these symbols are macros. Only globals NOT
 * inside the union are still declared here. */
/* Absolute hardware-address aliases (no allocation -- compiler generates
 * direct pointer literals). */
#define DAT_f088 (*(volatile uint8_t *)0xF088)
#define DAT_f580 ((uint8_t *)0xF580)
#define eepromPageScratch ((uint8_t *)0xF956u)
extern volatile uint8_t ir_status;
/* ROM data labels — defined as per-label const arrays in src/romdata.c.
 * optlnk packs them contiguously within `#pragma section P` in source
 * order, reproducing the original ROM byte layout. */
extern const uint8_t BATTLE_ANIM_P1_X[4];
extern const struct yx_pair BATTLE_ANIM_P3[9];
extern const struct yx_pair BATTLE_ANIM_P4[9];
extern const uint8_t BATTLE_OUTCOME_WEIGHTS[15];
extern const uint8_t CAPTURE_PROBS[5];
extern const uint8_t SOUND_PERIOD_TABLE[42];
extern const uint8_t IMG_POKEWALKER_LARGE[256];
extern const uint8_t IMG_FACE_NEUTRAL[32];
extern const uint8_t IMG_FACE_HAPPY[32];
extern const uint8_t IMG_FACE_SAD[32];
extern const uint8_t IMG_EXTRA_GLYPH_EMPTY[16];
extern const uint8_t IMG_POKEWALKER_IR_ACTIVE[16];
extern const uint8_t FONT_3BYTE_GLYPHS[108];
extern const uint16_t UNREF_SWITCH_TABLE_3FFA[8];
extern const uint8_t ANIM_BALL_DROP_Y[6];
extern const uint8_t ANIM_SPARKLES_XY[6];
extern const uint8_t ANIM_CLOUD_Y[5];
extern const uint8_t PAD_BD81[1];
extern const int8_t DOWSING_GRASS_BOB[2];
extern const uint16_t INTERACTION_REWARD_PTRS[38];
extern const int16_t FFT_TWIDDLE[80];
extern const uint8_t MUSIC_NOTE_HEIGHTS[6];
extern const uint8_t UNREF_SWITCH_LUT_72CE[8];
extern const uint8_t UNREF_SWITCH_LUT_7364[24];
extern const uint8_t UNREF_SWITCH_LUT_741E[26];
extern const uint8_t ROUTE_ICON_INDICES[8];
extern const uint8_t LCD_INIT_FALLBACK_SEQ[44];
extern const uint16_t UNREF_SWITCH_TABLE_8DD2[10];
extern const uint8_t FFT_BINS[10];
extern const uint16_t UNREF_SWITCH_TABLE_97B0[6];
extern const uint8_t MENU_ITEM_COSTS[6];
extern const uint8_t MAIN_MENU_Y_COORDS[6];
extern const uint8_t RADAR_STATE_X[4];
extern const uint8_t RADAR_STATE_Y_DIVISOR[3];
extern const uint8_t RADAR_FRAME_MULT[4];
extern const uint8_t RADAR_Y_COORDS[4];
extern const uint8_t PAD_BF29[1];
extern const uint16_t UNREF_SWITCH_TABLE_AA8E[19];
extern const uint16_t UNREF_SWITCH_TABLE_AD20[19];
extern const char FACTORY_STR_NG1[4];
extern const char FACTORY_STR_EEP[4];
extern const char FACTORY_STR_NG2[4];
extern const char FACTORY_STR_NG3[4];
extern const char FACTORY_STR_NG4[4];
extern const char FACTORY_STR_V[2];
extern const char FACTORY_STR_NG5[4];
extern const char FACTORY_STR_OK[3];
extern const char FACTORY_STR_NG6[4];
extern const uint8_t PAD_BF97[1];
extern const char NINTENDO_MAGIC[9];
extern const uint8_t PAD_BFA1[1];
extern const uint8_t FACTORY_TEST_SOUND[8];
extern const char HEX_DIGITS[16];
extern const uint16_t CRT_INIT_TABLE[5];
extern const uint8_t CRT_INIT_DATA[4];
extern const uint8_t ROM_TAIL_PADDING[56];


/* --- Sound Engine Globals (Fixed addresses) --- */
#define _pad_f7cd (*(uint8_t *)0xF7CDu)
#define _pad_f7c7 (*(uint8_t *)0xF7C7u)

#endif /* GLOBALS_H */
