#include "all_headers.h"

// ROM: 0x4f70  39.1%
void drv_eeprom_spi_init(void) {
  CKSTPR2 |= 0x10;
  SSER = 0;
  if (statusFlags & 0x10) {
    SSMR = 0x86;
  } else {
    SSMR = 0x87;
  }
}

// ROM: 0x4f92  99.6%
void drv_eeprom_spi_reset(void) {
  SSER = 0x80;
  SSMR = 0x86;
}

// ROM: 0x4fa0  66.7%
uint8_t drv_eeprom_spi_transfer(void) {
  uint8_t status;
  while (1) {
    status = SSSR;
    if (status & 0x40) {
      SSSR &= ~0x40;
      statusFlags |= 0x40;
      break;
    }
    if (status & 0x02) {
      break;
    }
  }
  return SSRDR;
}

// ROM: 0x552e  56.9%
uint8_t drv_eeprom_read_u8(uint16_t addr) {
  uint8_t status, val;
  uint8_t retries = 3;
  statusFlags &= ~0x40;

  while (retries--) {
    sys_wdt_kick();
    drv_eeprom_spi_init();
    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0xC0;
    PDR1 &= ~0x04;
    while (1) {
      while (!(SSSR & 0x04))
        ;
      SSTDR = 0x05; // RDSR
      drv_eeprom_spi_transfer();
      while (!(SSSR & 0x04))
        ;
      SSTDR = 0xFF;
      status = drv_eeprom_spi_transfer();
      if (!(status & 0x01))
        break;
    }
    while (!(SSSR & 0x08))
      ;
    PDR1 |= 0x04;

    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0xC0;
    PDR1 &= ~0x04;
    while (!(SSSR & 0x04))
      ;
    SSTDR = 0x03; // READ
    drv_eeprom_spi_transfer();
    while (!(SSSR & 0x04))
      ;
    SSTDR = (uint8_t)(addr >> 8);
    drv_eeprom_spi_transfer();
    while (!(SSSR & 0x04))
      ;
    SSTDR = (uint8_t)addr;
    drv_eeprom_spi_transfer();
    while (!(SSSR & 0x04))
      ;
    SSTDR = 0xFF;
    val = drv_eeprom_spi_transfer();

    while (!(SSSR & 0x08))
      ;
    PDR1 |= 0x04;
    drv_eeprom_spi_reset();
    if (!(statusFlags & 0x40))
      return val;
  }
  return 0xFF;
}

// ROM: 0x4fca  51.5%
void drv_eeprom_write_u8(uint16_t addr, uint8_t val) {
  uint8_t status;
  uint8_t retries = 3;
  statusFlags &= ~0x40;

  while (retries--) {
    sys_wdt_kick();
    drv_eeprom_spi_init();
    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0xC0;
    PDR1 &= ~0x04;
    while (1) {
      while (!(SSSR & 0x04))
        ;
      SSTDR = 0x05; // RDSR
      drv_eeprom_spi_transfer();
      while (!(SSSR & 0x04))
        ;
      SSTDR = 0xFF;
      status = drv_eeprom_spi_transfer();
      if (!(status & 0x01))
        break;
    }
    while (!(SSSR & 0x08))
      ;
    PDR1 |= 0x04;

    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0x80;
    PDR1 &= ~0x04;
    while (!(SSSR & 0x04))
      ;
    SSTDR = 0x06; // WREN
    while (!(SSSR & 0x08))
      ;
    PDR1 |= 0x04;
    PDR1 &= ~0x04;
    while (!(SSSR & 0x04))
      ;
    SSTDR = 0x02; // WRITE
    drv_eeprom_spi_transfer();
    while (!(SSSR & 0x04))
      ;
    SSTDR = (uint8_t)(addr >> 8);
    drv_eeprom_spi_transfer();
    while (!(SSSR & 0x04))
      ;
    SSTDR = (uint8_t)addr;
    drv_eeprom_spi_transfer();
    while (!(SSSR & 0x04))
      ;
    SSTDR = val;
    while (!(SSSR & 0x08))
      ;
    PDR1 |= 0x04;
    drv_eeprom_spi_reset();
    if (!(statusFlags & 0x40))
      return;
  }
}

