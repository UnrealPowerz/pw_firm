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

    if args.func:
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
