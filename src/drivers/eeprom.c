#include "all_headers.h"

// ROM: 0x4f70  93.6%
void drv_eeprom_spi_init(void) {
  CKSTPR2 |= 0x10;
  SSER = 0;
  if (statusFlags_BIT.lcd_dirty) {
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
  while (1) {
    if (SSSR_BIT.ORER) {
      SSSR_BIT.ORER = 0;
      statusFlags_BIT.eeprom_busy = 1;
      break;
    }
    if (SSSR_BIT.RDRF) {
      break;
    }
  }
  return SSRDR;
}

// ROM: 0x552e  83.5%  saves: er3,er4,er5,er6
uint8_t drv_eeprom_read_u8(uint16_t addr) {
  uint8_t status, val;
  uint8_t retries = 3;
  statusFlags_BIT.eeprom_busy = 0;

  while (retries--) {
    sys_wdt_kick();
    drv_eeprom_spi_init();
    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0xC0;
    PDR1 &= ~0x04;
    while (1) {
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0x05; // RDSR
      drv_eeprom_spi_transfer();
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0xFF;
      status = drv_eeprom_spi_transfer();
      if (!(status & 0x01))
        break;
    }
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x04;

    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0xC0;
    PDR1 &= ~0x04;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x03; // READ
    drv_eeprom_spi_transfer();
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = (uint8_t)(addr >> 8);
    drv_eeprom_spi_transfer();
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = (uint8_t)addr;
    drv_eeprom_spi_transfer();
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0xFF;
    val = drv_eeprom_spi_transfer();

    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x04;
    drv_eeprom_spi_reset();
    if (!(statusFlags_BIT.eeprom_busy))
      return val;
  }
  return 0xFF;
}

// ROM: 0x4fca  79.3%  saves: er3,er4,er5,er6
void drv_eeprom_write_u8(uint16_t addr, uint8_t val) {
  uint8_t status;
  uint8_t retries = 3;
  statusFlags_BIT.eeprom_busy = 0;

  while (retries--) {
    sys_wdt_kick();
    drv_eeprom_spi_init();
    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0xC0;
    PDR1 &= ~0x04;
    while (1) {
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0x05; // RDSR
      drv_eeprom_spi_transfer();
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0xFF;
      status = drv_eeprom_spi_transfer();
      if (!(status & 0x01))
        break;
    }
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x04;

    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0x80;
    PDR1 &= ~0x04;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x06; // WREN
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x04;
    PDR1 &= ~0x04;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x02; // WRITE
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = (uint8_t)(addr >> 8);
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = (uint8_t)addr;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = val;
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x04;
    drv_eeprom_spi_reset();
    if (!(statusFlags_BIT.eeprom_busy))
      return;
  }
}

// ROM: 0x5384  50.3%  saves: er3,er4,er5,er6
void drv_eeprom_read_block(uint16_t addr, void *buf, uint16_t size) {
  uint8_t *p = (uint8_t *)buf;
  uint16_t s;
  uint8_t retries = 3;
  statusFlags_BIT.eeprom_busy = 0;

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
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0x05; // RDSR
      drv_eeprom_spi_transfer();
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0xFF;
      if (!(drv_eeprom_spi_transfer() & 0x01))
        break;
    }
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x04;

    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0xC0;
    PDR1 &= ~0x04;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x03; // READ
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = (uint8_t)(addr >> 8);
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = (uint8_t)addr;
    drv_eeprom_spi_transfer();
    while (s--) {
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0xFF; // Read dummy
      *p++ = drv_eeprom_spi_transfer();
    }
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x04;
    drv_eeprom_spi_reset();
    if (!(statusFlags_BIT.eeprom_busy))
      return;
  }
}

// ROM: 0x524e  78.5%  saves: er3,er4,er5,er6
void drv_eeprom_write_block(uint16_t addr, const void *buf, uint16_t size) {
  const uint8_t *p = (const uint8_t *)buf;
  uint16_t s;
  uint16_t cur_addr = addr;
  uint8_t retries = 3;
  statusFlags_BIT.eeprom_busy = 0;

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
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0x05; // RDSR
        drv_eeprom_spi_transfer();
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0xFF;
        if (!(drv_eeprom_spi_transfer() & 0x01))
          break;
      }
      while (!SSSR_BIT.TEND)
        ;
      PDR1 |= 0x04;

      SSER &= 0x3F;
      SSSR = 0;
      SSER |= 0x80;
      PDR1 &= ~0x04;
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0x06; // WREN
      while (!SSSR_BIT.TEND)
        ;
      PDR1 |= 0x04;
      PDR1 &= ~0x04;
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0x02; // WRITE
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = (uint8_t)(cur_addr >> 8);
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = (uint8_t)cur_addr;
      while (s > 0) {
        while (!SSSR_BIT.TDRE)
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
      while (!SSSR_BIT.TEND)
        ;
      PDR1 |= 0x04;
    }
    drv_eeprom_spi_reset();
    if (!(statusFlags_BIT.eeprom_busy))
      return;
  }
}