// ROM: 0x5384  42.4%
void drv_eeprom_read_block(uint16_t addr, void *buf, uint16_t size) {
  uint8_t *p = (uint8_t *)buf;
  uint16_t s;
  uint8_t retries = 3;
  statusFlags &= ~0x40;

  while (retries--) {
    p = (uint8_t *)buf;
    s = size;
    sys_wdt_kick();
    drv_eeprom_spi_init();
    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0xC0;
    PDR1 &= ~0x04;
    while (1) {
      while (!(SSSR & 0x04))
        ;
      SSTDR = 0x05; // RDSR
      drv_eeprom_spi_transfer();
      while (!(SSSR & 0x04))
        ;
      SSTDR = 0xFF;
      if (!(drv_eeprom_spi_transfer() & 0x01))
        break;
    }
    while (!(SSSR & 0x08))
      ;
    PDR1 |= 0x04;

    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0xC0;
    PDR1 &= ~0x04;
    while (!(SSSR & 0x04))
      ;
    SSTDR = 0x03; // READ
    while (!(SSSR & 0x04))
      ;
    SSTDR = (uint8_t)(addr >> 8);
    while (!(SSSR & 0x04))
      ;
    SSTDR = (uint8_t)addr;
    drv_eeprom_spi_transfer();
    while (s--) {
      while (!(SSSR & 0x04))
        ;
      SSTDR = 0xFF; // Read dummy
      *p++ = drv_eeprom_spi_transfer();
    }
    while (!(SSSR & 0x08))
      ;
    PDR1 |= 0x04;
    drv_eeprom_spi_reset();
    if (!(statusFlags & 0x40))
      return;
  }
}

// ROM: 0x524e  43.9%
void drv_eeprom_write_block(uint16_t addr, const void *buf, uint16_t size) {
  const uint8_t *p = (const uint8_t *)buf;
  uint16_t s;
  uint16_t cur_addr = addr;
  uint8_t retries = 3;
  statusFlags &= ~0x40;

  while (retries--) {
    p = (const uint8_t *)buf;
    s = size;
    cur_addr = addr;
    while (s > 0) {
      uint8_t page_written = 0;
      sys_wdt_kick();
      drv_eeprom_spi_init();
      SSER &= 0x3F;
      SSSR = 0;
      SSER |= 0xC0;
      PDR1 &= ~0x04;
      while (1) {
        while (!(SSSR & 0x04))
          ;
        SSTDR = 0x05; // RDSR
        drv_eeprom_spi_transfer();
        while (!(SSSR & 0x04))
          ;
        SSTDR = 0xFF;
        if (!(drv_eeprom_spi_transfer() & 0x01))
          break;
      }
      while (!(SSSR & 0x08))
        ;
      PDR1 |= 0x04;

      SSER &= 0x3F;
      SSSR = 0;
      SSER |= 0x80;
      PDR1 &= ~0x04;
      while (!(SSSR & 0x04))
        ;
      SSTDR = 0x06; // WREN
      while (!(SSSR & 0x08))
        ;
      PDR1 |= 0x04;
      PDR1 &= ~0x04;
      while (!(SSSR & 0x04))
        ;
      SSTDR = 0x02; // WRITE
      drv_eeprom_spi_transfer();
      while (!(SSSR & 0x04))
        ;
      SSTDR = (uint8_t)(cur_addr >> 8);
      drv_eeprom_spi_transfer();
      while (!(SSSR & 0x04))
        ;
      SSTDR = (uint8_t)cur_addr;
      drv_eeprom_spi_transfer();
      while (s > 0) {
        while (!(SSSR & 0x04))
          ;
        SSTDR = *p++;
        cur_addr++;
        s--;
        page_written++;
        if ((cur_addr & 0x7F) == 0)
          break;
        if (page_written >= 0x80)
          break;
      }
      while (!(SSSR & 0x08))
        ;
      PDR1 |= 0x04;
    }
    drv_eeprom_spi_reset();
    if (!(statusFlags & 0x40))
      return;
  }
}

// ROM: 0x5742  51.6%
void drv_eeprom_fill(uint16_t addr, uint16_t size, uint8_t val) {
  uint8_t retries = 3;
  statusFlags &= ~0x40;

  while (retries--) {
    uint16_t s = size;
    uint32_t cur_addr = addr;
    while (s > 0) {
      uint8_t page_written = 0;
      sys_wdt_kick();
      drv_eeprom_spi_init();
      SSER &= 0x3F;
      SSSR = 0;
      SSER |= 0xC0;
      PDR1 &= ~0x04;
      while (1) {
        while (!(SSSR & 0x04))
          ;
        SSTDR = 0x05; // RDSR
        drv_eeprom_spi_transfer();
        while (!(SSSR & 0x04))
          ;
        SSTDR = 0xFF;
        if (!(drv_eeprom_spi_transfer() & 0x01))
          break;
      }
      while (!(SSSR & 0x08))
        ;
      PDR1 |= 0x04;

      SSER &= 0x3F;
      SSSR = 0;
      SSER |= 0x80;
      PDR1 &= ~0x04;
      while (!(SSSR & 0x04))
        ;
      SSTDR = 0x06; // WREN
      while (!(SSSR & 0x08))
        ;
      PDR1 |= 0x04;
      PDR1 &= ~0x04;
      while (!(SSSR & 0x04))
        ;
      SSTDR = 0x02; // WRITE
      drv_eeprom_spi_transfer();
      while (!(SSSR & 0x04))
        ;
      SSTDR = (uint8_t)(cur_addr >> 8);
      drv_eeprom_spi_transfer();
      while (!(SSSR & 0x04))
        ;
      SSTDR = (uint8_t)cur_addr;
      drv_eeprom_spi_transfer();
      while (s > 0) {
        while (!(SSSR & 0x04))
          ;
        SSTDR = val;
        cur_addr++;
        s--;
        page_written++;
        if ((cur_addr & 0x7F) == 0)
          break;
        if (page_written >= 0x80)
          break;
      }
      while (!(SSSR & 0x08))
        ;
      PDR1 |= 0x04;
    }
    drv_eeprom_spi_reset();
    if (!(statusFlags & 0x40))
      return;
  }
}

