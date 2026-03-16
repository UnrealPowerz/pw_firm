#!/usr/bin/env python3
"""compare_refs.py - Diff ASM vs C cross-references for decompilation review.

For each function, shows what the original assembly does that the C code
doesn't, and vice versa, across three categories:
  calls  - function calls and function-pointer references
  data   - global/extern variable accesses
  io     - IO register accesses (named in ASM, via _IOR addr in C big_consts)
  const  - large immediate constants >= 0x2C92 (ROM/EEPROM address references)

KNOWN ASYMMETRIES (filtered out automatically):
  ASM-only calls: common_prologue, common_epilogue_BA62  (compiler dedup routines)
  C-only calls:   _builtin_* from machine.h              (CCR/nop intrinsics)
  IO registers:   appear as named data_refs in ASM, as big_consts in C via _IOR(addr)

Usage:
  python3 compare_refs.py                    # full diff report (diffs only)
  python3 compare_refs.py --all              # include exact-match functions
  python3 compare_refs.py --func NAME        # single function
  python3 compare_refs.py --file unit.c      # all functions in one C file
  python3 compare_refs.py asm.json c.json    # use pre-generated JSON files
"""

import sys
import re
import os
import json
import subprocess
from pathlib import Path
from collections import defaultdict

SCRIPTS_DIR = Path(__file__).parent
PROJECT_DIR = SCRIPTS_DIR.parent

# ---------------------------------------------------------------------------
# Known noise — exclude from diffs
# ---------------------------------------------------------------------------

# ASM calls that never appear in C (compiler-generated shared prologues/epilogues)
ASM_NOISE_CALLS = {
    'common_prologue',
    'common_epilogue_BA62',
}

# C calls that never appear in ASM (compiler intrinsics for CCR / nop access)
C_NOISE_CALLS = {
    n for n in [
        '_builtin_set_ccr', '_builtin_get_ccr',
        '_builtin_and_ccr', '_builtin_or_ccr', '_builtin_xor_ccr',
        '_builtin_set_imask_ccr', '_builtin_get_imask_ccr',
        '_builtin_nop', '_builtin_sleep', '_builtin_trapa',
    ]
}

# ---------------------------------------------------------------------------
# IO register map from iodefine.h
# ---------------------------------------------------------------------------

def load_io_regs(path=PROJECT_DIR / 'iodefine.h'):
    """Return {name: addr} and {addr: name} for all _IOR/_IOW macros."""
    name2addr, addr2name = {}, {}
    try:
        with open(path, encoding='latin-1') as f:
            for line in f:
                m = re.match(r'#define\s+(\w+)\s+_IO[RW]\((0x[0-9A-Fa-f]+)\)', line)
                if m:
                    name = m.group(1)
                    addr = int(m.group(2), 16)
                    name2addr[name] = addr
                    addr2name[addr] = name
    except FileNotFoundError:
        pass
    return name2addr, addr2name

# ---------------------------------------------------------------------------
# Data loading
# ---------------------------------------------------------------------------

def _run(script):
    result = subprocess.run(
        ['python3', str(SCRIPTS_DIR / script)],
        capture_output=True, text=True,
        cwd=PROJECT_DIR,
    )
    if result.returncode != 0:
        print(f"Error running {script}:\n{result.stderr}", file=sys.stderr)
        sys.exit(1)
    return json.loads(result.stdout)


# ---------------------------------------------------------------------------
# Ignore list loading
# ---------------------------------------------------------------------------

def load_ignores(path=PROJECT_DIR / 'ignore_refs.txt'):
    """
    Load ignore list from file.
    Format: function:category:subset:item # explanation
    Returns: {func: {category: {subset: {item}}}}
    """
    ignores = defaultdict(lambda: defaultdict(lambda: defaultdict(set)))
    try:
        with open(path, encoding='utf-8') as f:
            for line in f:
                line = line.split('#')[0].strip()
                if not line:
                    continue
                parts = line.split(':')
                if len(parts) == 4:
                    func, cat, subset, item = parts
                    ignores[func][cat][subset].add(item)
    except FileNotFoundError:
        pass
    return ignores


