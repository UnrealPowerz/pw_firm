# ---------------------------------------------------------------------------
# compare_bin.py - Instruction-aware binary comparison of compiled output vs original ROM.
#
# This script is derived from logic in the objdiff project (https://github.com/encounter/objdiff)
# by Luke Street and contributors.
#
# Licensed under the Apache License, Version 2.0 or the MIT License.
# ---------------------------------------------------------------------------

import argparse
import re
import subprocess
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

# compare.py lives alongside this script
sys.path.insert(0, str(Path(__file__).parent))
import compare

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
P_BASE       = 0x0000   # binary now starts at 0 (vector table included)
ROM_SIZE     = 0xC000   # 48KB ROM

@dataclass
class FuncInfo:
    name: str
    abs_addr: int
    size: int
    unit: str
    our_instrs: list[compare.Instr]
    ref_instrs: list[compare.Instr]
    match_pct: float = 0.0

# ---------------------------------------------------------------------------
# Objdump parsing
# ---------------------------------------------------------------------------
def parse_objdump_line(line: str) -> tuple[Optional[int], Optional[compare.Instr]]:
    line = line.strip()
    if not line: return None, None
    m = re.match(r'^([0-9a-fA-F]+):\s+(?:(?:[0-9a-fA-F]{2}\s+)+)\s*(.*)', line)
    if not m: return None, None
    addr = int(m.group(1), 16)
    asm_text = m.group(2).strip()
    if not asm_text: return addr, None
    instr = compare.parse_instruction(asm_text)
    if instr is None and asm_text.startswith('.word'):
        args = [t.strip() for t in asm_text[5:].split(',')]
        instr = compare.Instr(raw=asm_text, mnemonic='.word', args=args, is_imm=[False]*len(args))
    return addr, instr

def _build_flat_to_unit() -> dict:
    """Map build/obj_*.s filename → unit path (e.g. 'system/save') from the src/ tree."""
    flat_to_unit = {}
    src_dir = Path("src")
    if src_dir.exists():
        for f in src_dir.rglob("*.c"):
            flat = 'obj_' + str(f).replace('/', '_').replace('.c', '.s')
            unit = str(f.relative_to(src_dir).with_suffix(''))
            flat_to_unit[flat] = unit
    return flat_to_unit

def get_gen_funcs(mar_addrs: dict) -> tuple[dict[str, list[compare.Instr]], dict[str, str]]:
    """Parse compiler-output .s files from build/ — no Docker, matches by name not address."""
    build_dir = Path("build")
    asm_files = sorted(build_dir.glob("obj_*.s"))
    if not asm_files:
        print("ERROR: No obj_*.s files found in build/. Run 'make' first.", file=sys.stderr)
        sys.exit(1)

    flat_to_unit = _build_flat_to_unit()
    print(f"Parsing {len(asm_files)} assembly files from build/...")

    instrs_by_func: dict[str, list[compare.Instr]] = {}
    func_to_unit: dict[str, str] = {}

    for asm_file in asm_files:
        unit_name = flat_to_unit.get(
            asm_file.name,
            asm_file.name.replace('obj_src_', '').replace('obj_', '').replace('.s', '')
        )
        for name, instrs in compare.parse_asm(asm_file).items():
            if name in mar_addrs:
                instrs_by_func[name] = instrs
                func_to_unit[name] = unit_name

    return instrs_by_func, func_to_unit

def get_ref_funcs(mar_addrs: dict, ref_bin: Path) -> dict[str, list[compare.Instr]]:
    args = ["./h8objdump", "-z", "-m", "h8300", "-D", "-b", "binary", str(ref_bin)]
    print(f"Disassembling reference binary {ref_bin}...")
    res = subprocess.run(args, capture_output=True, text=True)
    if res.returncode != 0:
        print("Error running objdump on ref binary:", file=sys.stderr)
        sys.exit(1)

    intervals = sorted([(m.addr, m.addr + m.size, name) for name, m in mar_addrs.items() if m.size > 0], key=lambda x: x[0])
    funcs = defaultdict(list)
    interval_idx = 0
    num_intervals = len(intervals)

    for line in res.stdout.splitlines():
        addr, instr = parse_objdump_line(line)
        if addr is None or instr is None: continue
        while interval_idx < num_intervals and intervals[interval_idx][1] <= addr:
            interval_idx += 1
        if interval_idx >= num_intervals: break
        start, end, func_name = intervals[interval_idx]
        if start <= addr < end:
            funcs[func_name].append(instr)
    return dict(funcs)

