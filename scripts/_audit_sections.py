#!/usr/bin/env python3
"""Validate main.mar data sections.

For each .SECTION B_*, walk the directives in order, counting bytes from
both labeled and *unlabeled* .RES/.DATA blocks. Report:
  - total byte count per section
  - per-label (name, offset, size)
  - any anonymous (unlabeled) bytes as `<gap>`
  - whether the total matches the expected hardware address range
"""
import re, pathlib
from collections import OrderedDict

p = pathlib.Path("/Users/aaronl/dev/pw_firm/main.mar")
lines = p.read_text().splitlines()

# H8/3937 hardware-derived expected ranges for the data sections.
EXPECTED = {
    "B_IO_REG_1": (0xF020, 0xF0FF),   # 0xE0 = 224  (FLMCR1 family)
    "B_UNUSED_2": (0xF100, 0xF77F),   # 0x680 = 1664 (104 anon + 3 named blocks)
    "B_RAM":      (0xF780, 0xFF7F),   # 0x800 = 2048
    "B_IO_REG2":  (0xFF80, 0xFFFF),   # 0x80  = 128
}

LABEL_RE  = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*):\s*$")
SECT_RE   = re.compile(r"^\s*\.SECTION\s+([A-Za-z_][A-Za-z0-9_]*)")
RESB_RE   = re.compile(r"^\s*\.RES\.B\s+(\d+)")
RESW_RE   = re.compile(r"^\s*\.RES\.W\s+(\d+)")
DATAB_RE  = re.compile(r"^\s*\.DATA\.B\s+(.+)$")
DATAW_RE  = re.compile(r"^\s*\.DATA\.W\s+(.+)$")
SDATAZ_RE = re.compile(r"^\s*\.SDATAZ\s+\"([^\"]*)\"")

# section -> list of (name_or_None, size)
sections = OrderedDict()
section = None
pending_labels = []   # labels seen but not yet sized

def emit(size):
    """Attribute `size` bytes to the current pending label(s).
    If multiple labels are pending, the first one(s) are zero-sized aliases
    sharing the address of the final label that actually gets the bytes."""
    global pending_labels
    if not section:
        return
    if not pending_labels:
        sections[section].append((None, size))
        return
    # All but the last pending label are 0-byte aliases at the same offset
    for n in pending_labels[:-1]:
        sections[section].append((n, 0))
    sections[section].append((pending_labels[-1], size))
    pending_labels = []

for ln in lines:
    m = SECT_RE.match(ln)
    if m:
        section = m.group(1)
        sections.setdefault(section, [])
        pending_labels = []
        continue
    m = LABEL_RE.match(ln)
    if m and section:
        pending_labels.append(m.group(1))
        continue
    m = RESB_RE.match(ln)
    if m:
        emit(int(m.group(1))); continue
    m = RESW_RE.match(ln)
    if m:
        emit(int(m.group(1)) * 2); continue
    m = DATAB_RE.match(ln)
    if m:
        n = len([x for x in m.group(1).split(",") if x.strip()])
        emit(n); continue
    m = DATAW_RE.match(ln)
    if m:
        n = len([x for x in m.group(1).split(",") if x.strip()])
        emit(n * 2); continue
    m = SDATAZ_RE.match(ln)
    if m:
        emit(len(m.group(1)) + 1); continue

# Report per-section: total + coverage + gaps
print(f"{'section':<15} {'#sym':>5} {'gaps':>5} {'bytes':>7} {'expected':>9} {'status':>9}")
print("-" * 64)
for sec, items in sections.items():
    if "B_" not in sec:
        # ignore P_ram (code)
        continue
    nsym  = sum(1 for n,_ in items if n is not None)
    ngap  = sum(1 for n,_ in items if n is None)
    total = sum(s for _, s in items)
    exp   = EXPECTED.get(sec)
    if exp:
        exp_size = exp[1] - exp[0] + 1
        status = "OK" if total == exp_size else f"DIFF {total - exp_size:+}"
    else:
        exp_size = "?"
        status = "no-ref"
    print(f"{sec:<15} {nsym:>5} {ngap:>5} {total:>7} {exp_size:>9} {status:>9}")

# Per-section detail of gaps
print()
for sec, items in sections.items():
    if "B_" not in sec:
        continue
    start = EXPECTED[sec][0]
    print(f"=== {sec} (start 0x{start:04X}) ===")
    off = 0
    for name, sz in items:
        if name is None and sz > 0:
            print(f"  0x{start+off:04X} {'<gap>':<28} {sz} B")
        off += sz

# Save name -> address map for B_RAM (the one we'll use later)
out_lines = []
for sec, items in sections.items():
    if sec not in EXPECTED:
        continue
    start = EXPECTED[sec][0]
    off = 0
    for name, sz in items:
        if name:
            out_lines.append(f"{start+off:04x} {name} {sec}")
        off += sz
pathlib.Path("/tmp/main_mar_data_map.txt").write_text("\n".join(out_lines) + "\n")
print(f"\nwrote /tmp/main_mar_data_map.txt ({len(out_lines)} symbols)")