def load_data(asm_path=None, c_path=None):
    """Load ASM and C ref data. Run parsers if JSON paths not given."""
    if asm_path:
        with open(asm_path) as f:
            asm_list = json.load(f)
    else:
        print("Running parse_mar.py...", file=sys.stderr)
        asm_list = _run('parse_mar.py')

    if c_path:
        with open(c_path) as f:
            c_list = json.load(f)
    else:
        print("Running parse_c.py...", file=sys.stderr)
        c_list = _run('parse_c.py')

    asm = {f['func']: f for f in asm_list}
    c   = {f['func']: f for f in c_list}
    ignores = load_ignores()
    return asm, c, ignores

# ---------------------------------------------------------------------------
# Address-named label helpers
# ---------------------------------------------------------------------------

# Labels like L_BF98, DAT_bc74, DAT_005c whose hex suffix IS their address.
# These can cross-match with C big_consts of the same value.
_ADDR_LABEL_RE = re.compile(r'^(?:L_|DAT_)([0-9A-Fa-f]+)$')

def label_to_addr(label):
    """If label encodes its own address (L_XXXX / DAT_XXXX), return that int, else None."""
    m = _ADDR_LABEL_RE.match(label)
    return int(m.group(1), 16) if m else None


# ---------------------------------------------------------------------------
# Per-function comparison
# ---------------------------------------------------------------------------

def compare_func(name, af, cf, io_name2addr, io_addr2name, ignores):
    """
    Return a dict describing all differences between the ASM and C versions.
    """
    func_ignores = ignores.get(name, {})

    # --- calls ---
    asm_calls = {c for c in af['calls'] if not (
        c.startswith('common_prologue') or
        c.startswith('common_epilogue_') or
        c.startswith('sys_epilogue_')
    )}
    c_calls   = set(cf['calls']) - C_NOISE_CALLS

    calls_missing = sorted((asm_calls - c_calls) - func_ignores.get('calls', {}).get('missing', set()))
    calls_extra   = sorted((c_calls - asm_calls) - func_ignores.get('calls', {}).get('extra', set()))

    # --- data refs ---
    asm_io_refs = {r: io_name2addr[r] for r in af['data_refs'] if r in io_name2addr}
    # Filter out switch tables and common displacements
    asm_data    = {r for r in af['data_refs'] if r not in io_name2addr and not (
        '__switchdataD_' in r or r in ('L_FFFE', 'L_FFFF')
    )}
    c_data      = set(cf['data_refs'])   # no IO regs here (they're in big_consts)

    # --- IO registers ---
    # C's big_consts that are known IO register addresses
    c_all_consts  = {int(v, 16) for v in cf['big_consts']}
    c_io_consts   = {v: io_addr2name[v] for v in c_all_consts if v in io_addr2name}
    c_data_consts = c_all_consts - set(c_io_consts)

    # --- Address-named label cross-match ---
    # For ASM data_refs that are NOT matched by name in C (e.g. "L_BF98", "DAT_bc74"),
    # check if C accesses the same address as a raw big_const instead.
    # Only applies to labels with hex-encoded addresses (L_XXXX / DAT_XXXX pattern).
    asm_unmatched_by_name = asm_data - c_data   # in ASM but not in C by name
    addr_label_cross = {}   # label -> addr (ASM label matched to C big_const)
    addr_label_missing_set = set()

    for r in asm_unmatched_by_name:
        addr = label_to_addr(r)
        if addr is not None and addr in c_data_consts:
            addr_label_cross[r] = addr
            c_data_consts = c_data_consts - {addr}   # consume so it doesn't show in const_extra
        else:
            addr_label_missing_set.add(r)

    # --- large constants (non-IO, non-addr-label) ---
    asm_consts = {int(v, 16) for v in af['big_consts']}

    # --- IO register cross-match ---
    asm_io_addrs     = set(asm_io_refs.values())
    c_io_addrs       = set(c_io_consts)
    io_matched_addrs = asm_io_addrs & c_io_addrs

    io_matched = sorted(
        f"{io_addr2name[addr]}(0x{addr:x})"
        for addr in io_matched_addrs
    )

    # Build data missing/extra, incorporating addr-label cross-matches
    # Truly missing from C: not matched by name AND not cross-matched by address
    data_missing = sorted((addr_label_missing_set | (asm_data - c_data - set(addr_label_cross))) - func_ignores.get('data', {}).get('missing', set()))
    data_extra   = sorted((c_data - asm_data) - func_ignores.get('data', {}).get('extra', set()))

    io_missing_set = {f"{lbl}(0x{addr:x})" for lbl, addr in asm_io_refs.items() if addr not in c_io_addrs}
    io_extra_set   = {f"0x{addr:x}→{io_addr2name[addr]}" for addr in c_io_consts if addr not in asm_io_addrs}

    io_missing = sorted(io_missing_set - func_ignores.get('io', {}).get('missing', set()))
    io_extra   = sorted(io_extra_set - func_ignores.get('io', {}).get('extra', set()))

    # --- large constants (non-IO, non-addr-label) ---
    const_missing_set = {f"0x{v:x}" for v in asm_consts - c_data_consts}
    const_extra_set   = {f"0x{v:x}" for v in c_data_consts - asm_consts}

    const_missing = sorted(const_missing_set - func_ignores.get('const', {}).get('missing', set()))
    const_extra   = sorted(const_extra_set - func_ignores.get('const', {}).get('extra', set()))

    return {
        'calls_missing': calls_missing,
        'calls_extra':   calls_extra,
        'data_missing':  data_missing,
        'data_extra':    data_extra,
        'io_missing':    io_missing,
        'io_extra':      io_extra,
        'io_matched':    io_matched,
        'const_missing': const_missing,
        'const_extra':   const_extra,
    }