def print_summary(funcs: list[FuncInfo], threshold: float = 0.0):
    units = defaultdict(lambda: {'pct': 0.0, 'count': 0})
    for fi in funcs:
        units[fi.unit]['pct'] += fi.match_pct
        units[fi.unit]['count'] += 1

    print(f"\n{'Unit':<35} {'Match Avg':>9}  {'Funcs':>5}")
    print('─' * 55)
    
    sorted_units = sorted(units.keys())
    grand_pct = 0.0
    grand_count = 0
    for u in sorted_units:
        data = units[u]
        avg = data['pct'] / data['count']
        grand_pct += data['pct']
        grand_count += data['count']
        if threshold and avg >= threshold: continue
        mark = '' if avg >= 99.9 else '*' if avg < 80 else ' '
        print(f"{mark}{u:<34}  {avg:>8.1f}%  {data['count']:>5}")
    
    print('─' * 55)
    if grand_count > 0:
        print(f"{'TOTAL':<35}  {grand_pct/grand_count:>8.1f}%  {grand_count:>5}")

# Control-flow mnemonics whose operands are branch/call targets. After
# linking these resolve identically; the textual diff (numeric ROM address vs
# our symbol/label) is noise, not a bug — so we never report their operands.
_CTRL_FLOW = {
    'jmp', 'jsr', 'bsr', 'rts', 'rte', 'bra', 'bt', 'bf',
    'bra/s', 'bsr/s', 'beq', 'bne', 'bhi', 'bls', 'bcc', 'bhs',
    'bcs', 'blo', 'bvc', 'bvs', 'bpl', 'bmi', 'bge', 'blt',
    'bgt', 'ble',
}

# A token we can meaningfully compare across the two disassemblies: a pure
# decimal number, optionally prefixed with '@' (data memory reference). This
# excludes symbolic call/branch targets (e.g. '@_gfx_draw_text_box', a label)
# whose numeric ROM form will never textually equal our symbol.
_NUMERIC_TOK = re.compile(r'(@?)(-?\d+)$')

# Stack-frame allocation: `add.w/sub.w #N, er7` (er7/r7/sp = stack pointer). The
# immediate N is the local-frame size, which differs purely by ch38's register
# allocation / spill decisions vs ROM — never a semantic constant. Exclude it.
_STACK_ADJ = re.compile(r'^(add|sub)\b.*,\s*(e?r7|sp)$', re.IGNORECASE)

# Function-call mnemonics whose targets we want to compare BY NAME (not by
# numeric address). Conditional branches are excluded — their targets are
# auto-generated labels that won't align across the two assemblers.
_CALL_MNEMONICS = {'jsr', 'jmp', 'bsr'}

# Compiler-runtime helpers ch38 may emit asymmetrically (callee-save trampolines,
# divmod) — codegen artifacts, not call-target bugs. Skip them.
_RUNTIME_PREFIXES = ('$', 'divmod', 'sp_regsv', 'sp_regrs')

# Heuristics for "this isn't a real function name, it's an intra-function label."
# Comparing those across the two assemblers is pure noise because the label names
# don't share a convention (ROM has `lab_XXXX` / `switchd_XXXX_caseD_Y`; ch38
# emits `l<digits>` and `$<name>$<id>` fragments for jumps within a function).
_LABEL_PATTERNS = [
    re.compile(r'^lab_[0-9a-f]+$'),
    re.compile(r'^switchd_[0-9a-f]+'),
    re.compile(r'^cased_'),
    re.compile(r'^l\d+$'),
    re.compile(r'^\$.+\$\d+$'),
    re.compile(r'.+__[a-z_]+$'),   # ROM's `func__internal_label` form
]


