#!/usr/bin/env python3
"""parse_mar.py - Analyze main.mar and extract per-function cross-references.

For each function, outputs:
  - calls:      functions called directly (jsr/bsr/jmp) or by address (#funcname)
  - call_ptrs:  functions whose addresses are loaded via #funcname (subset of calls)
  - data_refs:  named data/IO labels accessed via @label or #label
  - big_consts: raw hex immediates >= ADDR_THRESHOLD that could be data addresses

Usage:
  python3 parse_mar.py [main.mar] > refs.json
  python3 parse_mar.py [main.mar] --summary    (human-readable table)
  python3 parse_mar.py [main.mar] --func NAME  (detail for one function)
"""

import re
import sys
import json
from collections import defaultdict

ADDR_THRESHOLD = 0x2C92  # minimum constant to flag as potential address

# Branch mnemonics whose operands are always local labels — never external refs
BRANCH_MNEMONICS = {
    'beq', 'bne', 'bra', 'bcc', 'bcs', 'bls', 'bhi',
    'bge', 'bgt', 'ble', 'blt', 'bpl', 'bmi', 'bvs', 'bvc',
}

# Register name pattern — @r0, @er0, @er6+, etc. are not label references
REG_PAT = re.compile(
    r'^(r[0-7][lh]?|e[0-7]|er[0-7]|e0|r7|sp|pc|ccr|mach|macl)$',
    re.IGNORECASE,
)


def is_local_label(name):
    """LAB_XXXX labels are always local branch targets within a function."""
    return bool(re.match(r'^LAB_[0-9A-Fa-f]+$', name))


# ---------------------------------------------------------------------------
# Phase 1: collect all label definitions and function names
# ---------------------------------------------------------------------------

def collect_labels(lines):
    """Return (func_names, all_labels, label_section) from a full parse of lines."""
    func_names = set()
    all_labels = set()
    label_section = {}   # label -> section name
    current_section = 'P_ram'

    for line in lines:
        # Section changes
        m = re.match(r'\s+\.SECTION\s+(\w+)', line)
        if m:
            current_section = m.group(1)
            continue

        # Function names from Ghidra header comments
        m = re.match(r';\s+Function:\s+(\S+)', line)
        if m:
            func_names.add(m.group(1))
            continue

        # Label definitions: "name:" on a line by itself (optional trailing comment)
        m = re.match(r'^([A-Za-z_][A-Za-z0-9_]*):\s*(?:;.*)?$', line)
        if m:
            lbl = m.group(1)
            all_labels.add(lbl)
            label_section[lbl] = current_section

    return func_names, all_labels, label_section


# ---------------------------------------------------------------------------
# Phase 2: parse function bodies
# ---------------------------------------------------------------------------

def parse_functions(lines, func_names, all_labels):
    """Yield one dict per function with calls/data_refs/big_consts."""

    # Data labels = all named labels that are not function names and not LAB_XXXX
    data_labels = {
        l for l in all_labels
        if l not in func_names and not is_local_label(l)
    }

    current_func = None
    func_addr = None
    func_calls = set()       # direct calls (jsr/bsr/jmp to function)
    func_call_ptrs = set()   # addresses of functions loaded as immediates (#name)
    data_refs = set()        # @label and #label for data labels
    big_consts = set()       # raw int values >= ADDR_THRESHOLD

    def flush():
        if current_func is not None:
            yield_val = {
                'func': current_func,
                'addr': func_addr,
                'calls': sorted(func_calls | func_call_ptrs),
                'call_ptrs': sorted(func_call_ptrs),
                'data_refs': sorted(data_refs),
                'big_consts': sorted(big_consts),
            }
            return yield_val
        return None

    results = []

    for line in lines:
        # ---- New function header ----
        m = re.match(r';\s+Function:\s+(\S+)', line)
        if m:
            r = flush()
            if r:
                results.append(r)
            current_func = m.group(1)
            func_addr = None
            func_calls = set()
            func_call_ptrs = set()
            data_refs = set()
            big_consts = set()
            continue

        if current_func is None:
            continue

        # Address from header comment
        m = re.match(r';\s+Address:\s+([0-9a-fA-F]+)', line)
        if m and func_addr is None:
            func_addr = int(m.group(1), 16)
            continue

        # Skip blank lines and full comment lines
        stripped = line.strip()
        if not stripped or stripped.startswith(';'):
            continue

        # Skip label-definition-only lines inside the function body
        if re.match(r'^[A-Za-z_][A-Za-z0-9_]*:\s*(?:;.*)?$', stripped):
            continue

        # Strip trailing comments; get the code portion
        code = re.sub(r'\s*;.*', '', line).strip()
        if not code:
            continue

        # Parse mnemonic and operand string
        parts = re.split(r'\s+', code, maxsplit=1)
        mnemonic = parts[0].lower()
        operands = parts[1] if len(parts) > 1 else ''

        # ---- Calls: jsr / bsr ----
        if mnemonic in ('jsr', 'bsr'):
            # jsr @funcname:16  |  bsr funcname:8  |  jsr @r0 (indirect, skip)
            m = re.match(r'@?([A-Za-z_][A-Za-z0-9_]*)', operands)
            if m:
                lbl = m.group(1)
                if not REG_PAT.match(lbl) and lbl in func_names:
                    func_calls.add(lbl)

        # ---- Tail-calls: jmp @funcname:16 ----
        elif mnemonic == 'jmp':
            m = re.match(r'@([A-Za-z_][A-Za-z0-9_]*)', operands)
            if m:
                lbl = m.group(1)
                if lbl in func_names:
                    func_calls.add(lbl)
                # If lbl is LAB_XXXX it's a local jump — already filtered by func_names check

        # ---- Branch instructions: operands are always local labels ----
        elif mnemonic in BRANCH_MNEMONICS:
            pass  # nothing to extract

        # ---- All other instructions: scan for @label and #label ----
        else:
            # @label  or  @(label, reg) — memory operands
            for m in re.finditer(r'@\(?([A-Za-z_][A-Za-z0-9_]*)', operands):
                lbl = m.group(1)
                if REG_PAT.match(lbl):
                    continue
                if lbl in func_names:
                    func_calls.add(lbl)   # unusual but possible (e.g. indirect via known address)
                elif lbl in data_labels:
                    data_refs.add(lbl)

            # #label (loading address of a symbol, NOT a hex literal)
            # Negative lookahead avoids matching #H'...
            for m in re.finditer(r"#(?!H')([A-Za-z_][A-Za-z0-9_]*)", operands):
                lbl = m.group(1)
                if lbl in func_names:
                    func_call_ptrs.add(lbl)
                elif lbl in data_labels:
                    data_refs.add(lbl)

        # ---- Large hex immediates (scan whole line including .DATA.W etc.) ----
        for m in re.finditer(r"#H'([0-9A-Fa-f]+)", line):
            val = int(m.group(1), 16)
            if val >= ADDR_THRESHOLD:
                big_consts.add(val)

    # Flush the last function
    r = flush()
    if r:
        results.append(r)

    return results


