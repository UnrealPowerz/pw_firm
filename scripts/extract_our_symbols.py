#!/usr/bin/env python3
"""Extract addr->name symbol table from build/linked.map (optlnk Symbol List).

Writes build/our_symbols.txt in the same format as the hand-maintained
symbols.txt: `<hex_addr> <name> f` for functions, one per line.
"""
import re
import sys
from pathlib import Path

MAP = Path("build/linked.map")
OUT = Path("build/our_symbols.txt")


def parse(text):
    in_syms = False
    pending_name = None
    syms = []
    for line in text.splitlines():
        if line.startswith("*** Symbol List"):
            in_syms = True
            continue
        if not in_syms:
            continue
        if line.startswith("*** "):
            break
        s = line.strip()
        if not s:
            continue
        if s.startswith("SECTION=") or s.startswith("FILE=") or s.startswith("SYMBOL"):
            pending_name = None
            continue
        m = re.match(r"^_?([A-Za-z_][\w$]*)\s*$", s)
        if m:
            pending_name = m.group(1)
            continue
        m = re.match(r"^([0-9a-f]{8})\s+([0-9a-f]+)\s+(\w+)\s*,([a-z])", s)
        if m and pending_name:
            addr = int(m.group(1), 16)
            kind = m.group(3)
            # entry/func/none-with-code all map to "f"; data stays "d"
            tag = "d" if kind == "data" else "f"
            syms.append((addr, pending_name, tag))
            pending_name = None
    return syms


def main():
    syms = parse(MAP.read_text(errors="replace"))
    seen = {}
    for addr, name, tag in syms:
        if tag != "f":
            continue
        # If two symbols share an address, prefer the more specific name
        # (PowerON_Reset over __INITSCT-style helpers). First write wins
        # otherwise, since map order is roughly section order.
        if addr in seen:
            continue
        seen[addr] = name
    lines = [f"{a:04x} {seen[a]} f" for a in sorted(seen)]
    OUT.write_text("\n".join(lines) + "\n")
    print(f"wrote {len(lines)} function symbols -> {OUT}", file=sys.stderr)


if __name__ == "__main__":
    main()