def _is_label_like(name: str) -> bool:
    return any(p.match(name) for p in _LABEL_PATTERNS)


def _canon_call(tok: str) -> Optional[str]:
    """Canonicalise an @<symbol> call target for name comparison across the
    two disassemblies. Returns None when the target isn't a symbolic call
    (numeric address, register-indirect, or a junk literal)."""
    if not tok.startswith('@'):
        return None
    inner = tok[1:]
    if not inner or inner.startswith('-') or inner[0].isdigit() or inner[0] == 'e' and len(inner) <= 3:
        return None
    if re.fullmatch(r'-?\d+', inner):  # @123 numeric
        return None
    if re.fullmatch(r'e?r[0-7][lh]?', inner):  # @er3 register-indirect
        return None
    # ch38 prefixes our C symbols with '_'; local labels get '__$name$id'.
    # Strip ALL leading underscores so both `_drv_foo` and `__$label$N` lose
    # the prefix uniformly.
    inner = inner.lstrip('_')
    return inner.lower()


def _size_bits(raw: str) -> Optional[int]:
    m = re.search(r'\.(b|w|l)\b', raw.lower())
    return {'b': 8, 'w': 16, 'l': 32}.get(m.group(1)) if m else None


def _io_norm(v: int) -> int:
    """Normalise an 8-bit short-absolute I/O address to its full form.
    asm38 renders @0xfb:8 as 0xfffb; objdump renders it as 0xfb. Same register."""
    return (0xFF00 | v) if 0x80 <= v < 0x100 else v


def _operand_bug(o_raw: str, g_raw: str, o_tok: str, g_tok: str) -> bool:
    """True if two numeric operand tokens differ in a way that is a REAL bug
    (not signed/unsigned display, not an immediate-vs-memory mis-pairing, not
    an 8-bit-absolute I/O address rendered two ways).

    - The '@' (memory-reference) prefix must match on both sides, otherwise the
      aligner paired an immediate load with a memory access — meaningless.
    - The .b/.w/.l size must match; differing size means the aligner paired
      instructions of different width (e.g. a word load vs a byte load).
    - Immediate values are compared modulo the operand width so 0xF780 == -2176
      and 0x87 == -121 (signed display) are NOT flagged.
    - '@' addresses are I/O-normalised so @0xfb == @0xfffb is NOT flagged.
    """
    om = _NUMERIC_TOK.match(o_tok)
    gm = _NUMERIC_TOK.match(g_tok)
    if not om or not gm:
        return False
    if om.group(1) != gm.group(1):   # '@' prefix mismatch -> different forms
        return False
    if _size_bits(o_raw) != _size_bits(g_raw):  # width mispairing
        return False
    o_val = int(om.group(2))
    g_val = int(gm.group(2))
    if om.group(1) == '@':           # memory reference: I/O-normalise
        return _io_norm(o_val & 0xFFFF) != _io_norm(g_val & 0xFFFF)
    bits = _size_bits(o_raw) or 32
    mask = (1 << bits) - 1
    return (o_val & mask) != (g_val & mask)


def _func_operand_values(instrs: list[compare.Instr]) -> set[int]:
    """Collect every numeric operand value in a function, at byte/word/long
    masks, so we can ask 'does this ROM constant appear ANYWHERE on our side?'"""
    vals: set[int] = set()
    for ins in instrs:
        for a in ins.args:
            m = _NUMERIC_TOK.match(a)
            if not m:
                continue
            v = int(m.group(2))
            vals.add(v & 0xFFFFFFFF)
            vals.add(v & 0xFFFF)
            vals.add(v & 0xFF)
    return vals


