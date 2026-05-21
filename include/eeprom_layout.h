/* EEPROM layout — symbolic constants for every named region used by
 * save_*, drv_eeprom_*, and adjacent helpers.
 *
 * The Pokéwalker uses an external serial EEPROM (~48KB addressable
 * 0x0000..0xBFFF for code paths we've decompiled, plus higher regions
 * 0xC000+ for peer-log and asset data).
 *
 * --- Region map (addresses we have semantic understanding for) ---
 *
 * 0x0000..0x007F   reserved / system boot block
 *    0x0072        wake/reset counter (incremented in resetprg)
 *
 * 0x0080..0x00EC   small reliable-save state (each entry mirrored at +0x100)
 *    0x0080+/0x0180  accelerometer calibration (2 bytes)
 *    0x0083+/0x0183  16-byte unknown reliable block
 *    0x00AC+/0x01AC  LCD init sequence (64 bytes; drv_lcd_init loads this)
 *
 * 0x00ED..0x0155   trainer-record primary (104 bytes) + backup at 0x01ED..0x0255
 *    0x00ED+/0x01ED  trainer_record (struct trainer_record, 0x68 bytes)
 *
 * 0x0156..0x016E   main session-save primary (24 bytes) + backup at 0x0256
 *    0x0156+/0x0256  session_save block (struct session_save, 0x18 bytes)
 *
 * 0x016F..0x017F   misc reliable
 *    0x016F+/0x026F  staged-data commit marker (1 byte; 0xA5 then 0x00)
 *
 * 0x0278..0x2A00   graphics / sprite blob region (LCD blit sources)
 *
 * 0x8F00..0x9000   active trainer profile (0xBE bytes; read into local heap)
 *    0x8F00          trainer_profile_main
 *    0x8F52+         per-pokemon-slot 16-byte records (indexed by gCurSubstateY-1)
 *    0x8F8C          some 2-byte-per-slot table indexed by sub-state Y
 *
 * 0x91BE..0xA8BE   pokemon / animation asset banks (large reads)
 *
 * 0xB800..0xBEC8   step history flags region (1672 bytes; cleared by fill)
 *
 * 0xBA44..0xBF7A   misc lookup bytes (drv_eeprom_read_u8 single-byte reads
 *                  for hour-marker / item-status checks)
 *
 * 0xBD40..0xBEC8   wild-pokemon encounter table (0x188 bytes; written by
 *                  game_write_wild_poke, read by game_read_wild_poke)
 *
 * 0xC6FC..0xCE7F   asset/sound region
 *
 * 0xCE80..0xDE23   peer / interaction log
 *    0xCE80..0xCBCC  general log region (cleared 0xD4C bytes at startup)
 *    0xCE8C..0xCEEF  per-Y interaction context block (0x30 bytes)
 *    0xCEBC..0xCEC7  items log (4 bytes per slot × 3 slots)
 *    0xCEF0..0xCF0B  small per-pokemon stats block
 *    0xCF0C..0xDE23  peer slot table (24 slots × 0x88 bytes = 0x1450 bytes)
 *
 * 0xDE24..0xF38B   step-history sample buffer (0x1568 bytes; cleared at
 *                  session start AND in game_rotate_interaction_log via
 *                  the rolling-shift mechanism)
 *
 * 0xF6C0..        misc IR-comm region
 */

#ifndef EEPROM_LAYOUT_H
#define EEPROM_LAYOUT_H

/* ---- Boot / system ---- */
#define EEPROM_BOOT_COUNTER        0x0072  /* 1 byte: incremented each wake */

/* ---- Reliable-save small state (primary + backup mirrored at +0x100) ---- */
#define EEPROM_ACCEL_CAL           0x0080  /* 2 bytes; backup at +0x100 */
#define EEPROM_ACCEL_CAL_BACKUP    0x0180
#define EEPROM_RESV_0083           0x0083  /* 16 bytes; backup at +0x100 */
#define EEPROM_RESV_0083_BACKUP    0x0183
#define EEPROM_LCD_INIT_SEQ        0x00AC  /* 64 bytes; loaded by drv_lcd_init */
#define EEPROM_LCD_INIT_SEQ_BACKUP 0x01AC

