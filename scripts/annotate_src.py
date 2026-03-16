#!/usr/bin/env python3
"""annotate_src.py - Insert // ROM: 0xABCD  nn.n% comments above C function definitions.

Reads ROM addresses from main.mar (same format as compare_bin.py).
Uses libclang for robust C function detection.
Match percentages are read from the last build (build/obj_*.s vs COMPLETE_DUMP.bin).
Idempotent: updates existing // ROM: lines in place.

Usage:
  python3 scripts/annotate_src.py              # all src/**/*.c
  python3 scripts/annotate_src.py src/foo.c   # specific file(s)
"""

import sys
import re
import os
from pathlib import Path
from types import SimpleNamespace

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
    '-w',
]

# Matches both "// ROM: 0xabcd" and "// ROM: 0xabcd  100.0%"
ROM_COMMENT_RE = re.compile(r'^\s*// ROM: 0x[0-9a-fA-F]+(\s+[\d.]+%)?\s*$')
PRAGMA_AUTO_RE = re.compile(r'#pragma\s+option\s+.*?/\*\s*pragma:auto\s*\*/')


def parse_mar_addresses(mar_path):
    """Return {func_name: SimpleNamespace(addr, size)} from main.mar."""
    addrs = {}
    cur_func = None
    cur_addr = None
    with open(mar_path, encoding='latin-1') as f:
        for line in f:
            m = re.match(r';\s*Function:\s*(\S+)', line)
            if m:
                cur_func = m.group(1)
                cur_addr = None
                continue
            if cur_func:
                ma = re.match(r';\s*Address:\s*([0-9A-Fa-f]+)', line)
                ms = re.match(r';\s*Size:\s*(\d+)', line)
                if ma:
                    cur_addr = int(ma.group(1), 16)
                if ms and cur_addr is not None:
                    addrs[cur_func] = SimpleNamespace(addr=cur_addr, size=int(ms.group(1)))
                    cur_func = None
    return addrs


def get_match_scores(mar_addrs, ref_bin):
    """Return {func_name: float} match percentages, or {} if unavailable."""
    build_dir = PROJECT_DIR / 'build'
    if not list(build_dir.glob('obj_*.s')):
        print("  (skipping match scores: no build/obj_*.s files — run make first)", file=sys.stderr)
        return {}
    if not ref_bin.exists():
        print(f"  (skipping match scores: {ref_bin.name} not found)", file=sys.stderr)
        return {}

    orig_dir = os.getcwd()
    os.chdir(PROJECT_DIR)
    try:
        sys.path.insert(0, str(Path(__file__).parent))
        import compare_bin
        import compare

        gen_funcs, _ = compare_bin.get_gen_funcs(mar_addrs)
        ref_funcs = compare_bin.get_ref_funcs(mar_addrs, Path(ref_bin.name))

        scores = {}
        for name, ns in mar_addrs.items():
            if name.startswith('$') or name == '__INITSCT':
                continue
            gen_ins = gen_funcs.get(name, [])
            ref_ins = ref_funcs.get(name, [])
            if ref_ins:
                _, pct = compare.diff_functions(ref_ins, gen_ins)
                scores[name] = pct
        return scores
    except Exception as e:
        print(f"  Warning: match score computation failed: {e}", file=sys.stderr)
        return {}
    finally:
        os.chdir(orig_dir)


def get_func_lines(path):
    """Return list of (func_name, line_number_1indexed) for definitions in path."""
    path = str(path)
    idx = clx.Index.create()
    tu = idx.parse(path, args=PARSE_ARGS)
    results = []
    seen = set()
    for cursor in tu.cursor.walk_preorder():
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
        results.append((name, cursor.location.line))
    return results


def annotate_file(path, mar_addrs, scores):
    """Annotate one file. Returns True if file was modified."""
    path = Path(path)
    with open(path, encoding='latin-1') as f:
        lines = f.readlines()

    func_lines = get_func_lines(path)

    # Only keep functions that have a ROM address
    annotatable = [(name, lineno) for name, lineno in func_lines if name in mar_addrs]
    if not annotatable:
        return False

    # Process bottom-to-top so earlier insertions don't shift later indices
    annotatable.sort(key=lambda x: x[1], reverse=True)

    modified = False
    for name, lineno in annotatable:
        addr = mar_addrs[name].addr
        if name in scores:
            comment = f'// ROM: 0x{addr:04x}  {scores[name]:.1f}%\n'
        else:
            comment = f'// ROM: 0x{addr:04x}\n'

        fi = lineno - 1  # 0-indexed index of the function line

        # Insert before the pragma if one immediately precedes the function
        insert_before = fi
        if fi > 0 and PRAGMA_AUTO_RE.search(lines[fi - 1]):
            insert_before = fi - 1

        # Check if a ROM comment already sits just above the insertion point
        if insert_before > 0 and ROM_COMMENT_RE.match(lines[insert_before - 1]):
            if lines[insert_before - 1] != comment:
                lines[insert_before - 1] = comment
                modified = True
        else:
            lines.insert(insert_before, comment)
            modified = True

    if modified:
        with open(path, 'w', encoding='latin-1') as f:
            f.writelines(lines)

    return modified


def main():
    args = sys.argv[1:]

    mar_path = PROJECT_DIR / 'main.mar'
    if not mar_path.exists():
        print(f"Error: {mar_path} not found", file=sys.stderr)
        sys.exit(1)

    mar_addrs = parse_mar_addresses(mar_path)
    print(f"Loaded {len(mar_addrs)} function addresses from main.mar")

    ref_bin = PROJECT_DIR / 'COMPLETE_DUMP.bin'
    scores = get_match_scores(mar_addrs, ref_bin)
    if scores:
        print(f"Loaded match scores for {len(scores)} functions")

    if args:
        paths = [Path(a) for a in args if a.endswith('.c')]
    else:
        paths = sorted((PROJECT_DIR / 'src').rglob('*.c'))

    changed = 0
    for path in paths:
        if annotate_file(path, mar_addrs, scores):
            try:
                label = Path(path).resolve().relative_to(PROJECT_DIR)
            except ValueError:
                label = path
            print(f"  Updated: {label}")
            changed += 1

    print(f"Done: {changed}/{len(paths)} file(s) modified.")


if __name__ == '__main__':
    main()