def print_imm_report(funcs: list[FuncInfo], threshold: float = 0.0):
    """Surface per-instruction operand-value mismatches (wrong addresses,
    wrong constants, scrambled literal args) that the match% buries.

    For each function we align instructions, then for every aligned pair whose
    mnemonic + arg-count match but whose operands differ, print orig vs gen with
    the differing argument(s) called out. These are the semantic bugs invisible
    to the score: a scrambled `mov.w #imm` still counts as a near-match.

    Only operands that are pure numerics on BOTH sides are reported — literal
    immediates and resolved data addresses — so control-flow targets and
    symbol-vs-address noise are excluded.

    Each mismatch is tagged [MISSING] when the ROM operand value appears NOWHERE
    in our version of the function (a genuinely wrong/absent constant — high
    confidence) versus a value merely repositioned by the compiler's different
    instruction ordering (likely benign reorder). Functions with [MISSING]
    values are listed first.
    """
    total_mismatch = 0
    total_missing = 0
    funcs_with = 0
    collected = []  # (fi, diffs, n_missing)
    for fi in funcs:
        rows, _ = compare.diff_functions(fi.ref_instrs, fi.our_instrs)
        our_vals = _func_operand_values(fi.our_instrs)
        diffs = []  # (orig_raw, gen_raw, [(idx,o,g,is_imm,missing)...])
        n_missing = 0
        for row in rows:
            if row.orig is None or row.gen is None:
                continue
            if row.orig.mnemonic in _CTRL_FLOW:
                continue
            if _STACK_ADJ.match(row.orig.raw) or _STACK_ADJ.match(row.gen.raw):
                continue
            ad = []
            for d in compare.arg_diffs(row.orig, row.gen):
                if not _operand_bug(row.orig.raw, row.gen.raw, d[1], d[2]):
                    continue
                om = _NUMERIC_TOK.match(d[1])
                rom_val = int(om.group(2)) & 0xFFFFFFFF
                missing = rom_val not in our_vals
                if missing:
                    n_missing += 1
                ad.append((*d, missing))
            if ad:
                diffs.append((row.orig.raw, row.gen.raw, ad))
        if not diffs:
            continue
        if threshold and fi.match_pct >= threshold:
            continue
        collected.append((fi, diffs, n_missing))

    # Functions with high-confidence [MISSING] constants first, then by address.
    collected.sort(key=lambda c: (c[2] == 0, c[0].abs_addr))
    for fi, diffs, n_missing in collected:
        funcs_with += 1
        total_mismatch += len(diffs)
        total_missing += n_missing
        tag = f", {n_missing} MISSING" if n_missing else ""
        print(f"\n=== {fi.name} ===  {fi.abs_addr:04X}  {fi.match_pct:.1f}%  "
              f"({len(diffs)} operand mismatch{'es' if len(diffs) != 1 else ''}{tag})")
        for o_raw, g_raw, ad in diffs:
            kinds = ','.join('imm' if is_imm else 'arg' for _, _, _, is_imm, _ in ad)
            print(f"  ROM : {o_raw}")
            print(f"  ours: {g_raw}   [{kinds}]")
            for idx, o_a, g_a, _, missing in ad:
                flag = '  <-- [MISSING]' if missing else ''
                print(f"        arg{idx}: ROM {o_a}  !=  ours {g_a}{flag}")
    print(f"\n{'─'*60}")
    print(f"{funcs_with} function(s) with operand mismatches, "
          f"{total_mismatch} mismatched instruction(s), "
          f"{total_missing} high-confidence [MISSING] constant(s).")


def _call_seq(instrs: list[compare.Instr]) -> list[str]:
    """Ordered list of canonicalised call-target names in a function."""
    out = []
    for ins in instrs:
        if ins.mnemonic not in _CALL_MNEMONICS:
            continue
        if not ins.args:
            continue
        name = _canon_call(ins.args[0])
        if name is None:
            continue
        if any(name.startswith(p) for p in _RUNTIME_PREFIXES):
            continue
        if _is_label_like(name):
            continue
        out.append(name)
    return out


