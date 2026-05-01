/************************************************************************/
/*  iodefine.h - H8/38606R memory-mapped IO register definitions         */
/*                                                                        */
/*  Modeled on the Renesas H8/38602 Series Include File (38602.H), but   */
/*  with the access scheme adapted to avoid C-preprocessor recursion     */
/*  between flat-byte macros and per-field bit access.                   */
/*                                                                        */
/*  Two access styles are provided per register that has bit fields:     */
/*    REG       - byte/word lvalue, e.g.  RTCCR1 |= 0x10;                */
/*    REG_BIT   - bit-field accessor,  e.g.  RTCCR1_BIT.RST = 1;          */
/*                                                                        */
/*  Both macros expand to a single cast-and-dereference at a fixed       */
/*  address, so neither nests further macro expansions and the two       */
/*  cannot collide on rescan.                                            */
/*                                                                        */
/*  Bit-field declarations are MSB-first (top-down = bit 7 .. bit 0),    */
/*  matching the H8 ch38 compiler default.                                */
/*                                                                        */
/*  Corrections vs. 38602.H for the 38606R variant (verified against the */
/*  pocketwalker emulator, which successfully runs the firmware):         */
/*    - RTCCR1: 3860x layout (RUN bit 7, HR24 bit 6, PM bit 5,           */
/*      RST bit 4, INT bit 3) -- 38602.H labels bit 6 'MD', no INT.      */
/*    - TMB1:   bit 6 named CNT (38602.H names it STR).                  */
/*    - PMRB:   ADTSTCHG at bit 4 (38602.H has it at bit 3).             */
/************************************************************************/
#ifndef IODEFINE_H
#define IODEFINE_H

/* ============================================================== *
 *  Bit-field union types                                          *
 * ============================================================== */

union un_flmcr1 {
    unsigned char BYTE;
    struct {
        unsigned char    :1;
        unsigned char SWE:1;
        unsigned char ESU:1;
        unsigned char PSU:1;
        unsigned char EV :1;
        unsigned char PV :1;
        unsigned char E  :1;
        unsigned char P  :1;
    } BIT;
};

union un_flmcr2 {
    unsigned char BYTE;
    struct { unsigned char FLER:1; } BIT;
};

union un_flpwcr {
    unsigned char BYTE;
    struct { unsigned char PDWND:1; } BIT;
};

union un_ebr1 {
    unsigned char BYTE;
    struct {
        unsigned char    :3;
        unsigned char EB4:1;
        unsigned char EB3:1;
        unsigned char EB2:1;
        unsigned char EB1:1;
        unsigned char EB0:1;
    } BIT;
};

union un_fenr {
    unsigned char BYTE;
    struct { unsigned char FLSHE:1; } BIT;
};

union un_rtcflg {
    unsigned char BYTE;
    struct {
        unsigned char FOIFG    :1;
        unsigned char WKIFG    :1;
        unsigned char DYIFG    :1;
        unsigned char HRIFG    :1;
        unsigned char MNIFG    :1;
        unsigned char _1SEIFG  :1;
        unsigned char _05SEIFG :1;
        unsigned char _025SEIFG:1;
    } BIT;
};

union un_rsecdr {
    unsigned char BYTE;
    struct { unsigned char BSY:1; unsigned char SC1:3; unsigned char SC0:4; } BIT;
};

union un_rmindr {
    unsigned char BYTE;
    struct { unsigned char BSY:1; unsigned char MN1:3; unsigned char MN0:4; } BIT;
};

union un_rhrdr {
    unsigned char BYTE;
    struct { unsigned char BSY:1; unsigned char :1; unsigned char HR1:2; unsigned char HR0:4; } BIT;
};

union un_rwkdr {
    unsigned char BYTE;
    struct { unsigned char BSY:1; unsigned char :4; unsigned char WK:3; } BIT;
};

union un_rtccr1 {                       /* 3860x layout */
    unsigned char BYTE;
    struct {
        unsigned char RUN :1;
        unsigned char HR24:1;
        unsigned char PM  :1;
        unsigned char RST :1;
        unsigned char INT :1;
        unsigned char     :3;
    } BIT;
};

