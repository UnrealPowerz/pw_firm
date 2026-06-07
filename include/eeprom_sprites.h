/* EEPROM sprite-region offset constants.
 *
 * The EEPROM region from 0x0280 onwards contains all UI graphics
 * (sprites, icons, menu headings, glyphs, text-strip images). Render
 * functions hold the base 0x0280 in a `volatile uint16_t base` local
 * so ch38 keeps it in a register and uses `add.w rN, ...` per access
 * (matching ROM's `mov.l #0x400280, er4` register-pinning pattern).
 *
 * Use these constants as: `drv_eeprom_read_block(base + EEP_<name>, buf, size)`.
 * For functions that don't pin the base in a register, use the
 * absolute-address EEP_ABS_<name> constants (see bottom of file).
 *
 * Address map cross-referenced against dmitry.gr's published table
 * (see docs/renesas_manuals/PokéWalker hacking - Dmitry.GR.html). */

#ifndef EEPROM_SPRITES_H
#define EEPROM_SPRITES_H

#define EEP_SPRITE_BASE          0x0280  /* relative-offset anchor for all sprite reads */

/* ---- Glyph/icon sprites (offsets relative to EEP_SPRITE_BASE) ---- */
#define EEP_DIGITS               0x0000  /* "0123456789:-/" 8x16 each (0x280..0x41F absolute) */
#define EEP_DIGITS_COLON         0x0140  /* ":" glyph within digit sheet */
#define EEP_WATT_SYMBOL          0x01A0  /* WATT symbol 16x16 (0x420 abs) */
#define EEP_POKEBALL             0x01E0  /* pokeball 8x8 (0x460 abs) */
#define EEP_POKEBALL_EVENT       0x01F0  /* light-grey pokeball for events (0x470 abs) */
#define EEP_ITEM_SYMBOL          0x0208  /* item symbol 8x8 (0x488 abs) */
#define EEP_ITEM_SYMBOL_EVENT    0x0218  /* light-grey item symbol for events (0x498 abs) */
#define EEP_MAP_ICON_TINY        0x0228  /* tiny map icon 8x8 (0x4A8 abs) */
#define EEP_CARD_FACES           0x0238  /* 4 stamp suits 8x8 each (0x4B8 abs) */
#define EEP_ARROWS_8x8           0x0278  /* up/down/left/right arrows 3 variants each (0x4F8 abs) */
#define EEP_MENU_ARROW_LEFT      0x0338  /* left arrow for menu 8x16 (0x5B8 abs) */
#define EEP_MENU_ARROW_RIGHT     0x0358  /* right arrow for menu 8x16 (0x5D8 abs) */
#define EEP_MENU_RETURN_SYMBOL   0x0378  /* "return" symbol 8x16 (0x5F8 abs) */

/* ---- Menu heading bars (each 80x16) ---- */
#define EEP_MENU_HDG_POKERADAR   0x0690  /* "POKé RADAR" (0x910 abs) */
#define EEP_MENU_HDG_DOWSING     0x07D0  /* "DOWSING" (0xA50 abs) */
#define EEP_MENU_HDG_CONNECT     0x0910  /* "CONNECT" (0xB90 abs) */
#define EEP_MENU_HDG_TRAINER     0x0A50  /* "TRAINER CARD" (0xCD0 abs) */
#define EEP_MENU_HDG_INVENTORY   0x0B90  /* "POKéMON & ITEMS" (0xE10 abs) */
#define EEP_MENU_HDG_SETTINGS    0x0CD0  /* "SETTINGS" (0xF50 abs) */

/* ---- Main-menu row icons (each 16x16, packed sequentially) ---- */
#define EEP_MENU_ICONS           0x0E10  /* 6 icons * 0x40 bytes (0x1090 abs) */

/* ---- Trainer-card screen elements ---- */
#define EEP_TRAINER_PERSON_ICON  0x0F90  /* "person" icon (0x1210 abs) */
#define EEP_TRAINER_NAME_IMG     0x0FD0  /* trainer's name image 80x16 (0x1250 abs) */
#define EEP_TRAINER_ROUTE_SMALL  0x1110  /* small route image 16x16 (0x1390 abs) */
#define EEP_LABEL_STEPS          0x1150  /* "steps" frame 40x16 (0x13D0 abs) */
#define EEP_LABEL_TIME           0x11F0  /* "time" frame 32x16 (0x1470 abs) */
#define EEP_LABEL_DAYS           0x1270  /* "days" frame 40x16 (0x14F0 abs) */
#define EEP_LABEL_TOTAL_DAYS     0x1310  /* "total days:" frame 64x16 (0x1590 abs) */

/* ---- Settings-screen elements ---- */
#define EEP_LABEL_SOUND          0x1410  /* "sound" frame 40x16 (0x1690 abs) */
#define EEP_LABEL_SHADE          0x14B0  /* "shade" frame 40x16 (0x1730 abs) */
#define EEP_SPEAKER_NONE         0x1550  /* speaker no-waves 24x16 (0x17D0 abs) */
#define EEP_SPEAKER_LOW          0x15B0  /* speaker one-wave 24x16 (0x1830 abs) */
#define EEP_SPEAKER_HIGH         0x1610  /* speaker two-waves 24x16 (0x1890 abs) */
#define EEP_CONTRAST_BAR         0x1670  /* contrast bar 8x16 (0x18F0 abs) */

