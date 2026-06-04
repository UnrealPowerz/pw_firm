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
    f'-I{PROJECT_DIR / "h8inc"}',          # Renesas ch38 standard headers
    f'-I{PROJECT_DIR / "build/gen"}',      # auto-generated module headers
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


_FILE_CACHE = {}

def _read_extent(cursor):
    """Read the raw source text of `cursor.extent`. Used as a fallback when
    libclang's `get_tokens()` returns empty — which happens for INTEGER_LITERAL
    nodes produced by macro expansions like `*(volatile uint8_t *)0xF7B5u`
    (no enclosing parens around the literal). The literal's location points
    back to the macro definition file, which we can read directly."""
    e = cursor.extent
    if e.start.file is None or e.start.file.name != e.end.file.name:
        return None
    path = e.start.file.name
    if path not in _FILE_CACHE:
        try:
            _FILE_CACHE[path] = open(path, encoding='latin-1').read().encode('latin-1')
        except OSError:
            _FILE_CACHE[path] = None
    src = _FILE_CACHE[path]
    if src is None:
        return None
    return src[e.start.offset:e.end.offset].decode('latin-1', errors='replace')


def _token_int(cursor):
    """Return the integer value of an INTEGER_LITERAL cursor, or None.
    Tries `get_tokens()` first, then falls back to reading the source extent
    for macro-expanded literals where tokenization breaks."""
    toks = list(cursor.get_tokens())
    raw = toks[0].spelling if toks else _read_extent(cursor)
    if not raw:
        return None
    raw = raw.strip().rstrip('uUlL')
    try:
        if raw.startswith(('0x', '0X')):
            return int(raw, 16)
        elif raw.startswith('0') and len(raw) > 1:
            return int(raw, 8)
        else:
            return int(raw, 10)
    except ValueError:
        return None


_MACRO_RE = re.compile(
    r'#define\s+(\w+)\s+\(\s*\*\s*\(\s*(?:volatile\s+)?(?:struct\s+\w+|union\s+\w+|\w+)\s*(?:\s*\*\s*)+\)\s*'
    r'(0[xX][0-9A-Fa-f]+)[uU]?\s*\)'
)
# Pointer-only macros: `#define wakeupFlagMaybe ((uint8_t *)0xF7BBu)` — used
# as a base pointer (no deref at the macro site), e.g. `wakeupFlagMaybe[0]`.
_PTR_RE = re.compile(
    r'#define\s+(\w+)\s+\(\s*\(\s*(?:volatile\s+)?(?:struct\s+\w+|union\s+\w+|\w+)\s*(?:\s*\*\s*)+\)\s*'
    r'(0[xX][0-9A-Fa-f]+)[uU]?\s*\)'
)
# Plain integer constants: `#define EEPROM_TRAINER_PROFILE 0x8F00` — values
# passed to library functions (eeprom read/write, etc). libclang silently
# drops these from INTEGER_LITERAL extraction when the macro is just a bare
# literal, so we backfill via the same textual scan path.
_CONST_RE = re.compile(
    r'#define\s+(\w+)\s+(0[xX][0-9A-Fa-f]+)[uU]?\s*(?:/[/\*].*)?$',
    re.MULTILINE
)
# *_BIT macros: `#define statusFlags_BIT (((volatile T *)&statusFlags)->BIT)`
# inherits the address of the referenced base macro.
_BIT_RE = re.compile(
    r'#define\s+(\w+_BIT)\s+\(\(\(volatile\s+\w+\s*\*\)\s*&\s*(\w+)\s*\)\s*->\s*BIT\)'
)
# struct base macros: `#define g_save (*(struct session_save *)0xF780u)`
_STRUCT_RE = re.compile(
    r'#define\s+(\w+)\s+\(\s*\*\s*\(\s*struct\s+\w+\s*\*\s*\)\s*'
    r'(0[xX][0-9A-Fa-f]+)[uU]?\s*\)'
)
# struct field aliases: `#define totalSteps (g_save.totalSteps)`
_FIELD_RE = re.compile(r'#define\s+(\w+)\s+\(\s*(\w+)\.(\w+)\s*\)')

