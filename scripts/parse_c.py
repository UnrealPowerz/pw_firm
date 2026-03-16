#!/usr/bin/env python3
"""parse_c.py - Extract per-function cross-references from C source files.

For each function definition, extracts:
  - calls:      functions called directly, or referenced as function pointers
  - call_ptrs:  subset of calls used as values (not direct call targets)
  - data_refs:  global/extern variables accessed (reads and writes)
  - big_consts: integer literals >= 0x2C92 (potential data/IO addresses)

NOTE: IO registers from iodefine.h are accessed via _IOR/_IOW macros that
expand to pointer dereferences with hardcoded addresses. Those addresses appear
in big_consts (e.g. TCSRWD1 -> 0xF14B), not in data_refs.

Requires: pip install libclang

Usage:
  python3 parse_c.py                     # all *.c in project dir -> JSON
  python3 parse_c.py file1.c file2.c     # specific files -> JSON
  python3 parse_c.py --summary           # human-readable table
  python3 parse_c.py --func NAME         # detail for one function
"""

import sys
import re
import os
import glob
import json
from pathlib import Path

ADDR_THRESHOLD = 0x2C92
PROJECT_DIR = Path(__file__).parent.parent

try:
    import clang.cindex as clx
except ImportError:
    print("Error: libclang Python bindings not installed.", file=sys.stderr)
    print("Fix: pip install libclang", file=sys.stderr)
    sys.exit(1)

PARSE_ARGS = [
    '-std=c89',
    f'-I{PROJECT_DIR}',
    f'-I{PROJECT_DIR / "include"}',
    '-ferror-limit=0',
    '-w',   # suppress all warnings — we don't care about type correctness here
]

# ---------------------------------------------------------------------------
# Address extraction from source comments
# ---------------------------------------------------------------------------
# Matches:  /* Function: name\n   Address:  1a2b */
# or inline: /* Function: name  Address:  1a2b */
_ADDR_RE = re.compile(
    r'/\*\s*Function:\s*(\w+).*?Address:\s*([0-9a-fA-F]+)',
    re.DOTALL,
)

def extract_addresses(source: str) -> dict:
    """Return {func_name: int_addr} from Ghidra-style comments in source."""
    return {m.group(1): int(m.group(2), 16) for m in _ADDR_RE.finditer(source)}


# ---------------------------------------------------------------------------
# Core analysis
# ---------------------------------------------------------------------------

def _is_translation_unit(cursor):
    p = cursor.semantic_parent
    return p is not None and p.kind == clx.CursorKind.TRANSLATION_UNIT


def _token_int(cursor):
    """Return the integer value of an INTEGER_LITERAL cursor, or None."""
    toks = list(cursor.get_tokens())
    if not toks:
        return None
    raw = toks[0].spelling.rstrip('uUlL')
    try:
        if raw.startswith(('0x', '0X')):
            return int(raw, 16)
        elif raw.startswith('0') and len(raw) > 1:
            return int(raw, 8)
        else:
            return int(raw, 10)
    except ValueError:
        return None


def analyze_function(func_cursor):
    """
    Walk a FUNCTION_DECL (definition) and return:
      (calls, call_ptrs, data_refs, big_consts)
    all as sets.
    """
    calls = set()        # all function symbol references
    direct_calls = set() # functions appearing as direct CALL_EXPR callees
    data_refs = set()    # file-scope VAR_DECL references
    big_consts = set()   # integer literals >= ADDR_THRESHOLD

    for c in func_cursor.walk_preorder():
        kind = c.kind

        if kind == clx.CursorKind.CALL_EXPR:
            # Record the direct callee so we can separate call_ptrs later.
            ref = c.referenced
            if ref and ref.kind == clx.CursorKind.FUNCTION_DECL:
                name = ref.spelling
                if name:
                    direct_calls.add(name)

        elif kind == clx.CursorKind.DECL_REF_EXPR:
            ref = c.referenced
            if ref is None:
                continue
            name = ref.spelling
            if not name:
                continue
            ref_kind = ref.kind

            if ref_kind == clx.CursorKind.FUNCTION_DECL:
                calls.add(name)

            elif ref_kind == clx.CursorKind.VAR_DECL:
                # Only file-scope (global/extern) variables
                if _is_translation_unit(ref):
                    data_refs.add(name)

            # PARM_DECL, local VAR_DECL, TYPE_DECL etc. → ignore

        elif kind == clx.CursorKind.INTEGER_LITERAL:
            val = _token_int(c)
            if val is not None and val >= ADDR_THRESHOLD:
                big_consts.add(val)

    call_ptrs = calls - direct_calls
    return calls, call_ptrs, data_refs, big_consts


