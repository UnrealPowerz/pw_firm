# ---------------------------------------------------------------------------
# compare.py - H8/300H assembly diff tool for Pokewalker decompilation.
#
# This script is derived from the objdiff project (https://github.com/encounter/objdiff)
# by Luke Street and contributors.
#
# Licensed under the Apache License, Version 2.0 or the MIT License.
# ---------------------------------------------------------------------------

import argparse
import difflib
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Penalty constants (mirrors objdiff/objdiff-core/src/diff/code.rs)
# ---------------------------------------------------------------------------
PENALTY_IMM_DIFF       =   1
PENALTY_ARG_DIFF       =   5
PENALTY_OPCODE_REPLACE =  60
PENALTY_INSERT_DELETE  = 100


# ---------------------------------------------------------------------------
# Instruction representation
# ---------------------------------------------------------------------------
@dataclass
class Instr:
    """One disassembled instruction."""
    raw:     str          # full original line (stripped)
    mnemonic: str         # e.g. "mov.b", "bset", "jsr"
    args:    list[str]    # argument tokens, normalised
    is_imm:  list[bool]   # True for each arg that is a pure immediate


# ---------------------------------------------------------------------------
# Parsing helpers
# ---------------------------------------------------------------------------

# Matches H8 mnemonics: optional size suffix (.b/.w/.l), optional /condition
_MNEMONIC_RE = re.compile(r'^([a-zA-Z][a-zA-Z0-9/._]*)', re.IGNORECASE)

# Detects a bare numeric immediate (decimal, hex 0x… or H'…, or negative)
_IMM_RE = re.compile(
    r"^([+\-]?\d+|0x[0-9a-fA-F]+|H'[0-9a-fA-F]+|#[+\-]?\d+|#0x[0-9a-fA-F]+|#H'[0-9a-fA-F]+)$",
    re.IGNORECASE,
)


def _normalise_mnemonic(m: str) -> str:
    """Normalise a mnemonic for opcode comparison.

    The h8cc assembler emits mnemonics with explicit size qualifiers
    (e.g. MOV.B, BSET.B) while main.mar often omits the size suffix
    when it is the default (e.g. 'bset' instead of 'bset.b').

    Strategy: strip trailing .b/.w/.l so that 'bset' == 'bset.b',
    'mov.w' == 'mov.w' (both have .w so no drop), etc.

    We do NOT strip size from mnemonics where size is semantically
    meaningful and always explicit in both files (e.g. 'extu.w',
    'divxs.w', 'mulxu.w') — but SequenceMatcher handles those fine
    since both sides will either keep or drop the suffix consistently.
    """
    m = m.lower()
    # Strip trailing size suffix .b / .w / .l only
    m = re.sub(r'\.(b|w|l)$', '', m)
    return m


def _is_immediate(tok: str) -> bool:
    tok = tok.strip().lstrip('#')
    return bool(_IMM_RE.match(tok))


def _normalise_arg(tok: str) -> str:
    """Normalise an argument token for comparison.

    - Strip leading '#' from immediates so '#5' == '5'
    - Lower-case registers (R0L -> r0l)
    - Strip ':8' / ':16' addressing qualifiers (branch hints)
    - Normalise H'XX hex to integer string so H'BF == 191
    """
    tok = tok.strip()
    # strip branch distance hints
    tok = re.sub(r':(8|16|24|32)$', '', tok)
    # strip leading # for immediates
    if tok.startswith('#'):
        tok = tok[1:]
    # normalise H'XX hex literals to 0xXX
    tok = re.sub(r"H'([0-9a-fA-F]+)", lambda m: str(int(m.group(1), 16)), tok, flags=re.IGNORECASE)
    # normalise 0xXX to decimal
    tok = re.sub(r'0x([0-9a-fA-F]+)', lambda m: str(int(m.group(1), 16)), tok, flags=re.IGNORECASE)
    # alias for compiler built-in $divul$3
    if tok.lower() == "@$divul$3":
        tok = "@divmod32"
    # lower-case everything (registers, mnemonics in indirect)
    return tok.lower()


