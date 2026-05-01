#include "all_headers.h"

// ROM: 0x07f2  97.1%
void drv_ir_config_sci(void) {
  uint8_t tmp;
  uint8_t delay;

  CKSTPR1 |= 0x40;
  SPCR = 0x01;
  tmp = SSR3 & 0x84;
  SSR3 = tmp;
  SEMR = 0x00;
  SCR3 = 0x00;
  SMR3 = 0x00;
  BRR3 = 0x00;

  delay = 5;
  while (--delay)
    ;

  SCR3 = 0x10;
  IrCR = 0x80;
  SPCR = 0x11;
  SCR3 = 0x30;
}

// ROM: 0x0822  64.0%
void drv_ir_tx_u8(uint8_t val) {
  uint8_t saved = val;
  while (!SSR3_BIT.TDRE)
    ;
  TDR3 = saved ^ 0xAA;
}

// ROM: 0x0832  90.2%
void drv_ir_init_hw(void) {
  uint8_t tmp;
  drv_ir_config_sci();
  sys_delay_short();
  sys_delay_short();
  PDR3 = 0x00;
  sys_delay_short();
  sys_delay_short();
  CKSTPR2 |= 0x40;
  tmp = TCRW;
  tmp &= 0x8F;
  tmp |= 0x40;
  TCRW = tmp;
  TCRW_BIT.CCLR = 0;
  TIERW_BIT.IMIEA = 0;
  TMRW_BIT.CTS = 1;
  tmp = SSR3 & 0xC4;
  SSR3 = tmp;
  if (SSR3_BIT.RDRF) {
    rdr_data = RDR3;
  }
}

// ROM: 0x075c  97.5%
uint8_t *drv_ir_get_rx_ptr(void) {
  return (uint8_t *)(uintptr_t)&TX_PACKET_payload;
}

// ROM: 0x0762  97.9%
void drv_ir_init_pins(void) {
  DAT_f088 = 0x03;
  PDR3 = 0x01;
  PCR3 = 0x05;
}

// ROM: 0x082e  40.0%
void drv_ir_init_output_pins(void) { drv_ir_init_pins(); }

// ROM: 0x0772  69.5%
#pragma option speed=loop=1  /* pragma:auto */
void drv_ir_send_packet(uint8_t cmdType, uint8_t pktLen, uint8_t subtype) {
  uint8_t *pkt = (uint8_t *)&commandType;
  uint16_t crc;
  uint16_t i;
  uint16_t tcntSnap;

  pkt[0] = cmdType;
  pkt[1] = subtype;
  *(uint32_t *)(pkt + 4) = DAT_f8ba;
  pkt[2] = 0;
  pkt[3] = 0;

  crc = ir_calc_packet_checksum((uint8_t)pktLen + 8, pkt);
  pkt[2] = (uint8_t)(crc);
  pkt[3] = (uint8_t)(crc >> 8);

  i = 0;
  while (i < (uint16_t)pktLen + 8) {
    drv_ir_tx_u8(pkt[i]);
    i++;
  }

  while (!SSR3_BIT.TEND)
    ;

  tcntSnap = TCNT;
  while ((uint16_t)(TCNT - tcntSnap) < 2)
    ;

  if (SSR3_BIT.RDRF) {
    rdr_data = RDR3;
  }
}

// ROM: 0x0880  79.2%
void drv_ir_send_discovery(void) {
  drv_ir_init_hw();
  DAT_f7ad = 0x00;
  DAT_f8b6 = nextRandom;
  DAT_f8ba = DAT_f8b6;
  DAT_f8be = 0x01;
  DAT_f8bf = 0x00;
  DAT_f8c2 = 0x00;
  REQUESTED_POKEMON_ACTION_TYPE = 0xFF;
  *(uint8_t *)&DAT_f8c3 &= ~0x01;
  commandPos = 0x00;
  DAT_f8c1 = 0x00;
  lastCommandTime = TCNT;
  DAT_f8c5 = 0x00;
  drv_ir_tx_u8(0xFC);
}

// ROM: 0x1856  96.4%
void drv_ir_finish_and_execute(void) {
  IRR1 &= ~0x02;
  PDR3 = 0x01;
  SPCR = 0x01;
  SCR3 = 0x00;
  CKSTPR1 &= ~0x40;
  TCRW_BIT.CCLR = 1;
  TMRW_BIT.CTS = 0;
  CKSTPR2 &= ~0x40;
  ir_handle_remote_cmd();
}
