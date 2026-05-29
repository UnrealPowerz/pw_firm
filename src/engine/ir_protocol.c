#include "all_headers.h"

// ROM: 0x009a  90.9%
void ir_handle_remote_cmd(void) {
  switch (REQUESTED_POKEMON_ACTION_TYPE) {
  case 0xF0: goto enter_test_mode;
  case 0xFE: goto enter_debug_mode;
  case 0xE0: goto factory_reset_full;
  case 0x2A: goto factory_reset_partial;
  case 0x2C: goto factory_reset_minimal;
  case 0x38: goto start_new_walk;
  case 0x4E: goto end_walk_show_report;
  case 0x5A: goto restart_walk_clear_history;
  case 0x66: goto clear_walk_stats;
  case 0x16: goto start_peer_play;
  case 0xC0: goto show_menu_a3;
  case 0xC2: goto show_menu_a0;
  case 0xC4: goto show_menu_a2;
  case 0xC6: goto show_menu_a1;
  case 0xB8: goto show_menu_a4;
  case 0xBA: goto show_menu_a5;
  case 0xBC: goto show_menu_a6;
  case 0xBE: goto show_menu_a7;
  default:   goto default_handle_error;
  }

enter_test_mode:
  drv_lcd_init();
  currentlyActiveView = VIEW_DEBUG;
  diag_init_test_mode();
  goto finalize;
enter_debug_mode:
  currentlyActiveView = VIEW_ACCEL_DEBUG;
  sys_init_debug_mode();
  goto finalize;
factory_reset_full:
  drv_lcd_init();
  idleSeconds = 0xE10;
  sys_factory_reset_eeprom(1, 1);
  goto apply_volume_and_contrast;
factory_reset_partial:
  idleSeconds = 0xE10;
  sys_factory_reset_eeprom(0, 1);
  goto apply_volume_and_contrast;
factory_reset_minimal:
  idleSeconds = 0xE10;
  sys_factory_reset_eeprom(0, 0);
apply_volume_and_contrast:
  drv_sound_set_volume((RamCache_settingsByte >> 1) & 0x3);
  drv_lcd_set_contrast((RamCache_settingsByte >> 3) & 0xF);
  goto return_to_main_view;
start_new_walk:
  RamCache_STEP_COUNT_maybe = 0;
  game_start_walk();
  goto enter_walk_view;
end_walk_show_report:
  game_end_walk();
  ui_set_view(VIEW_WALK_DEPARTURE_ANIM);
  gCurSubstateY = 5;
  goto reset_substate_z;
restart_walk_clear_history:
  game_start_walk();
  drv_eeprom_fill(EEPROM_STEP_HIST_FLAGS, 0x06C8, 0);
enter_walk_view:
  ui_set_view(VIEW_WALK_ARRIVAL_ANIM);
  gCurSubstateY = 0;
  goto reset_substate_z;
clear_walk_stats:
  game_clear_stats();
  ui_set_view(VIEW_WALK_DEPARTURE_ANIM);
  gCurSubstateY = 6;
  goto reset_substate_z;
start_peer_play:
  ui_set_view(VIEW_PEER_PLAY);
  ui_start_peer_play_app();
  goto finalize;
show_menu_a3:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 3;
  goto finalize;
show_menu_a0:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 0;
  goto finalize;
show_menu_a2:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 2;
  goto finalize;
show_menu_a1:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 1;
  goto finalize;
show_menu_a4:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 4;
  goto finalize;
show_menu_a5:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 5;
  goto finalize;
show_menu_a6:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 6;
  goto finalize;
show_menu_a7:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  gCurSubstateY = 0;
  gCurSubstateZ = 0;
  gCurSubstateA = 7;
  goto finalize;

default_handle_error:
  if (irResultCode == 0)
    goto return_to_main_view;
  ui_set_view(VIEW_STEP_HISTORY);
reset_substate_z:
  gCurSubstateZ = 0;
  goto finalize;

return_to_main_view:
  ui_reset_substate();
  ui_set_view(VIEW_HOME);

finalize:
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

// ROM: 0x08d6  64.4%  saves: er3,er4,er5,er6
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
    commandPos = cmdPos_local + 1;
    *((uint8_t *)&commandType + cmdPos_local) = RDR3 ^ 0xAA;
    lastCommandTime = TCNT;
    goto finish_no_action;
  }
  timerDelta = (uint16_t)(TCNT - lastCommandTime);
  if (timerDelta <= 4)
    goto finish_no_action;
  if (timerDelta > 0x0C80) {
    irTimeoutRetryCount++;
    if (irHandshakeStep < 3 && irTimeoutRetryCount < 0x14) {
      uint32_t r = sys_get_rng();
      r = (r >> 5) & 0x0F;
      r *= 0x60;
      tcntSnap = TCNT;
      while ((uint16_t)(TCNT - tcntSnap) < (uint16_t)r)
        ;
      irHandshakeStep = 1;
      drv_ir_tx_u8(0xFC);
      goto LAB_182e;
    }
    irResultCode = irPacketReceivedFlag_BIT.b0 ? 2 : 1;
    goto do_action;
  }
  if (cmdPos_local == 0)
    goto finish_no_action;
  irPacketReceivedFlag_BIT.b0 = 1;
  if (cmdPos_local == 1) {
    commandPos = 0;
    cmdByte = commandType;
    if (cmdByte != 0xFC)
      goto finish_no_action;
    phase = irHandshakeStep;
    switch (phase) {
    case 1:
      irHandshakeStep = 2;
      drv_ir_send_packet(0xFA, 0x00, 2);
      break;
    case 2:
    case 4:
    case 3:
    default:
      break;
    }
    goto finish_no_action;
  }
  pktBase = (uint8_t *)&commandType;
  cmdLen = (uint8_t)cmdPos_local;
  crcExpected = ((uint16_t)pktBase[3] << 8) | pktBase[2];
  pktBase[2] = 0;
  pktBase[3] = 0;
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
        switch (phase) {
        case 1:
          irHandshakeStep = 3;
          drv_ir_send_packet(0xF8, 0x00, 2);
          sessionKey = *(uint32_t *)(pktBase + 4);
          sessionKey = nextSessionKey ^ sessionKey;
          goto LAB_182e;
        case 4:
        case 3:
        case 2: {
          uint32_t r = sys_get_rng();
          r = (r >> 5) & 0x0F;
          r *= 0x60;
          tcntSnap = TCNT;
          while ((uint16_t)(TCNT - tcntSnap) < (uint16_t)r)
            ;
          irHandshakeStep = 1;
          drv_ir_tx_u8(0xFC);
          lastCommandTime = TCNT;
          goto LAB_182e;
        }
        default:
          goto LAB_182e;
        }
      }
      irResultCode = 3;
      goto LAB_14bc;

    case 0xF4:
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
      if (!((byte_bits_t *)&payload[0x5B])->BIT.b0) {
        drv_ir_send_packet(0x12, 0x68, 2);
        irResultCode = 3;
        goto LAB_14bc;
      }
      if (!((byte_bits_t *)&DAT_f841)->BIT.b0) {
        drv_ir_send_packet(0x12, 0x68, 2);
        irResultCode = 3;
        goto LAB_14bc;
      }
      if (!((byte_bits_t *)&DAT_f841)->BIT.b1) {
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
      if (!((byte_bits_t *)&payload[0x5B])->BIT.b1) {
        drv_ir_send_packet(0x12, 0x68, 2);
        irResultCode = 4;
        goto LAB_14bc;
      }
      if (game_find_seen_peer((void *)(DAT_f7e6 + 0x10)) != 0) {
        drv_ir_send_packet(0x1C, 0x00, 2);
        irResultCode = 5;
        goto LAB_14bc;
      }
      drv_ir_send_packet(0x12, 0x68, 2);
      goto LAB_182e;

    case 0x12:
      memcpy(payload, (void *)DAT_f7e6, 0x68);
      save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
      if (!((byte_bits_t *)&payload[0x5B])->BIT.b0) {
        irResultCode = 3;
        goto LAB_14bc;
      }
      if (!((byte_bits_t *)&DAT_f841)->BIT.b0) {
        irResultCode = 3;
        goto LAB_14bc;
      }
      if (!((byte_bits_t *)&DAT_f841)->BIT.b1) {
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
      if (!((byte_bits_t *)&payload[0x5B])->BIT.b1) {
        irResultCode = 4;
        goto LAB_14bc;
      }
      if (game_find_seen_peer((void *)(DAT_f7e6 + 0x10)) != 0) {
        drv_ir_send_packet(0x1C, 0x00, 2);
        irResultCode = 5;
        goto LAB_14bc;
      }
      irSessionPhase = 1;
      *(uint16_t *)&irXferSrc = 0x91BE;
      *(uint16_t *)&irXferDst = 0xF400;
      irXferRemaining = 0x180;
      irXferChunkCount = 0;
      goto start_eeprom_tx;

    case 0x14:
      if (gCurSubstateY_BIT.b0) {
        uint8_t *rxptr;
        uint8_t i;
        drv_eeprom_write_block(0xF6C0, payload, 0x38);
        rxptr = drv_ir_get_rx_ptr();
        *(uint32_t *)rxptr = sessionSteps;
        *(uint16_t *)(rxptr + 4) = recentSessionSteps;
        drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, eepromPageScratch, 0x10);
        ((byte_bits_t *)&rxptr[0x37])->BIT.b0 =
            ((byte_bits_t *)&eepromPageScratch[0x0E])->BIT.b0;
        *(uint16_t *)(rxptr + 0x0E) = *(uint16_t *)eepromPageScratch;
        rxptr[0x36] = (uint8_t)((rxptr[0x36] & 0xE0) | (eepromPageScratch[0x0D] & 0x1F));
        rxptr[0x36] = (uint8_t)((rxptr[0x36] & ~(0x03 << 5)) |
                                (eepromPageScratch[0x0D] & 0x60));
        ((byte_bits_t *)&rxptr[0x36])->BIT.b7 =
            ((byte_bits_t *)&eepromPageScratch[0x0E])->BIT.b1;
        save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)trainerRecBuf, 0x68);
        *(uint32_t *)(rxptr + 0x08) = *(uint32_t *)trainerRecBuf;
        *(uint16_t *)(rxptr + 0x0C) = trainerRecBuf_loc;
        for (i = 0; i < 0x10; i++) {
          rxptr[0x26 + i] = ((uint8_t *)&DAT_f896)[i];
        }
        drv_eeprom_read_block(0x8F10, rxptr + 0x10, 0x16);
        drv_ir_send_packet(0x14, 0x38, 2);
      } else {
        drv_eeprom_write_block(0xF6C0, payload, 0x38);
        drv_ir_send_packet(0x16, 0x00, 2);
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
      save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, payload, 0x68);
      *(uint32_t *)(payload + 0x64) = totalSteps;
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
      drv_eeprom_write_block(0xCE8A, &watts, 2);
      memcpy(payload, (void *)trainerRecBuf, 0x68);
      drv_ir_send_packet(0x22, 0x68, 2);
      goto LAB_182e;

    case 0x32:
      ir_parse_rx_packet();
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
      drv_ir_send_packet(0x34, 0x00, 2);
      goto LAB_182e;

    case 0x36:
      irResultCode = 3;
      goto LAB_14bc;

    case 0x38:
      drv_ir_send_packet(0x38, 0x00, 2);
      cmdByte = 0x38;
      goto LAB_15e4;

    case 0x40:
      ir_parse_rx_packet();
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
      drv_ir_send_packet(0x42, 0x00, 2);
      goto LAB_182e;

    case 0x44:
      irResultCode = 3;
      goto LAB_14bc;

    case 0x4E:
      drv_ir_send_packet(0x50, 0x00, 2);
      cmdByte = 0x4E;
      goto LAB_1252;

    case 0x52:
      ir_parse_rx_packet();
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
      drv_ir_send_packet(0x54, 0x00, 2);
      goto LAB_182e;

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
      goto LAB_182e;

    case 0x66:
      drv_ir_send_packet(0x68, 0x00, 2);
      cmdByte = 0x66;
      goto LAB_15e4;

    case 0x64:
      irResultCode = 3;
      goto LAB_14bc;

    case 0xC0: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x10;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)trainerRecBuf, DAT_f840);
      drv_ir_send_packet(0xC0, 0x00, 2);
      cmdByte = 0xC0;
      goto LAB_1252;

    case 0xD0: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x1F;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)trainerRecBuf, DAT_f840);
      drv_ir_send_packet(0xC0, 0x00, 2);
      cmdByte = 0xC0;
      goto LAB_1252;

    case 0xC2: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x20;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)trainerRecBuf, DAT_f840);
      drv_ir_send_packet(0xC2, 0x00, 2);
      cmdByte = 0xC2;
      goto LAB_1252;

    case 0xD2: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x2F;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)trainerRecBuf, DAT_f840);
      drv_ir_send_packet(0xC2, 0x00, 2);
      cmdByte = 0xC2;
      goto LAB_1252;

    case 0xC4: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x40;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)trainerRecBuf, DAT_f840);
      drv_ir_send_packet(0xC4, 0x00, 2);
      cmdByte = 0xC4;
      goto LAB_1252;

    case 0xD4: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x4F;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)trainerRecBuf, DAT_f840);
      drv_ir_send_packet(0xC4, 0x00, 2);
      cmdByte = 0xC4;
      goto LAB_1252;

    case 0xC6: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x80;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)trainerRecBuf, DAT_f840);
      RamCache_settingsByte |= 0x01;
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
      drv_ir_send_packet(0xC6, 0x00, 2);
      cmdByte = 0xC6;
      goto LAB_1252;

    case 0xD6: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x8F;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)trainerRecBuf, DAT_f840);
      RamCache_settingsByte |= 0x01;
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&totalSteps, 0x18);
      drv_ir_send_packet(0xC6, 0x00, 2);
      cmdByte = 0xC6;
      goto LAB_1252;

    case 0xD8:
      irResultCode = 3;
      goto LAB_14bc;

    case 0x24:
      drv_ir_send_packet(0x26, 0x00, 2);
      goto LAB_182e;

    case 0x80:
    case 0x00: {
      uint16_t write_addr = ((uint16_t)subtype << 8) | (uint16_t)cmdByte;
      if (pktLen2 == 0x80) {
        drv_eeprom_write_page(write_addr, payload);
      } else {
        sys_lzss_decode((uint8_t *)payload, eepromPageScratch);
        drv_eeprom_write_page(write_addr, eepromPageScratch);
      }
      goto LAB_17ee;
    }

    case 0x82:
    case 0x02: {
      uint16_t write_addr = ((uint16_t)subtype << 8) | (uint16_t)(cmdByte & 0x80);
      drv_eeprom_write_block(write_addr, payload, pktLen2);
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
          *dst++ = ((uint8_t *)&DAT_f886)[i];
        }
        *dst = DAT_f840;
      }
      if (gfx_xor_rect_ram((void *)trainerRecBuf, DAT_f840) != 0) {
        drv_ir_send_packet(0x9E, 0x11, 2);
        irResultCode = 6;
        goto LAB_14bc;
      }
      drv_ir_send_packet(cmdByte, 0x11, 2);
      goto LAB_182e;

    case 0xA8:
    case 0xAA:
    case 0xAC:
    case 0xAE:
      ir_parse_rx_packet();
      {
        uint8_t i;
        uint8_t *dst = drv_ir_get_rx_ptr();
        for (i = 0; i < 0x10; i++) {
          *dst++ = ((uint8_t *)&DAT_f886)[i];
        }
        *dst = DAT_f840;
      }
      if (gfx_xor_rect_ram((void *)trainerRecBuf, DAT_f840) != 0) {
        drv_ir_send_packet(0x9E, 0x11, 2);
        irResultCode = 6;
        goto LAB_14bc;
      }
      drv_ir_send_packet(cmdByte, 0x11, 2);
      goto LAB_182e;

    case 0xB8:
    case 0xBA:
    case 0xBC:
    case 0xBE: {
      uint8_t flagByte = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      switch (cmdByte) {
      case 0xB8: flagByte |= 0x01; break;
      case 0xBA: flagByte |= 0x02; break;
      case 0xBC: flagByte |= 0x04; break;
      case 0xBE: flagByte |= 0x08; break;
      }
      REQUESTED_POKEMON_ACTION_TYPE = cmdByte;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, flagByte);
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

    case 0xF0: {
      save_write_reliable(EEPROM_RESV_0083, EEPROM_RESV_0083_BACKUP, payload, 0x28);
      drv_eeprom_write_block(0x0008, payload + 0x68, 0x08);
      gCurSubstateZ = 1;
      REQUESTED_POKEMON_ACTION_TYPE = 0xF0;
      switch (payload[0x70]) {
      case 0:
        save_write_reliable(EEPROM_LCD_INIT_SEQ, EEPROM_LCD_INIT_SEQ_BACKUP,
                            payload + 0x28, 0x40);
        break;
      case 1: {
        uint8_t i = 0;
        save_read_reliable(EEPROM_LCD_INIT_SEQ, EEPROM_LCD_INIT_SEQ_BACKUP,
                           eepromPageScratch, 0x40);
        do {
          if (payload[0x28 + i] != eepromPageScratch[i]) {
            gCurSubstateZ = 0;
            break;
          }
          i++;
        } while (i < 0x40);
        break;
      }
      case 2:
        break;
      case 3:
        save_write_reliable(EEPROM_LCD_INIT_SEQ, EEPROM_LCD_INIT_SEQ_BACKUP,
                            payload + 0x28, 0x40);
        REQUESTED_POKEMON_ACTION_TYPE = 0xE0;
        break;
      default:
        break;
      }
      drv_ir_send_packet(0xF0, 0x28, 2);
      goto LAB_182e;
    }

    case 0xFE:
      if (subtype == 1 && pktLen2 == 8) {
        drv_eeprom_write_block(0x0008, payload, 0x08);
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
      save_read_reliable(EEPROM_RESV_0083, EEPROM_RESV_0083_BACKUP, payload, 0x28);
      drv_ir_send_packet(0x2A, 0x28, 2);
      cmdByte = 0x2A;
      goto LAB_15e4;

    case 0x2C:
      save_read_reliable(EEPROM_RESV_0083, EEPROM_RESV_0083_BACKUP, payload, 0x28);
      drv_ir_send_packet(0x2A, 0x28, 2);
      cmdByte = 0x2C;
      goto LAB_15e4;

    case 0x0C: {
      uint16_t eaddr = ((uint16_t)pktBase[0] << 8) | pktBase[1];
      uint8_t chunk = pktBase[2];
      drv_eeprom_read_block(eaddr, payload, chunk);
      drv_ir_send_packet(0x0E, chunk, 2);
      goto LAB_182e;
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
      drv_eeprom_write_block(((uint16_t)subtype << 8) | pktBase[0], payload + 1, (uint16_t)(pktLen2 - 1));
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
      goto LAB_182e;
    }

    default:
      goto LAB_182e;
    }
  }

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

finish_after_tx:
LAB_182e: {
  uint16_t t = TCNT;
  t = (uint16_t)((t << 2) | (t >> 14));
  t &= 1;
  drv_lcd_set_start((uint8_t)t);
  commandPos = 0;
  lastCommandTime = TCNT;
}
finish_no_action: ;
}
