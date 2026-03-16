#!/usr/bin/env python3
"""
generate_headers.py - Generate .h files from .c files in src/ and output to build/gen/
"""

import os
import sys
import re
import subprocess
from pathlib import Path

# ─── Fake type defs injected before pycparser ────────────────────────────────
FAKE_DEFS = r"""
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef signed   char  int8_t;
typedef signed   short int16_t;
typedef signed   int   int32_t;
#define __noregsave
#define __interrupt
#define __attribute__(x)
"""

# ─── Fallback regex ──────────────────────────────────────────────────────────
# Handles nested parentheses in arguments (e.g., function pointers)
FALLBACK_RE = re.compile(
    r'^(?!(?:static|extern|inline|if|for|while|switch|return)\b)'
    r'([a-zA-Z][a-zA-Z0-9_ \t\*]*?)'
    r'(?:\s+|\s*\*|\*\s*)'
    r'([a-zA-Z_][a-zA-Z0-9_]*)'
    r'\s*\(((?:[^()]|\([^()]*\))*)\)\s*\{',
    re.MULTILINE,
)

def fallback_extract(filepath):
    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()
    results = []
    for m in FALLBACK_RE.finditer(content):
        ret, name, args = m.group(1), m.group(2), m.group(3)
        full_match = m.group(0)
        header_part = full_match.split('(')[0]
        ret = header_part[:header_part.rfind(name)].strip()
        
        ret  = ' '.join(ret.strip().split())
        args = ' '.join(args.strip().split())
        if name in {'main', 'if', 'while', 'for', 'switch', 'return', 'else', 'do'}:
            continue
        results.append('{} {}({});'.format(ret, name, args))
    return results

def extract_public_functions(filepath):
    # For now, always use fallback since pycparser is often missing in containers
    return fallback_extract(filepath)

def generate_header(unit, functions):
    guard = unit.upper().replace('/', '_').replace('.', '_') + '_H'
    lines = [
        '#ifndef {}'.format(guard),
        '#define {}'.format(guard),
        '',
        '#include "types.h"',
        '',
    ]
    for f in sorted(set(functions)):
        lines.append(f)
    lines += ['', '#endif /* {} */'.format(guard), '']
    return '\n'.join(lines)

def main():
    src_root = Path("src")
    include_gen = Path("build/gen")
    include_gen.mkdir(parents=True, exist_ok=True)

    c_files = list(src_root.rglob("*.c"))

    for c_file in c_files:
        # unit name relative to src/
        rel_path = c_file.relative_to(src_root)
        unit = str(rel_path.with_suffix(''))
        
        # Header path in build/gen/
        header_path = include_gen / rel_path.with_suffix('.h')
        header_path.parent.mkdir(parents=True, exist_ok=True)

        functions   = extract_public_functions(c_file)
        new_content = generate_header(unit, functions)

        if header_path.exists():
            with open(header_path) as f:
                if f.read() == new_content:
                    continue

        with open(header_path, 'w') as f:
            f.write(new_content)
        print(f"Generated {header_path}")

if __name__ == '__main__':
    main()