union un_rtccr2 {
    unsigned char BYTE;
    struct {
        unsigned char FOIE    :1;
        unsigned char WKIE    :1;
        unsigned char DYIE    :1;
        unsigned char HRIE    :1;
        unsigned char MNIE    :1;
        unsigned char _1SEIE  :1;
        unsigned char _05SEIE :1;
        unsigned char _025SEIE:1;
    } BIT;
};

union un_rtccsr {
    unsigned char BYTE;
    struct { unsigned char :1; unsigned char CKSO:3; unsigned char CKSI:4; } BIT;
};

union un_iccr1 {
    unsigned char BYTE;
    struct {
        unsigned char ICE :1;
        unsigned char RCVD:1;
        unsigned char MST :1;
        unsigned char TRS :1;
        unsigned char CKS :4;
    } BIT;
};

union un_iccr2 {
    unsigned char BYTE;
    struct {
        unsigned char BBSY  :1;
        unsigned char SCP   :1;
        unsigned char SDAO  :1;
        unsigned char SDAOP :1;
        unsigned char SCLO  :1;
        unsigned char       :1;
        unsigned char IICRST:1;
    } BIT;
};

union un_icmr {
    unsigned char BYTE;
    struct {
        unsigned char MLS :1;
        unsigned char WAIT:1;
        unsigned char     :2;
        unsigned char BCWP:1;
        unsigned char BC  :3;
    } BIT;
};

union un_icier {
    unsigned char BYTE;
    struct {
        unsigned char TIE  :1;
        unsigned char TEIE :1;
        unsigned char RIE  :1;
        unsigned char NAKIE:1;
        unsigned char STIE :1;
        unsigned char ACKE :1;
        unsigned char ACKBR:1;
        unsigned char ACKBT:1;
    } BIT;
};

union un_icsr {
    unsigned char BYTE;
    struct {
        unsigned char TDRE :1;
        unsigned char TEND :1;
        unsigned char RDRF :1;
        unsigned char NACKF:1;
        unsigned char STOP :1;
        unsigned char ALOVE:1;
        unsigned char AAS  :1;
        unsigned char ADZ  :1;
    } BIT;
};

union un_sar {
    unsigned char BYTE;
    struct { unsigned char SVA:7; unsigned char FS:1; } BIT;
};

union un_pfcr {
    unsigned char BYTE;
    struct {
        unsigned char      :3;
        unsigned char SSUS :1;
        unsigned char IRQ1S:2;
        unsigned char IRQ0S:2;
    } BIT;
};

union un_tmb1 {                         /* 38606R: bit 6 = CNT */
    unsigned char BYTE;
    struct {
        unsigned char RLD:1;
        unsigned char CNT:1;
        unsigned char    :3;
        unsigned char CKS:3;
    } BIT;
};

union un_cmcr {                         /* CMCR0 / CMCR1 share layout */
    unsigned char BYTE;
    struct {
        unsigned char CME :1;
        unsigned char CMIE:1;
        unsigned char CMR :1;
        unsigned char CMLS:1;
        unsigned char CRS :4;
    } BIT;
};

union un_cmdr {
    unsigned char BYTE;
    struct {
        unsigned char     :2;
        unsigned char CMF1:1;
        unsigned char CMF0:1;
        unsigned char     :2;
        unsigned char CDR1:1;
        unsigned char CDR0:1;
    } BIT;
};

union un_sscrh {
    unsigned char BYTE;
    struct {
        unsigned char MSS :1;
        unsigned char BIDE:1;
        unsigned char SOOS:1;
        unsigned char SOL :1;
        unsigned char SOLP:1;
        unsigned char SCKS:1;
        unsigned char CSS :2;
    } BIT;
};

union un_sscrl {
    unsigned char BYTE;
    struct {
        unsigned char      :1;
        unsigned char SSUMS:1;
        unsigned char SRES :1;
        unsigned char SCKOS:1;
        unsigned char CSOS :1;
    } BIT;
};

union un_ssmr {
    unsigned char BYTE;
    struct {
        unsigned char MLS :1;
        unsigned char CPOS:1;
        unsigned char CPHS:1;
        unsigned char     :2;
        unsigned char CKS :3;
    } BIT;
};

