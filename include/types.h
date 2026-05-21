/*
 * types.h - Standard integer type definitions for the Pokewalker decompilation.
 *
 * The ch38 compiler targets C89; stdint.h is not available.
 * On the H8/300H: char=8-bit, int=16-bit, long=32-bit.
 */

#ifndef TYPES_H
#define TYPES_H

typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned long uint32_t;

typedef signed char int8_t;
typedef signed int int16_t;
typedef signed long int32_t;

typedef uint32_t uintptr_t;
typedef uint16_t size_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef __clang__
/* Dummy macros to make H8 assembly blocks pass clang syntax check */
#define __noregsave
#define __interrupt(...)
#define __entry(...)
#define __regparam3
#define __sectop(section) 0
#define __secend(section) 0
#endif

typedef struct {
  uint8_t b0 : 1;
  uint8_t b1 : 1;
  uint8_t b2 : 1;
  uint8_t b3 : 1;
  uint8_t b4 : 1;
  uint8_t b5 : 1;
  uint8_t b6 : 1;
  uint8_t b7 : 1;
} bit_flags_t;

/* RAM-global bit-field unions.
 * ch38 allocates bit fields MSB-first, so the first declared field is bit 7.
 * Bit names are inferred from C call-site usage; rename freely.
 * These are paired with REG_BIT-style accessor macros in globals.h. */

typedef union {
    uint8_t BYTE;
    struct {
        uint8_t sleeping              : 1;  /* bit 7 -- 0x80 */
        uint8_t eeprom_busy           : 1;  /* bit 6 -- 0x40 */
        uint8_t pedometer_paused      : 1;  /* bit 5 -- 0x20 */
        uint8_t lcd_dirty             : 1;  /* bit 4 -- 0x10 */
        uint8_t button_event          : 1;  /* bit 3 -- 0x08 */
        uint8_t battery_check_request : 1;  /* bit 2 -- 0x04 */
        uint8_t low_battery           : 1;  /* bit 1 -- 0x02 */
        uint8_t tick                  : 1;  /* bit 0 -- 0x01 */
    } BIT;
} status_flags_t;

/* walker_status_flags: bits 0,1,2 are independent flags;
 * bits 3-4 are a 2-bit mode field (0x18 mask used as multi-bit compare in C).
 * Byte form is preserved for the multi-bit mask sites. */
typedef union {
    uint8_t BYTE;
    struct {
        uint8_t                : 3;  /* bits 7-5 unused */
        uint8_t mode           : 2;  /* bits 4-3 -- 0x18 mask in flat form */
        uint8_t walking        : 1;  /* bit 2 -- 0x04 */
        uint8_t session_active : 1;  /* bit 1 -- 0x02 */
        uint8_t input_pending  : 1;  /* bit 0 -- 0x01 */
    } BIT;
} walker_status_t;

/* RamCache_settingsByte packed user settings */
typedef union {
    uint8_t BYTE;
    struct {
        uint8_t          : 1;  /* bit 7 unused */
        uint8_t contrast : 4;  /* bits 6-3 -- (>>3 & 0xF) in flat form */
        uint8_t volume   : 2;  /* bits 2-1 -- (>>1 & 0x3) in flat form */
        uint8_t mute     : 1;  /* bit 0 -- 0x01 */
    } BIT;
} settings_byte_t;

/* pedTaskFlags: pedometer task dispatch flags (bits 0,1,2) */
typedef union {
    uint8_t BYTE;
    struct {
        uint8_t          : 5;  /* bits 7-3 unused */
        uint8_t rotate   : 1;  /* bit 2 -- 0x04 */
        uint8_t step     : 1;  /* bit 1 -- 0x02 */
        uint8_t init     : 1;  /* bit 0 -- 0x01 */
    } BIT;
} ped_task_flags_t;

/* buttonInputRaw: bits 1,2,3 are the three buttons (set by ISR). */
typedef union {
    uint8_t BYTE;
    struct {
        uint8_t        : 4;  /* bits 7-4 unused */
        uint8_t btn_l  : 1;  /* bit 3 -- 0x08 */
        uint8_t btn_m  : 1;  /* bit 2 -- 0x04 */
        uint8_t btn_r  : 1;  /* bit 1 -- 0x02 */
        uint8_t        : 1;  /* bit 0 unused */
    } BIT;
} button_input_t;

/* Generic LSB-named bit accessor: declared so b0..b2 map to byte bits 0..2.
 * Useful for casting RAM bytes that the ROM treats as a value AND as flags
 * (e.g. gCurSubstateY where bits 0/1 are bit-copied to/from other places). */
typedef union {
    uint8_t BYTE;
    struct {
        uint8_t b7 : 1;  /* bit 7 */
        uint8_t b6 : 1;
        uint8_t b5 : 1;
        uint8_t b4 : 1;
        uint8_t b3 : 1;
        uint8_t b2 : 1;
        uint8_t b1 : 1;
        uint8_t b0 : 1;  /* bit 0 */
    } BIT;
} byte_bits_t;

/* Per-tick handler installed in currentEventLoopFunc / passed to
 * sys_set_handler(). The main loop calls this from the foreground; both
 * sleep/active loops and the IR comm loop are installed via this pointer. */
typedef void (*event_loop_func_t)(void);

/* 0x68-byte trainer/peer-pokemon profile record. Persisted to EEPROM
 * 0x00ED (with redundant copy at 0x01ED) and exchanged via IR sync.
 * Field semantics are partial; *_at_NN names denote bytes whose role
 * isn't fully understood yet, kept named so they don't get accidentally
 * collapsed. flags_5b is a bit byte: b0/b1/b2 used as session/walking/
 * has-personalized markers (see save_flag_byte_t in src/game/session.c). */
struct trainer_record {
    uint32_t id;            /* +0x00 trainer/peer identity */
    uint32_t id_backup;     /* +0x04 backup; zeroed at session end, set from id at first peer init */
    uint16_t loc;           /* +0x08 location/route code */
    uint16_t loc_backup;    /* +0x0A backup; zeroed at session end */
    uint8_t  at_0c[4];      /* +0x0C..0x0F mixed access (uint32 / uint16 at +2 / byte at +1) */
    uint8_t  nickname[22];  /* +0x10..0x25 trainer nickname (utf-8 / sjis bytes) */
    uint8_t  marker_46;     /* +0x26 set to 0x46 by init_peer_identity */
    uint8_t  at_27;         /* +0x27 */
    uint8_t  at_28[16];     /* +0x28..0x37 pokemon move/data block; bit 0 of [0x37] read in social.c */
    uint8_t  flags_38[16];  /* +0x38..0x47 bit-array (gfx.c reads p[0x38+off] & (1<<bit)) */
    uint8_t  at_48[18];     /* +0x48..0x59 18-byte block written from DAT_f82e in session.c */
    uint8_t  at_5a;         /* +0x5A (unaccessed in current decomp; preserves 0x5B alignment) */
    volatile uint8_t  flags_5b;  /* +0x5B status flags (b0,b1,b2 used) */
    uint8_t  at_5c;         /* +0x5C set from DAT_f842 (battle context?) */
    uint8_t  at_5d;         /* +0x5D set from DAT_f843 */
    uint8_t  at_5e;         /* +0x5E zeroed in game_start_walk */
    uint8_t  at_5f;         /* +0x5F set to 2 in game_start_walk */
    uint8_t  at_60[8];      /* +0x60..0x67 trailing 8 bytes (purpose unknown) */
};

#endif /* TYPES_H */
