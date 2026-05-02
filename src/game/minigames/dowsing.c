#include "all_headers.h"

// ROM: 0x9c48  73.6%
void game_generate_encounter_dowsing(void) {
  uint32_t val;
  uint8_t *ram_ptr;
  uint8_t r6l;
  uint8_t r0l;
  uint8_t rem;
  uint8_t *be_ptr;

  gCurSubstateY = 0;
  if (((RamCache_settingsByte & 1)) != 0) {
    uint8_t byte_bf7a = drv_eeprom_read_u8(0xBF7A);
    sys_init_heap();
    ram_ptr = sbrk(0x68);
    if (gfx_xor_rect_ram(ram_ptr, byte_bf7a) != 0) {
      if ((drv_eeprom_read_u8(0xB800) & 0x20) != 0) {
        sys_init_heap();
        ram_ptr = sbrk(4);
        drv_eeprom_read_block(0xBF44, ram_ptr, 4);
        val = ((uint32_t)ram_ptr[1] << 16) | ((uint32_t)ram_ptr[2] << 8) |
              ram_ptr[3];
        if (val < sessionSteps) {
          if (((sys_get_rng() % 100) + val) >= ram_ptr[2]) {
            gCurSubstateY = 4;
            accelXPos = ((sys_get_rng() >> 3) & 1) + 3;
            return;
          }
        }
      }
    }
  }

  /* LAB_9cea */
  sys_init_heap();
  ram_ptr = sbrk(0x30);
  drv_eeprom_read_block(0xCE8C, ram_ptr, 0x30);
  rem = (uint8_t)(sys_get_rng() % 100);

  sys_init_heap();
  be_ptr = sbrk(0xBE);
  drv_eeprom_read_block(0x8F00, be_ptr, 0xBE);

  for (r6l = 0; r6l < 3; r6l++) {
    if (game_check_step_unlock((uint16_t)(r6l * 2), 0x82)) {
      if (be_ptr[0x88 + (r6l * 2)] >= rem) {
        gCurSubstateY = r6l + 1;
        accelXPos = (r6l * -1) + ((sys_get_rng() >> 3) & 1) +
                    3; /* Wait, it's sys_get_rng()>>3 & 1 + (-r6l + r0l) ->
                          accelXPos */
        return;
      }
    }
  }

  gCurSubstateY = 3;
  accelXPos = ((sys_get_rng() >> 3) & 1) + 1;
}

/* =============================================================================
 * game_init_dowsing – Initialize dowsing state
 * Address: 0x4792  Size: 60 bytes
 * ===========================================================================
 */
// ROM: 0x4792  80.5%
void game_init_dowsing(void) {
  uint16_t rnd;
  uint16_t item_slot;

  gCurSubstateZ = 0;
  DAT_f7d1 = 0;
  accelXPos = 2;

  rnd = (uint16_t)sys_get_rng();
  rnd <<= 3;
  rnd = (uint16_t)((uint8_t)(rnd >> 8));
  dowsing_item_pos = (uint8_t)((int16_t)rnd % 6);

  accelYPos = 0xFF;
  DAT_f7d5 = 0;
}

/* =============================================================================
 * ui_handle_dowsing – Button handler for dowsing state machine
 * Address: 0x47CE  Size: 462 bytes
 * ===========================================================================
 */