union un_sser {
    unsigned char BYTE;
    struct {
        unsigned char TE   :1;
        unsigned char RE   :1;
        unsigned char RSSTP:1;
        unsigned char      :1;
        unsigned char TEIE :1;
        unsigned char TIE  :1;
        unsigned char RIE  :1;
        unsigned char CEIE :1;
    } BIT;
};

union un_sssr {
    unsigned char BYTE;
    struct {
        unsigned char     :1;
        unsigned char ORER:1;
        unsigned char     :2;
        unsigned char TEND:1;
        unsigned char TDRE:1;
        unsigned char RDRF:1;
        unsigned char CE  :1;
    } BIT;
};

union un_tmrw {
    unsigned char BYTE;
    struct {
        unsigned char CTS  :1;
        unsigned char      :1;
        unsigned char BUFEB:1;
        unsigned char BUFEA:1;
        unsigned char      :1;
        unsigned char PWMD :1;
        unsigned char PWMC :1;
        unsigned char PWMB :1;
    } BIT;
};

union un_tcrw {
    unsigned char BYTE;
    struct {
        unsigned char CCLR:1;
        unsigned char CKS :3;
        unsigned char TOD :1;
        unsigned char TOC :1;
        unsigned char TOB :1;
        unsigned char TOA :1;
    } BIT;
};

union un_tierw {
    unsigned char BYTE;
    struct {
        unsigned char OVIE :1;
        unsigned char      :3;
        unsigned char IMIED:1;
        unsigned char IMIEC:1;
        unsigned char IMIEB:1;
        unsigned char IMIEA:1;
    } BIT;
};

union un_tsrw {
    unsigned char BYTE;
    struct {
        unsigned char OVF :1;
        unsigned char     :3;
        unsigned char IMFD:1;
        unsigned char IMFC:1;
        unsigned char IMFB:1;
        unsigned char IMFA:1;
    } BIT;
};

union un_tior0 {
    unsigned char BYTE;
    struct { unsigned char :1; unsigned char IOB:3; unsigned char :1; unsigned char IOA:3; } BIT;
};

union un_tior1 {
    unsigned char BYTE;
    struct { unsigned char :1; unsigned char IOD:3; unsigned char :1; unsigned char IOC:3; } BIT;
};

union un_aegsr {
    unsigned char BYTE;
    struct {
        unsigned char AHEGS :2;
        unsigned char ALEGS :2;
        unsigned char AIEGS :2;
        unsigned char ECPWME:1;
    } BIT;
};

union un_eccr {
    unsigned char BYTE;
    struct { unsigned char ACKH:2; unsigned char ACKL:2; unsigned char PWCK:3; } BIT;
};

union un_eccsr {
    unsigned char BYTE;
    struct {
        unsigned char OVH :1;
        unsigned char OVL :1;
        unsigned char     :1;
        unsigned char CH2 :1;
        unsigned char CUEH:1;
        unsigned char CUEL:1;
        unsigned char CRCH:1;
        unsigned char CRCL:1;
    } BIT;
};

union un_spcr {
    unsigned char BYTE;
    struct {
        unsigned char       :3;
        unsigned char SPC3  :1;
        unsigned char       :2;
        unsigned char SCINV1:1;
        unsigned char SCINV0:1;
    } BIT;
};

union un_smr3 {
    unsigned char BYTE;
    struct {
        unsigned char COM :1;
        unsigned char CHR :1;
        unsigned char PE  :1;
        unsigned char PM  :1;
        unsigned char STOP:1;
        unsigned char MP  :1;
        unsigned char CKS :2;
    } BIT;
};

union un_scr3 {
    unsigned char BYTE;
    struct {
        unsigned char TIE :1;
        unsigned char RIE :1;
        unsigned char TE  :1;
        unsigned char RE  :1;
        unsigned char MPIE:1;
        unsigned char TEIE:1;
        unsigned char CKE :2;
    } BIT;
};

union un_ssr3 {
    unsigned char BYTE;
    struct {
        unsigned char TDRE:1;
        unsigned char RDRF:1;
        unsigned char OER :1;
        unsigned char FER :1;
        unsigned char PER :1;
        unsigned char TEND:1;
        unsigned char MPBR:1;
        unsigned char MPBT:1;
    } BIT;
};

