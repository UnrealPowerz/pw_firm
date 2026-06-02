/*
 * gfx_consts.h - Named constants for gfx_* call arguments.
 *
 * Kept in a hand-written header (not the auto-generated build/gen/engine/gfx.h)
 * so editing here doesn't get clobbered on header regen. Included via
 * all_headers.h.
 */

#ifndef GFX_CONSTS_H
#define GFX_CONSTS_H

/* -----------------------------------------------------------------------
 * gfx_draw_text_box(y, index, borders, flags)
 *
 * `borders` is a bitmask consumed by gfx_add_borders_to_text:
 *   bit 0 (0x01) — top/bottom horizontal lines (OR 0x0101 into plane 0)
 *   bit 1 (0x02) — right-edge shadow stripe   (OR 0x8080 into plane 1)
 *   bit 2 (0x04) — left/right edge fill words (force 0xFFFF)
 *   bit 3 (0x08) — middle column dividers     (force 0xFFFF)
 *
 * `flags` bit 0 enables the animTick-driven blink overlay.
 * --------------------------------------------------------------------- */

#define TEXT_BOX_LINES    0x01u   /* horizontal top/bottom border */
#define TEXT_BOX_SHADOW   0x02u   /* right-edge shadow */
#define TEXT_BOX_EDGES    0x04u   /* left/right edge fill */
#define TEXT_BOX_DIVS     0x08u   /* mid-column dividers */

/* The three combinations actually used at call sites. Defined explicitly
   so call sites can name intent without composing the bit flags every time. */
enum text_box_style {
    TEXT_BOX_NO_SHADOW = TEXT_BOX_LINES | TEXT_BOX_EDGES | TEXT_BOX_DIVS,             /* 0x0D */
    TEXT_BOX_NO_LINES  = TEXT_BOX_SHADOW | TEXT_BOX_EDGES | TEXT_BOX_DIVS,            /* 0x0E */
    TEXT_BOX_FULL      = TEXT_BOX_LINES | TEXT_BOX_SHADOW | TEXT_BOX_EDGES | TEXT_BOX_DIVS  /* 0x0F */
};

/* gfx_draw_text_box `flags` values. */
#define TEXT_BOX_STATIC  0u
#define TEXT_BOX_BLINK   1u

/* -----------------------------------------------------------------------
 * gfx_draw_text_box `index` -> EEPROM string slot.
 *
 * Each slot is 0x180 bytes (96x16 px, 2 planes). 96x32 strings span TWO
 * consecutive slots; the second slot has no name of its own and is referenced
 * only via the first. EEPROM addr = 0x2530 + index * 0x180.
 *
 * Naming derived from in-game screenshots / reverse-engineered string content.
 * --------------------------------------------------------------------- */