// ROM: 0x47ce  78.0%
void ui_handle_dowsing(void) {
  uint8_t subZ;

  subZ = gCurSubstateZ;

  if (subZ == 0) {
    goto state0;
  } else if (subZ == 1) {
    goto state1;
  } else if (subZ == 2) {
    goto state2;
  } else if (subZ == 3) {
    goto state3;
  } else if (subZ == 4) {
    goto state4;
  }
  return;

state0:
  if (drv_button_is_triggered(0x02)) {
    if (DAT_f7d1 == accelYPos) {
      drv_sound_play(1);
      return;
    }
    drv_sound_play(0);
    gCurSubstateZ = 1;
    gCurSubstateA = 4;
    return;
  }

  if (drv_button_is_triggered(0x04)) {
    DAT_f7d1 = (uint8_t)(((int32_t)((uint16_t)DAT_f7d1 + 5)) % 6);
    drv_sound_play(2);
  }

  if (drv_button_is_triggered(0x08)) {
    DAT_f7d1 = (uint8_t)(((int32_t)((uint16_t)DAT_f7d1 + 1)) % 6);
    drv_sound_play(2);
    return;
  }
  return;

state1:
  if (gCurSubstateA != 0) {
    return;
  }
  if (drv_sound_is_playing()) {
    return;
  }
  if (DAT_f7d1 == dowsing_item_pos) {
    gCurSubstateZ = 2;
    ui_handle_dowsing_selection();
    drv_sound_play(5);
    return;
  }
  /* Wrong spot */
  gCurSubstateZ = 3;
  drv_sound_play(4);
  if (accelXPos == 2) {
    accelYPos = DAT_f7d1;
  }
  accelXPos--;
  return;

state2:
  if (!drv_button_is_triggered(0x0E)) {
    return;
  }
  if ((RamCache_settingsByte & 1)) {
    goto exit_to_main;
  }
  {
    uint8_t slot;
    slot = accelZPos;
    if (slot == 3) {
      gCurSubstateA = 1;
      game_start_peer_session();
      ui_set_view(7);
      return;
    }
    drv_eeprom_write_block((uint16_t)slot * 4 + 0xCEBC, (void *)&DAT_f7d8, 0x2);
  }
  if (walker_status_flags_BIT.walking) {
    uint8_t *trainer_buf;
    uint8_t *gift_buf;

    trainer_buf = (uint8_t *)sbrk(0xBE);
    drv_eeprom_read_block(0x8F00, trainer_buf, 0xBE);
    gift_buf = (uint8_t *)sbrk(0x88);
    game_log_interaction(trainer_buf, gift_buf, 0x0B, 0x00, (uint16_t)DAT_f7d8);
  }

exit_to_main:
  drv_sound_play(0);
  ui_reset_substate();
  ui_set_view(0);
  return;

state3:
  if (!drv_button_is_triggered(0x0E)) {
    return;
  }
  if (accelXPos != 0) {
    drv_sound_play(0);
    gCurSubstateZ = 4;
    return;
  }
  drv_sound_play(0);
  ui_reset_substate();
  ui_set_view(0);
  return;

state4:
  if (!drv_button_is_triggered(0x0E)) {
    return;
  }
  drv_sound_play(0);
  gCurSubstateZ = 0;
  return;
}

/* =============================================================================
 * game_read_wild_poke – Read encounter data from EEPROM (0x188 bytes from
 * 0xBD40) Address: 0x499C  Size: 18 bytes
 * ===========================================================================
 */
// ROM: 0x499c  52.5%
void game_read_wild_poke(void *ram_dst) {
  /* Wrap drv_eeprom_read_block for encounter data */
  drv_eeprom_read_block(0xBD40, ram_dst, 0x188);
  (void)0;
}

/* =============================================================================
 * game_write_wild_poke – Write encounter data to EEPROM (0x188 bytes to
 * 0xBD40) Address: 0x49AE  Size: 18 bytes
 * ===========================================================================
 */
// ROM: 0x49ae  52.5%
void game_write_wild_poke(void *ram_src) {
  /* Wrap drv_eeprom_write_block for encounter data */
  drv_eeprom_write_block(0xBD40, ram_src, 0x188);
  (void)0;
}