union un_semr {
    unsigned char BYTE;
    struct { unsigned char :4; unsigned char ABCS:1; } BIT;
};

union un_ircr {
    unsigned char BYTE;
    struct { unsigned char IrE:1; unsigned char IrCKS:3; } BIT;
};

union un_tmwd {
    unsigned char BYTE;
    struct { unsigned char :4; unsigned char CKS:4; } BIT;
};

union un_tcsrwd1 {
    unsigned char BYTE;
    struct {
        unsigned char B6WI  :1;
        unsigned char TCWE  :1;
        unsigned char B4WI  :1;
        unsigned char TCSRWE:1;
        unsigned char B2WI  :1;
        unsigned char WDON  :1;
        unsigned char B0WI  :1;
        unsigned char WRST  :1;
    } BIT;
};

union un_tcsrwd2 {
    unsigned char BYTE;
    struct {
        unsigned char OVF  :1;
        unsigned char B5WI :1;
        unsigned char WTIT :1;
        unsigned char B3WI :1;
        unsigned char IEOVF:1;
    } BIT;
};

union un_amr {
    unsigned char BYTE;
    struct { unsigned char :1; unsigned char TRGE:1; unsigned char CKS:2; unsigned char CH:4; } BIT;
};

union un_adsr {
    unsigned char BYTE;
    struct { unsigned char ADSF:1; unsigned char LADS:1; } BIT;
};

union un_pmr1 {
    unsigned char BYTE;
    struct {
        unsigned char       :2;
        unsigned char IRQAEC:1;
        unsigned char FTC1  :1;
        unsigned char AEVL  :1;
        unsigned char CLKOUT:1;
        unsigned char TMOW  :1;
        unsigned char AEVH  :1;
    } BIT;
};

union un_pmr3 {
    unsigned char BYTE;
    struct { unsigned char :7; unsigned char VCref:1; } BIT;
};

union un_pmrb {                         /* 38606R: ADTSTCHG at bit 4 */
    unsigned char BYTE;
    struct {
        unsigned char         :3;
        unsigned char ADTSTCHG:1;
        unsigned char         :2;
        unsigned char IRQ1    :1;
        unsigned char IRQ0    :1;
    } BIT;
};

/* Generic port byte unions: bits named B0..B7 a la 38602.H.    */
union un_port_lo3 {                     /* PDR1, PDR3, PUCR1, PUCR3 */
    unsigned char BYTE;
    struct { unsigned char :5; unsigned char B2:1; unsigned char B1:1; unsigned char B0:1; } BIT;
};
union un_port8 {                        /* PUCR8, PDR8 */
    unsigned char BYTE;
    struct { unsigned char :3; unsigned char B4:1; unsigned char B3:1; unsigned char B2:1; } BIT;
};
union un_port9 {                        /* PUCR9, PODR9, PDR9 */
    unsigned char BYTE;
    struct {
        unsigned char   :4;
        unsigned char B3:1;
        unsigned char B2:1;
        unsigned char B1:1;
        unsigned char B0:1;
    } BIT;
};
union un_portb {                        /* PDRB */
    unsigned char BYTE;
    struct {
        unsigned char   :2;
        unsigned char B5:1;
        unsigned char B4:1;
        unsigned char B3:1;
        unsigned char B2:1;
        unsigned char B1:1;
        unsigned char B0:1;
    } BIT;
};

union un_syscr1 {
    unsigned char BYTE;
    struct { unsigned char SSBY:1; unsigned char STS:3; unsigned char LSON:1; unsigned char TMA3:1; unsigned char MA:2; } BIT;
};

union un_syscr2 {
    unsigned char BYTE;
    struct { unsigned char :3; unsigned char NESEL:1; unsigned char DTON:1; unsigned char MSON:1; unsigned char SA:2; } BIT;
};

union un_iegr {
    unsigned char BYTE;
    struct {
        unsigned char NMIEG   :1;
        unsigned char         :1;
        unsigned char ADTRGNEG:1;
        unsigned char         :3;
        unsigned char IEG1    :1;
        unsigned char IEG0    :1;
    } BIT;
};

