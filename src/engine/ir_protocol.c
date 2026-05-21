#include "all_headers.h"

// ROM: 0x009a  90.9%
void ir_handle_remote_cmd(void) {
  uint8_t action;
  action = REQUESTED_POKEMON_ACTION_TYPE;
  if (action == 0xF0)
    goto case_f0;
  if (action == 0xFE)
    goto case_fe;
  if (action == 0xE0)
    goto case_e0;
  if (action == 0x2A)
    goto case_2a;
  if (action == 0x2C)
    goto case_2c;
  if (action == 0x38)
    goto case_38;
  if (action == 0x4E)
    goto case_4e;
  if (action == 0x5A)
    goto case_5a;
  if (action == 0x66)
    goto case_66;
  if (action == 0x16)
    goto case_16;
  if (action == 0xC0)
    goto case_c0;
  if (action == 0xC2)
    goto case_c2;
  if (action == 0xC4)
    goto case_c4;
  if (action == 0xC6)
    goto case_c6;
  if (action == 0xB8)
    goto case_b8;
  if (action == 0xBA)
    goto case_ba;
  if (action == 0xBC)
    goto case_bc;
  if (action != 0xBE)
    goto check_dat_f7ad;
  goto case_be;

case_f0:
  drv_lcd_init();
  currentlyActiveView = 0x16;
  diag_init_test_mode();
  goto epilogue;
case_fe:
  currentlyActiveView = 0x17;
  sys_init_debug_mode();
  goto epilogue;
case_e0:
  drv_lcd_init();
  idleSeconds = 0xE10;
  sys_factory_reset_eeprom(1, 1);
  goto load_settings;
case_2a:
  idleSeconds = 0xE10;
  sys_factory_reset_eeprom(0, 1);
  goto load_settings;
case_2c:
  idleSeconds = 0xE10;
  sys_factory_reset_eeprom(0, 0);
load_settings:
  drv_sound_set_volume((RamCache_settingsByte >> 1) & 0x3);
  drv_lcd_set_contrast((RamCache_settingsByte >> 3) & 0xF);
  goto set_initial;
case_38:
  RamCache_STEP_COUNT_maybe = 0;
  game_start_walk();
  goto set_view_f;
case_4e:
  game_end_walk();
  ui_set_view(0x10);
  gCurSubstateY = 5;
  goto clear_substate_z;
case_5a:
  game_start_walk();
  drv_eeprom_fill(EEPROM_STEP_HIST_FLAGS, 0x06C8, 0);
set_view_f:
  ui_set_view(0xF);
  gCurSubstateY = 0;
  goto clear_substate_z;
case_66:
  game_clear_stats();
  ui_set_view(0x10);
  gCurSubstateY = 6;
  goto clear_substate_z;
case_16:
  ui_set_view(0xD);
  ui_start_peer_play_app();
  goto epilogue;
case_c0:
  ui_set_view(0x11);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 3;
  goto epilogue;
case_c2:
  ui_set_view(0x11);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 0;
  goto epilogue;
case_c4:
  ui_set_view(0x11);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 2;
  goto set_substate_a;
case_c6:
  ui_set_view(0x11);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 1;
  goto set_substate_a;
case_b8:
  ui_set_view(0x11);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 4;
  goto set_substate_a;
case_ba:
  ui_set_view(0x11);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 5;
  goto set_substate_a;
case_bc:
  ui_set_view(0x11);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 6;
  goto set_substate_a;
case_be:
  ui_set_view(0x11);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 7;
set_substate_a:
  goto epilogue;

check_dat_f7ad:
  if (irResultCode == 0)
    goto set_initial;
  ui_set_view(0xE);
clear_substate_z:
  gCurSubstateZ = 0;
  goto epilogue;

set_initial:
  ui_reset_substate();
  ui_set_view(0);

epilogue:
  accelSampleCount = 0;
  game_reset_pedometer_flags();
  sys_set_handler(sys_main_loop_low_power);
  set_ccr(0x00);
  drv_rtc_get_time((uint8_t *)&rtcSec, (uint8_t *)&rtcMin,
                   (uint8_t *)&rtcHour);
}

