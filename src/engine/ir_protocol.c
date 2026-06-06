#include "all_headers.h"

/*
 * IR (infrared) communication protocol.
 *
 *   sys_begin_ir_session       Entry point — kicks off the IR connection app.
 *                              Stops any in-progress sound, renders the
 *                              connect-screen, disables interrupts, sends the
 *                              IR discovery probe, and installs ir_comm_loop
 *                              as the foreground event-loop handler. Guarded:
 *                              only allowed from the low-power loop.
 *
 *   ir_handle_remote_cmd       Post-session action dispatcher. After the IR
 *                              loop completes, this runs the action stored in
 *                              g.scratch2.ir.requestedPokemonAction (factory reset,
 *                              start/end walk, peer play, event reward, etc.).
 *
 *   ir_parse_rx_packet         Decode a received 0x68-byte payload into the
 *                              shared DAT_f7e6 record + extract BCD-encoded
 *                              scheduled-notify hour + sync RTC if a peer
 *                              clock value is present.
 *
 *   ir_calc_packet_checksum    16-bit one's-complement-style sum over the
 *                              packet bytes. Alternating bytes go into the
 *                              high/low byte of the running 32-bit sum, then
 *                              the upper/lower halves are folded back.
 *
 *   ir_comm_loop               The IR comm event loop. Drives the handshake,
 *                              parses incoming bytes, dispatches command opcodes,
 *                              and orchestrates the multi-phase EEPROM transfer
 *                              session.
 *
 * Handshake state machine — drives the SYN-style 3-way exchange before any
 * payload commands are accepted. Stored in `g.scratch2.ir.handshakeStep`:
 *
 *   0 = NONE          - no exchange in progress.
 *   1 = FC_SENT       - sent the 0xFC probe byte, waiting for peer 0xFA.
 *   2 = FA_RECEIVED   - peer replied with 0xFA; we sent our 0xFA back.
 *   3 = F8_SENT       - committed to a session key, sent 0xF8.
 *   4 = TRAINER_SENT  - sent our trainer record to the peer (post-handshake).
 *
 * Session phase — once the handshake completes, the multi-step EEPROM-pair
 * transfer is gated by `g.scratch2.ir.sessionPhase` (own pokemon → peer pokemon → pokedex
 * → etc.). Values 1..5; the 0x04 opcode advances the phase.
 *
 * LAB_xxxx labels at the bottom of ir_comm_loop are the ROM's exit fan-out:
 *
 *   LAB_182e            tx-done epilogue (random LCD reframe + reset cmd pos)
 *   LAB_14bc            drv_ir_finish_and_execute then -> LAB_182e
 *   LAB_1252            g.scratch2.ir.requestedPokemonAction = cmdByte then LAB_14bc
 *   LAB_15e4            g.scratch2.ir.requestedPokemonAction = cmdByte then LAB_182e
 *   LAB_17ee            send 0x04 ack then -> LAB_182e
 *   LAB_17ea            write trainer profile, then -> LAB_17ee
 *   start_eeprom_tx*    queue the next chunk of an EEPROM transfer
 *
 * The function is at 65.5% match and very score-sensitive; do not refactor
 * the control flow or the LAB layout without re-verifying.
 */

// ROM: 0x6954  83.2%
void sys_begin_ir_session(event_loop_func_t current_loop,
                          event_loop_func_t expected_loop) {
  /* Re-entrancy guard — only allowed from the low-power loop. */
  if (current_loop != expected_loop) {
    return;
  }
  /* Connect setup disables interrupts, so the sound-timer ISR can't run
   * during ir_comm_loop. If a beep was still playing when we got here, its
   * buffer (g.scratch1.accel.samples) will be overwritten by IR payload data, and
   * when we exit connect and interrupts come back, drv_sound_update will
   * try to play that garbage as notes — producing a continuous screech.
   * Stop any in-progress sound up front. */
  g.sound_dataPointer = NULL;
  if (!(g.sys_walkerFlags.BIT.session_active)) {
    drv_lcd_clear_pages(0x40);
    ui_render_happy_walker(0);
    drv_lcd_flip();
    drv_lcd_clear_pages(0x40);
    ui_render_happy_walker(1);
  } else {
    drv_lcd_clear_pages(0x40);
    ui_render_connecting_screen(0);
    gfx_draw_battery_low(0, 0x58);
    drv_lcd_flip();
    drv_lcd_clear_pages(0x40);
    ui_render_connecting_screen(1);
    gfx_draw_battery_low(0, 0x58);
  }
  drv_lcd_flip();
  set_ccr(0x80);
  drv_ir_send_discovery();
  sys_set_handler(ir_comm_loop);
}