union un_ienr1 {
    unsigned char BYTE;
    struct { unsigned char IENRTC:1; unsigned char :4; unsigned char IENEC2:1; unsigned char IEN1:1; unsigned char IEN0:1; } BIT;
};

union un_ienr2 {
    unsigned char BYTE;
    struct { unsigned char :1; unsigned char IENAD:1; unsigned char :3; unsigned char IENTB1:1; unsigned char :1; unsigned char IENEC:1; } BIT;
};

union un_osccr {
    unsigned char BYTE;
    struct { unsigned char SUBSTP:1; unsigned char RFCUT:1; unsigned char SUBSEL:1; unsigned char :3; unsigned char OSCF:1; } BIT;
};

union un_irr1 {
    unsigned char BYTE;
    struct { unsigned char :5; unsigned char IRREC2:1; unsigned char IRRI1:1; unsigned char IRRI0:1; } BIT;
};

union un_irr2 {
    unsigned char BYTE;
    struct { unsigned char :1; unsigned char IRRAD:1; unsigned char :3; unsigned char IRRTB1:1; unsigned char :1; unsigned char IRREC:1; } BIT;
};

union un_ckstpr1 {
    unsigned char BYTE;
    struct {
        unsigned char          :1;
        unsigned char S3CKSTP  :1;
        unsigned char          :1;
        unsigned char ADCKSTP  :1;
        unsigned char          :1;
        unsigned char TB1CKSTP :1;
        unsigned char FROMCKSTP:1;
        unsigned char RTCCKSTP :1;
    } BIT;
};

union un_ckstpr2 {
    unsigned char BYTE;
    struct {
        unsigned char          :1;
        unsigned char TWCKSTP  :1;
        unsigned char IICCKSTP :1;
        unsigned char SSUCKSTP :1;
        unsigned char AECCKSTP :1;
        unsigned char WDCKSTP  :1;
        unsigned char COMPCKSTP:1;
    } BIT;
};

/* ============================================================== *
 *  Access macros: REG = byte/word lvalue,  REG_BIT = bit struct   *
 *                                                                 *
 *  Each macro is a single cast-and-deref at the absolute address, *
 *  so neither expansion contains further macro references --      *
 *  preprocessor rescan can never recurse.                         *
 * ============================================================== */

/* Helpers (private) */
#define _U8AT(addr)        (*(volatile unsigned char *)(addr))
#define _U16AT(addr)       (*(volatile unsigned int  *)(addr))
#define _UAT(t,addr)       (*(volatile union t *)(addr))

/* ---- Flash (0xF020) ---- */
#define FLMCR1     _UAT(un_flmcr1, 0xF020).BYTE
#define FLMCR1_BIT _UAT(un_flmcr1, 0xF020).BIT
#define FLMCR2     _UAT(un_flmcr2, 0xF021).BYTE
#define FLMCR2_BIT _UAT(un_flmcr2, 0xF021).BIT
#define FLPWCR     _UAT(un_flpwcr, 0xF022).BYTE
#define FLPWCR_BIT _UAT(un_flpwcr, 0xF022).BIT
#define EBR1       _UAT(un_ebr1,   0xF023).BYTE
#define EBR1_BIT   _UAT(un_ebr1,   0xF023).BIT
#define FENR       _UAT(un_fenr,   0xF02B).BYTE
#define FENR_BIT   _UAT(un_fenr,   0xF02B).BIT