def has_diff(cmp):
    return any(cmp[k] for k in (
        'calls_missing', 'calls_extra',
        'data_missing',  'data_extra',
        'io_missing',    'io_extra',
        'const_missing', 'const_extra',
    ))

# ---------------------------------------------------------------------------
# Text formatting
# ---------------------------------------------------------------------------

def _fmt_row(label, missing, extra, *, indent='    '):
    lines = []
    if missing:
        lines.append(f"{indent}{label}  missing : {', '.join(missing)}")
    if extra:
        lines.append(f"{indent}{label}  extra   : {', '.join(extra)}")
    return lines


def format_func(name, af, cf, cmp, *, verbose=False):
    """Format one function's diff block."""
    asm_addr = af.get('addr')  # may be int or "0x..." string or None
    addr_s   = asm_addr if asm_addr else '?'
    c_file   = cf.get('file', '?')
    c_addr   = cf.get('addr')
    c_addr_s = c_addr if c_addr else ''

    header = f"[{'DIFF' if has_diff(cmp) else 'OK  '}]  {name}"
    info   = f"  asm:{addr_s}  c:{c_file}"
    if c_addr_s:
        info += f"@{c_addr_s}"

    lines = [header + info]

    if has_diff(cmp) or verbose:
        lines += _fmt_row('calls', cmp['calls_missing'], cmp['calls_extra'])
        lines += _fmt_row('data ', cmp['data_missing'],  cmp['data_extra'])
        lines += _fmt_row('io   ', cmp['io_missing'],    cmp['io_extra'])
        lines += _fmt_row('const', cmp['const_missing'], cmp['const_extra'])
        if verbose and cmp['io_matched']:
            lines.append(f"    io    matched  : {', '.join(cmp['io_matched'])}")

    return '\n'.join(lines)