def _split_args(args_str: str) -> list[str]:
    """Split the argument portion of an instruction into tokens.

    Handles: 'r0l, @er6', '@(H'3,er7)', '#H'BF', etc.
    We keep complex addressing modes intact as single tokens.
    """
    # Simple comma split that respects parentheses depth
    tokens = []
    depth = 0
    current = []
    for ch in args_str:
        if ch == '(':
            depth += 1
            current.append(ch)
        elif ch == ')':
            depth -= 1
            current.append(ch)
        elif ch == ',' and depth == 0:
            tokens.append(''.join(current).strip())
            current = []
        else:
            current.append(ch)
    if current:
        tokens.append(''.join(current).strip())
    return [t for t in tokens if t]


def parse_instruction(line: str) -> Optional[Instr]:
    """Parse one text assembly line into an Instr.

    Returns None for comments, directives, blank lines, and labels.
    Works on both main.mar format and h8cc .s output.
    """
    # Strip leading/trailing whitespace
    line = line.strip()

    # Skip empty, pure comments, directives, labels-only
    if not line:
        return None
    if line.startswith(';') or line.startswith('*') or line.startswith('.'):
        return None
    # h8cc source annotation comments
    if line.startswith(';***'):
        return None

    # Strip trailing comment
    line = re.sub(r'\s*;.*$', '', line).strip()
    if not line:
        return None

    # Strip label prefix if present (e.g. "LAB_0108:" or "_reset:")
    line = re.sub(r'^[A-Za-z_][A-Za-z0-9_$]*:\s*', '', line).strip()
    if not line:
        return None  # was a label-only line

    # Skip directives that may have been revealed by label strip
    if line.startswith('.'):
        return None

    # Extract mnemonic (first token)
    m = _MNEMONIC_RE.match(line)
    if not m:
        return None
    raw_mnemonic = m.group(1)
    rest = line[m.end():].strip()

    mnemonic = _normalise_mnemonic(raw_mnemonic)

    # Skip assembler directives that look like mnemonics (IMPORT, EXPORT, etc.)
    _DIRECTIVES = {
        'import', 'export', 'section', 'stack', 'cpu', 'end', 'align',
        'byte', 'word', 'lword', 'ascii', 'blkb', 'blkw', 'blkl',
        'equ', 'org', 'list', 'nolist', 'include',
    }
    if mnemonic in _DIRECTIVES:
        return None

    # Split the rest into argument tokens
    raw_args = _split_args(rest)
    norm_args = [_normalise_arg(a) for a in raw_args]
    is_imm = [_is_immediate(a) for a in raw_args]

    return Instr(
        raw=line,
        mnemonic=mnemonic,
        args=norm_args,
        is_imm=is_imm,
    )


# ---------------------------------------------------------------------------
# File parsers
# ---------------------------------------------------------------------------

def _strip_func_prefix(name: str) -> str:
    """Remove leading underscore(s) that h8cc adds."""
    return name.lstrip('_').lstrip('$')


def parse_mar(path: Path) -> dict[str, list[Instr]]:
    """Parse main.mar: extract functions by name -> instruction list."""
    funcs: dict[str, list[Instr]] = {}
    current_name: Optional[str] = None
    current_instrs: list[Instr] = []

    with path.open('r', encoding='latin-1') as f:
        for line in f:
            line = line.rstrip('\r\n')

            # Function header comment
            m = re.match(r';\s*Function:\s*(\S+)', line)
            if m:
                if current_name and current_instrs:
                    funcs[current_name] = current_instrs
                current_name = m.group(1)
                current_instrs = []
                continue

            if current_name is None:
                continue

            instr = parse_instruction(line)
            if instr:
                current_instrs.append(instr)

    if current_name and current_instrs:
        funcs[current_name] = current_instrs

    return funcs