/* ---- Inventory / large icons ---- */
#define EEP_TREASURE_CHEST       0x1690  /* large chest 32x24 (0x1910 abs) */
#define EEP_MAP_SCROLL_LARGE     0x1750  /* large map scroll 32x24 (0x19D0 abs) */
#define EEP_PRESENT_LARGE        0x1810  /* large present icon 32x24 (0x1A90 abs) */
#define EEP_BUSH_DARK            0x18D0  /* small dark bush 16x16 (0x1B50 abs) */
#define EEP_BUSH_LIGHT           0x1910  /* small light bush 16x16 (0x1B90 abs) */
#define EEP_BUSH_DARK_LARGE      0x1A30  /* dark bush 32x24 (0x1CB0 abs) */
#define EEP_WORD_BUBBLE_1EX      0x1AF0  /* word bubble + 1 exclamation 16x16 (0x1D70 abs) */
#define EEP_WORD_BUBBLE_2EX      0x1B30  /* word bubble + 2 exclamations 16x16 (0x1DB0 abs) */
#define EEP_WORD_BUBBLE_3EX      0x1B70  /* word bubble + 3 exclamations 16x16 (0x1DF0 abs) */
#define EEP_BUSH_CLICKED         0x1BB0  /* radiating lines (just-clicked bush) 16x16 (0x1E30 abs) */
#define EEP_HP_TICK              0x1DB0  /* HP-bar tick 8x8 (0x2030 abs) */
#define EEP_CATCH_STAR           0x1DC0  /* 5-point star (catch) 8x8 (0x2040 abs) */

/* ---- Misc ---- */
#define EEP_IR_XMIT_ICON         0x21D0  /* IR transmit "wifi arcs" 8x16 (0x2450 abs) */
#define EEP_MUSIC_NOTE_ICON      0x21F0  /* music note 8x8 (0x2470 abs) */
#define EEP_BLANK_ICON           0x2200  /* blank 8x8 (0x2480 abs) */
#define EEP_HOURS_FRAME          0x2210  /* "HOURS" frame (unused?) 40x16 (0x2490 abs) */

/* ---- Strings as image strips (96x16 each unless noted) ---- */
/* These live in the sprite region but are offsets within it; many beyond
 * the 16-bit unsigned base+offset range — we keep absolute addresses for
 * them below in EEP_ABS_TXT_*. */

/* ---- Absolute sprite addresses for sites that don't use the base register.
 *      (Several intentionally duplicate offsets above + 0x280, but use the
 *       absolute form when no `volatile base` is in scope.) ---- */
#define EEP_ABS_POKEBALL              0x0460  /* = base + EEP_POKEBALL */
#define EEP_ABS_ARROWS_8x8            0x04F8  /* = base + EEP_ARROWS_8x8 */
#define EEP_ABS_MENU_RETURN_SYMBOL    0x05F8  /* = base + EEP_MENU_RETURN_SYMBOL */
#define EEP_ABS_MENU_HDG_SETTINGS     0x0F50  /* = base + EEP_MENU_HDG_SETTINGS */
#define EEP_ABS_LABEL_SOUND           0x1690  /* = base + EEP_LABEL_SOUND */
#define EEP_ABS_LABEL_SHADE           0x1730  /* = base + EEP_LABEL_SHADE */
#define EEP_ABS_SPEAKER_NONE          0x17D0  /* speaker icons; 3x 0x60 bytes sequentially */
#define EEP_ABS_CONTRAST_BAR          0x18F0
#define EEP_ABS_TREASURE_CHEST        0x1910  /* = base + EEP_TREASURE_CHEST */
#define EEP_ABS_BUSH_DARK_LARGE       0x1CB0  /* = base + EEP_BUSH_DARK_LARGE */
#define EEP_ABS_CLOUD_APPEARED        0x1F70  /* "pokemon appeared" cloud 32x24 */
#define EEP_ABS_CATCH_STAR            0x2040  /* 8x8 catch star */
#define EEP_ABS_BATTLE_PLACARD_ABS    0x2050  /* attack/evade/catch placard 96x32 */
#define EEP_ABS_WALKER_BLANK_IMG      0x2350  /* pokewalker image, blank screen 32x32 */
#define EEP_ABS_IR_XMIT_ICON          0x2450  /* IR transmit "wifi arcs" 8x16 */
#define EEP_ABS_MUSIC_NOTE_ICON       0x2480

/* ---- Out-of-band data (random checksum area, peer play, special route, events) ---- */
#define EEP_ABS_RANDOM_CHK_TABLE      0x8CB0  /* random checksum descriptor addrs */

/* ---- Pedometer/walker route assets (used by walk_departure etc.; not via base) ---- */
#define EEP_ABS_HOME_ROUTE_IMG   0x8FBE  /* current "area" graphic 32x24 */
#define EEP_ABS_HOME_ROUTE_NAME  0x907E  /* current "area" textual name 80x16 */
#define EEP_ABS_POKE_ANIM_SMALL  0x91BE  /* current pokemon small-anim sprite 32x24x2 */
#define EEP_ABS_POKE_ANIM_LARGE  0x933E  /* current pokemon main-screen sprite 64x48x2 */
#define EEP_ABS_POKE_NAME_IMG    0x993E  /* current pokemon name image 80x16 */
#define EEP_ABS_SPECIAL_ROUTE_HOME 0xC83C  /* special route home-screen img 32x24 */

#endif /* EEPROM_SPRITES_H */
