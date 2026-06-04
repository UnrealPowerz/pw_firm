#!/usr/bin/env python3
"""compare_data_layout.py — diff linker-placed data addresses against main.mar.

The linker (optlnk) writes every defined data symbol's final address to
build/linked.map. main.mar's data sections (B_IO_REG_1, B_UNUSED_2, B_RAM,
B_IO_REG2, CP) define the *expected* addresses. This script cross-references
the two so:

  * any symbol our linker places at the WRONG address shows up as MISPLACED
    (the critical signal for the upcoming RAM-globals conversion — there's
    no other way to spot a BSS layout drift, since BSS bytes are zero on disk)
  * symbols we haven't defined yet show up as NOT_DEFINED
  * any unexpected symbols (e.g. C-internal helpers placed in RAM) show up
    as EXTRA

Usage:
    python3 scripts/compare_data_layout.py            # full diff report
    python3 scripts/compare_data_layout.py --misplaced-only
    python3 scripts/compare_data_layout.py --section B_RAM
"""
from __future__ import annotations
import argparse
import re
import sys
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MAP_FILE = ROOT / "build" / "linked.map"


_TYPE_SIZES = {
    'uint8_t':  1, 'int8_t':  1, 'char': 1,
    'uint16_t': 2, 'int16_t': 2, 'short': 2,
    'uint32_t': 4, 'int32_t': 4, 'long':  4,
    'event_loop_func_t': 2,  # function pointer on H8/300H = 2 bytes
}

def parse_b_ram_layout(layout_path, anchor_addr=0xF780):
    """Walk include/b_ram_layout.h and compute each struct member's address.

    Returns {name -> (addr, size)} for every `volatile <type> name[N?];`
    declaration. Sizes follow C type sizes on H8/300H (uint8=1, uint16=2,
    uint32=4, pointers=2). Pointer types (`T *name`) are also recognized."""
    out = {}
    if not layout_path.exists():
        return out
    # Two patterns: scalar/array decls, and pointer decls (with `*`).
    field_re = re.compile(
        r'volatile\s+(\w+)\s+(\*?)\s*(\w+)\s*(?:\[(\d+)\])?\s*;'
    )
    off = 0
    for line in layout_path.read_text().splitlines():
        # Strip C and C++ comments first to avoid false positives in
        # "/* note: volatile uint8_t foo */" style comments.
        ln = re.sub(r'/\*.*?\*/', '', line)
        ln = re.sub(r'//.*', '', ln)
        m = field_re.search(ln)
        if not m:
            continue
        type_name = m.group(1)
        is_ptr    = bool(m.group(2))
        name      = m.group(3)
        count     = int(m.group(4)) if m.group(4) else 1
        if is_ptr:
            elem_size = 2  # H8/300H pointer = 2 bytes
        else:
            elem_size = _TYPE_SIZES.get(type_name, 1)
        sz = elem_size * count
        out[name] = (anchor_addr + off, sz)
        off += sz
    return out


def parse_linker_map(path):
    """Return {name -> (addr, size, section)} from optlnk's Symbol List.
    Strips the leading underscore optlnk adds to every C symbol so names
    match what main.mar uses."""
    out = {}
    if not path.exists():
        return out
    in_symbol_list = False
    section = None
    pending = None  # last seen `  _NAME` line waiting for address row
    sym_re   = re.compile(r"^  ([A-Za-z_][A-Za-z0-9_]*)\s*$")
    addr_re  = re.compile(r"^\s+([0-9a-fA-F]{8})\s+([0-9a-fA-F]+)\s+data")
    sect_re  = re.compile(r"^SECTION=([A-Za-z_][A-Za-z0-9_]*)\s*$")
    for line in path.read_text().splitlines():
        if line.startswith("*** Symbol List"):
            in_symbol_list = True
            continue
        if not in_symbol_list:
            continue
        if line.startswith("*** "):
            break
        m = sect_re.match(line)
        if m:
            section = m.group(1)
            pending = None
            continue
        m = sym_re.match(line)
        if m:
            pending = m.group(1)
            continue
        if pending is None:
            continue
        m = addr_re.match(line)
        if m:
            addr = int(m.group(1), 16)
            size = int(m.group(2), 16)
            name = pending[1:] if pending.startswith("_") else pending
            out[name] = (addr, size, section)
            pending = None
    return out


def parse_expected_map(map_path):
    """Read expected name→(addr, section) from two sources:
       (1) the audit_sections.py output (`<addr> <name> <section>` rows
           covering the B_RAM / B_IO_REG_* / B_UNUSED_2 sections), and
       (2) symbols.txt `<addr> <name> d` rows (covers the CP/romdata
           section too).
    Sources are merged; audit_sections takes precedence (more authoritative
    section tagging)."""
    out = {}
    sym_path = ROOT / "symbols.txt"
    if sym_path.exists():
        for line in sym_path.read_text().splitlines():
            line = line.split('#')[0].strip()
            parts = line.split()
            if len(parts) == 3 and parts[2] == 'd':
                try:
                    addr = int(parts[0], 16)
                except ValueError:
                    continue
                out[parts[1]] = (addr, 'CP' if addr >= 0xBB0E and addr < 0xC000 else 'unknown')
    if map_path.exists():
        for line in map_path.read_text().splitlines():
            parts = line.split()
            if len(parts) == 3:
                try:
                    addr = int(parts[0], 16)
                except ValueError:
                    continue
                out[parts[1]] = (addr, parts[2])  # audit_sections wins
    return out