/* ---- Trainer record (0x68 bytes; struct trainer_record) ---- */
#define EEPROM_TRAINER_REC         0x00ED
#define EEPROM_TRAINER_REC_BACKUP  0x01ED
#define EEPROM_TRAINER_REC_SIZE    0x68

/* ---- Main session-save block (0x18 bytes; struct session_save) ---- */
#define EEPROM_SAVE_BLOCK          0x0156
#define EEPROM_SAVE_BLOCK_BACKUP   0x0256
#define EEPROM_SAVE_BLOCK_SIZE     0x18

/* ---- Save-staging commit marker (1 byte: 0xA5 = staged, 0x00 = committed) ---- */
#define EEPROM_STAGE_MARKER        0x016F
#define EEPROM_STAGE_MARKER_BACKUP 0x026F

/* ---- Active trainer profile (extended state; 0xBE bytes loaded to heap) ---- */
#define EEPROM_TRAINER_PROFILE     0x8F00
#define EEPROM_TRAINER_PROFILE_SIZE 0xBE
#define EEPROM_POKEMON_SLOTS       0x8F52  /* per-Y pokemon-slot table (0x10 stride) */
#define EEPROM_POKEMON_SLOT_STRIDE 0x10
#define EEPROM_SUBY_LOOKUP_TABLE   0x8F8C  /* 2-byte-per-Y lookup */

/* ---- Pedometer / step-history flags ---- */
#define EEPROM_STEP_HIST_FLAGS     0xB800
#define EEPROM_STEP_HIST_FLAGS_SIZE 0x06C8

/* ---- Wild pokemon encounter data ---- */
#define EEPROM_WILD_POKE           0xBD40
#define EEPROM_WILD_POKE_SIZE      0x188

/* ---- Misc single-byte status lookups (referenced as drv_eeprom_read_u8) ---- */
#define EEPROM_HOUR_MARKER         0xBF06  /* 1 byte - 'hour offset?' */
#define EEPROM_SPECIAL_BYTE        0xBF0D  /* 1 byte - 'special mode flag?' */
#define EEPROM_EEP_STR             0xBF7A  /* 1 byte (start of "EEP" diagnostic string in ROM) */

/* ---- Peer / interaction log ---- */
#define EEPROM_LOG_REGION          0xCE80  /* full region cleared on startup */
#define EEPROM_LOG_REGION_SIZE     0x0D4C
#define EEPROM_LOG_CONTEXT         0xCE8C  /* per-Y interaction context (0x30 bytes) */
#define EEPROM_LOG_CONTEXT_SIZE    0x30
#define EEPROM_LOG_ITEMS           0xCEBC  /* items log (4 bytes per slot) */
#define EEPROM_LOG_ITEMS_STRIDE    0x04
#define EEPROM_LOG_POKE_STATS      0xCEF0  /* per-pokemon stats; 0x1C bytes */
#define EEPROM_LOG_POKE_STATS_SIZE 0x1C

/* ---- Peer record slot ring (24 slots × 0x88 bytes) ---- */
#define EEPROM_PEER_SLOT_BASE      0xCF0C
#define EEPROM_PEER_SLOT_STRIDE    0x88
#define EEPROM_PEER_SLOT_COUNT     24
#define EEPROM_PEER_SLOT_TYPE_OFFS 0x84   /* offset of the "type" byte within each slot */

/* ---- Step-history sample buffer (cleared at session start and end) ---- */
#define EEPROM_STEP_HIST           0xDE24
#define EEPROM_STEP_HIST_SIZE      0x1568

#endif /* EEPROM_LAYOUT_H */