// ROM: 0x009a  91.2%
void ir_handle_remote_cmd(void) {
  switch (g.scratch2.ir.requestedPokemonAction) {
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
  g.ui_activeView = VIEW_FACTORY_TEST;
  diag_init_test_mode();
  goto finalize;
enter_debug_mode:
  g.ui_activeView = VIEW_ACCEL_DEBUG;
  sys_init_accel_debug();
  goto finalize;
factory_reset_full:
  drv_lcd_init();
  g.session_idleSeconds = 0xE10;
  sys_factory_reset_eeprom(1, 1);
  goto apply_volume_and_contrast;
factory_reset_partial:
  g.session_idleSeconds = 0xE10;
  sys_factory_reset_eeprom(0, 1);
  goto apply_volume_and_contrast;
factory_reset_minimal:
  g.session_idleSeconds = 0xE10;
  sys_factory_reset_eeprom(0, 0);
apply_volume_and_contrast:
  drv_sound_set_volume((g.save_settings.BYTE >> 1) & 0x3);
  drv_lcd_set_contrast((g.save_settings.BYTE >> 3) & 0xF);
  goto return_to_main_view;
start_new_walk:
  g.save_walkStepCount = 0;
  game_start_walk();
  goto enter_walk_view;
end_walk_show_report:
  game_end_walk();
  ui_set_view(VIEW_WALK_DEPARTURE_ANIM);
  g.viewstate.Y.BYTE = 5;
  goto reset_substate_z;
restart_walk_clear_history:
  game_start_walk();
  drv_eeprom_fill(EEPROM_STEP_HIST_FLAGS, 0x06C8, 0);
enter_walk_view:
  ui_set_view(VIEW_WALK_ARRIVAL_ANIM);
  g.viewstate.Y.BYTE = 0;
  goto reset_substate_z;
clear_walk_stats:
  game_clear_stats();
  ui_set_view(VIEW_WALK_DEPARTURE_ANIM);
  g.viewstate.Y.BYTE = 6;
  goto reset_substate_z;
start_peer_play:
  ui_set_view(VIEW_PEER_PLAY);
  ui_start_peer_play_app();
  goto finalize;
show_menu_a3:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  g.viewstate.Y.BYTE = 0;
  g.viewstate.Z = 0;
  g.viewstate.A = 3;
  goto finalize;
show_menu_a0:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  g.viewstate.Y.BYTE = 0;
  g.viewstate.Z = 0;
  g.viewstate.A = 0;
  goto finalize;
show_menu_a2:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  g.viewstate.Y.BYTE = 0;
  g.viewstate.Z = 0;
  g.viewstate.A = 2;
  goto finalize;
show_menu_a1:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  g.viewstate.Y.BYTE = 0;
  g.viewstate.Z = 0;
  g.viewstate.A = 1;
  goto finalize;
show_menu_a4:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  g.viewstate.Y.BYTE = 0;
  g.viewstate.Z = 0;
  g.viewstate.A = 4;
  goto finalize;
show_menu_a5:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  g.viewstate.Y.BYTE = 0;
  g.viewstate.Z = 0;
  g.viewstate.A = 5;
  goto finalize;
show_menu_a6:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  g.viewstate.Y.BYTE = 0;
  g.viewstate.Z = 0;
  g.viewstate.A = 6;
  goto finalize;
show_menu_a7:
  ui_set_view(VIEW_EVENT_REWARD_ANIM);
  g.viewstate.Y.BYTE = 0;
  g.viewstate.Z = 0;
  g.viewstate.A = 7;
  goto finalize;

default_handle_error:
  if (g.ir_resultCode == 0)
    goto return_to_main_view;
  ui_set_view(VIEW_STEP_HISTORY);
reset_substate_z:
  g.viewstate.Z = 0;
  goto finalize;

return_to_main_view:
  ui_reset_substate();
  ui_set_view(VIEW_HOME);

finalize:
  g.accel_sampleCount = 0;
  game_reset_pedometer_flags();
  sys_set_handler(sys_main_loop_low_power);
  set_ccr(0x00);
  drv_rtc_get_time((uint8_t *)&g.rtc_seconds, (uint8_t *)&g.rtc_minutes,
                   (uint8_t *)&g.rtc_hours);
}

// ROM: 0x03b4  55.8%  saves: er2,r3,er4,er5,er6 -> sys_epilogue_0700
void ir_parse_rx_packet(void) {
  uint32_t peer_rtc;
  uint8_t hour_raw;
  uint16_t tens, units, bcd;
  uint8_t *payload;

  /* Stash the received payload into the shared DAT_f7e6 record so the rest
     of the comm-loop case-arms can read it without another copy. */
  payload = drv_ir_get_rx_ptr();
  memcpy(payload, (void *)DAT_f7e6, 0x68);

  /* g.scratch1.ir.flags_5b packs an hour in bits 7..3 (top 5 bits, valid if < 0x18).
     Decode to BCD for the notify-time scheduler. */
  hour_raw = g.scratch1.ir.flags_5b.BYTE;
  if ((hour_raw & 0xF8) < 0xC0) {
    bcd = (uint16_t)(hour_raw >> 3);
    tens = bcd / 10;
    units = bcd - (tens * 10);
    g.notif_scheduledHour = (uint8_t)((tens << 4) | units);
  }

  /* If the peer included a wall-clock time, adopt it. */
  peer_rtc = g.scratch1.peerSync.rcvdRtcTime;
  if (peer_rtc != 0) {
    g.save_rtcTime = peer_rtc;
    drv_rtc_set_time(peer_rtc);
  }
}

// ROM: 0x0714  50.3%  saves: er3,er4,er5,er6
#pragma option noregexpansion /* pragma:auto */
uint16_t ir_calc_packet_checksum(uint8_t length, uint8_t *data) {
  uint32_t sum;
  uint32_t i;
  uint32_t len;
  uint8_t *ptr;
  uint8_t byte;
  uint16_t hi;
  uint16_t lo;

  ptr = data;
  len = (uint32_t)length;
  sum = 0x0002;   /* seed; ROM does `sub.l er5,er5; mov.b #H'2,r5l` */
  i = 0;

  /* Even-indexed bytes go into the upper byte of the running word, odd-indexed
     bytes into the lower byte. Effectively this folds two bytes at a time
     into a 16-bit accumulator that grows into a 32-bit sum on overflow. */
  while (i < len) {
    byte = *ptr++;
    if (i & 1) {
      sum += (uint32_t)byte;
    } else {
      sum += (uint32_t)byte << 8;
    }
    i++;
  }

  /* Fold the 32-bit sum into 16 bits twice (one's-complement-style carry
     wrap-around). */
  hi = (uint16_t)(sum >> 16);
  lo = (uint16_t)(sum);
  sum = (uint32_t)hi + (uint32_t)lo;
  hi = (uint16_t)(sum >> 16);
  sum = (uint32_t)(uint16_t)sum + (uint32_t)hi;

  return (uint16_t)sum;
}

// ROM: 0x08d6  65.5%  saves: er3,er4,er5,er6
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
  cmdPos_local = g.ir_commandPos;
  if (SSR3_BIT.RDRF) {
    if (cmdPos_local >= 0x88) {
      g.scratch2.ir.rdrData = RDR3;
      g.ir_resultCode = 0x08;
      goto do_action;
    }
    g.ir_commandPos = cmdPos_local + 1;
    *((uint8_t *)&g.scratch2.ir.commandType + cmdPos_local) = RDR3 ^ 0xAA;
    g.ir_lastCommandTime = TCNT;
    goto finish_no_action;
  }
  timerDelta = (uint16_t)(TCNT - g.ir_lastCommandTime);
  if (timerDelta <= 4)
    goto finish_no_action;
  if (timerDelta > 0x0C80) {
    g.scratch2.ir.timeoutRetryCount++;
    if (g.scratch2.ir.handshakeStep < 3 && g.scratch2.ir.timeoutRetryCount < 0x14) {
      uint32_t r = sys_get_rng();
      r = (r >> 5) & 0x0F;
      r *= 0x60;
      tcntSnap = TCNT;
      while ((uint16_t)(TCNT - tcntSnap) < (uint16_t)r)
        ;
      g.scratch2.ir.handshakeStep = 1;
      drv_ir_tx_u8(0xFC);
      goto LAB_182e;
    }
    g.ir_resultCode = g.scratch2.ir.packetReceivedFlag.BIT.b0 ? 2 : 1;
    goto do_action;
  }
  if (cmdPos_local == 0)
    goto finish_no_action;
  g.scratch2.ir.packetReceivedFlag.BIT.b0 = 1;
  if (cmdPos_local == 1) {
    g.ir_commandPos = 0;
    cmdByte = g.scratch2.ir.commandType;
    if (cmdByte != 0xFC)
      goto finish_no_action;
    phase = g.scratch2.ir.handshakeStep;
    switch (phase) {
    case 1:
      g.scratch2.ir.handshakeStep = 2;
      drv_ir_send_packet(0x00, 0xFA, 2);
      break;
    case 2:
    case 4:
    case 3:
    default:
      break;
    }
    goto finish_no_action;
  }
  pktBase = (uint8_t *)&g.scratch2.ir.commandType;
  cmdLen = (uint8_t)cmdPos_local;
  crcExpected = ((uint16_t)pktBase[3] << 8) | pktBase[2];
  pktBase[2] = 0;
  pktBase[3] = 0;
  g.ir_commandPos = 0;
  crcCalc = ir_calc_packet_checksum(cmdLen, pktBase);
  if (crcCalc != crcExpected) {
    g.scratch2.ir.crcRetryCount++;
    if (g.scratch2.ir.crcRetryCount < 0x14) {
      goto finish_no_action;
    }
    g.ir_resultCode = 2;
    goto do_action;
  }
  *(uint32_t *)((uint8_t *)&g.scratch2.ir.commandType + 2) = *(uint32_t *)(pktBase + 4);
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
    e1val = g.scratch2.ir.xferRemaining;
    e2val = pktLen2;
    addr = g.scratch2.ir.xferSrc;

    cmdByte = pktBase[0];
    if (cmdByte < 0xF8) {
      if (*(uint32_t *)(pktBase + 4) != g.scratch2.ir.sessionKey)
        goto LAB_182e;
      phase = g.scratch2.ir.handshakeStep;
      if (phase < 3)
        goto LAB_182e;
    }

    switch (cmdByte) {
    case 0xFA:
      if (subtype == 1 || subtype == 2) {
        phase = g.scratch2.ir.handshakeStep;
        switch (phase) {
        case 1:
          g.scratch2.ir.handshakeStep = 3;
          drv_ir_send_packet(0x00, 0xF8, 2);
          g.scratch2.ir.sessionKey = *(uint32_t *)(pktBase + 4);
          g.scratch2.ir.sessionKey = g.scratch2.ir.sessionKeyNext ^ g.scratch2.ir.sessionKey;
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
          g.scratch2.ir.handshakeStep = 1;
          drv_ir_tx_u8(0xFC);
          g.ir_lastCommandTime = TCNT;
          goto LAB_182e;
        }
        default:
          goto LAB_182e;
        }
      }
      g.ir_resultCode = 3;
      goto LAB_14bc;

    case 0xF4:
      goto LAB_14bc;

    case 0xF8:
      if (subtype != 2) {
        g.ir_resultCode = 3;
        goto LAB_14bc;
      }
      phase = g.scratch2.ir.handshakeStep;
      if (phase >= 3) {
        goto LAB_182e;
      }
      g.scratch2.ir.sessionKey = *(uint32_t *)(pktBase + 4);
      g.scratch2.ir.sessionKey = g.scratch2.ir.sessionKeyNext ^ g.scratch2.ir.sessionKey;
      g.scratch2.ir.handshakeStep = 4;
      save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)g.scratch1.secondTrainer.buf, 0x68);
      drv_ir_send_packet(0x68, 0x10, 2);
      g.viewstate.Y.BIT.b0 = 0;
      goto LAB_182e;

    case 0x10:
      g.viewstate.Y.BIT.b0 = 1;
      memcpy(payload, (void *)DAT_f7e6, 0x68);
      save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)g.scratch1.secondTrainer.buf, 0x68);
      if (!((byte_bits_t *)&payload[0x5B])->BIT.b0) {
        drv_ir_send_packet(0x68, 0x12, 2);
        g.ir_resultCode = 3;
        goto LAB_14bc;
      }
      if (!g.scratch1.ir.flags_5b.BIT.b0) {
        drv_ir_send_packet(0x68, 0x12, 2);
        g.ir_resultCode = 3;
        goto LAB_14bc;
      }
      if (!g.scratch1.ir.flags_5b.BIT.b1) {
        drv_ir_send_packet(0x68, 0x12, 2);
        g.ir_resultCode = 3;
        goto LAB_14bc;
      }
      if (*(uint8_t *)(payload + 0x5C) != DAT_f842) {
        drv_ir_send_packet(0x68, 0x12, 2);
        g.ir_resultCode = 3;
        goto LAB_14bc;
      }
      if (DAT_f844 != 0) {
        drv_ir_send_packet(0x68, 0x12, 2);
        g.ir_resultCode = 3;
        goto LAB_14bc;
      }
      if (!((byte_bits_t *)&payload[0x5B])->BIT.b1) {
        drv_ir_send_packet(0x68, 0x12, 2);
        g.ir_resultCode = 4;
        goto LAB_14bc;
      }
      if (game_find_seen_peer((void *)(DAT_f7e6 + 0x10)) != 0) {
        drv_ir_send_packet(0x00, 0x1C, 2);
        g.ir_resultCode = 5;
        goto LAB_14bc;
      }
      drv_ir_send_packet(0x68, 0x12, 2);
      goto LAB_182e;

    case 0x12:
      memcpy(payload, (void *)DAT_f7e6, 0x68);
      save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)g.scratch1.secondTrainer.buf, 0x68);
      if (!((byte_bits_t *)&payload[0x5B])->BIT.b0) {
        g.ir_resultCode = 3;
        goto LAB_14bc;
      }
      if (!g.scratch1.ir.flags_5b.BIT.b0) {
        g.ir_resultCode = 3;
        goto LAB_14bc;
      }
      if (!g.scratch1.ir.flags_5b.BIT.b1) {
        g.ir_resultCode = 3;
        goto LAB_14bc;
      }
      if (*(uint8_t *)(payload + 0x5C) != DAT_f842) {
        g.ir_resultCode = 3;
        goto LAB_14bc;
      }
      if (DAT_f844 != 0) {
        g.ir_resultCode = 3;
        goto LAB_14bc;
      }
      if (!((byte_bits_t *)&payload[0x5B])->BIT.b1) {
        g.ir_resultCode = 4;
        goto LAB_14bc;
      }
      if (game_find_seen_peer((void *)(DAT_f7e6 + 0x10)) != 0) {
        drv_ir_send_packet(0x00, 0x1C, 2);
        g.ir_resultCode = 5;
        goto LAB_14bc;
      }
      g.scratch2.ir.sessionPhase = 1;
      *(uint16_t *)&g.scratch2.ir.xferSrc = 0x91BE;
      *(uint16_t *)&g.scratch2.ir.xferDst = 0xF400;
      g.scratch2.ir.xferRemaining = 0x180;
      g.scratch2.ir.xferChunkCount = 0;
      goto start_eeprom_tx;

    case 0x14:
      if (g.viewstate.Y.BIT.b0) {
        uint8_t *rxptr;
        uint8_t i;
        drv_eeprom_write_block(0xF6C0, payload, 0x38);
        rxptr = drv_ir_get_rx_ptr();
        *(uint32_t *)rxptr = g.session_steps;
        *(uint16_t *)(rxptr + 4) = g.session_recentSteps;
        drv_eeprom_read_block(EEPROM_TRAINER_PROFILE, eepromPageScratch, 0x10);
        ((byte_bits_t *)&rxptr[0x37])->BIT.b0 =
            ((byte_bits_t *)&eepromPageScratch[0x0E])->BIT.b0;
        *(uint16_t *)(rxptr + 0x0E) = *(uint16_t *)eepromPageScratch;
        rxptr[0x36] = (uint8_t)((rxptr[0x36] & 0xE0) | (eepromPageScratch[0x0D] & 0x1F));
        rxptr[0x36] = (uint8_t)((rxptr[0x36] & ~(0x03 << 5)) |
                                (eepromPageScratch[0x0D] & 0x60));
        ((byte_bits_t *)&rxptr[0x36])->BIT.b7 =
            ((byte_bits_t *)&eepromPageScratch[0x0E])->BIT.b1;
        save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)g.scratch1.secondTrainer.buf, 0x68);
        *(uint32_t *)(rxptr + 0x08) = *(uint32_t *)g.scratch1.secondTrainer.buf;
        *(uint16_t *)(rxptr + 0x0C) = g.scratch1.secondTrainer.loc;
        for (i = 0; i < 0x10; i++) {
          rxptr[0x26 + i] = ((uint8_t *)&DAT_f896)[i];
        }
        drv_eeprom_read_block(0x8F10, rxptr + 0x10, 0x16);
        drv_ir_send_packet(0x38, 0x14, 2);
      } else {
        drv_eeprom_write_block(0xF6C0, payload, 0x38);
        drv_ir_send_packet(0x00, 0x16, 2);
      }
      goto LAB_182e;

    case 0x16:
      if (g.viewstate.Y.BIT.b0) {
        drv_ir_send_packet(0x00, 0x16, 2);
      }
      cmdByte = 0x16;
      goto LAB_1252;

    case 0x1C:
      g.ir_resultCode = 5;
      goto LAB_14bc;

    case 0x20:
      save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, payload, 0x68);
      *(uint32_t *)(payload + 0x64) = g.save_totalSteps;
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&g.save_totalSteps, 0x18);
      drv_eeprom_write_block(0xCE8A, &g.save_watts, 2);
      memcpy(payload, (void *)g.scratch1.secondTrainer.buf, 0x68);
      drv_ir_send_packet(0x68, 0x22, 2);
      goto LAB_182e;

    case 0x32:
      ir_parse_rx_packet();
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&g.save_totalSteps, 0x18);
      drv_ir_send_packet(0x00, 0x34, 2);
      goto LAB_182e;

    case 0x36:
      g.ir_resultCode = 3;
      goto LAB_14bc;

    case 0x38:
      drv_ir_send_packet(0x00, 0x38, 2);
      cmdByte = 0x38;
      goto LAB_15e4;

    case 0x40:
      ir_parse_rx_packet();
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&g.save_totalSteps, 0x18);
      drv_ir_send_packet(0x00, 0x42, 2);
      goto LAB_182e;

    case 0x44:
      g.ir_resultCode = 3;
      goto LAB_14bc;

    case 0x4E:
      drv_ir_send_packet(0x00, 0x50, 2);
      cmdByte = 0x4E;
      goto LAB_1252;

    case 0x52:
      ir_parse_rx_packet();
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&g.save_totalSteps, 0x18);
      drv_ir_send_packet(0x00, 0x54, 2);
      goto LAB_182e;

    case 0x5A:
      drv_ir_send_packet(0x00, 0x5A, 2);
      cmdByte = 0x5A;
      goto LAB_1252;

    case 0x56:
      g.ir_resultCode = 3;
      goto LAB_14bc;

    case 0x60:
      ir_parse_rx_packet();
      save_read_reliable(EEPROM_TRAINER_REC, EEPROM_TRAINER_REC_BACKUP, (void *)g.scratch1.secondTrainer.buf, 0x68);
      drv_ir_send_packet(0x00, 0x62, 2);
      goto LAB_182e;

    case 0x66:
      drv_ir_send_packet(0x00, 0x68, 2);
      cmdByte = 0x66;
      goto LAB_15e4;

    case 0x64:
      g.ir_resultCode = 3;
      goto LAB_14bc;

    case 0xC0: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x10;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)g.scratch1.secondTrainer.buf, g.scratch1.ir.eventBitIndex);
      drv_ir_send_packet(0x00, 0xC0, 2);
      cmdByte = 0xC0;
      goto LAB_1252;

    case 0xD0: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x1F;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)g.scratch1.secondTrainer.buf, g.scratch1.ir.eventBitIndex);
      drv_ir_send_packet(0x00, 0xC0, 2);
      cmdByte = 0xC0;
      goto LAB_1252;

    case 0xC2: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x20;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)g.scratch1.secondTrainer.buf, g.scratch1.ir.eventBitIndex);
      drv_ir_send_packet(0x00, 0xC2, 2);
      cmdByte = 0xC2;
      goto LAB_1252;

    case 0xD2: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x2F;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)g.scratch1.secondTrainer.buf, g.scratch1.ir.eventBitIndex);
      drv_ir_send_packet(0x00, 0xC2, 2);
      cmdByte = 0xC2;
      goto LAB_1252;

    case 0xC4: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x40;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)g.scratch1.secondTrainer.buf, g.scratch1.ir.eventBitIndex);
      drv_ir_send_packet(0x00, 0xC4, 2);
      cmdByte = 0xC4;
      goto LAB_1252;

    case 0xD4: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x4F;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)g.scratch1.secondTrainer.buf, g.scratch1.ir.eventBitIndex);
      drv_ir_send_packet(0x00, 0xC4, 2);
      cmdByte = 0xC4;
      goto LAB_1252;

    case 0xC6: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x80;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)g.scratch1.secondTrainer.buf, g.scratch1.ir.eventBitIndex);
      g.save_settings.BYTE |= 0x01;
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&g.save_totalSteps, 0x18);
      drv_ir_send_packet(0x00, 0xC6, 2);
      cmdByte = 0xC6;
      goto LAB_1252;

    case 0xD6: {
      uint8_t bf = drv_eeprom_read_u8(EEPROM_STEP_HIST_FLAGS);
      bf |= 0x8F;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, bf);
    }
      save_set_event_bit((void *)g.scratch1.secondTrainer.buf, g.scratch1.ir.eventBitIndex);
      g.save_settings.BYTE |= 0x01;
      save_write_reliable(EEPROM_SAVE_BLOCK, EEPROM_SAVE_BLOCK_BACKUP, (void *)&g.save_totalSteps, 0x18);
      drv_ir_send_packet(0x00, 0xC6, 2);
      cmdByte = 0xC6;
      goto LAB_1252;

    case 0xD8:
      g.ir_resultCode = 3;
      goto LAB_14bc;

    case 0x24:
      drv_ir_send_packet(0x00, 0x26, 2);
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
        phase = g.scratch2.ir.sessionPhase;
        if (phase == 1)
          goto handle_0x04_phase1;
        if (phase == 3)
          goto handle_0x04_phase3;
        if (phase == 5)
          goto handle_0x04_phase5;
        goto LAB_182e;
      } else {
        /* peer ACK'd previous chunk; send the next one via WRITE_RAW.
         * (ROM splits this in two via LAB_135a/LAB_1362, but the
         * min(remaining, 0x80) clamp in start_eeprom_tx covers both.) */
        g.scratch2.ir.xferRemaining = e1val;
        goto LAB_1362;
      }

    handle_0x04_phase1:
      g.scratch2.ir.sessionPhase = 3;
      *(uint16_t *)&g.scratch2.ir.xferSrc = 0x993E;
      *(uint16_t *)&g.scratch2.ir.xferDst = (uint16_t)DAT_f580;
      g.scratch2.ir.xferRemaining = 0x140;
      g.scratch2.ir.xferChunkCount = 0;
      goto start_eeprom_tx;

    handle_0x04_phase3:
      g.scratch2.ir.sessionPhase = 5;
      *(uint16_t *)&g.scratch2.ir.xferSrc = 0xCC00;
      *(uint16_t *)&g.scratch2.ir.xferDst = 0xDC00;
      g.scratch2.ir.xferRemaining = 0x224;
      g.scratch2.ir.xferChunkCount = 0;
      goto start_eeprom_tx;

    handle_0x04_phase5:
      g.scratch2.ir.sessionPhase = 2;
      *(uint16_t *)&g.scratch2.ir.xferSrc = 0x91BE;
      *(uint16_t *)&g.scratch2.ir.xferDst = 0xF400;
      g.scratch2.ir.xferRemaining = 0x180;
      g.scratch2.ir.xferChunkCount = 0;
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
        *dst = g.scratch1.ir.eventBitIndex;
      }
      if (save_check_event_bit((void *)g.scratch1.secondTrainer.buf, g.scratch1.ir.eventBitIndex) != 0) {
        drv_ir_send_packet(0x11, 0x9E, 2);
        g.ir_resultCode = 6;
        goto LAB_14bc;
      }
      drv_ir_send_packet(0x11, cmdByte, 2);
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
        *dst = g.scratch1.ir.eventBitIndex;
      }
      if (save_check_event_bit((void *)g.scratch1.secondTrainer.buf, g.scratch1.ir.eventBitIndex) != 0) {
        drv_ir_send_packet(0x11, 0x9E, 2);
        g.ir_resultCode = 6;
        goto LAB_14bc;
      }
      drv_ir_send_packet(0x11, cmdByte, 2);
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
      g.scratch2.ir.requestedPokemonAction = cmdByte;
      drv_eeprom_write_u8(EEPROM_STEP_HIST_FLAGS, flagByte);
      drv_ir_send_packet(0x00, (uint8_t)(cmdByte + 0x10), 2);
    }
      goto LAB_14bc;

    case 0x9E:
      drv_ir_send_packet(0x00, 0x9E, 2);
      g.ir_resultCode = 7;
      goto LAB_14bc;

    case 0x9C:
      drv_ir_send_packet(0x00, 0x9C, 2);
      g.ir_resultCode = 7;
      goto LAB_14bc;

    case 0xF0: {
      save_write_reliable(EEPROM_RESV_0083, EEPROM_RESV_0083_BACKUP, payload, 0x28);
      drv_eeprom_write_block(0x0008, payload + 0x68, 0x08);
      g.viewstate.Z = 1;
      g.scratch2.ir.requestedPokemonAction = 0xF0;
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
            g.viewstate.Z = 0;
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
        g.scratch2.ir.requestedPokemonAction = 0xE0;
        break;
      default:
        break;
      }
      drv_ir_send_packet(0x28, 0xF0, 2);
      goto LAB_182e;
    }

    case 0xFE:
      if (subtype == 1 && pktLen2 == 8) {
        drv_eeprom_write_block(0x0008, payload, 0x08);
        drv_ir_send_packet(0x00, 0xFE, 2);
        cmdByte = 0xFE;
        goto LAB_15e4;
      }
      goto LAB_182e;

    case 0x1A:
      drv_eeprom_write_block(addr, payload, 0x80);
      drv_ir_send_packet(0x40, 0x1A, 2);
      break;

    case 0x2A:
      save_read_reliable(EEPROM_RESV_0083, EEPROM_RESV_0083_BACKUP, payload, 0x28);
      drv_ir_send_packet(0x28, 0x2A, 2);
      cmdByte = 0x2A;
      goto LAB_15e4;

    case 0x2C:
      save_read_reliable(EEPROM_RESV_0083, EEPROM_RESV_0083_BACKUP, payload, 0x28);
      drv_ir_send_packet(0x28, 0x2A, 2);
      cmdByte = 0x2C;
      goto LAB_15e4;

    case 0x0C: {
      uint16_t eaddr = ((uint16_t)pktBase[0] << 8) | pktBase[1];
      uint8_t chunk = pktBase[2];
      drv_eeprom_read_block(eaddr, payload, chunk);
      drv_ir_send_packet(chunk, 0x0E, 2);
      goto LAB_182e;
    }

    case 0x0E:
      drv_eeprom_write_block(addr, payload, 0x80);
      g.scratch2.ir.xferSrc += e2val;
      g.scratch2.ir.xferDst += e2val;
      g.scratch2.ir.xferRemaining -= e2val;
      g.scratch2.ir.xferChunkCount++;
      if (g.scratch2.ir.xferRemaining == 0) {
        if (g.scratch2.ir.sessionPhase == 2) {
          g.scratch2.ir.sessionPhase = 4;
          *(uint16_t *)&g.scratch2.ir.xferSrc = 0x993E;
          *(uint16_t *)&g.scratch2.ir.xferDst = (uint16_t)DAT_f580;
          g.scratch2.ir.xferRemaining = 0x140;
          g.scratch2.ir.xferChunkCount = 0;
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
      uint8_t *dst = (uint8_t *)g.scratch2.ir.xferDst;
      while (i < (uint8_t)(pktLen2 - 1)) {
        *dst++ = *src++;
        i++;
      }
      drv_ir_send_packet(0x00, subtype, 2);
      goto LAB_182e;
    }

    default:
      goto LAB_182e;
    }
  }

