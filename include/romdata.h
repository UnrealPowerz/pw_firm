/* ROM data labels.
 *
 * Defined as per-label const arrays in src/romdata.c. optlnk packs them
 * contiguously within `#pragma section P` in source order, reproducing the
 * original ROM byte layout (verified by scripts/compare_data_layout.py).
 *
 * Split off from globals.h: these aren't runtime "globals" — they're
 * read-only ROM data and have nothing to do with the B_RAM g struct. */

#ifndef ROMDATA_H
#define ROMDATA_H

#include "types.h"

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

#endif /* ROMDATA_H */
