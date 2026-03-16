/*
 * iodefine.h - Memory-mapped IO register definitions for the H8/38606R.
 *
 * Derived from the H8/38602 Series Include File (38602.H) supplied with the
 * Renesas IDE, adjusted for use with the ch38 C compiler (C89 mode).
 * The 38606R is in the same 3860x family; peripheral register layouts are
 * identical except for memory/flash sizes.  Addresses were verified against
 * the assembly listing in main.mar.
 *
 * Usage: #include "iodefine.h"
 * Each macro expands to a dereferenced volatile pointer so that:
 *   IENR2 = 0x04;      -- byte write
 *   x = IENR2;         -- byte read
 *
 * Where bit-field access is needed, use the struct versions, e.g.:
 *   IENR2_st.BIT.IENTB1 = 1;
 * But prefer direct byte masks for maximum code-generation compatibility.
 */

#ifndef IODEFINE_H
#define IODEFINE_H

/* -----------------------------------------------------------------------
 * Helper macro: volatile byte at an absolute address.
 * ---------------------------------------------------------------------- */
#define _IOR(addr) (*(volatile unsigned char *)(addr))
#define _IOW(addr) (*(volatile unsigned int *)(addr))

/* -----------------------------------------------------------------------
 * Flash control registers  (base 0xF020)
 * ---------------------------------------------------------------------- */
#define FLMCR1 _IOR(0xF020)
#define FLMCR2 _IOR(0xF021)
#define FLPWCR _IOR(0xF022)
#define EBR1 _IOR(0xF023)
#define FENR _IOR(0xF02B)

/* -----------------------------------------------------------------------
 * RTC registers  (base 0xF067)
 * ---------------------------------------------------------------------- */
#define RTCFLG _IOR(0xF067)
#define RSECDR _IOR(0xF068)
#define RMINDR _IOR(0xF069)
#define RHRDR _IOR(0xF06A)
#define RWKDR _IOR(0xF06B)
#define RTCCR1 _IOR(0xF06C)
#define RTCCR2 _IOR(0xF06D)
#define RTCCSR _IOR(0xF06F)

/* -----------------------------------------------------------------------
 * IIC2 registers  (base 0xF078)
 * ---------------------------------------------------------------------- */
#define ICCR1 _IOR(0xF078)
#define ICCR2 _IOR(0xF079)
#define ICMR _IOR(0xF07A)
#define ICIER _IOR(0xF07B)
#define ICSR _IOR(0xF07C)
#define SAR _IOR(0xF07D)
#define ICDRT _IOR(0xF07E)
#define ICDRR _IOR(0xF07F)

/* -----------------------------------------------------------------------
 * Pin function control register  (0xF085)
 * ---------------------------------------------------------------------- */
#define PFCR _IOR(0xF085)

/* -----------------------------------------------------------------------
 * IO port registers  (base 0xF086)
 * ---------------------------------------------------------------------- */
#define PUCR8 _IOR(0xF086)
#define PUCR9 _IOR(0xF087)
#define PODR9 _IOR(0xF08C)

/* Port mode registers */
#define PMR1 _IOR(0xFFC0)
#define PMR3 _IOR(0xFFC2)
#define PMRB _IOR(0xFFCA)

/* Port data registers */
#define PDR1 _IOR(0xFFD4)
#define PDR3 _IOR(0xFFD6)
#define PDR8 _IOR(0xFFDB)
#define PDR9 _IOR(0xFFDC)
#define PDRB _IOR(0xFFDE)

/* Port pull-up control registers */
#define PUCR1 _IOR(0xFFE0)
#define PUCR3 _IOR(0xFFE1)

/* Port control registers (direction) */
#define PCR1 _IOR(0xFFE4)
#define PCR3 _IOR(0xFFE6)
#define PCR8 _IOR(0xFFEB)
#define PCR9 _IOR(0xFFEC)

/* -----------------------------------------------------------------------
 * Timer B1  (base 0xF0D0)
 * ---------------------------------------------------------------------- */
