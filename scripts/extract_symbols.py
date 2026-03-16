import sys
import re

def extract_symbols(filename):
    try:
        with open(filename, 'r') as f:
            content = f.read()
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found.", file=sys.stderr)
        return

    # Define the regex pattern for function comment blocks
    # Format:
    # ; Function: <name>
    # ; Address:  <addr>
    pattern = re.compile(r'; Function:\s+([^\n]+)\n; Address:\s+([0-9a-fA-F]+)')

    matches = pattern.findall(content)

    for name, addr in matches:
        # Ensure address has 0x prefix or correct hex format if needed
        # User requested <hex addr> <name> f
        # Usually Ghidra likes 0x prefix, but user provided example without it.
        # I'll stick to the requested format exactly.
        print(f"{addr.strip()} {name.strip()} f")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 extract_symbols.py <filename>", file=sys.stderr)
    else:
        extract_symbols(sys.argv[1])
