#ifndef GLOBALS_H
#define GLOBALS_H

#include "types.h"

/* 0xF7E6..0xF865: 128-byte multi-purpose scratch region. The bytes are
 * time-multiplexed across several subsystems. Each view is exposed as a
 * named union member describing that view's interpretation of the bytes.
 *
 *   - ir:            IR-staged trainer record (0x00..0x67); see types.h
 *                    `struct trainer_record`. Mirrored layout with wider-typed
 *                    fields where the ROM uses uint16/uint32 access.
 *   - fft:           int16_t[32] step-detect magnitude bins (offsets 0..0x3F).
 *   - accel:         X-axis accel sample buffer at +0x40 (also reused as the
 *                    sound playback buffer, see drivers/sound.c).
 *   - peerSync:      uint32 RTC time at +0x60 (received from peer during sync).
 *   - secondTrainer: 8-byte start + uint16 loc of a SECOND trainer record at
 *                    +0x68..+0x71. The full 0x68 bytes overflow the union
 *                    into the next BSS region — known latent issue. */
union pw_scratch {
    uint8_t bytes[0x80];
    int16_t fft[32];
    struct {
        uint32_t id;              /* +0x00 trainer id (DAT_f7e6 uint32 view) */
        uint32_t id_backup;       /* +0x04 DAT_f7ea */
        uint16_t loc;             /* +0x08 DAT_f7ee */
        uint16_t loc_backup;      /* +0x0A DAT_f7f0 */
        uint32_t at_0c_w;         /* +0x0C DAT_f7f2 (uint32 view; trainer_record.at_0c[4]) */
        uint8_t  midBlock[0x30];  /* +0x10..0x3F nickname + marker_46 + at_27 + at_28 + flags_38[0..7] */
        uint8_t  flags_38_tail[8];/* +0x40..0x47 last 8 bytes of trainer_record.flags_38 (overlays accel.samples) */
        uint8_t  at_48[18];       /* +0x48..0x59 DAT_f82e block (matches trainer_record.at_48) */
        uint8_t  eventBitIndex;   /* +0x5A DAT_f840 (event-bit index passed to save_set_event_bit) */
        volatile uint8_t flags_5b;/* +0x5B DAT_f841 (b0/b1 used; bits 7..3 pack an hour value) */
        uint8_t  at_5c;           /* +0x5C DAT_f842 (battle context?) */
        uint8_t  at_5d;           /* +0x5D DAT_f843 */
        uint16_t at_5e_w;         /* +0x5E DAT_f844 (uint16; checked != 0) */
        uint8_t  at_60[8];        /* +0x60..0x67 trailing block (overlays peerSync.rcvdRtcTime) */
    } ir;
    struct {
        uint8_t _pad0[0x40];      /* +0x00..0x3F unused by this view */
        int8_t samples[8];        /* +0x40..0x47 X-axis accel samples (also sound playback buffer) */
    } accel;
    struct {
        uint8_t _pad0[0x60];      /* +0x00..0x5F unused by this view */
        uint32_t rcvdRtcTime;     /* +0x60 peer-RTC during IR sync (overlays ir.at_60[0..3]) */
    } peerSync;
    struct {
        uint8_t _pad0[0x68];      /* +0x00..0x67 unused by this view */
        uint8_t  buf[8];          /* +0x68 start of 2nd trainer record (overflows union; see header) */
        uint16_t loc;             /* +0x70 trainer_record.loc of the 2nd record */
        uint8_t  tail[14];        /* +0x72..0x7F trailing bytes (full 2nd record overflows here) */
    } secondTrainer;
};

/* 0xF866..0xF955: 240-byte multi-purpose scratch region (sibling of scratch1).
 * Time-multiplexed across subsystems; each view names its interpretation.
 *
 *   - accel: Y-samples uint8[64] at 0..0x3F and Z-samples uint8[16] at 0x40..0x4F.
 *   - ir:    TX/RX byte-iteration buffer at 0x00..0x4F (overlaps accel), session
 *            state at 0x50..0x67, command header at 0x68..0x6F, packet payload
 *            at 0x70..0xEF.
 *   - ped:   step-detect counters at offsets corresponding to ir.payload[0x10..0x19]
 *            (used while IR is idle). */
