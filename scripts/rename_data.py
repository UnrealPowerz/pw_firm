#!/usr/bin/env python3
"""rename_data.py — apply word-boundary symbol renames across the project.

Usage:
    python3 scripts/rename_data.py OLD NEW [OLD2 NEW2 ...]

Updates: src/**/*.{c,h}, include/**/*.h, main.mar.
Skips:   build/, scripts/, anything containing globals.s.bak (kept as
         frozen reference for the .s-era symbol layout).

Use this whenever you rename a global so C source, headers, and the
disassembly stay in sync. After running, rebuild and re-run
scripts/data_usage.py to refresh the inventory.
"""
from __future__ import annotations
import re, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

def collect_files() -> list[Path]:
    paths = []
    for sub in ("src", "include"):
        for ext in ("c", "h"):
            paths.extend((ROOT / sub).rglob(f"*.{ext}"))
    paths.append(ROOT / "main.mar")
    return [p for p in paths if "globals.s.bak" not in p.name]

def apply_renames(pairs: list[tuple[str, str]]) -> None:
    files = collect_files()
    patterns = [(re.compile(rf"\b{re.escape(o)}\b"), n, o) for o, n in pairs]
    total = 0
    for fp in files:
        try:
            text = fp.read_text()
        except UnicodeDecodeError:
            continue
        new = text
        file_changes = 0
        for pat, repl, old in patterns:
            new, k = pat.subn(repl, new)
            file_changes += k
        if file_changes:
            fp.write_text(new)
            print(f"  {fp.relative_to(ROOT)}: {file_changes} replacements")
            total += file_changes
    print(f"total: {total}")

def main():
    args = sys.argv[1:]
    if len(args) < 2 or len(args) % 2 != 0:
        print(__doc__); sys.exit(1)
    pairs = list(zip(args[0::2], args[1::2]))
    print("applying renames:")
    for o, n in pairs:
        print(f"  {o} -> {n}")
    apply_renames(pairs)

if __name__ == "__main__":
    main()
