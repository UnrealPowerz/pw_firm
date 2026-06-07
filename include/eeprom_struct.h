/* EEPROM sprite region as a struct anchored at 0x280.
 *
 * Used to access sprites via field-name addressing, e.g.
 *   drv_eeprom_read_block(SPR_OFF(menu_hdg_trainer), buf, sizeof(SPR.menu_hdg_trainer));
 *
 * The key codegen property: because the struct is at address 0x280 (a runtime
 * pointer expression, not a folded constant), ch38 loads 0x280 into a register
 * and computes each member address via `add.w`, exactly like ROM's
 * `mov.l #0x400280, er4 / add.w r4, r0` pattern. This is closer to ROM than
 * the `volatile uint16_t base = 0x280` workaround which spills 0x280 to stack.
 *
 * Field layout cross-referenced against dmitry.gr's writeup
 * (docs/renesas_manuals/PokéWalker hacking - Dmitry.GR.html, "EEPROM map"). */

#ifndef EEPROM_STRUCT_H
#define EEPROM_STRUCT_H

struct sprite_region {
    uint8_t  digits[0x1A0];               /* 0x0280: "0123456789:-/" 8x16 each */
    uint8_t  watt_symbol[0x40];           /* 0x0420: WATT symbol 16x16 */
    uint8_t  pokeball[0x10];              /* 0x0460: pokeball 8x8 */
    uint8_t  pokeball_event[0x10];        /* 0x0470: light-grey pokeball 8x8 */
    uint8_t  _gap_0480[0x8];              /* 0x0480: unused */
    uint8_t  item_symbol[0x10];           /* 0x0488: item symbol 8x8 */
    uint8_t  item_symbol_event[0x10];     /* 0x0498: light-grey item symbol 8x8 */
    uint8_t  map_icon_tiny[0x10];         /* 0x04A8: tiny map icon 8x8 */
    uint8_t  card_faces[0x40];            /* 0x04B8: 4 stamp suits 8x8 each */
    uint8_t  arrows_8x8[0xC0];            /* 0x04F8: up/down/left/right 3 variants */
    /* Three 8x16 menu glyphs — read individually or as a contiguous strip.
     * Use SPR_SPAN(menu_arrow_left, menu_return_symbol) for the trio. */
    uint8_t  menu_arrow_left[0x20];       /* 0x05B8: left arrow */
    uint8_t  menu_arrow_right[0x20];      /* 0x05D8: right arrow */
    uint8_t  menu_return_symbol[0x20];    /* 0x05F8: "return" symbol */
    uint8_t  _gap_0618[0x20];             /* 0x0618: unused */
    uint8_t  more_msg_or_mask[0x10];      /* 0x0638: "more message" OR mask */
    uint8_t  more_msg_and_mask[0x8];      /* 0x0648: "more message" AND mask */
    uint8_t  medicine_vial[0x10];         /* 0x0650: medicine vial 8x8 */
    uint8_t  low_battery[0x10];           /* 0x0660: low battery 8x8 */
    uint8_t  talk_bubbles_br[0x240];      /* 0x0670: large bubbles from BR, 6×24x16 */
    uint8_t  talk_bubble_bl[0x60];        /* 0x08B0: large bubble from BL */

    /* Menu heading bars (each 80x16 = 0x140 bytes) */
    uint8_t  menu_hdg_pokeradar[0x140];   /* 0x0910 */
    uint8_t  menu_hdg_dowsing[0x140];     /* 0x0A50 */
    uint8_t  menu_hdg_connect[0x140];     /* 0x0B90 */
    uint8_t  menu_hdg_trainer[0x140];     /* 0x0CD0 */
    uint8_t  menu_hdg_inventory[0x140];   /* 0x0E10 */
    uint8_t  menu_hdg_settings[0x140];    /* 0x0F50 */

    /* Main-menu row icons (6 × 16x16 = 6 × 0x40) */
    uint8_t  menu_icons[6][0x40];         /* 0x1090 */

