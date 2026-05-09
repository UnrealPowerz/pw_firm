#include "all_headers.h"

// ROM: 0xb0ae  66.4%
void save_write_magic(void) {
  uint8_t i;
  i = 0;
  do {
    drv_eeprom_write_u8(i, ((uint8_t *)0xBF98)[i]);
    i++;
  } while (i < 8);
}

// ROM: 0xb0c8  71.6%
uint8_t save_verify_magic(void) {
  uint8_t i;
  uint8_t eep_val;
  int16_t rom_val;

  i = 0;
  do {
    eep_val = drv_eeprom_read_u8(i);
    rom_val = (int16_t)(int8_t)((uint8_t *)0xBF98)[i];
    if ((int16_t)(uint16_t)eep_val != rom_val) {
      return 0;
    }
    i++;
  } while (i < 8);
  return 1;
}

// ROM: 0xb1ae  78.8%  saves: r3,r4,er5,r6 -> er5,er6
void sys_factory_reset_eeprom(uint8_t b, uint8_t a) {
  uint8_t *ptr;
  register volatile uint8_t *flags_ptr;
  uint32_t zero = 0;
  uint16_t i;

  sessionSteps = zero;
  DAT_f7a0 = 0;
  walker_status_flags &= ~0x06;

  ptr = (uint8_t *)drv_ir_get_rx_ptr();
  save_read_reliable(0x00ED, 0x01ED, ptr, 0x68);

  *((uint32_t *)(ptr)) = zero;
  *((uint32_t *)(ptr + 4)) = zero;
  *((uint16_t *)(ptr + 8)) = 0;
  *((uint16_t *)(ptr + 10)) = 0;
  *((uint32_t *)(ptr + 12)) = zero;

  for (i = 0; i < 18; i++) {
    ptr[0x48 + i] = 0;
  }

  if (b) {
    for (i = 0; i < 16; i++) {
      ptr[0x38 + i] = 0;
    }
  }

  flags_ptr = ptr + 0x5B;
  *flags_ptr &= ~0x01;
  *flags_ptr &= ~0x02;
  *flags_ptr &= ~0x04;
  ptr[0x5B] &= 0x07;
  ptr[0x5A] = 0;
  ptr[0x5E] = 0;
  ptr[0x5F] = 2;
  *((uint32_t *)(ptr + 0x60)) = zero;

  save_read_reliable(0x0083, 0x0183, ptr + 0x28, 0x10);
  if (a) {
    *((uint32_t *)(ptr + 0x64)) = zero;
  }
  save_write_reliable(0x00ED, 0x01ED, ptr, 0x68);
  game_reset_step_data(a);

  if (a) {
    drv_eeprom_fill(0xCE80, 0x0D4C, 0);
  } else {
    save_clear_data();
    save_init_defaults();
    save_delete_step_history();
  }

  if (b) {
    drv_eeprom_fill(0xB800, 0x06C8, 0);
  }

  drv_eeprom_fill(0xDE24, 0x1568, 0);
}

// ROM: 0xb2e2  81.8%  saves: er4,er5,er6
void sys_sync_eeprom_on_startup(void) {
  uint8_t magic;

  if (save_verify_magic() != 0) {
    save_read_reliable(0x0156, 0x0256, (void *)&totalSteps, 0x18);
    if ((RamCache_settingsByte & 0x78) > 0x48) {
      RamCache_settingsByte = (RamCache_settingsByte & 0x87) | 0x20;
    }
    game_sync_walk_status();
  } else {
    sys_factory_reset_eeprom(1, 1);
    drv_sound_set_volume((RamCache_settingsByte >> 1) & 0x3);
    drv_lcd_set_contrast((RamCache_settingsByte >> 3) & 0xF);
    save_write_magic();
  }

  save_read_reliable(0x016F, 0x026F, &magic, 1);
  if (magic == 0xA5) {
    save_commit_staged_data();
    magic = 0;
    save_write_reliable(0x016F, 0x026F, &magic, 1);
  }
}

// ROM: 0x0426  82.4%
#pragma option noregexpansion  /* pragma:auto */
void save_commit_staged_data(void) {
  void *buf;
  uint16_t src1;
  uint32_t dst1;
  uint16_t count2;
  uint16_t src2;
  uint32_t dst2;

  sys_init_heap();
  buf = sbrk(0x80);

  src1 = 0xD700;
  dst1 = 0x528F00;
  count2 = (uint16_t)(dst1 >> 16);

  while (count2 != 0) {
    drv_eeprom_read_block(src1, buf, 0x80);
    drv_eeprom_write_block(dst1 & 0xFFFF, buf, 0x80);
    src1 += 0x80;
    dst1 += 0x80;
    count2--;
  }

  src2 = 0xD480;
  dst2 = 0x5CC00;
  count2 = 0;

  while (count2 < (uint16_t)(dst2 >> 16)) {
    drv_eeprom_read_block(src2, buf, 0x80);
    drv_eeprom_write_block(dst2 & 0xFFFF, buf, 0x80);
    src2 += 0x80;
    dst2 += 0x80;
    count2++;
  }
}

