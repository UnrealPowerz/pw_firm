#include "all_headers.h"

/*
 * Large-walker-portrait renderers and IR/result overlays.
 *
 * These don't belong to one specific view — they're shared between several:
 *
 *   ui_render_empty_eeprom  - boot screen shown when no save data exists.
 *   ui_render_sad_walker    - VIEW_STEP_HISTORY when no session was active;
 *                             also auto-exits to home after 8 ticks.
 *   ui_render_happy_walker  - called from system/main.c during IR app /
 *                             arrival flows; takes a show-IR-icon flag.
 *   ui_render_connecting_screen         - "Connecting..." overlay used by the IR app.
 *   ui_render_step_history  - VIEW_STEP_HISTORY renderer for the post-IR-
 *                             exchange result screen. Always draws the
 *                             step-history background graphic; overlays an
 *                             error text-box based on g.irResultCode (set by
 *                             ir_protocol.c). See per-function comment.
 */

// ROM: 0x703c  70.4%
void ui_render_empty_eeprom(void) {
  uint8_t *buf;
  uint16_t i;

  sys_init_heap();
  buf = sbrk(0x100);

  for (i = 0; i < 0x100; i++) {
    buf[i] = IMG_POKEWALKER_LARGE[i];
  }

  for (i = 0; i < 0x20; i++) {
    uint8_t pix;
    pix = IMG_FACE_NEUTRAL[i];
    buf[0x50 + i] |= (pix * 8);
    buf[0x50 + i + 0x40] |= (pix / 0x20);
  }

  {
    uint8_t frame;
    frame = (g.animTick >> 2) & 1;
    if (frame) {
      for (i = 0; i < 0x10; i++) {
        uint8_t pix;
        pix = IMG_EXTRA_GLYPH_EMPTY[i];
        buf[0xD8 + i] |= (pix * 0x10);
      }
      drv_lcd_blit(0x20, 0x10, buf, 0x20, 0x20);

      for (i = 0; i < 0x10; i++) {
        buf[i] = IMG_EXTRA_GLYPH_EMPTY[i] / 0x10;
      }
      drv_lcd_blit(0x2C, 0x30, buf, 8, 8);
    } else {
      drv_lcd_blit(0x20, 0x10, buf, 0x20, 0x20);
    }
  }
}

// ROM: 0x711a  24.0%
void ui_render_sad_walker(void) {
  uint8_t *buf;
  uint8_t *dst;
  uint16_t i;

  sys_init_heap();
  buf = sbrk(0x100);

  for (i = 0; i < 0x100; i++) {
    buf[i] = IMG_POKEWALKER_LARGE[i];
  }

  dst = buf + 0x50;
  for (i = 0; i < 0x20; i++) {
    uint8_t pix;
    pix = IMG_FACE_SAD[i];
    dst[i] |= (pix * 8);
    dst[i + 0x40] |= (pix / 0x20);
  }

  drv_lcd_blit(0x20, 0x10, buf, 0x20, 0x20);

  {
    uint8_t tmp = g.gCurSubstateZ + 1;
    g.gCurSubstateZ = tmp;
    if (tmp > 0x08) {
      ui_reset_substate();
      ui_set_view(VIEW_HOME);
    }
  }
}

// ROM: 0x71a4  72.2%
void ui_render_happy_walker(uint8_t show_ir) {
  uint8_t *buf;
  uint8_t *dst;
  uint16_t i;

  sys_init_heap();
  buf = sbrk(0x100);

  if (show_ir) {
    drv_lcd_blit(0x2C, 0, (void *)IMG_POKEWALKER_IR_ACTIVE, 8, 8);
  }

  for (i = 0; i < 0x100; i++) {
    buf[i] = IMG_POKEWALKER_LARGE[i];
  }

  dst = buf + 0x50;
  for (i = 0; i < 0x20; i++) {
    uint8_t pix;
    pix = IMG_FACE_HAPPY[i];
    dst[i] |= (pix * 8);
    dst[i + 0x40] |= (pix / 0x20);
  }

  drv_lcd_blit(0x20, 0x10, buf, 0x20, 0x20);
}