def analyze_file(path: str) -> list:
    """Parse one .c file and return a list of function-ref dicts."""
    path = str(path)
    with open(path, encoding='latin-1') as f:
        source = f.read()

    addr_map = extract_addresses(source)

    idx = clx.Index.create()
    tu = idx.parse(path, args=PARSE_ARGS)

    # Warn about hard errors (but continue)
    errors = [d for d in tu.diagnostics if d.severity >= 3]
    if errors:
        print(f"  [{os.path.basename(path)}] {len(errors)} parse error(s):", file=sys.stderr)
        for e in errors[:3]:
            print(f"    {e.spelling}", file=sys.stderr)

    results = []
    seen = set()  # deduplicate (declaration vs definition)

    for cursor in tu.cursor.walk_preorder():
        # Only process definitions in this file (skip included headers)
        if not (cursor.location.file and cursor.location.file.name == path):
            continue
        if cursor.kind != clx.CursorKind.FUNCTION_DECL:
            continue
        if not cursor.is_definition():
            continue

        name = cursor.spelling
        if name in seen:
            continue
        seen.add(name)

        calls, call_ptrs, data_refs, big_consts = analyze_function(cursor)
        addr = addr_map.get(name)

        results.append({
            'func': name,
            'addr': addr,
            'file': os.path.basename(path),
            'calls': sorted(calls),
            'call_ptrs': sorted(call_ptrs),
            'data_refs': sorted(data_refs),
            'big_consts': sorted(big_consts),
        })

    return results


# ---------------------------------------------------------------------------
# Multi-file driver
# ---------------------------------------------------------------------------

def analyze_all(paths) -> list:
    all_results = []
    for p in sorted(paths):
        print(f"  {os.path.basename(p)}", file=sys.stderr)
        all_results.extend(analyze_file(p))
    return all_results


# ---------------------------------------------------------------------------
# Output helpers
# ---------------------------------------------------------------------------

def to_json(results) -> str:
    out = []
    for r in results:
        out.append({
            'func': r['func'],
            'addr': f"0x{r['addr']:04x}" if r['addr'] is not None else None,
            'file': r['file'],
            'calls': r['calls'],
            'call_ptrs': r['call_ptrs'],
            'data_refs': r['data_refs'],
            'big_consts': [f"0x{v:x}" for v in r['big_consts']],
        })
    return json.dumps(out, indent=2)


def print_summary(results):
    header = (
        f"{'Function':<45} {'File':<25} {'Addr':>6}"
        f"  {'Calls':>5}  {'DataRefs':>8}  {'BigConsts':>9}"
    )
    print(header)
    print('-' * len(header))
    for r in results:
        addr_s = f"0x{r['addr']:04x}" if r['addr'] is not None else '?'
        print(
            f"{r['func']:<45} {r['file']:<25} {addr_s:>6}"
            f"  {len(r['calls']):>5}  {len(r['data_refs']):>8}  {len(r['big_consts']):>9}"
        )


def print_func_detail(results, name):
    for r in results:
        if r['func'] == name:
            addr_s = f"0x{r['addr']:04x}" if r['addr'] is not None else '?'
            print(f"Function: {r['func']}  addr={addr_s}  file={r['file']}")
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
    paths = []
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
        elif a.endswith('.c'):
            paths.append(a)
        i += 1

    if not paths:
        paths = sorted(Path(PROJECT_DIR / 'src').rglob('*.c'))

    print(f"Analyzing {len(paths)} C file(s)...", file=sys.stderr)
    results = analyze_all(paths)
    print(f"  {len(results)} function definitions found", file=sys.stderr)

    if mode == 'json':
        print(to_json(results))
    elif mode == 'summary':
        print_summary(results)
    elif mode == 'func':
        print_func_detail(results, func_filter)


if __name__ == '__main__':
    main()
