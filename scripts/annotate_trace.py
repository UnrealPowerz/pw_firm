#!/usr/bin/env python3
"""Annotate emulator PC traces with function names.

Reads a symbol table (`<hex_addr> <name> [f|d]` per line) and rewrites stdin
lines, replacing `PC=0xXXXX` with `PC=0xXXXX[name+off]`.

Optional --calls mode: emit one line per call (BSR/JSR detection by symbol
boundary crossings going *into* a symbol, ignoring fall-through). Useful for
seeing the call flow without the per-instruction noise.

Usage:
    annotate_trace.py [--symbols=FILE] [--calls] [--unique-loops] < trace.log

Defaults to build/our_symbols.txt. Pass --symbols=symbols.txt for the original.
"""
import argparse
import re
import sys
from pathlib import Path


def load_symbols(path):
    syms = {}
    for line in Path(path).read_text().splitlines():
        parts = line.split()
        if len(parts) < 2:
            continue
        try:
            addr = int(parts[0], 16)
        except ValueError:
            continue
        syms[addr] = parts[1]
    return syms


def make_lookup(syms):
    """Return (sorted_addrs, names) for bisect-style lookup."""
    addrs = sorted(syms)
    names = [syms[a] for a in addrs]
    return addrs, names


def find_sym(addrs, names, pc):
    """Return (name, offset, sym_start). offset is bytes past sym_start."""
    import bisect

    i = bisect.bisect_right(addrs, pc) - 1
    if i < 0:
        return None, pc, None
    return names[i], pc - addrs[i], addrs[i]


PC_RE = re.compile(r"PC=0x([0-9A-Fa-f]+)")


def annotate(line, addrs, names):
    def repl(m):
        pc = int(m.group(1), 16)
        name, off, _ = find_sym(addrs, names, pc)
        if name is None:
            return m.group(0)
        return f"{m.group(0)}[{name}+{off:x}]"

    return PC_RE.sub(repl, line)


def call_flow(lines, addrs, names):
    """Emit one line per symbol entry (call) and exit, with depth indent.

    Heuristic: when the symbol containing PC changes between consecutive
    instructions, emit a transition. Compress tight loops by skipping repeats
    of the same (caller_sym -> callee_sym) within a window of 1 step.
    """
    cur_sym = None
    cur_start = None
    depth_stack = []
    out = []
    for line in lines:
        m = PC_RE.search(line)
        if not m:
            continue
        pc = int(m.group(1), 16)
        name, off, start = find_sym(addrs, names, pc)
        if start == cur_start:
            continue
        # Transition. Decide call vs return: if returning to a sym we previously
        # called from, treat as return.
        if depth_stack and depth_stack[-1] == start:
            depth_stack.pop()
            out.append(f"{'  ' * len(depth_stack)}<- back to {name}")
        else:
            if cur_start is not None:
                depth_stack.append(cur_start)
            out.append(f"{'  ' * (len(depth_stack))}-> {name} (0x{pc:04x})")
        cur_sym = name
        cur_start = start
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--symbols", default="build/our_symbols.txt")
    ap.add_argument("--calls", action="store_true",
                    help="emit one line per call/return instead of per instruction")
    args = ap.parse_args()

    syms = load_symbols(args.symbols)
    addrs, names = make_lookup(syms)

    lines = sys.stdin.readlines()
    if args.calls:
        for ln in call_flow(lines, addrs, names):
            print(ln)
    else:
        for ln in lines:
            sys.stdout.write(annotate(ln, addrs, names))


if __name__ == "__main__":
    main()
