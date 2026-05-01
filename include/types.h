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

/* statusFlags bit-field union.
 * ch38 allocates bit fields MSB-first, so the first declared field is bit 7.
 * Bit names are inferred from C call-site usage; rename freely. */
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

#endif /* TYPES_H */
