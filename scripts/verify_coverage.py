#!/usr/bin/env python3
"""
verify_coverage.py - List missing functions in Pokewalker decompilation.

Only compares main.mar (ground truth) against function names found in src/.
Renamed functions or library symbols will be reported as missing unless 
implemented with their original names.
"""

import re
import os
import sys

def parse_original_functions(mar_path):
    """Parse main.mar for all (address, name) pairs."""
    with open(mar_path, encoding="latin-1") as f:
        content = f.read()
    func_list = []
    # Match "; Function: name" followed by "; Address: addr"
    for m in re.finditer(
        r"; Function:\s*(\S+)\s*\n;\s*Address:\s*([0-9a-fA-F]+)", content
    ):
        func_list.append((int(m.group(2), 16), m.group(1)))
    func_list.sort()
    return func_list

def get_functions_in_file(filepath):
    """Return set of all function names defined in a file."""
    with open(filepath, encoding="utf-8", errors="ignore") as f:
        content = f.read()
    
    # Matches: type name(args) { ... }
    # Accounts for pointers, spaces, and nested parentheses (for function pointers)
    pattern = r'[a-zA-Z_][a-zA-Z0-9_ \t\*]*?[\s\*]([a-zA-Z_][a-zA-Z0-9_]*)\s*\(((?:[^()]|\([^()]*\))*)\)\s*\{'
    
    names = set()
    keywords = {'if', 'while', 'for', 'switch', 'return', 'else', 'do'}
    for m in re.finditer(pattern, content):
        name = m.group(1)
        if name not in keywords:
            names.add(name)
    return names

def main():
    mar_path = "main.mar"
    if not os.path.exists(mar_path):
        print(f"Error: {mar_path} not found.")
        sys.exit(1)

    func_list = parse_original_functions(mar_path)
    
    # Scan src/ for all implemented functions
    implemented_names = set()
    for root, _, files in os.walk("src"):
        for f in files:
            if f.endswith(".c") or f.endswith(".s"):
                implemented_names.update(get_functions_in_file(os.path.join(root, f)))

    orig_names = {name for addr, name in func_list}
    missing = [(addr, name) for addr, name in func_list if name not in implemented_names]
    extra = sorted([name for name in implemented_names if name not in orig_names])

    print(f"Total Original (MAR): {len(func_list)}")
    print(f"Total Implemented in C: {len(implemented_names)}")
    print(f"Functions matching MAR: {len(func_list) - len(missing)}")
    print(f"Coverage: {(len(func_list) - len(missing))/len(func_list)*100:.1f}%")
    
    print(f"\nMissing functions ({len(missing)} total):")
    print("-" * 40)
    for addr, name in missing:
        print(f"  0x{addr:04x}  {name}")

    print(f"\nExtra functions in C ({len(extra)} total):")
    print("-" * 40)
    for name in extra:
        print(f"  {name}")

if __name__ == "__main__":
    main()
