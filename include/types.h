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

/* DAT_f7a7: pedometer task dispatch flags (bits 0,1,2) */
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

#endif /* TYPES_H */