def print_calls_report(funcs: list[FuncInfo],
                       mar_funcs: dict,
                       threshold: float = 0.0):
    """Compare the SEQUENCE of function-call targets per function, ROM (from
    main.mar's symbolic disassembly) vs ours (from build/obj_*.s). Aligns the
    two call sequences and reports substitutions ("ROM called X here, ours
    calls Y"), plus calls present on one side only.

    This catches wrong-function-call bugs that the immediate-value diff misses
    because control-flow targets are filtered as ROM-numeric-vs-ours-symbolic
    noise. Conditional branches aren't compared (their auto-generated label
    names won't align across the two assemblers).
    """
    import difflib
    total_diffs = 0
    funcs_with = 0
    collected = []
    for fi in funcs:
        rom_ins = mar_funcs.get(fi.name)
        if rom_ins is None:
            continue
        rom_seq = _call_seq(rom_ins)
        our_seq = _call_seq(fi.our_instrs)
        if not rom_seq and not our_seq:
            continue
        sm = difflib.SequenceMatcher(None, rom_seq, our_seq, autojunk=False)
        diffs = []
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if tag == 'equal':
                continue
            if tag == 'replace':
                pairs = min(i2 - i1, j2 - j1)
                for k in range(pairs):
                    diffs.append(('replace', rom_seq[i1 + k], our_seq[j1 + k]))
                for k in range(pairs, i2 - i1):
                    diffs.append(('rom_only', rom_seq[i1 + k], None))
                for k in range(pairs, j2 - j1):
                    diffs.append(('our_only', None, our_seq[j1 + k]))
            elif tag == 'delete':
                for k in range(i1, i2):
                    diffs.append(('rom_only', rom_seq[k], None))
            elif tag == 'insert':
                for k in range(j1, j2):
                    diffs.append(('our_only', None, our_seq[k]))
        if not diffs:
            continue
        if threshold and fi.match_pct >= threshold:
            continue
        collected.append((fi, diffs, rom_seq, our_seq))

    # Functions with `replace` diffs first (most actionable), then by address.
    collected.sort(key=lambda c: (not any(d[0] == 'replace' for d in c[1]),
                                  c[0].abs_addr))
    for fi, diffs, rom_seq, our_seq in collected:
        funcs_with += 1
        total_diffs += len(diffs)
        n_replace = sum(1 for d in diffs if d[0] == 'replace')
        tag = f"  ({n_replace} substitution{'s' if n_replace != 1 else ''})" if n_replace else "  (insert/delete only)"
        print(f"\n=== {fi.name} ===  {fi.abs_addr:04X}  {fi.match_pct:.1f}%"
              f"  ROM:{len(rom_seq)} calls  ours:{len(our_seq)} calls{tag}")
        for kind, r, o in diffs:
            if kind == 'replace':
                print(f"  SUBST : ROM calls {r}   ours calls {o}")
            elif kind == 'rom_only':
                print(f"  ROM-only : {r}")
            else:
                print(f"  ours-only: {o}")
    print(f"\n{'─'*60}")
    print(f"{funcs_with} function(s) with call-sequence diffs, "
          f"{total_diffs} discrepancies total.")


def print_detailed_list(funcs: list[FuncInfo], threshold: float = 0.0):
    print(f"\n{'Addr':<6} {'Size':>5} {'Match':>8}  {'Function':<40} {'Unit'}")
    print('─' * 100)
    
    # Sort by address
    sorted_funcs = sorted(funcs, key=lambda x: x.abs_addr)
    
    for fi in sorted_funcs:
        if threshold and fi.match_pct >= threshold:
            continue
        print(f"{fi.abs_addr:>04X}  {fi.size:>5}  {fi.match_pct:>7.1f}%  {fi.name:<40} {fi.unit}")
    print('─' * 100)