/* ---- RTC (0xF067) ---- */
#define RTCFLG     _UAT(un_rtcflg, 0xF067).BYTE
#define RTCFLG_BIT _UAT(un_rtcflg, 0xF067).BIT
#define RSECDR     _UAT(un_rsecdr, 0xF068).BYTE
#define RSECDR_BIT _UAT(un_rsecdr, 0xF068).BIT
#define RMINDR     _UAT(un_rmindr, 0xF069).BYTE
#define RMINDR_BIT _UAT(un_rmindr, 0xF069).BIT
#define RHRDR      _UAT(un_rhrdr,  0xF06A).BYTE
#define RHRDR_BIT  _UAT(un_rhrdr,  0xF06A).BIT
#define RWKDR      _UAT(un_rwkdr,  0xF06B).BYTE
#define RWKDR_BIT  _UAT(un_rwkdr,  0xF06B).BIT
#define RTCCR1     _UAT(un_rtccr1, 0xF06C).BYTE
#define RTCCR1_BIT _UAT(un_rtccr1, 0xF06C).BIT
#define RTCCR2     _UAT(un_rtccr2, 0xF06D).BYTE
#define RTCCR2_BIT _UAT(un_rtccr2, 0xF06D).BIT
#define RTCCSR     _UAT(un_rtccsr, 0xF06F).BYTE
#define RTCCSR_BIT _UAT(un_rtccsr, 0xF06F).BIT

/* ---- IIC2 (0xF078) ---- */
#define ICCR1      _UAT(un_iccr1, 0xF078).BYTE
#define ICCR1_BIT  _UAT(un_iccr1, 0xF078).BIT
#define ICCR2      _UAT(un_iccr2, 0xF079).BYTE
#define ICCR2_BIT  _UAT(un_iccr2, 0xF079).BIT
#define ICMR       _UAT(un_icmr,  0xF07A).BYTE
#define ICMR_BIT   _UAT(un_icmr,  0xF07A).BIT
#define ICIER      _UAT(un_icier, 0xF07B).BYTE
#define ICIER_BIT  _UAT(un_icier, 0xF07B).BIT
#define ICSR       _UAT(un_icsr,  0xF07C).BYTE
#define ICSR_BIT   _UAT(un_icsr,  0xF07C).BIT
#define SAR        _UAT(un_sar,   0xF07D).BYTE
#define SAR_BIT    _UAT(un_sar,   0xF07D).BIT
#define ICDRT      _U8AT(0xF07E)
#define ICDRR      _U8AT(0xF07F)

/* ---- Pin function (0xF085) ---- */
#define PFCR       _UAT(un_pfcr, 0xF085).BYTE
#define PFCR_BIT   _UAT(un_pfcr, 0xF085).BIT

/* ---- IO ports (0xF086 + 0xFFC0..) ---- */
#define PUCR8      _UAT(un_port8,    0xF086).BYTE
#define PUCR8_BIT  _UAT(un_port8,    0xF086).BIT
#define PUCR9      _UAT(un_port9,    0xF087).BYTE
#define PUCR9_BIT  _UAT(un_port9,    0xF087).BIT
#define PODR9      _UAT(un_port9,    0xF08C).BYTE
#define PODR9_BIT  _UAT(un_port9,    0xF08C).BIT

#define PMR1       _UAT(un_pmr1,     0xFFC0).BYTE
#define PMR1_BIT   _UAT(un_pmr1,     0xFFC0).BIT
#define PMR3       _UAT(un_pmr3,     0xFFC2).BYTE
#define PMR3_BIT   _UAT(un_pmr3,     0xFFC2).BIT
#define PMRB       _UAT(un_pmrb,     0xFFCA).BYTE
#define PMRB_BIT   _UAT(un_pmrb,     0xFFCA).BIT

#define PDR1       _UAT(un_port_lo3, 0xFFD4).BYTE
#define PDR1_BIT   _UAT(un_port_lo3, 0xFFD4).BIT
#define PDR3       _UAT(un_port_lo3, 0xFFD6).BYTE
#define PDR3_BIT   _UAT(un_port_lo3, 0xFFD6).BIT
#define PDR8       _UAT(un_port8,    0xFFDB).BYTE
#define PDR8_BIT   _UAT(un_port8,    0xFFDB).BIT
#define PDR9       _UAT(un_port9,    0xFFDC).BYTE
#define PDR9_BIT   _UAT(un_port9,    0xFFDC).BIT
#define PDRB       _UAT(un_portb,    0xFFDE).BYTE
#define PDRB_BIT   _UAT(un_portb,    0xFFDE).BIT