#define TMB1 _IOR(0xF0D0)      /* Timer mode register B1      */
#define TCB1_TLB1 _IOR(0xF0D1) /* Timer count register B1     */

/* -----------------------------------------------------------------------
 * Comparator  (base 0xF0DC)
 * ---------------------------------------------------------------------- */
#define CMCR0 _IOR(0xF0DC)
#define CMCR1 _IOR(0xF0DD)
#define CMDR _IOR(0xF0DE)

/* -----------------------------------------------------------------------
 * SSU (Synchronous Serial Unit / SPI)  (base 0xF0E0)
 * ---------------------------------------------------------------------- */
#define SSCRH _IOR(0xF0E0)
#define SSCRL _IOR(0xF0E1)
#define SSMR _IOR(0xF0E2)
#define SSER _IOR(0xF0E3)
#define SSSR _IOR(0xF0E4)
#define SSRDR _IOR(0xF0E9)
#define SSTDR _IOR(0xF0EB)

/* -----------------------------------------------------------------------
 * Timer W  (base 0xF0F0)
 * ---------------------------------------------------------------------- */
#define TMRW _IOR(0xF0F0)
#define TCRW _IOR(0xF0F1)
#define TIERW _IOR(0xF0F2)
#define TSRW _IOR(0xF0F3)
#define TIOR0 _IOR(0xF0F4)
#define TIOR1 _IOR(0xF0F5)
#define TCNT _IOW(0xF0F6)
#define GRA _IOW(0xF0F8)
#define GRB _IOW(0xF0FA)
#define GRC _IOW(0xF0FC)
#define GRD _IOW(0xF0FE)

/* -----------------------------------------------------------------------
 * AEC  (base 0xFF8C)
 * ---------------------------------------------------------------------- */
#define ECPWCR _IOW(0xFF8C)
#define ECPWDR _IOW(0xFF8E)
#define AEGSR _IOR(0xFF93)
#define ECCR _IOR(0xFF95)
#define ECCSR _IOR(0xFF96)
#define EC _IOW(0xFF97)

/* -----------------------------------------------------------------------
 * SCI3 (IR serial interface)  (base 0xFF91)
 * ---------------------------------------------------------------------- */
#define SPCR _IOR(0xFF91)
#define SMR3 _IOR(0xFF98)
#define BRR3 _IOR(0xFF99)
#define SCR3 _IOR(0xFF9A)
#define TDR3 _IOR(0xFF9B)
#define SSR3 _IOR(0xFF9C)
#define RDR3 _IOR(0xFF9D)
#define SEMR _IOR(0xFFA6)
#define IrCR _IOR(0xFFA7)

/* -----------------------------------------------------------------------
 * WDT  (base 0xFFB0)
 * ---------------------------------------------------------------------- */
#define TMWD _IOR(0xFFB0)
#define TCSRWD1 _IOR(0xFFB1)
#define TCSRWD2 _IOR(0xFFB2)
#define TCWD _IOR(0xFFB3)

/* -----------------------------------------------------------------------
 * A/D converter  (base 0xFFBC)
 * ---------------------------------------------------------------------- */
#define ADRR _IOW(0xFFBC)
#define AMR _IOR(0xFFBE)
#define ADSR _IOR(0xFFBF)

/* -----------------------------------------------------------------------
 * System control / interrupt registers  (fixed addresses)
 * ---------------------------------------------------------------------- */
#define SYSCR1 _IOR(0xFFF0)
#define SYSCR2 _IOR(0xFFF1)
#define IEGR _IOR(0xFFF2)
#define IENR1 _IOR(0xFFF3)
#define IENR2 _IOR(0xFFF4)
#define OSCCR _IOR(0xFFF5)
#define IRR1 _IOR(0xFFF6)
#define IRR2 _IOR(0xFFF7)
#define CKSTPR1 _IOR(0xFFFA)
#define CKSTPR2 _IOR(0xFFFB)

/* -----------------------------------------------------------------------
 * Bit manipulation helpers (matching H8 bset/bclr/bld semantics)
 * ---------------------------------------------------------------------- */
#define BIT(n) (1 << (n))

#endif /* IODEFINE_H */