def main():
    ap = argparse.ArgumentParser(description='Binary comparison vs ROM')
    ap.add_argument('ref_bin', nargs='?', default='COMPLETE_DUMP.bin', help='Reference binary')
    ap.add_argument('--mar', default='main.mar', help='Original disassembly')
    ap.add_argument('--unit', help='Show detail for a unit')
    ap.add_argument('--func', help='Show detail for a function')
    ap.add_argument('--list', '-l', action='store_true', help='Show detailed list of all functions')
    ap.add_argument('--imm', action='store_true', help='Report operand-value mismatches (wrong addr/const/scrambled args) hidden by the match%%')
    ap.add_argument('--calls', action='store_true', help='Compare call-target name sequences vs main.mar (finds wrong-function-call bugs)')
    ap.add_argument('--threshold', type=float, default=0.0, help='Filter summary')
    args = ap.parse_args()

    mar_path = Path(args.mar)
    mar_addrs = {}
    if mar_path.exists():
        from types import SimpleNamespace
        cur_func = None
        cur_addr = None
        with mar_path.open('r', encoding='latin-1') as f:
            for line in f:
                m = re.match(r';\s*Function:\s*(\S+)', line)
                if m: cur_func = m.group(1); cur_addr = None; continue
                if cur_func:
                    ma = re.match(r';\s*Address:\s*([0-9A-Fa-f]+)', line)
                    ms = re.match(r';\s*Size:\s*(\d+)', line)
                    if ma: cur_addr = int(ma.group(1), 16)
                    if ms and cur_addr is not None:
                        mar_addrs[cur_func] = SimpleNamespace(addr=cur_addr, size=int(ms.group(1)))
                        cur_func = None

    print(f"Loaded {len(mar_addrs)} function definitions from {args.mar}")
    gen_funcs, func_to_unit = get_gen_funcs(mar_addrs)
    ref_funcs = get_ref_funcs(mar_addrs, Path(args.ref_bin))

    print(f"Comparing {len(gen_funcs)} generated functions vs reference...")
    funcs_info = []
    for name in sorted(mar_addrs.keys()):
        # Skip compiler runtime library functions (from lib3hn.lib) — not part of decompilation
        if name.startswith('$') or name == '__INITSCT':
            continue
        mfunc = mar_addrs[name]
        gen_ins = gen_funcs.get(name, [])
        ref_ins = ref_funcs.get(name, [])
        if not ref_ins: continue
        
        _, pct = compare.diff_functions(ref_ins, gen_ins)
        funcs_info.append(FuncInfo(name, mfunc.addr, mfunc.size, func_to_unit.get(name, "unknown"), gen_ins, ref_ins, pct))

    if args.imm:
        subset = funcs_info
        if args.func:
            subset = [f for f in funcs_info if f.name == args.func or f.name == args.func.lstrip('_')]
        elif args.unit:
            subset = [f for f in funcs_info if f.unit == args.unit]
        print_imm_report(subset, args.threshold)
    elif args.calls:
        subset = funcs_info
        if args.func:
            subset = [f for f in funcs_info if f.name == args.func or f.name == args.func.lstrip('_')]
        elif args.unit:
            subset = [f for f in funcs_info if f.unit == args.unit]
        mar_funcs = compare.parse_mar(mar_path)
        print_calls_report(subset, mar_funcs, args.threshold)
    elif args.func:
        matches = [f for f in funcs_info if f.name == args.func or f.name == args.func.lstrip('_')]
        if not matches:
            print(f"Function '{args.func}' not found in generated objects.")
            sys.exit(1)
        for fi in matches:
            rows, pct = compare.diff_functions(fi.ref_instrs, fi.our_instrs)
            compare.print_function_diff(fi.name, rows, pct)
    elif args.unit:
        print(f"\n=== Unit: {args.unit} ===")
        found = False
        for fi in sorted([f for f in funcs_info if f.unit == args.unit], key=lambda x: x.abs_addr):
            print(f"{fi.name:<40} {fi.abs_addr:>04X}  {fi.match_pct:>6.1f}%")
            found = True
        if not found:
            print(f"No functions found for unit '{args.unit}'")
    elif args.list:
        print_detailed_list(funcs_info, args.threshold)
    else:
        print_summary(funcs_info, args.threshold)

if __name__ == '__main__':
    main()