enum text_box_string {
    TEXT_CONNECTING              = 0,   /* "Connecting..."                 */
    TEXT_NO_TRAINER_FOUND        = 1,   /* "No trainer found"              */
    TEXT_CANNOT_COMPLETE_CONN    = 2,   /* "Cannot complete this connection" (96x32) */
    TEXT_CANNOT_CONNECT          = 4,   /* "Cannot connect"                */
    TEXT_OTHER_TRAINER_UNAVAIL   = 5,   /* "Other trainer is unavailable" (96x32) */
    TEXT_ALREADY_RECEIVED_EVENT  = 7,   /* "Already received this event"  (96x32) */
    TEXT_CANNOT_CONNECT_AGAIN    = 9,   /* "Cannot connect to trainer again" (96x32) */
    TEXT_COULD_NOT_RECEIVE       = 11,  /* "Could not receive..."         (96x32) */
    TEXT_PEER_HAS_ARRIVED        = 13,  /* "<peer> has arrived!"           */
    TEXT_PEER_HAS_LEFT           = 14,  /* "<peer> has left."              */
    TEXT_RECEIVED                = 15,  /* "received!"                     */
    TEXT_COMPLETED               = 16,  /* "Completed!"                    */
    TEXT_SPECIAL_MAP             = 17,  /* "Special Map"                   */
    TEXT_STAMP                   = 18,  /* "Stamp"                         */
    TEXT_SPECIAL_ROUTE           = 19,  /* "Special Route"                 */
    TEXT_NEED_MORE_WATTS         = 20,  /* "Need more Watts."              */
    TEXT_NO_POKEMON_HELD         = 21,  /* "No Pokemon held!"              */
    TEXT_NOTHING_HELD            = 22,  /* "Nothing held!"                 */
    TEXT_DISCOVER_AN_ITEM        = 23,  /* "Discover an item!"   (dowsing prompt) */
    TEXT_FOUND                   = 24,  /* "found!"                        */
    TEXT_NOTHING_FOUND           = 25,  /* "Nothing found!"                */
    TEXT_ITS_NEAR                = 26,  /* "It's near!"                    */
    TEXT_ITS_FAR_AWAY            = 27,  /* "It's far away..."              */
    TEXT_FIND_A_POKEMON          = 28,  /* "Find a Pokemon!"     (radar prompt)   */
    TEXT_FOUND_SOMETHING_EX      = 29,  /* "Found something!"              */
    TEXT_IT_GOT_AWAY             = 30,  /* "It got away..."                */
    TEXT_APPEARED                = 31,  /* "<poke> appeared!"              */
    TEXT_WAS_CAUGHT              = 32,  /* "<poke> was caught!"            */
    TEXT_FLED                    = 33,  /* "fled..."                       */
    TEXT_WAS_TOO_STRONG          = 34,  /* "was too strong."               */
    TEXT_ATTACKED                = 35,  /* "attacked!"                     */
    TEXT_EVADED                  = 36,  /* "evaded!"                       */
    TEXT_CRITICAL_HIT            = 37,  /* "A critical hit!"               */
    TEXT_BLANK                   = 38,  /* (intentionally blank)           */
    TEXT_THREW_POKEBALL          = 39,  /* "Threw a Poke Ball."            */
    TEXT_ALMOST_HAD_IT           = 40,  /* "Almost had it!"                */
    TEXT_STARE_DOWN              = 41,  /* "Stare down!"                   */
    TEXT_LOST                    = 42,  /* "lost!"                         */
    TEXT_WALKER_HAS_ARRIVED      = 43,  /* "<walker> has arrived"          */
    TEXT_HAD_ADVENTURES          = 44,  /* "Had adventures!"               */
    TEXT_PLAY_BATTLED            = 45,  /* "Play-battled."                 */
    TEXT_WENT_FOR_A_RUN          = 46,  /* "Went for a run."               */
    TEXT_WENT_FOR_A_WALK         = 47,  /* "Went for a walk."              */
    TEXT_PLAYED_A_BIT            = 48,  /* "Played a bit."                 */
    TEXT_HERES_A_GIFT            = 49,  /* "Here's a gift..."              */
    TEXT_CHEERED                 = 50,  /* "cheered!"                      */
    TEXT_IS_VERY_HAPPY           = 51,  /* "is very happy!"                */
    TEXT_IS_HAVING_FUN           = 52,  /* "is having fun!"                */
    TEXT_IS_FEELING_GOOD         = 53,  /* "is feeling good!"              */
    TEXT_IS_HAPPY                = 54,  /* "is happy."                     */
    TEXT_IS_SMILING              = 55,  /* "is smiling."                   */
    TEXT_IS_CHEERFUL             = 56,  /* "is cheerful."                  */
    TEXT_IS_BEING_PATIENT        = 57,  /* "is being patient."             */
    TEXT_SITS_QUIETLY            = 58,  /* "sits quietly."                 */
    TEXT_TURNED_TO_LOOK          = 59,  /* "turned to look."               */
    TEXT_IS_LOOKING_AROUND       = 60,  /* "is looking around."            */
    TEXT_IS_LOOKING_THIS_WAY     = 61,  /* "is looking this way."          */
    TEXT_IS_DAYDREAMING          = 62,  /* "is daydreaming."               */
    TEXT_FOUND_SOMETHING         = 63,  /* "Found something."              */
    TEXT_WHAT                    = 64,  /* "What?"                         */
    TEXT_JOINED_YOU              = 65,  /* "joined you!"                   */
    TEXT_REWARD                  = 66,  /* "Reward"                        */
    TEXT_GOOD_JOB                = 67,  /* "Good job!"                     */
    TEXT_SWITCH                  = 68   /* "Switch?"           (80x16)     */
};

/* -----------------------------------------------------------------------
 * drv_sound_play(index) sound IDs.
 *
 * Names inferred from call-site context. Confidence levels in trailing
 * comments — verify in the emulator before treating as ground truth.
 *
 * Unused ID: 8 (no caller in current decomp).
 * --------------------------------------------------------------------- */
enum sound_id {
    SND_CONFIRM      = 0,     /* OK / advance / exit-to-home confirmation       — HIGH */
    SND_BACK         = 1,     /* go-back / boundary-hit / decline ("can't pick")— HIGH */
    SND_CURSOR       = 2,     /* cursor move, settings-value change             — HIGH */
    SND_RADAR_LOCK   = 3,     /* radar: correct slot selected                   — MED  (only caller: ui_handle_radar_grass_menu) */
    SND_FAIL         = 4,     /* minigame miss/failure: dowsing wrong slot,
                                  radar fail, post-battle-loss exit beep        — HIGH */
    SND_DOWSE_HIT    = 5,     /* dowsing: correct slot revealed                 — HIGH (only caller: state_digging success) */
    SND_ANIM_CUE     = 6,     /* arrival / event animation / gift-fanfare stage — MED  (animations + social state==4) */
    SND_FANFARE      = 7,     /* milestone unlock (10M steps), pokemon caught   — MED  (trainer_card milestone + battle states 14/15) */
    /* SND_UNUSED_8  = 8, */  /* no callers */
    SND_GIFT         = 9,     /* social: gift received (state==2 transition)    — LOW  (one caller, near TEXT_RECEIVED draw) */
    SND_BATTLE_START = 10,    /* battle intro                                   — HIGH (only caller: game_start_battle) */
    SND_ATTACK_HIT   = 0x0B,  /* battle: attack lands (move_type 0 + anim)      — HIGH */
    SND_ATTACK_MISS  = 0x0C,  /* battle: defender evades (move_type 1)          — HIGH */
    SND_CRIT_HIT     = 0x0D,  /* battle: critical hit (move_type 2)             — HIGH */
    SND_FLED         = 0x0E,  /* battle/radar: ran out / escaped / lost state   — MED  (all sites transition into "lost" state 7 or radar-failure view) */
    SND_BALL_THROW   = 0x0F   /* battle: BTN_R starts capture sequence (state 10→11→12→13) — MED */
};

#endif /* GFX_CONSTS_H */