def parse_asm(path: Path) -> dict[str, list[Instr]]:
    """Parse h8cc .s output: extract functions by name -> instruction list.

    h8cc annotates real function entry labels with:
        _funcname:    ; function: funcname
    and internal branch/loop labels with:
        Lnnn:
    or:
        __$labelname$N:   ; label: labelname

    We only start a new function on lines that carry a '; function:' comment.
    All other labels (branch targets, goto labels) are part of the current
    function body and their instructions stay in the current function.
    """
    funcs: dict[str, list[Instr]] = {}
    current_name: Optional[str] = None
    current_instrs: list[Instr] = []

    with path.open('r', encoding='latin-1') as f:
        lines = [line.rstrip('\r\n') for line in f]

    i = 0
    while i < len(lines):
        line = lines[i]

        # Detect a real function entry label — h8cc emits "; function: name"
        # sometimes on the same line, sometimes on the next.
        m = re.match(r'^([A-Za-z_][A-Za-z0-9_$]*):\s*(?:;.*)?$', line)
        if m:
            label = m.group(1)
            is_func = False
            if '; function:' in line:
                is_func = True
            elif i + 1 < len(lines) and re.match(r'^\s*;\s*function:', lines[i+1]):
                is_func = True
            
            if is_func:
                internal_name = _strip_func_prefix(label)
                if current_name is not None:
                    funcs[current_name] = current_instrs
                current_name = internal_name
                current_instrs = []
                i += 1
                continue

        if current_name is None:
            i += 1
            continue

        # Branch-target / goto labels — keep them as part of current function
        # (they don't start a new function, just skip the label itself)
        if re.match(r'^[A-Za-z_][A-Za-z0-9_$]*:\s*$', line):
            i += 1
            continue

        instr = parse_instruction(line)
        if instr:
            current_instrs.append(instr)
        i += 1

    if current_name is not None:
        funcs[current_name] = current_instrs

    return funcs


# ---------------------------------------------------------------------------
# Diffing and scoring
# ---------------------------------------------------------------------------

@dataclass
class InstrMatch:
    """One row in the aligned diff output."""
    orig:  Optional[Instr]
    gen:   Optional[Instr]
    penalty: int
    note:  str  # 'eq', 'imm', 'arg', 'op', 'ins', 'del', 'replace'


def _score_arg_pair(o_arg: str, g_arg: str, o_is_imm: bool, g_is_imm: bool) -> int:
    """Return penalty for a single mismatched argument pair."""
    if o_arg == g_arg:
        return 0
    if o_is_imm and g_is_imm:
        return PENALTY_IMM_DIFF
    return PENALTY_ARG_DIFF


def arg_diffs(orig: Instr, gen: Instr) -> list[tuple[int, str, str, bool]]:
    """For an aligned pair with matching mnemonic+arg-count, return the
    per-argument mismatches as (index, orig_arg, gen_arg, is_immediate).

    Returns [] when the mnemonic or arg count differ (those are structural
    mismatches, not operand-value mismatches), or when the args fully match.
    """
    if orig.mnemonic != gen.mnemonic or len(orig.args) != len(gen.args):
        return []
    out = []
    for idx, (o_a, g_a, o_i, g_i) in enumerate(
            zip(orig.args, gen.args, orig.is_imm, gen.is_imm)):
        if o_a != g_a:
            out.append((idx, o_a, g_a, o_i and g_i))
    return out


def score_instruction_pair(orig: Instr, gen: Instr) -> tuple[int, str]:
    """Compare two instructions with matching opcodes. Returns (penalty, note)."""
    if orig.raw == gen.raw:
        return 0, 'eq'

    # Opcode mismatch
    if orig.mnemonic != gen.mnemonic:
        return PENALTY_OPCODE_REPLACE, 'op'

    # Same opcode — compare args
    if len(orig.args) != len(gen.args):
        # Arg count mismatch is treated like a full replace
        return PENALTY_OPCODE_REPLACE, 'op'

    total = 0
    for o_a, g_a, o_i, g_i in zip(orig.args, gen.args, orig.is_imm, gen.is_imm):
        total += _score_arg_pair(o_a, g_a, o_i, g_i)

    if total == 0:
        return 0, 'eq'
    # Classify highest penalty type
    note = 'imm' if all(o_i or g_i for o_i, g_i in zip(orig.is_imm, gen.is_imm)) else 'arg'
    return total, note


