/* IR command byte enum — the cmd field (first byte) of every IR packet.
 *
 * Names mostly mirror picowalker_core/src/ir/ir.h; aliases and the few
 * Pokewalker-specific opcodes (0xB8..0xBE, 0xD8, 0xE0, 0xF0, 0xFE) are
 * cross-referenced to where they're handled in src/engine/ir_protocol.c.
 *
 * The peer-play / EEPROM-transfer flow is documented in ir_protocol.c's
 * header comment. */

#ifndef IR_CMD_H
#define IR_CMD_H

enum ir_cmd {
    /* ---- EEPROM transfer ---- */
    /* WRITE_CMP* = LZSS-compressed page write; WRITE_RAW* = raw page write.
     * The trailing _00 / _80 selects which 128-byte half of the destination
     * page the packet writes to. Subtype = high byte of the dst address. */
    IR_CMD_EEPROM_WRITE_CMP_00 = 0x00,    /* compressed, dst low-half */
    IR_CMD_EEPROM_WRITE_RAW_00 = 0x02,    /* raw,        dst low-half */
    IR_CMD_EEPROM_WRITE_ACK    = 0x04,    /* peer's ACK after a write chunk */
    IR_CMD_RAM_WRITE           = 0x06,    /* (subtype<<8)|payload[0] = RAM dst */
    IR_CMD_EEPROM_WRITE_RND    = 0x0A,    /* random-length write at subtype<<8 | payload[0] */
    IR_CMD_EEPROM_READ_REQ     = 0x0C,    /* "send me payload[0..2] = src_hi,src_lo,chunk" */
    IR_CMD_EEPROM_READ_RSP     = 0x0E,    /* response with the requested bytes */
    IR_CMD_EEPROM_WRITE_CMP_80 = 0x80,    /* compressed, dst high-half */
    IR_CMD_EEPROM_WRITE_RAW_80 = 0x82,    /* raw,        dst high-half */

    /* ---- Peer-play exchange ---- */
    IR_CMD_PEER_PLAY_START     = 0x10,    /* master sends its trainer record */
    IR_CMD_PEER_PLAY_RSP       = 0x12,    /* slave responds with its trainer record */
    IR_CMD_PEER_PLAY_DX        = 0x14,    /* peer_play_data_t exchange (both sides) */
    IR_CMD_PEER_PLAY_END       = 0x16,    /* end of peer-play; transitions to celebration view */
    IR_CMD_PEER_PLAY_SEEN      = 0x1C,    /* "peer was seen too recently" rejection */

    /* ---- Identity ---- */
    IR_CMD_IDENTITY_REQ        = 0x20,    /* game requests our identity */
    IR_CMD_IDENTITY_RSP        = 0x22,    /* our identity reply */
    IR_CMD_IDENTITY_SEND       = 0x32,    /* master's identity sent to slave (canonical) */
    IR_CMD_IDENTITY_ACK        = 0x34,    /* ack of IDENTITY_SEND */
    IR_CMD_IDENTITY_SEND_ALIAS1 = 0x40,   /* alias for 0x32 — slot 1 */
    IR_CMD_IDENTITY_ACK_ALIAS1 = 0x42,
    IR_CMD_IDENTITY_SEND_ALIAS2 = 0x52,   /* alias for 0x32 — slot 2 */
    IR_CMD_IDENTITY_ACK_ALIAS2 = 0x54,
    IR_CMD_IDENTITY_SEND_ALIAS3 = 0x60,   /* alias for 0x32 — slot 3 */
    IR_CMD_IDENTITY_ACK_ALIAS3 = 0x62,

    /* ---- Ping / connection ---- */
    IR_CMD_PING                = 0x24,
    IR_CMD_PONG                = 0x26,
    IR_CMD_CONNECT_COMPLETE    = 0x66,
    IR_CMD_CONNECT_COMPLETE_ACK = 0x68,

    /* ---- Session-failure codes ("NOCOMPLETE" family) ----
     * Each is paired with the IDENTITY_SEND_ALIAS variant of the same slot. */
    IR_CMD_NOCOMPLETE          = 0x36,    /* canonical */
    IR_CMD_NOCOMPLETE_ALIAS3   = 0x44,    /* only seen from g2w (game-to-walker) */
    IR_CMD_NOCOMPLETE_ALIAS1   = 0x56,
    IR_CMD_NOCOMPLETE_ALIAS2   = 0x64,
    IR_CMD_NORX                = 0x9C,    /* "no rx" timeout error */
    IR_CMD_NORX_ACK            = 0x9E,

