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

# ─── Register save analysis (from main.mar bodies) ──────────────────────────
RE_PUSH_L      = re.compile(r'^\s+push\.l\s+(er[0-7])\b', re.I)
RE_PUSH_W      = re.compile(r'^\s+push\.w\s+(r[0-7][lh]?|e[0-7])\b', re.I)
RE_MOV_AT_R7   = re.compile(r'^\s+mov\.[lwb]\s+(er[0-7]|r[0-7][lh]?|e[0-7]),\s*@-r7\b', re.I)
RE_FN_ENTRY    = re.compile(r'^\w+:\s*$')
RE_LAB         = re.compile(r'^LAB_[0-9a-f]+:\s*$', re.I)
RE_TAIL        = re.compile(r'^\s+jmp\s+@(sys_epilogue\w*|\$\w+(?:\$\d+)?)', re.I)
RE_PRE_HELPER  = re.compile(r'^\s+(jsr|bsr)\s+@(sys_prologue\w*|\$\w+(?:\$\d+)?)', re.I)
RE_STACK_ADJ   = re.compile(r'^\s+(subs|adds)\s', re.I)

# Helper functions and the registers they save/restore (verified by reading
# their bodies in main.mar).
HELPER_SAVES = {
    # ch38 runtime helpers ($-prefixed)
    '$sp_regsv$3':                          'er3,er4,er5,er6',
    '$spregld2$3':                          'er3,er4,er5,er6',
    '$sp_rgsv3$3':                          'er3,er4,er5,er6+',  # bigger save-set
    # ROM-side compiler-emitted helpers (sys_prologue/sys_epilogue prefix).
    # Tail-call epilogue names already encode their register list, so this map
    # is mainly for the prologue side where the encoding isn't lossless.
    'sys_prologue_er2_er3_er4_er5_er6':     'er2,er3,er4,er5,er6',
}

# Parse `sys_epilogue_<reg>_<reg>_..._<hex>` -> register list
def parse_epilogue_name(name):
    if not name.startswith('sys_epilogue_'):
        return None
    parts = name[len('sys_epilogue_'):].split('_')
    regs = [p for p in parts if re.match(r'^(er?[0-7][lh]?|e[0-7])$', p, re.I)]
    return ','.join(regs) if regs else None


def parse_function_bodies(mar_path):
    """Yield (name, [body lines]) for each ; Function: header in main.mar."""
    cur = None
    body = []
    with open(mar_path, encoding='latin-1') as f:
        for line in f:
            m = re.match(r';\s*Function:\s*(\S+)', line)
            if m:
                if cur:
                    yield cur, body
                cur = m.group(1)
                body = []
                continue
            if cur and line.strip() and not line.startswith(';'):
                body.append(line.rstrip())
    if cur:
        yield cur, body


RE_POP_L  = re.compile(r'^\s+pop\.l\s+(er[0-7])\b', re.I)
RE_POP_W  = re.compile(r'^\s+pop\.w\s+(r[0-7][lh]?|e[0-7])\b', re.I)


def analyze_register_use(body):
    """Given a function's main.mar body, return a short string describing
    callee-saved registers (or '' if it neither saves nor is a helper).

    Shared epilogue helpers (functions whose first instruction is `pop`)
    are labelled 'epilogue: <regs>' so they don't masquerade as ordinary
    "no saves" leaf functions."""
    saves = []
    helper_pre = None
    helper_post = None
    is_epilogue_helper = False
    epilogue_regs = []

    started = False
    for ln in body:
        st = ln.strip()
        if not st:
            continue
        if not started:
            if RE_FN_ENTRY.match(st):
                started = True
            continue
        if RE_LAB.match(st):
            break
        # Shared epilogue helpers begin with pop instructions
        m = RE_POP_L.match(ln) or RE_POP_W.match(ln)
        if m:
            is_epilogue_helper = True
            epilogue_regs.append(m.group(1).lower())
            continue
        if is_epilogue_helper:
            break  # done collecting pops
        m = RE_PUSH_L.match(ln) or RE_PUSH_W.match(ln) or RE_MOV_AT_R7.match(ln)
        if m:
            saves.append(m.group(1).lower())
            continue
        if RE_STACK_ADJ.match(ln):
            continue
        m = RE_PRE_HELPER.match(ln)
        if m and not saves:
            helper_pre = m.group(2)
        break

    if is_epilogue_helper:
        return f'epilogue helper, restores: {",".join(epilogue_regs)}'

    for ln in reversed(body[-8:]):
        m = RE_TAIL.match(ln)
        if m:
            helper_post = m.group(1)
            break

    pre_regs = ''
    if saves:
        pre_regs = ','.join(saves)
    elif helper_pre:
        pre_regs = HELPER_SAVES.get(helper_pre, helper_pre)

    post_regs = ''
    if helper_post:
        post_regs = (parse_epilogue_name(helper_post)
                     or HELPER_SAVES.get(helper_post)
                     or helper_post)

    pieces = []
    if pre_regs:
        pieces.append(pre_regs)
    if post_regs and post_regs != pre_regs:
        pieces.append(f'-> {post_regs}')
    return ' '.join(pieces)


def get_register_info(mar_path):
    """Return {func_name: 'saves: ...'} for every function in main.mar."""
    info = {}
    for name, body in parse_function_bodies(mar_path):
        s = analyze_register_use(body)
        if s:
            info[name] = s
    return info

PARSE_ARGS = [
    '-std=c89',
    f'-I{PROJECT_DIR}',
    f'-I{PROJECT_DIR / "include"}',
    '-ferror-limit=0',
    '-w',
]

# Matches "// ROM: 0xabcd" plus optional %% and trailing register info
# ("saves: ..." or "epilogue helper, ...").
ROM_COMMENT_RE = re.compile(
    r'^\s*// ROM: 0x[0-9a-fA-F]+(\s+[\d.]+%)?'
    r'(\s+(?:saves: |epilogue\s).+)?\s*$'
)
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


def annotate_file(path, mar_addrs, scores, reg_info):
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
        head = f'// ROM: 0x{addr:04x}'
        if name in scores:
            head += f'  {scores[name]:.1f}%'
        if name in reg_info:
            ri = reg_info[name]
            # Epilogue helpers report a full phrase; everything else is a save list
            if ri.startswith('epilogue '):
                head += f'  {ri}'
            else:
                head += f'  saves: {ri}'
        comment = head + '\n'

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

    reg_info = get_register_info(mar_path)
    if reg_info:
        print(f"Extracted register-save info for {len(reg_info)} functions")

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
        if annotate_file(path, mar_addrs, scores, reg_info):
            try:
                label = Path(path).resolve().relative_to(PROJECT_DIR)
            except ValueError:
                label = path
            print(f"  Updated: {label}")
            changed += 1

    print(f"Done: {changed}/{len(paths)} file(s) modified.")


if __name__ == '__main__':
    main()