#define PUCR1      _UAT(un_port_lo3, 0xFFE0).BYTE
#define PUCR1_BIT  _UAT(un_port_lo3, 0xFFE0).BIT
#define PUCR3      _UAT(un_port_lo3, 0xFFE1).BYTE
#define PUCR3_BIT  _UAT(un_port_lo3, 0xFFE1).BIT
#define PCR1       _U8AT(0xFFE4)
#define PCR3       _U8AT(0xFFE6)
#define PCR8       _U8AT(0xFFEB)
#define PCR9       _U8AT(0xFFEC)

/* ---- Timer B1 (0xF0D0) ---- */
#define TMB1       _UAT(un_tmb1, 0xF0D0).BYTE
#define TMB1_BIT   _UAT(un_tmb1, 0xF0D0).BIT
#define TCB1_TLB1  _U8AT(0xF0D1)

/* ---- Comparator (0xF0DC) ---- */
#define CMCR0      _UAT(un_cmcr, 0xF0DC).BYTE
#define CMCR0_BIT  _UAT(un_cmcr, 0xF0DC).BIT
#define CMCR1      _UAT(un_cmcr, 0xF0DD).BYTE
#define CMCR1_BIT  _UAT(un_cmcr, 0xF0DD).BIT
#define CMDR       _UAT(un_cmdr, 0xF0DE).BYTE
#define CMDR_BIT   _UAT(un_cmdr, 0xF0DE).BIT

/* ---- SSU (0xF0E0) ---- */
#define SSCRH      _UAT(un_sscrh, 0xF0E0).BYTE
#define SSCRH_BIT  _UAT(un_sscrh, 0xF0E0).BIT
#define SSCRL      _UAT(un_sscrl, 0xF0E1).BYTE
#define SSCRL_BIT  _UAT(un_sscrl, 0xF0E1).BIT
#define SSMR       _UAT(un_ssmr,  0xF0E2).BYTE
#define SSMR_BIT   _UAT(un_ssmr,  0xF0E2).BIT
#define SSER       _UAT(un_sser,  0xF0E3).BYTE
#define SSER_BIT   _UAT(un_sser,  0xF0E3).BIT
#define SSSR       _UAT(un_sssr,  0xF0E4).BYTE
#define SSSR_BIT   _UAT(un_sssr,  0xF0E4).BIT
#define SSRDR      _U8AT(0xF0E9)
#define SSTDR      _U8AT(0xF0EB)

/* ---- Timer W (0xF0F0) ---- */
#define TMRW       _UAT(un_tmrw,  0xF0F0).BYTE
#define TMRW_BIT   _UAT(un_tmrw,  0xF0F0).BIT
#define TCRW       _UAT(un_tcrw,  0xF0F1).BYTE
#define TCRW_BIT   _UAT(un_tcrw,  0xF0F1).BIT
#define TIERW      _UAT(un_tierw, 0xF0F2).BYTE
#define TIERW_BIT  _UAT(un_tierw, 0xF0F2).BIT
#define TSRW       _UAT(un_tsrw,  0xF0F3).BYTE
#define TSRW_BIT   _UAT(un_tsrw,  0xF0F3).BIT
#define TIOR0      _UAT(un_tior0, 0xF0F4).BYTE
#define TIOR0_BIT  _UAT(un_tior0, 0xF0F4).BIT
#define TIOR1      _UAT(un_tior1, 0xF0F5).BYTE
#define TIOR1_BIT  _UAT(un_tior1, 0xF0F5).BIT
#define TCNT       _U16AT(0xF0F6)
#define GRA        _U16AT(0xF0F8)
#define GRB        _U16AT(0xF0FA)
#define GRC        _U16AT(0xF0FC)
#define GRD        _U16AT(0xF0FE)

/* ---- AEC (0xFF8C) ---- */
#define ECPWCR     _U16AT(0xFF8C)
#define ECPWDR     _U16AT(0xFF8E)
#define AEGSR      _UAT(un_aegsr, 0xFF93).BYTE
#define AEGSR_BIT  _UAT(un_aegsr, 0xFF93).BIT
#define ECCR       _UAT(un_eccr,  0xFF95).BYTE
#define ECCR_BIT   _UAT(un_eccr,  0xFF95).BIT
#define ECCSR      _UAT(un_eccsr, 0xFF96).BYTE
#define ECCSR_BIT  _UAT(un_eccsr, 0xFF96).BIT
#define EC         _U16AT(0xFF97)