/* LAB_1362 / LAB_1366 (ROM) — push a chunk of OUR EEPROM to peer via
 * CMD_EEPROM_WRITE_RAW. cmd byte = (dst_lo & 0x80) | 0x02 → 0x02 or 0x82
 * to indicate which 128-byte half of the page; subtype = dst_hi byte. */
LAB_1362:
start_eeprom_tx: {
  uint16_t chunk = (g.scratch2.ir.xferRemaining > 0x80) ? 0x80 : g.scratch2.ir.xferRemaining;
  uint8_t *rxptr = drv_ir_get_rx_ptr();
  drv_eeprom_read_block(g.scratch2.ir.xferSrc, rxptr, chunk);
  {
    uint8_t dst_hi = (uint8_t)(g.scratch2.ir.xferDst >> 8);
    uint8_t dst_lo = (uint8_t)g.scratch2.ir.xferDst;
    uint8_t cmd = (uint8_t)((dst_lo & 0x80) | 0x02);
    drv_ir_send_packet((uint8_t)chunk, cmd, dst_hi);
  }
  g.scratch2.ir.xferSrc += chunk;
  g.scratch2.ir.xferDst += chunk;
  g.scratch2.ir.xferRemaining -= chunk;
  g.scratch2.ir.xferChunkCount++;
  goto LAB_182e;
}