union pw_scratch2 {
    uint8_t bytes[0xF0];
    struct {
        int8_t y[64];                               /* +0x00..0x3F Y-axis accel samples */
        int8_t z[16];                               /* +0x40..0x4F Z-axis accel samples */
        uint8_t _pad0[0xA0];                        /* +0x50..0xEF unused by this view */
    } accel;
    struct {
        uint8_t _headRegion[0x20];                  /* +0x00..0x1F unused by IR (overlays accel.y[0..31]) */
        uint8_t txResponseA[16];                    /* +0x20..0x2F first TX-response payload buffer
                                                     *   - DAT_f886 = &[0]  (16-byte block copied to rxptr in case 0xA6/0xA8/0xAA/0xAC/0xAE)
                                                     *   - DAT_f88e = &[8]  (g.save_watts/20 stashed here) */
        uint8_t txResponseB[16];                    /* +0x30..0x3F second TX-response payload buffer
                                                     *   - DAT_f896 = &[0]  (16-byte block copied to rxptr+0x26 in case 0x14) */
        uint8_t _midRegion[16];                     /* +0x40..0x4F unused by IR (overlays accel.z[0..15]; DAT_f8a9 was unused) */
        volatile uint32_t sessionKeyNext;           /* +0x50 */
        volatile uint32_t sessionKey;               /* +0x54 */
        volatile uint8_t  handshakeStep;            /* +0x58 */
        volatile uint8_t  timeoutRetryCount;        /* +0x59 */
        volatile uint8_t  _pad_at_5a;               /* +0x5A */
        volatile uint8_t  at_5b;                    /* +0x5B (DAT_f8c1; init=0 with other session state, role unclear) */
        volatile uint8_t  crcRetryCount;            /* +0x5C */
        volatile byte_bits_t packetReceivedFlag;    /* +0x5D */
        volatile uint8_t  requestedPokemonAction;   /* +0x5E */
        volatile uint8_t  sessionPhase;             /* +0x5F */
        volatile uint16_t xferRemaining;            /* +0x60 */
        volatile uint16_t xferSrc;                  /* +0x62 */
        volatile uint16_t xferDst;                  /* +0x64 */
        volatile uint8_t  xferChunkCount;           /* +0x66 */
        volatile uint8_t  rdrData;                  /* +0x67 */
        volatile uint8_t  commandType;              /* +0x68 */
        volatile uint8_t  commandSubtype;           /* +0x69 (unused) */
        volatile uint8_t  commandCrcLo;             /* +0x6A */
        volatile uint8_t  commandCrcHi;             /* +0x6B */
        volatile uint32_t commandSessionToken;      /* +0x6C */
        volatile uint8_t  payload[0x80];            /* +0x70..0xEF IR payload */
    } ir;
    struct {
        uint8_t _pad0[0x80];                        /* +0x00..0x7F unused (overlaps accel + ir session+cmd) */
        volatile uint32_t stepDetectAccumulator;    /* +0x80 = ir.payload[0x10] */
        volatile uint32_t pendingStepDetect;        /* +0x84 = ir.payload[0x14] */
        volatile uint8_t  _resetByte;               /* +0x88 = ir.payload[0x18] — set to 0 alongside step-detect state (DAT_f8ee) */
        volatile uint8_t  isNotWalking;             /* +0x89 = ir.payload[0x19] */
    } ped;
};

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
    volatile byte_bits_t Y;     /* 0xF7CE */
    volatile uint8_t  Z;        /* 0xF7CF */
    volatile uint8_t  A;        /* 0xF7D0 */
    union {
        struct {
            volatile byte_bits_t at_d1; /* 0xF7D1 */
            volatile uint8_t  at_d2;   /* 0xF7D2 */
            volatile uint8_t  at_d3;   /* 0xF7D3 */
            volatile uint8_t  at_d4;   /* 0xF7D4 */
            volatile uint8_t  at_d5;   /* 0xF7D5 */
            volatile uint8_t  at_d6;   /* 0xF7D6 */
            volatile uint8_t  at_d7;   /* 0xF7D7 */
            volatile byte_bits_t at_d8; /* 0xF7D8 */
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
        /* Battle minigame's interpretation. Documented in battle.c header. */
        struct {
            volatile byte_bits_t hpRemaining;       /* 0xF7D1 = at_d1 — HP/wiggle bar segments */
            volatile uint8_t     animTick;          /* 0xF7D2 = at_d2 — per-state animation tick */
            volatile uint8_t     stateDwellLen;     /* 0xF7D3 = at_d3 — frames to wait before advancing */
            volatile uint8_t     spriteY;           /* 0xF7D4 = at_d4 — pokemon sprite y position */
            volatile uint8_t     spriteX;           /* 0xF7D5 = at_d5 — pokemon/ball sprite x position */
            volatile uint8_t     wattsToForfeitHi;  /* 0xF7D6 = at_d6 — uint16 hi byte (g.save_watts paid on loss) */
            volatile uint8_t     wattsToForfeitLo;  /* 0xF7D7 = at_d7 — uint16 lo byte */
            volatile byte_bits_t flags;             /* 0xF7D8 = at_d8 — packed flags (see battle.c header) */
            volatile uint8_t     wiggleSuccessCount;/* 0xF7D9 = at_d9 — 3 caps capture */
        } battle;
        /* Pokeradar minigame's interpretation. Documented in radar.c header. */
        struct {
            volatile byte_bits_t roundsCompleted; /* 0xF7D1 = at_d1 */
            volatile uint8_t     roundsRequired;  /* 0xF7D2 = at_d2 (rolled at init) */
            volatile uint8_t     secretPatchIndex;/* 0xF7D3 = at_d3 (0..3, re-rolled each round) */
            volatile uint8_t     timerSubTick;    /* 0xF7D4 = at_d4 */
            volatile uint8_t     timeRemaining;   /* 0xF7D5 = at_d5 (countdown / fade-frame) */
            volatile uint8_t     lockAnimTimer;   /* 0xF7D6 = at_d6 (RADAR_LOCK_ANIM) */
            volatile uint8_t     _at_d7;          /* 0xF7D7 — unused by radar */
            volatile byte_bits_t _at_d8;          /* 0xF7D8 — unused by radar */
            volatile uint8_t     _at_d9;          /* 0xF7D9 — unused by radar */
        } radar;
        /* Peer-play post-IR celebration view (game_calculate_interaction_reward
         * + ui_render_peer_play). Documented in peer_play.c. */
        struct {
            volatile byte_bits_t resultTextIndex;  /* 0xF7D1 = at_d1 — TEXT_BOX index 0x2C..0x30 (also bit-read in home.c) */
            volatile uint8_t     subTextOrSlot;    /* 0xF7D2 = at_d2 — sub-text id / dowsing-item slot 0..9 */
            volatile uint8_t     wattsAwarded;     /* 0xF7D3 = at_d3 — clamped to 1..99 */
            volatile uint8_t     _at_d4;           /* 0xF7D4 — unused by peer_play */
            volatile uint8_t     _at_d5;           /* 0xF7D5 — unused by peer_play */
            volatile uint8_t     _at_d6;           /* 0xF7D6 — unused by peer_play */
            volatile uint8_t     _at_d7;           /* 0xF7D7 — unused by peer_play */
            volatile byte_bits_t _at_d8;           /* 0xF7D8 — unused by peer_play */
            volatile uint8_t     _at_d9;           /* 0xF7D9 — unused by peer_play */
        } peerPlay;
        /* Factory test view (multi-stage; bytes serve different roles per stage).
         * Documented in factory_test.c. */
        struct {
            volatile byte_bits_t currentInputByte;  /* 0xF7D1 = at_d1 — live button-input or accel-sample */
            volatile uint8_t     expectedInputByte; /* 0xF7D2 = at_d2 — expected value to compare against */
            volatile uint8_t     testResultGate;    /* 0xF7D3 = at_d3 — 0 = NG, nonzero = OK */
            volatile uint8_t     calibValueLo;      /* 0xF7D4 = at_d4 — accel-cal uint16 (lo byte) / generic counter */
            volatile uint8_t     calibValueHi;      /* 0xF7D5 = at_d5 — accel-cal uint16 (hi byte) */
            volatile uint8_t     _at_d6;            /* 0xF7D6 — unused */
            volatile uint8_t     _at_d7;            /* 0xF7D7 — unused */
            volatile byte_bits_t _at_d8;            /* 0xF7D8 — unused */
            volatile uint8_t     _at_d9;            /* 0xF7D9 — unused */
        } factoryTest;
        /* Accel debug view (factory-test follow-on; technician wiggles device
         * until displayCounter matches calTarget). Documented in accel_debug.c. */
        struct {
            volatile byte_bits_t displayCounter; /* 0xF7D1 = at_d1 — drawn as ASCII digit; matches calTarget triggers SPI 0xA7 */
            volatile uint8_t     _at_d2;         /* 0xF7D2 — init 0; not otherwise read */
            volatile uint8_t     _at_d3;         /* 0xF7D3 — unused */
            volatile uint8_t     _at_d4;         /* 0xF7D4 — init 0; not otherwise read */
            volatile uint8_t     _at_d5;         /* 0xF7D5 — unused */
            volatile uint8_t     _at_d6;         /* 0xF7D6 — unused */
            volatile uint8_t     _at_d7;         /* 0xF7D7 — unused */
            volatile byte_bits_t calByte0;       /* 0xF7D8 = at_d8 — first byte of 8-byte EEPROM accel cal (overflows into ped_axis* fields) */
            volatile uint8_t     calTarget;      /* 0xF7D9 = at_d9 — second byte of cal; target value that displayCounter must reach */
        } accelDebug;
        /* Home-view standby state machine (LCD fade-out/in). Owned by power.c
         * (sys_enter_standby + sys_update_standby_state) and read by home.c
         * (ui_render_home_route) to choose draw path. */
        struct {
            volatile byte_bits_t flags; /* 0xF7D1 = at_d1 — packed:
                                         *   b0 = entered deep standby
                                         *   b1 = standby active (locks state)
                                         *   b2 = fade direction / visual blink toggle */
            volatile uint8_t _at_d2;    /* 0xF7D2..0xF7D9 unused by homeStandby */
            volatile uint8_t _at_d3;
            volatile uint8_t _at_d4;
            volatile uint8_t _at_d5;
            volatile uint8_t _at_d6;
            volatile uint8_t _at_d7;
            volatile byte_bits_t _at_d8;
            volatile uint8_t _at_d9;
        } homeStandby;
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
extern volatile uint8_t status_flags_f7f1;   /* DAT_f7f1 */

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

/* --- Pedometer & Activity --- */
/* Persisted save block at 0xF780..F797 (see struct session_save in globals.c).
 * Each member is exposed as a macro so existing call sites referring to the
 * field by its top-level name continue to compile and so &name remains a
 * valid pointer to the underlying byte. */


/* --- Substate Management & Sensor Data --- */
/* g.viewstate.Y -- bits 0/1 used as flags via bset/bclr/bst in ROM. */

/* g.viewstate.v.bytes.at_d8 -- bit 0 used as a flag in battle.c via bset/bclr in ROM. */

/* ir_packetReceivedFlag -- bit 0 used; ROM emits bset/bclr. */

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
/* 0xF7D8 is also accessed as a 16-bit word in dowsing (item ID) — disassembly
 * shows mov.w @g.viewstate.v.bytes.at_d8 + drv_eeprom_write_block size 2. The byte alias above
 * is used by battle.c and pedometer.c for flag/limit bytes. */
#define DAT_f7d8_w  (*(volatile uint16_t *)&g.viewstate.v.dowsing.awardedItemHi)

/* g.scratch1 / g.scratch2 typed views (no plain struct equivalent). */
#define DAT_f886                  (g.scratch2.ir.txResponseA)         /* uint8_t[16] -- decays to ptr */
#define DAT_f88e                  (&g.scratch2.ir.txResponseA[8])     /* uint8_t * — DAT_f88e[0] = txResponseA[8] */
#define DAT_f896                  (g.scratch2.ir.txResponseB)         /* uint8_t[16] -- decays to ptr */
#define DAT_f8c1                  (g.scratch2.ir.at_5b)

#define DAT_f7e6        (g.scratch1.bytes)              /* uint8_t[128] -- decays to (uint8_t *) */
#define DAT_f7f2        (g.scratch1.ir.at_0c_w)
#define DAT_f82e        (g.scratch1.ir.at_48)
#define DAT_f842        (g.scratch1.ir.at_5c)
#define DAT_f843        (g.scratch1.ir.at_5d)
#define DAT_f844        (g.scratch1.ir.at_5e_w)


/* --- Control Flow --- */
/* Per-tick handler invoked from the main loop; type defined in types.h. */

/* --- RNG & Memory --- */

/* --- LCD & EEPROM --- */

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

#endif /* GLOBALS_H */