def format_report(asm, c, ignores, io_name2addr, io_addr2name,
                  *, show_all=False, file_filter=None, func_filter=None):
    lines = []

    # Group C functions by file
    by_file = defaultdict(list)
    for name, cf in c.items():
        by_file[cf.get('file', '?')].append(name)

    # Functions only in ASM
    asm_only = sorted(n for n in asm if n not in c)

    # --- Header ---
    lines += [
        '=' * 72,
        'ASSEMBLY vs C REFERENCE COMPARISON',
        f'  ASM functions : {len(asm)}',
        f'  C functions   : {len(c)}',
        f'  Not in C yet  : {len(asm_only)}',
        '=' * 72,
        '',
    ]

    # --- Functions missing from C ---
    if asm_only and not func_filter and not file_filter:
        lines.append('FUNCTIONS NOT YET IN C:')
        for n in asm_only:
            af = asm[n]
            addr_s = af['addr'] if af.get('addr') else '?'
            lines.append(f"  {n:<50s} asm:{addr_s}")
        lines += ['', '=' * 72, '']

    # --- Per-function diffs, grouped by file ---
    if func_filter:
        # Single function
        name = func_filter
        if name not in asm:
            lines.append(f"ERROR: '{name}' not found in ASM data.")
        elif name not in c:
            af = asm[name]
            addr_s = f"0x{af['addr']:04x}" if af.get('addr') else '?'
            lines.append(f"'{name}' (asm:{addr_s}) has no C implementation yet.")
        else:
            cmp = compare_func(name, asm[name], c[name], io_name2addr, io_addr2name, ignores)
            lines.append(format_func(name, asm[name], c[name], cmp, verbose=True))
        return '\n'.join(lines)

    files_to_show = sorted(by_file)
    if file_filter:
        files_to_show = [f for f in files_to_show if f == file_filter]
        if not files_to_show:
            lines.append(f"No C functions found for file '{file_filter}'.")
            return '\n'.join(lines)

    total_diff = total_ok = 0

    for fname in files_to_show:
        func_names = sorted(by_file[fname])
        file_funcs = []
        for name in func_names:
            if name not in asm:
                continue
            cmp = compare_func(name, asm[name], c[name], io_name2addr, io_addr2name, ignores)
            is_diff = has_diff(cmp)
            if is_diff:
                total_diff += 1
            else:
                total_ok += 1
            if is_diff or show_all:
                file_funcs.append((name, cmp))

        if file_funcs:
            lines.append(f'── {fname} ' + '─' * max(0, 68 - len(fname)))
            for name, cmp in file_funcs:
                lines.append(format_func(name, asm[name], c[name], cmp))
            lines.append('')

    lines += [
        '=' * 72,
        f'SUMMARY: {total_diff} functions with diffs, {total_ok} exact matches',
        '=' * 72,
    ]

    return '\n'.join(lines)

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    asm_path = c_path = None
    show_all = False
    func_filter = file_filter = None

    args = sys.argv[1:]
    i = 0
    while i < len(args):
        a = args[i]
        if a == '--all':
            show_all = True
        elif a == '--func':
            i += 1
            func_filter = args[i]
        elif a == '--file':
            i += 1
            file_filter = args[i]
        elif a.endswith('.json'):
            if asm_path is None:
                asm_path = a
            else:
                c_path = a
        i += 1

    io_name2addr, io_addr2name = load_io_regs()
    print(f"IO registers from iodefine.h: {len(io_name2addr)}", file=sys.stderr)

    asm, c, ignores = load_data(asm_path, c_path)
    print(f"Loaded {len(asm)} ASM functions, {len(c)} C functions", file=sys.stderr)

    report = format_report(
        asm, c, ignores, io_name2addr, io_addr2name,
        show_all=show_all,
        file_filter=file_filter,
        func_filter=func_filter,
    )
    print(report)


if __name__ == '__main__':
    main()
