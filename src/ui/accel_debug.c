#include "all_headers.h"

/*
 * Accel debug view (VIEW_ACCEL_DEBUG).
 *
 * Entered from the final stage of the factory test (or directly via IR
 * command 0xFE). Displays live axis thresholds + step counters as hex,
 * and emits SPI command 0xA7 when g.DAT_f7d1 matches g.DAT_f7d8_1 (acceptance
 * gesture from the technician).
 *
 * The view has no per-tick input — sys_noop is its event handler; rendering
 * does all the work.
 */

// ROM: 0xaebc  66.1%
void sys_init_accel_debug(void) {
  g.ped_sampleCount = 0;
  g.gCurSubstateY = 0x10;
  g.gCurSubstateA = 0;
  g.DAT_f7d1 = 0;
  g.accelXPos = 0;
  g.accelYPos = 0;
  g.accelZPos = 0;
  drv_eeprom_read_block(8, (void *)&g.DAT_f7d8, 8);
  walker_status_flags_BIT.session_active = 1;
}

// ROM: 0xaef8  100.0%
void sys_noop(void) {}

// Reason: ROM uses `$sp_regsv$3` prologue + `subs #6, r7`; ch38 emits a 12-byte
//   subs after the helper. Body structure (gfx_draw_string hoisted to r4,
//   g.ped_activityTimer/g.ped_stepTimer resets, digit-to-ASCII conversions via add #0x30,
//   buf writes via @r6) matches. ch38 stores str-buffer locals differently
//   from ROM (different sp offsets) so every `@(N, sp)` access diverges.
// Class: cannot-fix-without-compiler-change (sp_regsv$3 helper + stack
//   local layout)
// ROM: 0xaefa  42.6%
#pragma option noregexpansion /* pragma:auto */
void ui_render_accel_debug(void) {
  uint8_t buf[6];
  void (*draw_string)(uint8_t, uint8_t, const char *);
  uint8_t *hexTable;
  uint8_t *p;
  uint8_t *q;

  draw_string = gfx_draw_string;
  g.ped_activityTimer = 0x3C;
  g.ped_stepTimer = 0x1E;

  /* Use buf[0..1] for 2-char strings via stack pointer */
  p = buf;
  q = p + 1;

  /* Draw g.gCurSubstateA as ASCII digit at position 0x4C */
  *p = (uint8_t)(g.gCurSubstateA + 0x30);
  *q = 0;
  draw_string(0x4C, 0x00, (const char *)p);

  /* Draw g.DAT_f7d8 as ASCII digit at position 0x54 */
  *p = (uint8_t)(g.DAT_f7d8 + 0x30);
  *q = 0;
  draw_string(0x54, 0x00, (const char *)p);

  hexTable = (uint8_t *)HEX_DIGITS;

  /* Draw hex digits of g.ped_axisStepThresholdLo at 0x820 */
  {
    uint16_t val = g.ped_axisStepThresholdLo;
    uint16_t tmp;
    uint8_t *d;
    d = p;
    tmp = val / 0x1000;
    *d = hexTable[tmp & 0xF];
    d = q;
    tmp = val;
    *d = hexTable[tmp & 0xF];

    d = (p + 2);
    tmp = (val >> 4);
    *d = hexTable[tmp & 0xF];
    d = (p + 3);
    tmp = (val >> 4);
    *d = hexTable[tmp & 0xF];

    /* Reset and recompute properly */
    *p = hexTable[(val >> 12) & 0xF];
    *q = hexTable[(val >> 8) & 0xF];
    *(p + 2) = hexTable[(val >> 4) & 0xF];
    *(p + 3) = hexTable[val & 0xF];
    *(p + 4) = 0;
    draw_string(0x20, 0x08, (const char *)p);
  }

  /* Draw hex digits of g.ped_axisStepThresholdHi at 0x840 */
  {
    uint16_t val = g.ped_axisStepThresholdHi;
    *p = hexTable[(val >> 12) & 0xF];
    *q = hexTable[(val >> 8) & 0xF];
    *(p + 2) = hexTable[(val >> 4) & 0xF];
    *(p + 3) = hexTable[val & 0xF];
    *(p + 4) = 0;
    draw_string(0x40, 0x08, (const char *)p);
  }

  /* Draw hex digits of g.ped_axisIdleThreshold at 0x1020 */
  {
    uint16_t val = g.ped_axisIdleThreshold;
    *p = hexTable[(val >> 12) & 0xF];
    *q = hexTable[(val >> 8) & 0xF];
    *(p + 2) = hexTable[(val >> 4) & 0xF];
    *(p + 3) = hexTable[val & 0xF];
    *(p + 4) = 0;
    draw_string(0x20, 0x10, (const char *)p);
  }

  /* Draw g.DAT_f7d1 as ASCII digit at 0x104C */
  *p = (uint8_t)(g.DAT_f7d1 + 0x30);
  *q = 0;
  draw_string(0x4C, 0x10, (const char *)p);

  /* Draw g.DAT_f7d8_1 (the byte after g.DAT_f7d8) at 0x1054 */
  *p = g.DAT_f7d8_1;
  draw_string(0x54, 0x10, (const char *)p);

  /* If g.DAT_f7d1 == g.DAT_f7d8_1, send SPI command 0xA7 and draw check mark */
  if (g.DAT_f7d1 == g.DAT_f7d8_1) {
    PDR1 &= ~0x01;
    PDR1 &= ~0x02;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xA7;
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x01;
    draw_string(0x08, 0x20, FACTORY_STR_OK);
  }
}
