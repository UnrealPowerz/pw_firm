# Contributing

This document describes a suggested workflow for improving the decompilation — fixing functions, raising match scores, and filling in unimplemented code.

---

## The Workflow

### 1. Pick a function to work on

Two tools help find where to focus:

```bash
# Summary table of match scores by unit (requires: make + COMPLETE_DUMP.bin)
python3 scripts/compare_bin.py

# List functions from main.mar not yet implemented in C
python3 scripts/verify_coverage.py --verbose
```

`compare_bin.py` requires the project to have been compiled (`make`) and `COMPLETE_DUMP.bin` to be present in the project root. It prints a table sorted alphabetically by unit. Units marked with `*` have an average match score below 80% and are the primary candidates for improvement. Start with units where the issues are likely C-level (wrong types, wrong loop structure) rather than compiler-version artifacts.

### 2. Read the original assembly

`main.mar` is the ground truth. Every function has a header like:

```
; Function: my_function
; Address: 0x1234
; Size: 0x40
```

followed by the disassembly. Read the assembly before touching the C. Understand the register usage, loop structure, and what data is being accessed. The annotations in `main.mar` often include symbolic names that tell you what global variables or I/O registers are involved.

### 3. Edit the C source

Source files live in `src/`. Find the file containing your function and edit it.

Key rules:
- **C89 syntax** — declare all variables at the top of the block, before any statements.
- **`#include "all_headers.h"`** in every `.c` file — do not include individual headers.
- **Do not rename functions or globals.** Names are linked to `main.mar`. If the assembly says `game_check_battle_end`, the C function must be `game_check_battle_end`.
- **Do not write `.h` files.** Headers are auto-generated. Run `make headers` if prototypes are stale.

### 4. Compile

```bash
# Compile a single file to assembly code (fast)
make asm SRC=src/game/battle.c

# Full build (required before compare_bin.py sees the new code)
make
```

### 5. Compare

```bash
# All functions in a unit with their scores
python3 scripts/compare_bin.py --unit game/battle

# Detailed side-by-side diff for one function
python3 scripts/compare_bin.py --func game_check_battle_end

# Detailed list of all functions sorted by address
python3 scripts/compare_bin.py --list
```

The comparison is instruction-aware: it aligns instructions by address and shows which ones match, which differ, and where code is inserted or missing.

**Caveat — a high score does not mean the code is correct.** `compare_bin.py` compares raw instruction encodings. It does not verify that a `jsr` instruction calls the right function, or that a memory access targets the right global variable. Two `jsr` instructions at the same address look identical to the tool regardless of what they call. A function can score 100% while calling the wrong function or reading the wrong global. Always cross-check against `main.mar` and use `compare_refs.py` to verify that calls and data references actually match.

### 6. Iterate

Common causes of low match scores and how to fix them:

**Wrong integer types.** The H8/300H is a 16-bit machine — most local variables should be `uint16_t` or `int16_t`. Using `uint8_t` where a `uint16_t` is expected causes extra zero-extend instructions. Using `uint32_t` unnecessarily causes 32-bit operations. Match the size to what the assembly actually does.

**Wrong loop structure.** A `for` loop and a `do-while` loop can produce different code. If the assembly tests the condition at the bottom of the loop, use `do { ... } while (cond)`. If it tests at the top and there's a branch-over on entry, use `while` or `for`.

**Wrong cast placement.** A cast in the wrong place changes when truncation or extension happens and can shift instructions around. Watch for `(uint8_t)` casts that suppress sign-extension.

**Wrong expression evaluation order.** Sub-expressions that get evaluated in a different order will cause the compiler to load registers differently. Compare the instruction sequence carefully.

**Missing or extra increment/decrement.** Post-increment vs pre-increment, or incrementing a different variable than expected, are common sources of divergence.

### 7. Annotate

Once you're satisfied with a function's score, run the annotation tool to record the current match percentage in the source:

```bash
python3 scripts/annotate_src.py src/game/battle.c
```

This inserts or updates a `// ROM: 0x1234  87.5%` comment above each function definition. These comments are idempotent — re-running the tool updates them in place. They serve as a progress record and help identify which functions still need work.

---

## Cross-Reference Analysis

When a function's score is stuck and the instruction diff isn't pointing to an obvious fix, check whether the C code is calling the right functions and accessing the right globals:

```bash
# Show calls/data/IO refs that differ between ASM and C for one function
python3 scripts/compare_refs.py --func my_function

# Show all diffing functions in a file
python3 scripts/compare_refs.py --file src/game/social.c
```

The output groups differences into:
- **calls** — function calls or function-pointer references
- **data** — global/extern variable accesses
- **io** — I/O register accesses
- **const** — large constants (ROM/EEPROM addresses, table offsets)

An ASM-only call means the C code is missing a function call that should be there. A C-only call means the C code is calling something the original didn't.

---

## What Cannot Be Fixed in C

Some divergence is not fixable at the C level:

**Compiler version differences.** The compiler version used for the original firmware is unknown. Two instruction patterns appear in the ROM that this project's compiler (ch38 v6.02.02) does not reproduce:
- `bset Rn, Rd` vs a shift loop for `1 << variable`
- `bld/bcc` vs `btst/beq` for single-bit tests

Functions that use these patterns will plateau below 100%. The comparison tool will show these as mismatched instructions in otherwise-correct code.

**Hand-written assembly in the original.** A small number of routines in the ROM are custom assembly, not compiler output. These are implemented in `src/globals.s` and `src/romdata.s`, or as library functions that were linked in unchanged. They are not expected to be decompiled to C.

When a function is genuinely stuck at a ceiling due to these issues, annotate it with the best-achievable score and move on.

---

## Conventions Summary

- C89: declare variables at the top of each block
- `#include "all_headers.h"` in every `.c` file; no other includes needed except `#include <machine.h>` for CCR/nop builtins
- Do not write `.h` files — they are auto-generated
- Do not rename functions or global variables
- Interrupt handlers: `__interrupt` before the function; `__noregsave` to skip register save/restore
- Inline assembly: `#pragma asm` / `#pragma endasm` blocks