/* LAB_17b0 (ROM) — ask peer for a chunk via CMD_EEPROM_READ_REQ.
 * Used only by phase 5 (after case 0x04 ACK), where we want to PULL data
 * from peer. The 3-byte body of the request is (src_hi, src_lo, chunk). */
start_eeprom_tx_alt:
LAB_17b0: {
  uint16_t chunk = (g.scratch2.ir.xferRemaining > 0x80) ? 0x80 : g.scratch2.ir.xferRemaining;
  uint8_t *p = drv_ir_get_rx_ptr();
  p[0] = (uint8_t)(g.scratch2.ir.xferSrc >> 8);
  p[1] = (uint8_t)(g.scratch2.ir.xferSrc);
  p[2] = (uint8_t)chunk;
  drv_ir_send_packet(0x03, 0x0C, 2);
  goto LAB_182e;
}

LAB_17ea:
LAB_17ee:
  drv_ir_send_packet(0x00, 0x04, 2);
  goto LAB_182e;

LAB_15e4:
  g.scratch2.ir.requestedPokemonAction = cmdByte;
  goto LAB_182e;

LAB_1252:
  g.scratch2.ir.requestedPokemonAction = cmdByte;
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
  g.ir_commandPos = 0;
  g.ir_lastCommandTime = TCNT;
}
finish_no_action: ;
}