    /* ---- Walk lifecycle ---- */
    IR_CMD_WALKER_RESET_1      = 0x2A,    /* clear events only */
    IR_CMD_WALKER_RESET_0      = 0x2C,    /* don't clear events or lifetime */
    IR_CMD_WALK_START_INIT     = 0x38,    /* first-time walk-start handshake */
    IR_CMD_WALK_END_REQ        = 0x4E,
    IR_CMD_WALK_END_ACK        = 0x50,
    IR_CMD_WALK_START          = 0x5A,
    IR_CMD_WALKER_RESET_3      = 0xE0,    /* clear events + lifetime */

    /* ---- Pokewalker-specific event/menu commands ----
     * 0xC0..0xC6 — event delivered (bit set in EEPROM_STEP_HIST_FLAGS).
     * 0xD0..0xD6 — event delivered with extra "stamps" (extra bits set).
     * 0xB8..0xBE — auxiliary "show menu" actions on the walker side.
     * 0xD8       — generic reject (sets ir_resultCode=3). */
    IR_CMD_EVENT_MAP           = 0xC0,    /* sets step_hist bit 4 */
    IR_CMD_EVENT_POKEMON       = 0xC2,    /* sets step_hist bit 5 */
    IR_CMD_EVENT_ITEM          = 0xC4,    /* sets step_hist bit 6 */
    IR_CMD_EVENT_ROUTE         = 0xC6,    /* sets step_hist bit 7 + settings bit 0 */
    IR_CMD_EVENT_MAP_STAMPS    = 0xD0,    /* sets step_hist 0x1F */
    IR_CMD_EVENT_POKEMON_STAMPS = 0xD2,   /* sets step_hist 0x2F */
    IR_CMD_EVENT_ITEM_STAMPS   = 0xD4,    /* sets step_hist 0x4F */
    IR_CMD_EVENT_ROUTE_STAMPS  = 0xD6,    /* sets step_hist 0x8F + settings bit 0 */
    IR_CMD_EVENT_REJECT        = 0xD8,    /* generic reject -> resultCode=3 */
    IR_CMD_SHOW_MENU_A4        = 0xB8,    /* drives EVENT_REWARD_ANIM A=4 */
    IR_CMD_SHOW_MENU_A5        = 0xBA,    /* A=5 */
    IR_CMD_SHOW_MENU_A6        = 0xBC,    /* A=6 */
    IR_CMD_SHOW_MENU_A7        = 0xBE,    /* A=7 */

    /* ---- Repeat / advertising probes ----
     * The 0xA0..0xAE family is a 4-slot "repeat me" probe sent during the
     * identity exchange. Even subcodes (0xA0/A2/A4/A6) trigger one path,
     * odd subcodes (0xA8/AA/AC/AE) the other. Both arms copy a 16-byte
     * staging buffer into the rx response. */
    IR_CMD_REPEAT_PROBE_A0     = 0xA0,
    IR_CMD_REPEAT_PROBE_A2     = 0xA2,
    IR_CMD_REPEAT_PROBE_A4     = 0xA4,
    IR_CMD_REPEAT_PROBE_A6     = 0xA6,
    IR_CMD_REPEAT_PROBE_A8     = 0xA8,
    IR_CMD_REPEAT_PROBE_AA     = 0xAA,
    IR_CMD_REPEAT_PROBE_AC     = 0xAC,
    IR_CMD_REPEAT_PROBE_AE     = 0xAE,

    /* ---- Test / debug ---- */
    IR_CMD_FACTORY_TEST        = 0xF0,    /* enter factory test view */
    IR_CMD_DEBUG_MODE          = 0xFE,    /* enter accel-debug view */

    /* ---- Handshake / session control ----
     * 1) Both walkers tx 0xFC until one sees the other's 0xFC.
     * 2) The one that sees first responds with 0xFA (asserts master role).
     * 3) The other receives 0xFA and answers 0xF8 (slave ack); both sides
     *    XOR-mix their session-ID halves and the session is established. */
    IR_CMD_DISCONNECT          = 0xF4,    /* end session */
    IR_CMD_SLAVE_ACK           = 0xF8,    /* "I'll be slave" — finalizes session ID */
    IR_CMD_ASSERT_MASTER       = 0xFA,    /* "I'm master" — brings new session ID */
    IR_CMD_ADVERTISING         = 0xFC     /* single-byte probe; no session ID, no CRC */
};

#endif /* IR_CMD_H */