// ROM: 0x5874  52.8%
void drv_eeprom_write_page(uint16_t addr, const void *buf) {
  const uint8_t *p = (const uint8_t *)buf;
  uint8_t status;
  uint8_t retries = 3;
  uint8_t i;
  statusFlags &= ~0x40;

  while (retries--) {
    sys_wdt_kick();
    drv_eeprom_spi_init();
    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0xC0;
    PDR1 &= ~0x04;
    while (1) {
      while (!(SSSR & 0x04))
        ;
      SSTDR = 0x05; // RDSR
      drv_eeprom_spi_transfer();
      while (!(SSSR & 0x04))
        ;
      SSTDR = 0xFF;
      status = drv_eeprom_spi_transfer();
      if (!(status & 0x01))
        break;
    }
    while (!(SSSR & 0x08))
      ;
    PDR1 |= 0x04;

    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0x80;
    PDR1 &= ~0x04;
    while (!(SSSR & 0x04))
      ;
    SSTDR = 0x06; // WREN
    while (!(SSSR & 0x08))
      ;
    PDR1 |= 0x04;
    PDR1 &= ~0x04;
    while (!(SSSR & 0x04))
      ;
    SSTDR = 0x02; // WRITE
    drv_eeprom_spi_transfer();
    while (!(SSSR & 0x04))
      ;
    SSTDR = (uint8_t)(addr >> 8);
    drv_eeprom_spi_transfer();
    while (!(SSSR & 0x04))
      ;
    SSTDR = (uint8_t)addr;
    drv_eeprom_spi_transfer();
    for (i = 0; i < 0x80; i++) {
      while (!(SSSR & 0x04))
        ;
      SSTDR = *p++;
    }
    while (!(SSSR & 0x08))
      ;
    PDR1 |= 0x04;
    drv_eeprom_spi_reset();
    if (!(statusFlags & 0x40))
      return;
  }
}

// ROM: 0x5634  54.2%
void drv_eeprom_write_u8_reliable(uint16_t addr, uint8_t val) {
  uint8_t status;
  uint8_t retries = 3;
  statusFlags &= ~0x40;

  while (retries--) {
    drv_eeprom_spi_init();
    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0xC0;
    PDR1 &= ~0x04;
    while (1) {
      while (!(SSSR & 0x04))
        ;
      SSTDR = 0x05; // RDSR
      drv_eeprom_spi_transfer();
      while (!(SSSR & 0x04))
        ;
      SSTDR = 0xFF;
      status = drv_eeprom_spi_transfer();
      if (!(status & 0x01))
        break;
    }
    while (!(SSSR & 0x08))
      ;
    PDR1 |= 0x04;

    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0x80;
    PDR1 &= ~0x04;
    while (!(SSSR & 0x04))
      ;
    SSTDR = 0x06; // WREN
    while (!(SSSR & 0x08))
      ;
    PDR1 |= 0x04;
    PDR1 &= ~0x04;
    while (!(SSSR & 0x04))
      ;
    SSTDR = 0x02; // WRITE
    drv_eeprom_spi_transfer();
    while (!(SSSR & 0x04))
      ;
    SSTDR = (uint8_t)(addr >> 8);
    drv_eeprom_spi_transfer();
    while (!(SSSR & 0x04))
      ;
    SSTDR = (uint8_t)addr;
    drv_eeprom_spi_transfer();
    while (!(SSSR & 0x04))
      ;
    SSTDR = val;
    while (!(SSSR & 0x08))
      ;
    PDR1 |= 0x04;
    drv_eeprom_spi_reset();
    if (!(statusFlags & 0x40))
      return;
  }
}