// ROM: 0x49c0  81.7%
#pragma option noregexpansion /* pragma:auto */
void game_check_wild_encounter(void) {
  uint8_t *battle_buf;
  uint8_t *encounter_data;
  uint8_t *route_data;
  uint32_t steps_required;
  uint16_t rnd_pct;
  uint16_t encounter_rate;

  sys_init_heap();
  battle_buf = (uint8_t *)sbrk(0x68);
  encounter_data = (uint8_t *)sbrk(0x188);
  route_data = (uint8_t *)sbrk(0x7C);

  drv_eeprom_read_block(0xBF00, route_data, 0x7C);

  /* Byte-swap 16-bit value at offset 0x4A into 32-bit step requirement */
  {
    uint16_t w;
    w = *((uint16_t *)(route_data + 0x4A));
    steps_required = (uint32_t)(((w & 0xFF00) >> 8) | ((w & 0xFF) << 8));
  }

  if (sessionSteps < steps_required) {
    goto no_encounter;
  }

  rnd_pct = (uint16_t)((int16_t)((uint16_t)sys_get_rng() >> 3) % 100);
  encounter_rate = (uint16_t)route_data[0x4C];
  if (rnd_pct >= encounter_rate) {
    goto no_encounter;
  }

  game_read_wild_poke(encounter_data);
  if (*((uint16_t *)(encounter_data + 0x6)) != 0) {
    goto no_encounter;
  }

  if (gfx_xor_rect_ram(battle_buf, route_data[0x7B]) != 0) {
    goto encounter;
  }

no_encounter:
  DAT_f7d5 = (uint8_t)((accelXPos << 2) + 2);
  game_add_watts(10);
  return;

encounter:
  save_set_event_bit(battle_buf, route_data[0x7B]);

  {
    uint8_t *trainer_buf;
    trainer_buf = (uint8_t *)sbrk(0xBE);
    drv_eeprom_read_block(0x8F00, trainer_buf, 0xBE);

    gCurSubstateY = 0x0A;

    *((uint32_t *)encounter_data) = *((uint32_t *)route_data);
    *((uint16_t *)(encounter_data + 0x4)) = *((uint16_t *)(route_data + 0x4));
    *((uint16_t *)(encounter_data + 0x6)) = *((uint16_t *)(route_data + 0x48));

    drv_eeprom_read_block(0xCA3C, encounter_data + 0x8, 0x180);
    game_write_wild_poke(encounter_data);

    {
      uint8_t flags;
      flags = drv_eeprom_read_u8(0xB800);
      flags |= 0x40;
      drv_eeprom_write_u8(0xB800, flags);
    }

    if (walker_status_flags_BIT.walking) {
      uint8_t *gift_buf;
      gift_buf = (uint8_t *)sbrk(0x88);
      game_log_interaction(trainer_buf, gift_buf, 0x0C, 0x01,
                           (uint16_t)DAT_f7d8);
    }
  }
}

/* =============================================================================
 * ui_handle_dowsing_selection – Dowsing item selection handler
 * Address: 0x4AF2  Size: 170 bytes
 * ===========================================================================
 */
// ROM: 0x4af2  80.9%
void ui_handle_dowsing_selection(void) {
  uint8_t *item_table;
  uint8_t *trainer_buf;
  uint8_t i;

  sys_init_heap();
  item_table = (uint8_t *)sbrk(0x0C);
  drv_eeprom_read_block(0xCEBC, item_table, 0x0C);

  accelZPos = save_find_empty_slot_32bit(item_table);

  if ((RamCache_settingsByte & 1)) {
    game_check_wild_encounter();
    return;
  }

  trainer_buf = (uint8_t *)sbrk(0xBE);
  drv_eeprom_read_block(0x8F00, trainer_buf, 0xBE);

  for (i = 0; i < 10; i++) {
    uint16_t rnd_pct;
    uint16_t slot_rate;

    if (game_check_step_unlock((uint16_t)i * 2, 0xA0)) {
      continue;
    }

    rnd_pct = (uint16_t)((int16_t)((uint16_t)sys_get_rng() >> 3) % 100);
    slot_rate = (uint16_t)trainer_buf[0xB4 + i];

    if (rnd_pct < slot_rate) {
      break;
    }
  }

  if (i > 9) {
    i = 9;
  }

  gCurSubstateY = i;
  DAT_f7d8 = *((uint16_t *)(trainer_buf + 0x8C + (uint16_t)i * 2));
}