_MACRO_ADDR_CACHE = None
def _load_macro_addrs():
    """Build {macro_name -> int_addr} for header macros that expand to a
    fixed memory address. Covers three patterns from globals.h:
      (1) `(*(volatile T *)0xADDRu)`              — simple volatile globals
      (2) `(((volatile T *)&BASE)->BIT)`          — *_BIT aliases
      (3) `(g_save.field)` for struct-wrapped     — addr = base + offsetof
    Used to backfill big_consts for macro-expanded INTEGER_LITERAL nodes."""
    global _MACRO_ADDR_CACHE
    if _MACRO_ADDR_CACHE is not None:
        return _MACRO_ADDR_CACHE
    m = {}
    struct_bases = {}     # macro_name -> base_addr  (for g_save-like)
    field_aliases = []    # list of (alias, base_macro, field_name)
    bit_aliases = []      # list of (alias, base_macro)
    for header in (PROJECT_DIR / 'include').rglob('*.h'):
        try:
            text = header.read_text(encoding='latin-1')
        except OSError:
            continue
        for match in _MACRO_RE.finditer(text):
            m[match.group(1)] = int(match.group(2), 16)
        for match in _PTR_RE.finditer(text):
            m.setdefault(match.group(1), int(match.group(2), 16))
        for match in _CONST_RE.finditer(text):
            m.setdefault(match.group(1), int(match.group(2), 16))
        for match in _STRUCT_RE.finditer(text):
            base, addr = match.group(1), int(match.group(2), 16)
            m[base] = addr
            struct_bases[base] = addr
        for match in _BIT_RE.finditer(text):
            bit_aliases.append((match.group(1), match.group(2)))
        for match in _FIELD_RE.finditer(text):
            field_aliases.append((match.group(1), match.group(2), match.group(3)))

    # Resolve *_BIT macros → inherit base address (one byte, same address)
    for alias, base in bit_aliases:
        if base in m:
            m.setdefault(alias, m[base])

    # Resolve struct-field aliases: addr = struct_base + offsetof(field).
    # Parse the referenced struct definition for field offsets so each alias
    # resolves to its exact byte (e.g. g_save.watts → 0xF78E, not 0xF780).
    type_sizes = {'uint8_t': 1, 'int8_t': 1, 'uint16_t': 2, 'int16_t': 2,
                  'uint32_t': 4, 'int32_t': 4}
    def _struct_offsets(text, struct_name):
        body = re.search(rf'struct\s+{struct_name}\s*\{{(.*?)\}}\s*;',
                         text, re.DOTALL)
        if not body:
            return None
        # Strip C and C++ comments before splitting on ';' — multi-line
        # comments inside field declarations can otherwise contain
        # semicolons that confuse the field parser.
        b = re.sub(r'/\*.*?\*/', '', body.group(1), flags=re.DOTALL)
        b = re.sub(r'//[^\n]*', '', b)
        offsets = {}
        off = 0
        for ln in b.split(';'):
            ln = ln.strip()
            if not ln:
                continue
            mm = re.match(r'(?:volatile\s+)?(\w+)\s+(\w+)(?:\s*\[\s*(\d+)\s*\])?', ln)
            if not mm:
                continue
            ty, name, arr = mm.group(1), mm.group(2), mm.group(3)
            sz = type_sizes.get(ty, 1)
            if arr:
                sz *= int(arr)
            offsets[name] = (off, sz)
            off += sz
        return offsets

    # Map struct base macro -> (struct_type_name, header_text)
    struct_meta = {}  # base_name -> (struct_name, header_text)
    base_struct_re = re.compile(
        r'#define\s+(\w+)\s+\(\s*\*\s*\(\s*struct\s+(\w+)\s*\*\s*\)\s*0[xX][0-9A-Fa-f]+'
    )
    for header in (PROJECT_DIR / 'include').rglob('*.h'):
        try:
            text = header.read_text(encoding='latin-1')
        except OSError:
            continue
        for mm in base_struct_re.finditer(text):
            base, stname = mm.group(1), mm.group(2)
            struct_meta[base] = (stname, text)

    for alias, base, field in field_aliases:
        if base in struct_bases and base in struct_meta:
            stname, htext = struct_meta[base]
            offsets = _struct_offsets(htext, stname)
            if offsets and field in offsets:
                m.setdefault(alias, struct_bases[base] + offsets[field][0])
                continue
        # Fallback: base address
        if base in struct_bases:
            m.setdefault(alias, struct_bases[base])

    _MACRO_ADDR_CACHE = m
    return m


def _function_source(cursor):
    """Read the raw C source text for a function definition."""
    e = cursor.extent
    if e.start.file is None or e.start.file.name != e.end.file.name:
        return ''
    try:
        return open(e.start.file.name, encoding='latin-1').read()[e.start.offset:e.end.offset]
    except OSError:
        return ''


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

    # Backfill big_consts: scan the function's source text for any known
    # volatile-pointer macro names (statusFlags, accelXPos, ...) and add
    # their addresses. This recovers the literals libclang silently drops
    # when tokenizing macro-expanded INTEGER_LITERAL nodes. Also record
    # ALL referenced macros (even those below ADDR_THRESHOLD) for the
    # downstream paired-32-bit-literal cross-match in compare_refs.py.
    macros = _load_macro_addrs()
    macros_used = set()
    if macros:
        body = _function_source(func_cursor)
        for tok in set(re.findall(r'\b[A-Za-z_]\w*\b', body)):
            addr = macros.get(tok)
            if addr is None:
                continue
            macros_used.add(tok)
            if addr >= ADDR_THRESHOLD:
                big_consts.add(addr)

    call_ptrs = calls - direct_calls
    return calls, call_ptrs, data_refs, big_consts, macros_used


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

        calls, call_ptrs, data_refs, big_consts, macros_used = analyze_function(cursor)
        addr = addr_map.get(name)

        results.append({
            'func': name,
            'addr': addr,
            'file': os.path.basename(path),
            'calls': sorted(calls),
            'call_ptrs': sorted(call_ptrs),
            'data_refs': sorted(data_refs),
            'big_consts': sorted(big_consts),
            'macros_used': sorted(macros_used),
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
            'macros_used': r.get('macros_used', []),
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