def regenerate_expected_map(target_path):
    """Re-run scripts/_audit_sections.py to refresh /tmp/main_mar_data_map.txt
    from main.mar. Keeps the expected addresses in sync if main.mar moves."""
    script = ROOT / "scripts" / "_audit_sections.py"
    if script.exists():
        subprocess.run(["python3", str(script)], capture_output=True, text=True)
        return Path("/tmp/main_mar_data_map.txt")
    return target_path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--misplaced-only", action="store_true",
                    help="Only report symbols where our linker put them at the wrong address.")
    ap.add_argument("--section", help="Restrict to one main.mar section (B_RAM, CP, etc.)")
    ap.add_argument("--expected", default="/tmp/main_mar_data_map.txt",
                    help="Path to the expected-address map (default refreshes from main.mar)")
    args = ap.parse_args()

    expected_path = Path(args.expected)
    if not expected_path.exists():
        expected_path = regenerate_expected_map(expected_path)

    expected = parse_expected_map(expected_path)
    actual   = parse_linker_map(MAP_FILE)

    # The _b_ram struct holds 113 RAM members at known offsets. Surface
    # each member as a per-symbol entry in `actual` so the audit also
    # validates the C struct's per-member layout against main.mar.
    b_ram_layout = parse_b_ram_layout(ROOT / "include" / "b_ram_layout.h")
    for name, (addr, sz) in b_ram_layout.items():
        actual.setdefault(name, (addr, sz, 'B_RAM'))

    # The last two main.mar B_RAM labels (DAT_fef0, initialStackPosition)
    # are aliases for the .data destination (R) and the bytes right below
    # the stack base (S). Both regions are placed by the linker via the
    # separate `-start=B,R/FEF0` and `-start=S/FF80` anchors. If the
    # corresponding R/S sections appear in the map at the expected
    # addresses, the two labels are implicitly correctly placed.
    section_starts = {}
    section_re = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)\s*$")
    in_mapping = False
    for line in MAP_FILE.read_text().splitlines() if MAP_FILE.exists() else []:
        if line.startswith("*** Mapping"):
            in_mapping = True
            continue
        if not in_mapping:
            continue
        if line.startswith("*** "):
            in_mapping = False
            continue
        m = section_re.match(line)
        if m:
            current = m.group(1)
            continue
        am = re.match(r"^\s+([0-9a-fA-F]{8})\s+[0-9a-fA-F]{8}", line)
        if am and 'current' in dir() and current:
            section_starts.setdefault(current, int(am.group(1), 16))
            current = None
    if section_starts.get("R") == 0xFEF0:
        actual.setdefault("DAT_fef0", (0xFEF0, 0, "R"))
    if section_starts.get("S") == 0xFF80:
        # initialStackPosition is the 8 bytes below the stack base
        actual.setdefault("initialStackPosition", (0xFF78, 8, "S"))

    if args.section:
        expected = {n: v for n, v in expected.items() if v[1] == args.section}

    # Three buckets
    misplaced = []     # in both, different address  -> BUG
    matched   = []     # in both, same address       -> OK
    not_defined = []   # expected, not in linker map -> NOT_DEFINED
    extra     = []     # in linker map, not expected -> EXTRA (data only)

    for name, (exp_addr, exp_section) in sorted(expected.items(), key=lambda kv: kv[1][0]):
        if name in actual:
            act_addr, act_size, act_section = actual[name]
            if act_addr == exp_addr:
                matched.append((name, exp_addr, exp_section))
            else:
                misplaced.append((name, exp_addr, act_addr, exp_section))
        else:
            not_defined.append((name, exp_addr, exp_section))

    # When restricting to one section, only flag EXTRA for symbols that fall
    # inside that section's address range (else they're "extra" relative to a
    # different section entirely, which is just noise).
    section_range = None
    if args.section:
        addrs = [a for n, (a, s) in expected.items()]
        if addrs:
            section_range = (min(addrs), max(addrs))
    for name, (act_addr, act_size, act_section) in sorted(actual.items(), key=lambda kv: kv[1][0]):
        if name in expected:
            continue
        if section_range and not (section_range[0] <= act_addr <= section_range[1] + 0xFF):
            continue
        extra.append((name, act_addr, act_section))

    print(f"{'═' * 70}")
    print(f"  Data-layout audit: linked.map vs main.mar")
    print(f"  expected: {len(expected)} symbols")
    print(f"  actual:   {len(actual)} symbols")
    print(f"{'═' * 70}")

    if misplaced:
        print(f"\n⚠  MISPLACED ({len(misplaced)}) — linker put these at the wrong address:")
        for name, exp, act, sec in misplaced:
            print(f"  {name:<40} expected 0x{exp:04X}  got 0x{act:04X}  ({sec})")
    else:
        print("\n✓  No misplaced symbols.")

    if not args.misplaced_only:
        if matched:
            print(f"\n✓  MATCHED ({len(matched)}):")
            for name, addr, sec in matched[:10]:
                print(f"  {name:<40} 0x{addr:04X}  ({sec})")
            if len(matched) > 10:
                print(f"  ... ({len(matched) - 10} more)")

        if not_defined:
            by_sec = {}
            for n, a, s in not_defined:
                by_sec.setdefault(s, []).append((n, a))
            print(f"\n·  NOT_DEFINED in our build ({len(not_defined)}):")
            for sec in sorted(by_sec):
                items = by_sec[sec]
                print(f"  [{sec}] {len(items)} symbols  e.g. {', '.join(n for n,_ in items[:5])}{', ...' if len(items)>5 else ''}")

        if extra:
            print(f"\n·  EXTRA in our build ({len(extra)}):")
            for name, addr, sec in extra[:15]:
                print(f"  {name:<40} 0x{addr:04X}  ({sec})")
            if len(extra) > 15:
                print(f"  ... ({len(extra) - 15} more)")

    # Exit code reflects misplacements (the actual blockers)
    sys.exit(1 if misplaced else 0)


if __name__ == "__main__":
    main()
