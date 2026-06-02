/*
 * input_consts.h - Named masks for the button driver.
 *
 * Bit positions match button_input_t in types.h, which is the layout used
 * by buttonInputRaw_BIT / buttonTrigger_BIT:
 *   bit 1 (0x02) — btn_r  (PDRB.B0)
 *   bit 2 (0x04) — btn_m  (PDRB.B2)
 *   bit 3 (0x08) — btn_l  (PDRB.B4)
 *
 * The l/m/r names mirror the bit-field labels; physical-to-pin mapping
 * is a separate concern handled in drv_button_read().
 */

#ifndef INPUT_CONSTS_H
#define INPUT_CONSTS_H

#define BTN_R    0x02u    /* btn_r — bit 1 */
#define BTN_M    0x04u    /* btn_m — bit 2 */
#define BTN_L    0x08u    /* btn_l — bit 3 */

#define BTN_LM   (BTN_L | BTN_M)              /* 0x0A */
#define BTN_ANY  (BTN_L | BTN_M | BTN_R)      /* 0x0E — any of the three */

#endif /* INPUT_CONSTS_H */