// ROM: 0x722c  75.4%
void ui_render_connecting_screen(uint8_t show_ir) {
  uint8_t *buf;

  sys_init_heap();
  buf = sbrk(0x180);

  if (show_ir) {
    drv_eeprom_read_block(0x2450, buf, 0x20);
    drv_lcd_blit(0x2C, 0, buf, 8, 0x10);
  }

  drv_eeprom_read_block(0x2350, buf, 0x100);
  drv_lcd_blit(0x20, 0x10, buf, 0x20, 0x20);
  gfx_draw_text_box(0x30, TEXT_CONNECTING, TEXT_BOX_FULL, TEXT_BOX_STATIC);
}

/*
 * Renders VIEW_STEP_HISTORY — the screen shown after a peer/IR exchange
 * completes (success or failure). The "step-history graph" background art
 * at EEPROM 0x2350 is always drawn; on top of that, an error text-box is
 * overlaid based on g.irResultCode (set by ir_protocol.c):
 *
 *   g.irResultCode  day  text
 *   1             0    "No trainer found"
 *   2             1    "Cannot connect"
 *   3             2    "Cannot complete this connection"
 *   4             3    "No Pokemon held!"
 *   5             4    "Cannot connect to trainer again"
 *   6             5    "Already received this event"
 *   7             6    "Could not receive..."
 *   8+            -    no overlay (background + battery only)
 *
 * The literal 3 / 0x0A / 8 / 0x0C in the 96x32-text branches are the
 * continuation slots of the preceding 96x32 string (see gfx_consts.h).
 */
// ROM: 0x728a  85.4%
void ui_render_step_history(void) {
  uint8_t *buf;
  uint8_t day;

  sys_init_heap();
  buf = sbrk(0x180);

  drv_eeprom_read_block(0x2350, buf, 0x100);
  drv_lcd_blit(0x20, 0x10, buf, 0x20, 0x20);

  day = g.irResultCode - 1;
  if (day > 7) {
    gfx_draw_battery_low(0x58, 0);
    return;
  }

  switch (day) {
  case 0:
    gfx_draw_text_box(0x30, TEXT_NO_TRAINER_FOUND, TEXT_BOX_FULL, TEXT_BOX_BLINK);
    break;
  case 1:
    gfx_draw_text_box(0x30, TEXT_CANNOT_CONNECT, TEXT_BOX_FULL, TEXT_BOX_BLINK);
    break;
  case 2:
    gfx_draw_text_box(0x20, TEXT_CANNOT_COMPLETE_CONN, TEXT_BOX_NO_SHADOW, TEXT_BOX_STATIC);
    gfx_draw_text_box(0x30, 3, TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
    break;
  case 4:
    gfx_draw_text_box(0x20, TEXT_CANNOT_CONNECT_AGAIN, TEXT_BOX_NO_SHADOW, TEXT_BOX_STATIC);
    gfx_draw_text_box(0x30, 0x0A, TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
    break;
  case 5:
    gfx_draw_text_box(0x20, TEXT_ALREADY_RECEIVED_EVENT, TEXT_BOX_NO_SHADOW, TEXT_BOX_STATIC);
    gfx_draw_text_box(0x30, 8, TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
    break;
  case 6:
    gfx_draw_text_box(0x20, TEXT_COULD_NOT_RECEIVE, TEXT_BOX_NO_SHADOW, TEXT_BOX_STATIC);
    gfx_draw_text_box(0x30, 0x0C, TEXT_BOX_NO_LINES, TEXT_BOX_BLINK);
    break;
  case 3:
    gfx_draw_text_box(0x30, TEXT_NO_POKEMON_HELD, TEXT_BOX_FULL, TEXT_BOX_BLINK);
    break;
  }

  gfx_draw_battery_low(0x58, 0);
}