// ROM: 0x5742  79.9%  saves: er3,er4,er5,er6
void drv_eeprom_fill(uint16_t addr, uint16_t size, uint8_t val) {
  uint8_t retries = 3;
  statusFlags_BIT.eeprom_busy = 0;

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
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0x05; // RDSR
        drv_eeprom_spi_transfer();
        while (!SSSR_BIT.TDRE)
          ;
        SSTDR = 0xFF;
        if (!(drv_eeprom_spi_transfer() & 0x01))
          break;
      }
      while (!SSSR_BIT.TEND)
        ;
      PDR1 |= 0x04;

      SSER &= 0x3F;
      SSSR = 0;
      SSER |= 0x80;
      PDR1 &= ~0x04;
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0x06; // WREN
      while (!SSSR_BIT.TEND)
        ;
      PDR1 |= 0x04;
      PDR1 &= ~0x04;
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0x02; // WRITE
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = (uint8_t)(cur_addr >> 8);
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = (uint8_t)cur_addr;
      while (s > 0) {
        while (!SSSR_BIT.TDRE)
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
      while (!SSSR_BIT.TEND)
        ;
      PDR1 |= 0x04;
    }
    drv_eeprom_spi_reset();
    if (!(statusFlags_BIT.eeprom_busy))
      return;
  }
}

// ROM: 0x5874  65.9%  saves: er3,er4,er5,er6
void drv_eeprom_write_page(uint16_t addr, const void *buf) {
  const uint8_t *p = (const uint8_t *)buf;
  uint8_t status;
  uint8_t retries = 3;
  uint8_t i;
  statusFlags_BIT.eeprom_busy = 0;

  while (retries--) {
    sys_wdt_kick();
    drv_eeprom_spi_init();
    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0xC0;
    PDR1 &= ~0x04;
    while (1) {
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0x05; // RDSR
      drv_eeprom_spi_transfer();
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0xFF;
      status = drv_eeprom_spi_transfer();
      if (!(status & 0x01))
        break;
    }
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x04;

    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0x80;
    PDR1 &= ~0x04;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x06; // WREN
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x04;
    PDR1 &= ~0x04;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x02; // WRITE
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = (uint8_t)(addr >> 8);
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = (uint8_t)addr;
    for (i = 0; i < 0x80; i++) {
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = *p++;
    }
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x04;
    drv_eeprom_spi_reset();
    if (!(statusFlags_BIT.eeprom_busy))
      return;
  }
}

// ROM: 0x5634  75.8%
void drv_eeprom_write_u8_reliable(uint16_t addr, uint8_t val) {
  uint8_t status;
  uint8_t retries = 3;
  statusFlags_BIT.eeprom_busy = 0;

  while (retries--) {
    drv_eeprom_spi_init();
    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0xC0;
    PDR1 &= ~0x04;
    while (1) {
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0x05; // RDSR
      drv_eeprom_spi_transfer();
      while (!SSSR_BIT.TDRE)
        ;
      SSTDR = 0xFF;
      status = drv_eeprom_spi_transfer();
      if (!(status & 0x01))
        break;
    }
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x04;

    SSER &= 0x3F;
    SSSR = 0;
    SSER |= 0x80;
    PDR1 &= ~0x04;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x06; // WREN
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x04;
    PDR1 &= ~0x04;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = 0x02; // WRITE
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = (uint8_t)(addr >> 8);
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = (uint8_t)addr;
    while (!SSSR_BIT.TDRE)
      ;
    SSTDR = val;
    while (!SSSR_BIT.TEND)
      ;
    PDR1 |= 0x04;
    drv_eeprom_spi_reset();
    if (!(statusFlags_BIT.eeprom_busy))
      return;
  }
}