// ROM: 0x50d8  84.0%  saves: er3,er4,er5,er6
void save_write_reliable(uint16_t primary, uint16_t backup, uint8_t *buf,
                               uint16_t size) {
  uint8_t checksum = 1;
  uint16_t i;

  drv_eeprom_write_block(primary, buf, size);
  for (i = 0; i < size; i++)
    checksum += buf[i];
  drv_eeprom_write_u8(primary + size, checksum);
  drv_eeprom_write_block(backup, buf, size);
  drv_eeprom_write_u8(backup + size, checksum);
}

// ROM: 0x5128  85.2%  saves: er3,er4,er5,er6
#pragma option noregexpansion  /* pragma:auto */
void save_read_reliable(uint16_t primary, uint16_t backup, uint8_t *buf,
                              uint16_t size) {
  uint8_t status = 0;
  uint8_t checksums[2] = {1, 1}; // 0 = backup, 1 = primary
  uint8_t chk1, chk2;
  uint8_t i;

  drv_eeprom_read_block(primary, buf, size);
  for (i = 0; i < size; i++)
    checksums[1] += buf[i];

  drv_eeprom_read_block(backup, buf, size);
  for (i = 0; i < size; i++)
    checksums[0] += buf[i];

  chk1 = drv_eeprom_read_u8(backup + size);
  if (checksums[0] == chk1)
    status |= 1;

  chk2 = drv_eeprom_read_u8(primary + size);
  if (checksums[1] == chk2)
    status |= 2;

  switch (status) {
  case 0:
    for (i = 0; i < size; i++)
      buf[i] = 0xFF;
    drv_eeprom_write_block(primary, buf, size);
    drv_eeprom_write_u8(primary + size, 0xFF);
    drv_eeprom_write_block(backup, buf, size);
    drv_eeprom_write_u8(backup + size, 0xFF);
    break;
  case 1:
    drv_eeprom_read_block(backup, buf, size);
    drv_eeprom_write_block(primary, buf, size);
    drv_eeprom_write_u8(primary + size, checksums[0]);
    break;
  case 2:
    drv_eeprom_read_block(primary, buf, size);
    drv_eeprom_write_block(backup, buf, size);
    drv_eeprom_write_u8(backup + size, checksums[1]);
    break;
  case 3:
    if (checksums[0] != checksums[1]) {
      drv_eeprom_write_block(primary, buf, size);
      drv_eeprom_write_u8(primary + size, checksums[0]);
    }
    break;
  }
}

// ROM: 0x1d22  45.1%  saves: r6,r5
#pragma option speed=register  /* pragma:auto */
void save_set_event_bit(void *ptr, uint8_t val) {
  uint8_t *p = (uint8_t *)ptr;
  uint8_t bit = val & 0x7;
  uint8_t offset = val >> 3;

  if (val == 0)
    return;

  /* This matches the bit-setting logic in assembly */
  save_read_reliable(0x00ED, 0x01ED, p, 0x68);
  p[0x38 + offset] |= (1 << bit);
  save_write_reliable(0x00ED, 0x01ED, p, 0x68);
}

// ROM: 0x1eca  69.1%
uint8_t save_find_empty_reward_slot(void *ptr) {
  uint8_t *p = (uint8_t *)ptr;
  uint8_t i;

  for (i = 0; i < 3; i++) {
    uint16_t val = *(uint16_t *)(p + (i * 16));
    if (val == 0) {
      return i;
    }
  }
  return 3;
}

// ROM: 0x1f1c  69.1%
uint8_t save_find_empty_slot_32bit(void *ptr) {
  uint8_t *p = (uint8_t *)ptr;
  uint8_t i;

  for (i = 0; i < 3; i++) {
    uint32_t val = *(uint32_t *)(p + (i * 4));
    if (val == 0) {
      return i;
    }
  }
  return 3;
}

/*
 * Address: 0x187E
 */
// ROM: 0x187e  72.2%
void save_clear_data(void) { drv_eeprom_fill(0xCE8C, 0x0064, 0); }

#pragma noregsave(save_init_defaults)
// ROM: 0x188c  96.9%
void save_init_defaults(void) {
  uint16_t addr = 0xCF0C;
  uint8_t i = 0x18;
  do {
    drv_eeprom_write_u8(addr + 0x84, 0);
    addr += 0x88;
  } while (--i != 0);
}

/*
 * Address: 0x18A8
 */
// ROM: 0x18a8  72.2%
void save_delete_step_history(void) { drv_eeprom_fill(0xCEF0, 0x001C, 0); }