/* =============================================================================
 * ui_render_dowsing_grass – Draw tall-grass search animation
 * Address: 0x4B9C  Size: 314 bytes
 * ===========================================================================
 */
// ROM: 0x4b9c  66.4%
void ui_render_dowsing_grass(void) {
  uint8_t *buf;
  uint16_t base;
  uint16_t j;

  base = 0x280;
  sys_init_heap();
  buf = (uint8_t *)sbrk(0x180);

  /* Player sprite */
  drv_eeprom_read_block(base + (uint16_t)accelXPos * 0x20, buf, 0x20);
  drv_lcd_blit(0x40, 8, buf, 0x10, 8);

  /* Background top */
  drv_eeprom_read_block(0x1950 + base, buf, 0x80);
  drv_lcd_blit(0x20, 0x10, buf, 0x20, 0x10);

  drv_eeprom_read_block(0x19D0 + base, buf, 0x60);
  drv_lcd_blit(0x48, 0, buf, 0x18, 0x10);

  /* Grass background */
  {
    uint16_t bg_addr;
    if ((RamCache_settingsByte & 1)) {
      bg_addr = 0xC83C;
    } else {
      bg_addr = 0x8FBE;
    }
    drv_eeprom_read_block(bg_addr, buf, 0xC0);
  }
  drv_lcd_blit(0, 0, buf, 0x20, 0x18);

  if (gCurSubstateA != 0) {
    gCurSubstateA--;
  }

  /* Draw 6 selection circles */
  drv_eeprom_read_block(0x18D0 + base, buf, 0x80);
  for (j = 0; j < 6; j++) {
    uint8_t x_pos;
    x_pos = (uint8_t)(j * 0x10);
    if ((uint8_t)j == accelYPos) {
      drv_lcd_blit(x_pos, 0x18, buf + 0x40, 0x10, 8);
    } else if ((uint8_t)j != DAT_f7d1) {
      drv_lcd_blit(x_pos, 0x18, buf, 0x10, 8);
    }
  }

  /* Bobbing grass */
  {
    uint8_t anim;
    anim = DAT_f7ac & 0x03;
    gfx_draw_animated_grass(
        0x10, 0x10, (int8_t)*((volatile uint8_t *)(0xBD82 + anim)), buf);
  }

  /* Highlighted cursor */
  {
    uint8_t cursor_x;
    cursor_x = (uint8_t)(DAT_f7d1 * 0x10);
    drv_lcd_blit(cursor_x, 0x18, buf, 0x10, 8);
  }

  gfx_draw_text_box(0x30, 0x11, 0x22, 0x00);
}