    /* Trainer-card screen */
    uint8_t  trainer_person_icon[0x40];   /* 0x1210 */
    uint8_t  trainer_name_img[0x140];     /* 0x1250 */
    uint8_t  trainer_route_small[0x40];   /* 0x1390 */
    uint8_t  label_steps[0xA0];           /* 0x13D0 */
    uint8_t  label_time[0x80];            /* 0x1470 */
    uint8_t  label_days[0xA0];            /* 0x14F0 */
    uint8_t  label_total_days[0x100];     /* 0x1590 */

    /* Settings screen */
    uint8_t  label_sound[0xA0];           /* 0x1690 */
    uint8_t  label_shade[0xA0];           /* 0x1730 */
    /* Speaker icons (no-waves / one-wave / two-waves), 24x16 each, often
     * read together. Use SPR_SPAN(speaker_none, speaker_high) for the trio. */
    uint8_t  speaker_none[0x60];          /* 0x17D0 */
    uint8_t  speaker_low[0x60];           /* 0x1830 */
    uint8_t  speaker_high[0x60];          /* 0x1890 */
    uint8_t  contrast_bar[0x20];          /* 0x18F0 */

    /* Inventory / large icons */
    uint8_t  treasure_chest[0xC0];        /* 0x1910 */
    uint8_t  map_scroll_large[0xC0];      /* 0x19D0 */
    uint8_t  present_large[0xC0];         /* 0x1A90 */
    /* Small bush variants — read together by dowsing. */
    uint8_t  bush_dark[0x40];             /* 0x1B50 */
    uint8_t  bush_light[0x40];            /* 0x1B90 */
    uint8_t  left_string_unref[0x80];     /* 0x1BD0 */
    uint8_t  blank_img_16x24[0x60];       /* 0x1C50 */
    uint8_t  bush_dark_large[0xC0];       /* 0x1CB0 */
    uint8_t  word_bubble_1ex[0x40];       /* 0x1D70 */
    uint8_t  word_bubble_2ex[0x40];       /* 0x1DB0 */
    uint8_t  word_bubble_3ex[0x40];       /* 0x1DF0 */
    uint8_t  bush_clicked_lines[0x40];    /* 0x1E30 */
    uint8_t  star_attack[0x80];           /* 0x1E70 */
    uint8_t  star_critical[0x80];         /* 0x1EF0 */
    uint8_t  cloud_appeared[0xC0];        /* 0x1F70 */
    uint8_t  hp_tick[0x10];               /* 0x2030 */
    uint8_t  catch_star[0x10];            /* 0x2040 */
    uint8_t  battle_placard[0x300];       /* 0x2050 */
    uint8_t  walker_blank_img[0x100];     /* 0x2350 */
    uint8_t  ir_xmit_icon[0x20];          /* 0x2450 */
    uint8_t  music_note_icon[0x10];       /* 0x2470 */
    uint8_t  blank_icon[0x10];            /* 0x2480 */
    uint8_t  hours_frame[0xA0];           /* 0x2490 */
    /* ...string strips and beyond — extend as needed */
};

/* The struct exists only as a typed addressing overlay — no instance is
 * allocated. SPR resolves to the dereference of a pointer to 0x280; ch38
 * loads 0x280 into a register and computes member addresses via add. */
#define SPR  (*(volatile struct sprite_region *)0x280)

/* Address-of a sprite_region member, cast to uint16 for drv_eeprom_* APIs. */
#define SPR_OFF(member)  ((uint16_t)(unsigned long)&SPR.member)

/* Size of a sprite_region member (so call sites don't repeat the size). */
#define SPR_SIZE(member) ((uint16_t)sizeof(SPR.member))

/* For reads that span multiple consecutive fields, sum the sizeofs.
 * Sum-of-sizeofs folds to a compile-time constant; SPR_OFF subtraction
 * doesn't (ch38 keeps each address live and computes the diff at runtime).
 * Use: drv_eeprom_read_block(SPR_OFF(first), buf,
 *                            sizeof(SPR.first) + sizeof(SPR.second) + ...); */

#endif /* EEPROM_STRUCT_H */
