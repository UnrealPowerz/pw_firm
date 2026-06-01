#!/bin/bash
# Run the pocketwalker emulator with a fresh copy of eeprom.bin as the save.
#
# Usage:
#   scripts/run_emu.sh                       # our compiled ROM (build/linked.bin)
#   scripts/run_emu.sh --orig                # original ROM (COMPLETE_DUMP.bin)
#   scripts/run_emu.sh 2>/tmp/pw_trace.log   # capture trace
# Extra args after the mode flag are forwarded to the emulator.
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
EMU="$REPO/pocketwalker/desktop/build/release/release/pocketwalker"
SEED="$REPO/eeprom.bin"

ROM="$REPO/build/linked.bin"
if [[ "${1-}" == "--orig" ]]; then
    ROM="$REPO/COMPLETE_DUMP.bin"
    shift
fi

# The emulator looks for a .sav next to the ROM (same basename, .sav extension).
SAV="${ROM%.bin}.sav"

if [[ ! -f "$ROM" ]]; then
    echo "ROM not found: $ROM" >&2
    [[ "$ROM" == *"linked.bin" ]] && echo "  (run \`make\` first)" >&2
    exit 1
fi
if [[ ! -f "$SEED" ]]; then
    echo "EEPROM seed not found: $SEED" >&2
    exit 1
fi
if [[ ! -x "$EMU" ]]; then
    echo "Emulator not built: $EMU" >&2
    exit 1
fi

cp "$SEED" "$SAV"
exec "$EMU" "$ROM" "$@"