/* ---- SCI3 (0xFF91) ---- */
#define SPCR       _UAT(un_spcr, 0xFF91).BYTE
#define SPCR_BIT   _UAT(un_spcr, 0xFF91).BIT
#define SMR3       _UAT(un_smr3, 0xFF98).BYTE
#define SMR3_BIT   _UAT(un_smr3, 0xFF98).BIT
#define BRR3       _U8AT(0xFF99)
#define SCR3       _UAT(un_scr3, 0xFF9A).BYTE
#define SCR3_BIT   _UAT(un_scr3, 0xFF9A).BIT
#define TDR3       _U8AT(0xFF9B)
#define SSR3       _UAT(un_ssr3, 0xFF9C).BYTE
#define SSR3_BIT   _UAT(un_ssr3, 0xFF9C).BIT
#define RDR3       _U8AT(0xFF9D)
#define SEMR       _UAT(un_semr, 0xFFA6).BYTE
#define SEMR_BIT   _UAT(un_semr, 0xFFA6).BIT
#define IrCR       _UAT(un_ircr, 0xFFA7).BYTE
#define IrCR_BIT   _UAT(un_ircr, 0xFFA7).BIT

/* ---- WDT (0xFFB0) ---- */
#define TMWD       _UAT(un_tmwd,    0xFFB0).BYTE
#define TMWD_BIT   _UAT(un_tmwd,    0xFFB0).BIT
#define TCSRWD1    _UAT(un_tcsrwd1, 0xFFB1).BYTE
#define TCSRWD1_BIT _UAT(un_tcsrwd1,0xFFB1).BIT
#define TCSRWD2    _UAT(un_tcsrwd2, 0xFFB2).BYTE
#define TCSRWD2_BIT _UAT(un_tcsrwd2,0xFFB2).BIT
#define TCWD       _U8AT(0xFFB3)

/* ---- A/D (0xFFBC) ---- */
#define ADRR       _U16AT(0xFFBC)
#define AMR        _UAT(un_amr,  0xFFBE).BYTE
#define AMR_BIT    _UAT(un_amr,  0xFFBE).BIT
#define ADSR       _UAT(un_adsr, 0xFFBF).BYTE
#define ADSR_BIT   _UAT(un_adsr, 0xFFBF).BIT

/* ---- System / interrupt control ---- */
#define SYSCR1     _UAT(un_syscr1,  0xFFF0).BYTE
#define SYSCR1_BIT _UAT(un_syscr1,  0xFFF0).BIT
#define SYSCR2     _UAT(un_syscr2,  0xFFF1).BYTE
#define SYSCR2_BIT _UAT(un_syscr2,  0xFFF1).BIT
#define IEGR       _UAT(un_iegr,    0xFFF2).BYTE
#define IEGR_BIT   _UAT(un_iegr,    0xFFF2).BIT
#define IENR1      _UAT(un_ienr1,   0xFFF3).BYTE
#define IENR1_BIT  _UAT(un_ienr1,   0xFFF3).BIT
#define IENR2      _UAT(un_ienr2,   0xFFF4).BYTE
#define IENR2_BIT  _UAT(un_ienr2,   0xFFF4).BIT
#define OSCCR      _UAT(un_osccr,   0xFFF5).BYTE
#define OSCCR_BIT  _UAT(un_osccr,   0xFFF5).BIT
#define IRR1       _UAT(un_irr1,    0xFFF6).BYTE
#define IRR1_BIT   _UAT(un_irr1,    0xFFF6).BIT
#define IRR2       _UAT(un_irr2,    0xFFF7).BYTE
#define IRR2_BIT   _UAT(un_irr2,    0xFFF7).BIT
#define CKSTPR1    _UAT(un_ckstpr1, 0xFFFA).BYTE
#define CKSTPR1_BIT _UAT(un_ckstpr1,0xFFFA).BIT
#define CKSTPR2    _UAT(un_ckstpr2, 0xFFFB).BYTE
#define CKSTPR2_BIT _UAT(un_ckstpr2,0xFFFB).BIT

#endif /* IODEFINE_H */