// ROM: 0x4cd6  61.7%
#pragma option speed =loop=1 /* pragma:auto */
void ui_render_dowsing(void) {
  uint8_t *buf;
  uint8_t *spr;
  uint16_t base;
  uint16_t j;

  base = 0x280;

  if (gCurSubstateZ == 1) {
    ui_render_dowsing_grass();
    goto end;
  }

  sys_init_heap();
  spr = (uint8_t *)sbrk(0x140);
  buf = (uint8_t *)sbrk(0x180);

  /* Load full sprite sheet, then pick frame by accelXPos */
  drv_eeprom_read_block(base, spr, 0x140);
  spr += (uint16_t)accelXPos * 0x20;
  drv_lcd_blit(0x40, 8, spr, 0x20, 0x10);

  /* Background pieces */
  drv_eeprom_read_block(0x1950 + base, buf, 0x80);
  drv_lcd_blit(0x20, 0x10, buf, 0x20, 0x10);

  drv_eeprom_read_block(0x19D0 + base, buf, 0x60);
  drv_lcd_blit(0x48, 0, buf, 0x18, 0x10);

  /* Grass background */
  {
    uint16_t bg_addr;
    if ((RamCache_settingsByte & 1)) {
      bg_addr = 0xC83C;
    } else {
      bg_addr = 0x8FBE;
    }
    drv_eeprom_read_block(bg_addr, buf, 0xC0);
  }
  drv_lcd_blit(0, 0, buf, 0x20, 0x18);

  if (gCurSubstateA != 0) {
    gCurSubstateA--;
  }

  /* Draw 6 probe circles */
  drv_eeprom_read_block(0x18D0 + base, buf, 0x80);
  for (j = 0; j < 6; j++) {
    uint8_t x_pos;
    x_pos = (uint8_t)(j * 0x10);

    if (gCurSubstateZ == 2 && (uint8_t)j == DAT_f7d1) {
      continue;
    }
    if ((uint8_t)j == accelYPos) {
      drv_lcd_blit(x_pos, 0x18, buf + 0x40, 0x10, 8);
    } else {
      drv_lcd_blit(x_pos, 0x18, buf, 0x10, 8);
    }
  }

  /* Overlay based on substateZ */
  if (gCurSubstateZ == 0) {
    /* Idle: dousing rod animated */
    uint8_t anim_sel;
    anim_sel = DAT_f7ac & 0x01;
    drv_eeprom_read_block(0x278 + base + (uint16_t)anim_sel * 0x10, buf, 0x10);
    {
      uint8_t rod_x;
      rod_x = (uint8_t)(DAT_f7d1 * 0x10 + 0x04);
      drv_lcd_blit(rod_x, 0x28, buf, 8, 8);
    }
    gfx_draw_text_box(0x17, 0x0F, 0x30, 0x01);

  } else if (gCurSubstateZ == 2) {
    /* Found item */
    drv_eeprom_read_block(0x208 + base, buf, 0x10);
    {
      uint8_t item_x;
      item_x = (uint8_t)(DAT_f7d1 * 0x10 + 0x04);
      drv_lcd_blit(item_x, 0x18, buf, 8, 8);
    }

    if (DAT_f7d5 != 0) {
      gfx_draw_value_with_icon(0x02, 0x20, 0x0D, (uint16_t)DAT_f7d5);
      gfx_draw_text_box(0x0F, 0x0E, 0x30, 0x02);
    } else {
      uint8_t subY;
      subY = gCurSubstateY;
      if (subY >= 0x0A) {
        gfx_draw_event_item_name(0x00, 0x20, 0, 0x0D);
      } else {
        gfx_draw_item_name(0x00, 0x20, subY, 0x0D);
      }
      gfx_draw_text_box(0x18, 0x0E, 0x30, 0x01);
    }

  } else if (gCurSubstateZ == 3) {
    /* Wrong spot */
    gfx_draw_text_box(0x19, 0x0F, 0x30, 0x01);

    if (accelXPos != 0) {
      uint16_t k;
      drv_eeprom_read_block(0x208 + base, buf, 0x10);
      for (k = 3; k > 0; k--) {
        uint8_t item_x;
        item_x = (uint8_t)(dowsing_item_pos * 0x10 + 0x04);
        drv_lcd_blit(item_x, 0x16, buf, 8, 8);
      }
    }

  } else if (gCurSubstateZ == 4) {
    /* Result screen - proximity indicator */
    uint16_t dist;
    int16_t diff;
    diff = (int16_t)(uint16_t)DAT_f7d1 - (int16_t)(uint16_t)dowsing_item_pos;
    if (diff < 0) {
      dist = (uint16_t)(-diff);
    } else {
      dist = (uint16_t)diff;
    }

    if (dist < 2) {
      gfx_draw_text_box(0x30, 0x1A, 0x0F, 0x01);
    } else {
      gfx_draw_text_box(0x30, 0x1B, 0x0F, 0x01);
    }
  }

end:
  gfx_draw_battery_low(0, 0x58);
}