# ---------------------------------------------------------------------------
# Output helpers
# ---------------------------------------------------------------------------

def to_json(results):
    out = []
    for r in results:
        out.append({
            'func': r['func'],
            'addr': f"0x{r['addr']:04x}" if r['addr'] is not None else None,
            'calls': r['calls'],
            'call_ptrs': r['call_ptrs'],
            'data_refs': r['data_refs'],
            'big_consts': [f"0x{v:x}" for v in r['big_consts']],
        })
    return json.dumps(out, indent=2)


def print_summary(results):
    header = f"{'Function':<40} {'Addr':>6}  {'Calls':>5}  {'DataRefs':>8}  {'BigConsts':>9}"
    print(header)
    print('-' * len(header))
    for r in results:
        addr_s = f"0x{r['addr']:04x}" if r['addr'] is not None else '?'
        print(
            f"{r['func']:<40} {addr_s:>6}  "
            f"{len(r['calls']):>5}  {len(r['data_refs']):>8}  {len(r['big_consts']):>9}"
        )


def print_func_detail(results, name):
    for r in results:
        if r['func'] == name:
            addr_s = f"0x{r['addr']:04x}" if r['addr'] is not None else '?'
            print(f"Function: {r['func']}  addr={addr_s}")
            print()
            print(f"  Calls ({len(r['calls'])}):")
            for c in r['calls']:
                tag = ' [ptr]' if c in r['call_ptrs'] else ''
                print(f"    {c}{tag}")
            print()
            print(f"  Data refs ({len(r['data_refs'])}):")
            for d in r['data_refs']:
                print(f"    {d}")
            print()
            print(f"  Big constants ({len(r['big_consts'])}):")
            for v in r['big_consts']:
                print(f"    0x{v:x}  ({v})")
            return
    print(f"Function '{name}' not found.", file=sys.stderr)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    path = 'main.mar'
    mode = 'json'
    func_filter = None

    args = sys.argv[1:]
    i = 0
    while i < len(args):
        a = args[i]
        if a == '--summary':
            mode = 'summary'
        elif a == '--func':
            mode = 'func'
            i += 1
            func_filter = args[i]
        elif not a.startswith('--'):
            path = a
        i += 1

    print(f"Parsing {path}...", file=sys.stderr)
    with open(path, 'r', encoding='latin-1') as f:
        lines = f.readlines()

    func_names, all_labels, label_section = collect_labels(lines)
    print(
        f"  {len(func_names)} function names, "
        f"{len(all_labels)} total labels, "
        f"{len(all_labels) - len(func_names)} data/IO labels",
        file=sys.stderr,
    )

    results = parse_functions(lines, func_names, all_labels)
    print(f"  {len(results)} functions parsed", file=sys.stderr)

    if mode == 'json':
        print(to_json(results))
    elif mode == 'summary':
        print_summary(results)
    elif mode == 'func':
        print_func_detail(results, func_filter)


if __name__ == '__main__':
    main()