// ROM: 0x03b4  55.4%  saves: er2,r3,er4,er5,er6 -> sys_epilogue_0700
void ir_parse_rx_packet(void) {
  uint32_t poke_ptr;
  uint8_t raw;
  uint16_t tens, units, bcd;
  uint8_t *payload;

  payload = drv_ir_get_rx_ptr();
  memcpy(payload, (void *)DAT_f7e6, 0x68);

  raw = DAT_f841;
  if ((raw & 0xF8) < 0xC0) {
    bcd = (uint16_t)(raw >> 3);
    tens = bcd / 10;
    units = bcd - (tens * 10);
    scheduledNotifyHour = (uint8_t)((tens << 4) | units);
  }

  poke_ptr = peerRcvdRtcTime;
  if (poke_ptr != 0) {
    rtcTime = poke_ptr;
    drv_rtc_set_time(poke_ptr);
  }
}

// ROM: 0x0714  50.3%  saves: er3,er4,er5,er6
#pragma option noregexpansion /* pragma:auto */
uint16_t ir_calc_packet_checksum(uint8_t length, uint8_t *data) {
  uint32_t sum;
  uint32_t i;
  uint32_t len;
  uint8_t *ptr;
  uint8_t b;
  uint16_t hi;
  uint16_t lo;

  ptr = data;
  len = (uint32_t)length;
  sum = 0;
  i = 0;

  while (i < len) {
    b = *ptr++;
    if (i & 1) {
      sum += (uint32_t)b;
    } else {
      sum += (uint32_t)b << 8;
    }
    i++;
  }

  hi = (uint16_t)(sum >> 16);
  lo = (uint16_t)(sum);
  sum = (uint32_t)hi + (uint32_t)lo;
  hi = (uint16_t)(sum >> 16);
  sum = (uint32_t)(uint16_t)sum + (uint32_t)hi;

  return (uint16_t)sum;
}

