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

#endif /* TYPES_H */