def diff_functions(orig: list[Instr], gen: list[Instr]) -> tuple[list[InstrMatch], float]:
    """Align two instruction sequences and return scored rows + match percent."""
    # Extract opcode sequences for Patience alignment
    orig_ops = [i.mnemonic for i in orig]
    gen_ops  = [i.mnemonic for i in gen]

    # Use SequenceMatcher (Ratcliff/Obershelp, closest available to Patience in stdlib)
    # Setting autojunk=False is important to avoid skipping rare opcodes
    sm = difflib.SequenceMatcher(None, orig_ops, gen_ops, autojunk=False)
    opcodes = sm.get_opcodes()

    rows: list[InstrMatch] = []
    total_penalty = 0

    for tag, i1, i2, j1, j2 in opcodes:
        if tag == 'equal':
            for oi, gi in zip(range(i1, i2), range(j1, j2)):
                pen, note = score_instruction_pair(orig[oi], gen[gi])
                rows.append(InstrMatch(orig[oi], gen[gi], pen, note))
                total_penalty += pen

        elif tag == 'replace':
            # Pair up as many as possible, then emit orphans as insert/delete
            pairs = min(i2 - i1, j2 - j1)
            for k in range(pairs):
                o = orig[i1 + k]
                g = gen[j1 + k]
                if o.mnemonic == g.mnemonic:
                    pen, note = score_instruction_pair(o, g)
                else:
                    pen, note = PENALTY_OPCODE_REPLACE, 'replace'
                rows.append(InstrMatch(o, g, pen, note))
                total_penalty += pen
            # Extras from the longer side
            for k in range(pairs, i2 - i1):
                rows.append(InstrMatch(orig[i1 + k], None, PENALTY_INSERT_DELETE, 'del'))
                total_penalty += PENALTY_INSERT_DELETE
            for k in range(pairs, j2 - j1):
                rows.append(InstrMatch(None, gen[j1 + k], PENALTY_INSERT_DELETE, 'ins'))
                total_penalty += PENALTY_INSERT_DELETE

        elif tag == 'delete':
            for oi in range(i1, i2):
                rows.append(InstrMatch(orig[oi], None, PENALTY_INSERT_DELETE, 'del'))
                total_penalty += PENALTY_INSERT_DELETE

        elif tag == 'insert':
            for gi in range(j1, j2):
                rows.append(InstrMatch(None, gen[gi], PENALTY_INSERT_DELETE, 'ins'))
                total_penalty += PENALTY_INSERT_DELETE

    max_penalty = len(orig) * PENALTY_INSERT_DELETE
    if max_penalty == 0:
        match_pct = 100.0 if not gen else 0.0
    else:
        match_pct = max(0.0, (1.0 - total_penalty / max_penalty)) * 100.0

    return rows, match_pct


# ---------------------------------------------------------------------------
# Display
# ---------------------------------------------------------------------------

# ANSI colour codes
_GREEN  = '\033[32m'
_YELLOW = '\033[33m'
_RED    = '\033[31m'
_CYAN   = '\033[36m'
_RESET  = '\033[0m'
_BOLD   = '\033[1m'

def _colour(text: str, code: str) -> str:
    if sys.stdout.isatty():
        return f'{code}{text}{_RESET}'
    return text

NOTE_COLOUR = {
    'eq':      _GREEN,
    'imm':     _YELLOW,
    'arg':     _YELLOW,
    'op':      _RED,
    'replace': _RED,
    'del':     _RED,
    'ins':     _CYAN,
}

NOTE_SYMBOL = {
    'eq':      '  ',
    'imm':     '~ ',
    'arg':     '~ ',
    'op':      '! ',
    'replace': '! ',
    'del':     '- ',
    'ins':     '+ ',
}


def print_function_diff(name: str, rows: list[InstrMatch], match_pct: float):
    pct_colour = _GREEN if match_pct >= 95 else (_YELLOW if match_pct >= 70 else _RED)
    print(f'\n{_BOLD}=== {name} ==={_RESET}  {_colour(f"{match_pct:.1f}%", pct_colour)}')
    print(f'{"Original":<45}  {"Generated":<45}')
    print('-' * 92)
    for row in rows:
        sym   = NOTE_SYMBOL.get(row.note, '  ')
        col   = NOTE_COLOUR.get(row.note, '')
        orig_s = row.orig.raw[:44] if row.orig else ''
        gen_s  = row.gen.raw[:44]  if row.gen  else ''
        line = f'{sym}{orig_s:<45}  {gen_s:<45}'
        print(_colour(line, col))