// ROM: 0x08d6  38.1%  saves: er3,er4,er5,er6
#pragma option speed =inline /* pragma:auto */
void ir_comm_loop(void) {
  uint16_t timerDelta;
  uint16_t tcntSnap;
  uint8_t cmdPos_local;
  uint8_t cmdByte;
  uint8_t *pktBase;
  uint8_t cmdLen;
  uint8_t recvByte;
  uint8_t phase;
  uint16_t crcExpected;
  uint16_t crcCalc;

  sys_wdt_kick();
  {
    uint8_t ssr = SSR3 & 0xC4;
    SSR3 = ssr;
  }
  cmdPos_local = commandPos;
  if (SSR3_BIT.RDRF) {
    if (cmdPos_local >= 0x88) {
      rdr_data = RDR3;
      irResultCode = 0x08;
      goto do_action;
    }
    recvByte = RDR3 ^ 0xAA;
    *((uint8_t *)&commandType + cmdPos_local) = recvByte;
    commandPos = cmdPos_local + 1;
    lastCommandTime = TCNT;
    goto finish_no_action;
  }
  timerDelta = (uint16_t)(TCNT - lastCommandTime);
  if (timerDelta <= 4)
    goto finish_no_action;
  if (timerDelta > 0x0C80)
    goto long_timeout;
  if (cmdPos_local == 0)
    goto finish_no_action;
  irPacketReceivedFlag_BIT.b0 = 1;
  if (cmdPos_local == 1) {
    commandPos = 0;
    cmdByte = commandType;
    if (cmdByte != 0xFC)
      goto finish_no_action;
    phase = irHandshakeStep;
    if (phase == 1) {
      irHandshakeStep = 2;
      drv_ir_send_packet(0xFA, 0x00, 2);
    }
    goto finish_no_action;
  }
  pktBase = (uint8_t *)&commandType;
  cmdLen = (uint8_t)cmdPos_local;
  {
    uint8_t savedCrc0 = pktBase[2];
    uint8_t savedCrc1 = pktBase[3];
    crcExpected = (uint16_t)savedCrc0 | ((uint16_t)savedCrc1 << 8);
    pktBase[2] = 0;
    pktBase[3] = 0;
  }
  commandPos = 0;
  crcCalc = ir_calc_packet_checksum(cmdLen, pktBase);
  if (crcCalc != crcExpected) {
    irCrcRetryCount++;
    if (irCrcRetryCount < 0x14) {
      goto finish_no_action;
    }
    irResultCode = 2;
    goto do_action;
  }
  *(uint32_t *)((uint8_t *)&commandType + 2) = *(uint32_t *)(pktBase + 4);
  {
    uint8_t subtype;
    uint8_t pktLen2;
    uint8_t *payload;
    uint16_t e1val;
    uint16_t addr;

    uint8_t e2val;

    subtype = pktBase[1];
    pktLen2 = (uint8_t)(cmdLen - 8);
    payload = drv_ir_get_rx_ptr();
    e1val = irXferRemaining;
    e2val = pktLen2;
    addr = irXferSrc;

    cmdByte = pktBase[0];
    if (cmdByte < 0xF8) {
      if (*(uint32_t *)(pktBase + 4) != sessionKey)
        goto LAB_182e;
      phase = irHandshakeStep;
      if (phase < 3)
        goto LAB_182e;
    }

    switch (cmdByte) {
    case 0xFA:
      if (subtype == 1 || subtype == 2) {
        phase = irHandshakeStep;
        if (phase == 1) {
          irHandshakeStep = 3;
          drv_ir_send_packet(0xF8, 0x00, 2);
          sessionKey = *(uint32_t *)(pktBase + 4);
          sessionKey = nextSessionKey ^ sessionKey;
          goto LAB_182e;
        }
        if (phase == 4 || phase == 3 || phase == 2) {
          goto retry_with_random_delay;
        }
        goto LAB_182e;
      }
      irResultCode = 3;
      goto LAB_14bc;

    case 0xF8:
      if (subtype != 2) {
        irResultCode = 3;
        goto LAB_14bc;
      }
      phase = irHandshakeStep;
      if (phase >= 3) {
        goto LAB_182e;
      }
      sessionKey = *(uint32_t *)(pktBase + 4);
      sessionKey = nextSessionKey ^ sessionKey;
      irHandshakeStep = 4;
      save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
      drv_ir_send_packet(0x10, 0x68, 2);
      gCurSubstateY_BIT.b0 = 0;
      goto LAB_182e;

    case 0x10:
      gCurSubstateY_BIT.b0 = 1;
      memcpy(payload, (void *)DAT_f7e6, 0x68);
      save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
      if (!(*(uint8_t *)(payload + 0x5B) & 0x01)) {
        drv_ir_send_packet(0x12, 0x68, 2);
        irResultCode = 3;
        goto LAB_14bc;
      }
      if (!(DAT_f841 & 0x01)) {
        drv_ir_send_packet(0x12, 0x68, 2);
        irResultCode = 3;
        goto LAB_14bc;
      }
      if (!(DAT_f841 & 0x02)) {
        drv_ir_send_packet(0x12, 0x68, 2);
        irResultCode = 3;
        goto LAB_14bc;
      }
      if (*(uint8_t *)(payload + 0x5C) != DAT_f842) {
        drv_ir_send_packet(0x12, 0x68, 2);
        irResultCode = 3;
        goto LAB_14bc;
      }
      if (DAT_f844 != 0) {
        drv_ir_send_packet(0x12, 0x68, 2);
        irResultCode = 3;
        goto LAB_14bc;
      }
      if (!(*(uint8_t *)(payload + 0x5B) & 0x02)) {
        drv_ir_send_packet(0x12, 0x68, 2);
        irResultCode = 4;
        goto LAB_14bc;
      }
      game_find_seen_peer((void *)(DAT_f7e6 + 0x10));
      drv_ir_send_packet(0x12, 0x68, 2);
      goto LAB_182a_send;

    case 0x12:
      memcpy(payload, (void *)DAT_f7e6, 0x68);
      save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
      if (!(*(uint8_t *)(payload + 0x5B) & 0x01)) {
        irResultCode = 3;
        goto LAB_14bc;
      }
      if (!(DAT_f841 & 0x01)) {
        irResultCode = 3;
        goto LAB_14bc;
      }
      if (!(DAT_f841 & 0x02)) {
        irResultCode = 3;
        goto LAB_14bc;
      }
      if (*(uint8_t *)(payload + 0x5C) != DAT_f842) {
        irResultCode = 3;
        goto LAB_14bc;
      }
      if (DAT_f844 != 0) {
        irResultCode = 3;
        goto LAB_14bc;
      }
      if (!(*(uint8_t *)(payload + 0x5B) & 0x02)) {
        irResultCode = 4;
        goto LAB_14bc;
      }
      game_find_seen_peer((void *)(DAT_f7e6 + 0x10));
      irSessionPhase = 1;
      *(uint16_t *)&irXferSrc = 0x91BE;
      *(uint16_t *)&irXferDst = 0xF400;
      irXferRemaining = 0x180;
      irXferChunkCount = 0;
      goto start_eeprom_tx;

    case 0x14:
      if (gCurSubstateY_BIT.b0) {
        drv_eeprom_write_block(0xF6C0, payload, 0x38);
        *(payload + 0x36) = (uint8_t)((*(payload + 0x36) & ~(0x03 << 5)) |
                                      (((uint8_t)(cmdLen - 8) & 3) << 5));
        drv_ir_send_packet(0x14, 0x38, 2);
      } else {
        drv_eeprom_write_block(irXferDst, payload, 0x80);
        drv_ir_send_packet(0x16, 0x00, 2);
        goto LAB_182a_send;
      }
      goto LAB_182e;

    case 0x16:
      if (gCurSubstateY_BIT.b0) {
        drv_ir_send_packet(0x16, 0x00, 2);
      }
      cmdByte = 0x16;
      goto LAB_1252;

    case 0x1C:
      irResultCode = 5;
      goto LAB_14bc;

    case 0x20:
      save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
      *(uint32_t *)(payload + 0x64) = totalSteps;
      save_write_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
      drv_eeprom_write_block(EEPROM_TRAINER_PROFILE, &watts, 2);
      memcpy(payload, (void *)trainerRecBuf, 0x68);
      drv_ir_send_packet(0x22, 0x68, 2);
      goto LAB_182a_send;

    case 0x32:
      ir_parse_rx_packet();
      save_write_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
      drv_ir_send_packet(0x34, 0x00, 2);
      goto LAB_182a_send;

    case 0x36:
      irResultCode = 3;
      goto LAB_14bc;

    case 0x38:
      drv_ir_send_packet(0x38, 0x00, 2);
      cmdByte = 0x38;
      goto LAB_15e4;

    case 0x40:
      ir_parse_rx_packet();
      save_write_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
      drv_ir_send_packet(0x42, 0x00, 2);
      goto LAB_182a_send;

    case 0x44:
      irResultCode = 3;
      goto LAB_14bc;

    case 0x4E:
      drv_ir_send_packet(0x50, 0x00, 2);
      cmdByte = 0x4E;
      goto LAB_1252;

    case 0x52:
      ir_parse_rx_packet();
      save_write_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
      drv_ir_send_packet(0x54, 0x00, 2);
      goto LAB_182a_send;

    case 0x5A:
      drv_ir_send_packet(0x5A, 0x00, 2);
      cmdByte = 0x5A;
      goto LAB_1252;

    case 0x56:
      irResultCode = 3;
      goto LAB_14bc;

    case 0x60:
      ir_parse_rx_packet();
      save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
      drv_ir_send_packet(0x62, 0x00, 2);
      goto LAB_182a_send;

    case 0x66:
      drv_ir_send_packet(0x68, 0x00, 2);
      cmdByte = 0x66;
      goto LAB_15e4;

    case 0x64:
      irResultCode = 3;
      goto LAB_14bc;

    case 0xC0: {
      uint8_t bf = drv_eeprom_read_u8(e1val);
      bf |= 0x10;
      drv_eeprom_write_u8(e1val, bf);
    }
      save_set_event_bit(NULL, 0);
      drv_ir_send_packet(0xC0, 0x00, 2);
      cmdByte = 0xC0;
      goto LAB_1252;

    case 0xD0: {
      uint8_t bf = drv_eeprom_read_u8(e1val);
      bf |= 0x1F;
      drv_eeprom_write_u8(e1val, bf);
    }
      save_set_event_bit(NULL, 0);
      drv_ir_send_packet(0xC0, 0x00, 2);
      cmdByte = 0xC0;
      goto LAB_1252;

    case 0xC2: {
      uint8_t bf = drv_eeprom_read_u8(e1val);
      bf |= 0x20;
      drv_eeprom_write_u8(e1val, bf);
    }
      save_set_event_bit(NULL, 0);
      drv_ir_send_packet(0xC2, 0x00, 2);
      cmdByte = 0xC2;
      goto LAB_1252;

    case 0xD2: {
      uint8_t bf = drv_eeprom_read_u8(e1val);
      bf |= 0x2F;
      drv_eeprom_write_u8(e1val, bf);
    }
      save_set_event_bit(NULL, 0);
      drv_ir_send_packet(0xC2, 0x00, 2);
      cmdByte = 0xC2;
      goto LAB_1252;

    case 0xC4: {
      uint8_t bf = drv_eeprom_read_u8(e1val);
      bf |= 0x40;
      drv_eeprom_write_u8(e1val, bf);
    }
      save_set_event_bit(NULL, 0);
      drv_ir_send_packet(0xC4, 0x00, 2);
      cmdByte = 0xC4;
      goto LAB_1252;

    case 0xD4: {
      uint8_t bf = drv_eeprom_read_u8(e1val);
      bf |= 0x4F;
      drv_eeprom_write_u8(e1val, bf);
    }
      save_set_event_bit(NULL, 0);
      drv_ir_send_packet(0xC4, 0x00, 2);
      cmdByte = 0xC4;
      goto LAB_1252;

    case 0xC6: {
      uint8_t bf = drv_eeprom_read_u8(e1val);
      bf |= 0x80;
      drv_eeprom_write_u8(e1val, bf);
    }
      save_set_event_bit(NULL, 0);
      RamCache_settingsByte |= 0x01;
      save_write_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
      drv_ir_send_packet(0xC6, 0x00, 2);
      cmdByte = 0xC6;
      goto LAB_1252;

    case 0xD6: {
      uint8_t bf = drv_eeprom_read_u8(e1val);
      bf |= 0x8F;
      drv_eeprom_write_u8(e1val, bf);
    }
      save_set_event_bit(NULL, 0);
      RamCache_settingsByte |= 0x01;
      save_write_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
      drv_ir_send_packet(0xC6, 0x00, 2);
      cmdByte = 0xC6;
      goto LAB_1252;

    case 0xD8:
      irResultCode = 3;
      goto LAB_14bc;

    case 0x24:
      drv_ir_send_packet(0x26, 0x00, 2);
      goto LAB_182a_send;

    case 0x80:
    case 0x00: {
      uint16_t read_addr = e1val + (uint16_t)cmdByte;
      drv_eeprom_read_block(read_addr, (uint8_t *)(uintptr_t)&DAT_f841, 0x80);
      if (cmdByte == 0x80) {
        drv_eeprom_write_page(read_addr, payload);
      } else {
        sys_lzss_decode((uint8_t *)payload, eepromPageScratch);
        drv_eeprom_write_page(read_addr, eepromPageScratch);
      }
      goto LAB_17ee;
    }

    case 0x82:
    case 0x02: {
      uint16_t write_addr = e1val + ((uint16_t)(cmdByte & 0x80));
      drv_eeprom_write_block(write_addr, &DAT_f841, 0x80);
      goto LAB_17ea;
    }

    case 0x04:
      if (e1val == 0) {
        phase = irSessionPhase;
        if (phase == 1)
          goto handle_0x04_phase1;
        if (phase == 3)
          goto handle_0x04_phase3;
        if (phase == 5)
          goto handle_0x04_phase5;
        goto LAB_182e;
      } else {
        irXferRemaining = e1val;
        if (e1val <= 0x80)
          goto start_eeprom_tx;
        goto LAB_1362;
      }

    handle_0x04_phase1:
      irSessionPhase = 3;
      *(uint16_t *)&irXferSrc = 0x993E;
      *(uint16_t *)&irXferDst = (uint16_t)DAT_f580;
      irXferRemaining = 0x140;
      irXferChunkCount = 0;
      goto start_eeprom_tx;

    handle_0x04_phase3:
      irSessionPhase = 5;
      *(uint16_t *)&irXferSrc = 0xCC00;
      *(uint16_t *)&irXferDst = 0xDC00;
      irXferRemaining = 0x224;
      irXferChunkCount = 0;
      goto start_eeprom_tx;

    handle_0x04_phase5:
      irSessionPhase = 2;
      *(uint16_t *)&irXferSrc = 0x91BE;
      *(uint16_t *)&irXferDst = 0xF400;
      irXferRemaining = 0x180;
      irXferChunkCount = 0;
      goto start_eeprom_tx_alt;

    case 0xA0:
    case 0xA2:
    case 0xA4:
    case 0xA6:
      ir_parse_rx_packet();
      {
        uint8_t i;
        uint8_t *dst = drv_ir_get_rx_ptr();
        for (i = 0; i < 0x10; i++) {
          *dst++ = *(uint8_t *)((uint16_t)&DAT_f886 + i);
        }
        *dst = DAT_f840;
      }
      gfx_xor_rect_ram(NULL, 0);
      drv_ir_send_packet(0x9E, 0x11, 2);
      goto LAB_182a_send;

    case 0xA8:
    case 0xAA:
    case 0xAC:
    case 0xAE:
      ir_parse_rx_packet();
      {
        uint8_t i;
        uint8_t *dst = drv_ir_get_rx_ptr();
        for (i = 0; i < 0x10; i++) {
          *dst++ = *(uint8_t *)((uint16_t)&DAT_f886 + i);
        }
        *dst = DAT_f840;
      }
      gfx_xor_rect_ram(NULL, 0);
      drv_ir_send_packet(0x9E, 0x11, 2);
      goto LAB_182a_send;

    case 0xB8:
    case 0xBA:
    case 0xBC:
    case 0xBE: {
      uint8_t flagByte = drv_eeprom_read_u8(e1val);
      if (cmdByte == 0xB8)
        flagByte |= 0x01;
      else if (cmdByte == 0xBA)
        flagByte |= 0x02;
      else if (cmdByte == 0xBC)
        flagByte |= 0x04;
      else if (cmdByte == 0xBE)
        flagByte |= 0x08;
      REQUESTED_POKEMON_ACTION_TYPE = cmdByte;
      drv_eeprom_write_u8(e1val, flagByte);
      drv_ir_send_packet((uint8_t)(cmdByte + 0x10), 0x00, 2);
    }
      goto LAB_14bc;

    case 0x9E:
      drv_ir_send_packet(0x9E, 0x00, 2);
      irResultCode = 7;
      goto LAB_14bc;

    case 0x9C:
      drv_ir_send_packet(0x9C, 0x00, 2);
      irResultCode = 7;
      goto LAB_14bc;

    case 0xF0:
      save_write_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
      drv_eeprom_write_block(addr, payload, 0x80);
      gCurSubstateZ = 1;
      REQUESTED_POKEMON_ACTION_TYPE = 0xF0;
      {
        uint8_t giftType = *(uint8_t *)(payload + 0x70);
        if (giftType == 1) {
          save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
          save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
        }
      }
      drv_ir_send_packet(0xF0, 0x28, 2);
      goto LAB_182a_send;

    case 0xFE:
      if (subtype == 1 && pktLen2 == 8) {
        drv_eeprom_write_block(addr, payload, 0x80);
        drv_ir_send_packet(0xFE, 0x00, 2);
        cmdByte = 0xFE;
        goto LAB_15e4;
      }
      goto LAB_182e;

    case 0x1A:
      drv_eeprom_write_block(addr, payload, 0x80);
      drv_ir_send_packet(0x1A, 0x40, 2);
      break;

    case 0x2A:
      save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
      drv_ir_send_packet(0x2A, 0x28, 2);
      cmdByte = 0x2A;
      goto LAB_15e4;

    case 0x2C:
      save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
      drv_ir_send_packet(0x2A, 0x28, 2);
      cmdByte = 0x2C;
      goto LAB_15e4;

    case 0x0C: {
      uint16_t eaddr = addr + pktBase[2];
      drv_eeprom_read_block(addr, payload, 0x80);
      drv_ir_send_packet(0x0E, pktBase[2], 2);
      goto LAB_182a_send;
    }

    case 0x0E:
      drv_eeprom_write_block(addr, payload, 0x80);
      irXferSrc += e2val;
      irXferDst += e2val;
      irXferRemaining -= e2val;
      irXferChunkCount++;
      if (irXferRemaining == 0) {
        if (irSessionPhase == 2) {
          irSessionPhase = 4;
          *(uint16_t *)&irXferSrc = 0x993E;
          *(uint16_t *)&irXferDst = (uint16_t)DAT_f580;
          irXferRemaining = 0x140;
          irXferChunkCount = 0;
          goto start_eeprom_tx;
        }
      }
      goto LAB_182e;

    case 0x0A:
      drv_eeprom_write_block(addr, payload, 0x80);
      goto LAB_17ee;

    case 0x06: {
      uint8_t i = 0;
      uint8_t *src = payload + 1;
      uint8_t *dst = (uint8_t *)irXferDst;
      while (i < (uint8_t)(pktLen2 - 1)) {
        *dst++ = *src++;
        i++;
      }
      drv_ir_send_packet(subtype, 0x00, 2);
      goto LAB_182a_send;
    }

    default:
      goto LAB_182e;
    }
  }

long_timeout:
  irTimeoutRetryCount++;
  if (irHandshakeStep < 3 && irTimeoutRetryCount < 0x14) {
    goto LAB_0980_retry;
  }
  irResultCode = irPacketReceivedFlag_BIT.b0 ? 2 : 1;
  goto do_action;

LAB_0980_retry: {
  uint32_t r = sys_get_rng();
  r = (r >> 5) & 0x0F;
  r *= 0x60;
  tcntSnap = TCNT;
  while ((uint16_t)(TCNT - tcntSnap) < (uint16_t)r)
    ;
}
  irHandshakeStep = 1;
  drv_ir_tx_u8(0xFC);
  goto LAB_182e;

retry_with_random_delay: {
  uint32_t r = sys_get_rng();
  r = (r >> 5) & 0x0F;
  r *= 0x60;
  tcntSnap = TCNT;
  while ((uint16_t)(TCNT - tcntSnap) < (uint16_t)r)
    ;
}
  irHandshakeStep = 1;
  drv_ir_tx_u8(0xFC);
  goto LAB_182e;

LAB_1362:
start_eeprom_tx:
start_eeprom_tx_alt:
LAB_17b0: {
  uint16_t chunk = (irXferRemaining > 0x80) ? 0x80 : irXferRemaining;
  drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, &watts, 2);
  DAT_f88e[0] = (uint8_t)((watts / 20) & 0xFF);
  drv_eeprom_write_block(EEPROM_TRAINER_PROFILE, &watts, 2);
  {
    uint8_t *p = drv_ir_get_rx_ptr();
    p[0] = (uint8_t)(irXferSrc >> 8);
    p[1] = (uint8_t)(irXferSrc);
    p[2] = (uint8_t)chunk;
  }
  drv_ir_send_packet(0x0C, 0x03, 2);
  goto LAB_182e;
}

LAB_17ea:
  drv_eeprom_write_block(EEPROM_TRAINER_PROFILE, &watts, 2);
LAB_17ee:
  drv_ir_send_packet(0x04, 0x00, 2);
  goto LAB_182e;

LAB_182a_send:
  drv_ir_send_packet(0x00, 0x00, 2);
LAB_15e4:
  REQUESTED_POKEMON_ACTION_TYPE = cmdByte;
  goto LAB_182e;

LAB_1252:
  REQUESTED_POKEMON_ACTION_TYPE = cmdByte;
LAB_14bc:
  drv_ir_finish_and_execute();
  goto LAB_182e;

do_action:
  drv_ir_finish_and_execute();

finish_no_action:
finish_after_tx:
LAB_182e: {
  uint16_t t = TCNT;
  t = (uint16_t)((t << 2) | (t >> 14));
  t &= 1;
  drv_lcd_set_start((uint8_t)t);
}
}