def print_summary(results: list[tuple[str, float, int, int]]):
    """Print a summary table of all functions."""
    print(f'\n{_BOLD}{"Function":<48}  {"Orig":>5}  {"Gen":>5}  {"Match%":>7}{_RESET}')
    print('-' * 72)
    total_orig = total_gen = 0
    for name, pct, n_orig, n_gen in results:
        pct_colour = _GREEN if pct >= 95 else (_YELLOW if pct >= 70 else _RED)
        pct_str = _colour(f'{pct:6.1f}%', pct_colour)
        print(f'  {name:<46}  {n_orig:>5}  {n_gen:>5}  {pct_str}')
        total_orig += n_orig
        total_gen  += n_gen
    print('-' * 72)
    # Overall score
    all_pcts = [r[1] for r in results]
    avg = sum(all_pcts) / len(all_pcts) if all_pcts else 0
    avg_col = _GREEN if avg >= 95 else (_YELLOW if avg >= 70 else _RED)
    print(f'  {"AVERAGE":<46}  {total_orig:>5}  {total_gen:>5}  {_colour(f"{avg:6.1f}%", avg_col)}')


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Compare h8cc .s output against main.mar using objdiff scoring.'
    )
    parser.add_argument('asm', help='Compiled assembly file (h8cc .s output)')
    parser.add_argument(
        '--mar', default='main.mar',
        help='Original disassembly (default: main.mar)'
    )
    parser.add_argument(
        '--func', '-f', metavar='FUNC',
        help='Show detailed diff for a specific function'
    )
    parser.add_argument(
        '--detail', '-d', action='store_true',
        help='Show detailed diff for all functions (implies --all)'
    )
    parser.add_argument(
        '--threshold', '-t', type=float, default=0.0,
        help='Only show functions below this match%% (e.g. --threshold 95)'
    )
    args = parser.parse_args()

    mar_path = Path(args.mar)
    asm_path = Path(args.asm)

    if not mar_path.exists():
        sys.exit(f'Error: {mar_path} not found')
    if not asm_path.exists():
        sys.exit(f'Error: {asm_path} not found')

    print(f'Parsing {mar_path}...', end=' ', flush=True)
    orig_funcs = parse_mar(mar_path)
    print(f'{len(orig_funcs)} functions')

    print(f'Parsing {asm_path}...', end=' ', flush=True)
    gen_funcs = parse_asm(asm_path)
    print(f'{len(gen_funcs)} functions')

    # Build the set of functions to compare
    if args.func:
        names_to_diff = [args.func]
    else:
        # All functions present in the compiled output
        names_to_diff = list(gen_funcs.keys())

    results = []
    detail_rows = {}

    for name in names_to_diff:
        if name not in orig_funcs:
            print(f'  [skip] {name}: not found in {mar_path}', file=sys.stderr)
            continue
        if name not in gen_funcs:
            print(f'  [skip] {name}: not found in {asm_path}', file=sys.stderr)
            continue

        orig = orig_funcs[name]
        gen  = gen_funcs[name]
        rows, pct = diff_functions(orig, gen)
        results.append((name, pct, len(orig), len(gen)))
        if args.detail or args.func:
            detail_rows[name] = rows

    if args.func and args.func in detail_rows:
        _, pct, n_orig, n_gen = next(r for r in results if r[0] == args.func)
        print_function_diff(args.func, detail_rows[args.func], pct)
    elif args.detail:
        for name, pct, n_orig, n_gen in results:
            if args.threshold and pct >= args.threshold:
                continue
            print_function_diff(name, detail_rows[name], pct)
    else:
        # Filter by threshold if requested
        display = [r for r in results if r[1] < args.threshold] if args.threshold else results
        print_summary(display if display else results)
        if args.threshold and len(display) < len(results):
            skipped = len(results) - len(display)
            print(f'\n  ({skipped} function(s) at or above {args.threshold}% threshold hidden)')


if __name__ == '__main__':
    main()
