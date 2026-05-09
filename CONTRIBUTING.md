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

**Wrong IO register access style.** `include/iodefine.h` exposes each register that has bit fields under two macros: `REG` for byte/word access and `REG_BIT` for bit-field access. Pick the one that matches what the assembly does:

- Single-bit test in an `if`/`while` — use `REG_BIT.FIELD`. The compiler emits `bld/bcc` (matching the ROM); the byte form `(REG & 0xN)` produces `btst/beq` instead.
- Single-bit set/clear, especially when chained on the same peripheral (e.g. `TCRW_BIT.CCLR = 0; TIERW_BIT.IMIEA = 0; TMRW_BIT.CTS = 1;`) — use `REG_BIT.FIELD = N`. The compiler reuses the address register across the chain and emits the same `mov.w + bclr/bset @rN` sequence the ROM uses.
- Multi-bit constant store, byte read-modify-write with a mask (`tmp = REG; tmp &= 0x8F; tmp |= 0x40; REG = tmp;`) — use the byte form `REG`. Bit-field rewrites here would force a sequence of bit ops the ROM didn't use.
- Never `(uint8_t *)0xF0` or similar bare-address pointer tricks. Those literally point at low RAM (0x00F0), not the IO register at 0xF0F0; the original ROM only got the right address because of `mov.b #imm, r0l` preserving the high byte across instructions, which the C decompiler can't express.

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

**Compiler version differences.** The compiler version used for the original firmware is unknown. One instruction pattern appears in the ROM that this project's compiler (ch38 v6.02.02) does not reproduce:
- `bset Rn, Rd` vs a shift loop for `1 << variable`

(The calling-convention helpers `$sp_regsv$3` / `$spregld2$3` look like a systemic blocker but actually aren't. We tested both directions: `-regparam=3` eliminates them and helps several large functions, but ch38 default (`-regparam=2`) keeps them and helps more small functions on net. Either way the project total lands within 0.2pp of the same number. The likely explanation is that the original firmware used **mixed `-regparam` settings per file**, which we cannot reproduce because `#pragma option regparam` is not valid in source code. Current Makefile uses `-stack=medium -cmncode` without `-regparam=3` for the slightly higher net total of 72.3%.)

Functions that use this pattern will plateau below 100% — the comparison tool will show it as mismatched instructions in otherwise-correct code.

(The `bld/bcc` vs `btst/beq` divergence used to be listed here too, but it *is* fixable: it just requires the C source to use `REG_BIT.FIELD` from `iodefine.h` instead of `(REG & 0xMASK)` for single-bit tests on IO registers. See "Wrong IO register access style" in the Iterate section.)

A second class of unfixable divergence is RAM bytes that the original code declared as bit-field unions (e.g. status/setting flag bytes accessed with `bld/bst/bnot`). The decompilation has these as plain `uint8_t` and accesses them with `& 0xMASK`, so the compiler emits `mov+and+cmp` instead of the ROM's bit instructions. Closing this gap would require defining bit-field unions for those RAM globals — possible in principle but not yet done.

**Hand-written assembly in the original.** A small number of routines in the ROM are custom assembly, not compiler output. These are implemented in `src/globals.s` and `src/romdata.s`, or as library functions that were linked in unchanged. They are not expected to be decompiled to C.

When a function is genuinely stuck at a ceiling due to these issues, annotate it with the best-achievable score and move on.

### Annotating known-stuck functions

When you investigate a function and conclude it cannot be improved (or only at very high effort), leave a structured comment block above its definition so future contributors don't repeat the work:

```c
/* Reason: <few-word category>.
 * <one or two sentences of detail explaining what was tried and why it
 * cannot or should not be pushed further>.
 * Class: <one of the classes below> */
// ROM: 0xADDR  XX.X%
return_type function_name(...) { ... }
```

Pick the **Class** from this list so we can later grep/sort by category and look for systemic fixes (e.g. a compiler flag that resolves a whole class at once):

| Class | Meaning |
|---|---|
| `cannot-fix-without-compiler-change` | Caused by ch38 codegen choices we have no flag to override — register allocation, calling-convention helpers, addressing-mode preference. |
| `compiler-runtime-helper` | The "function" is actually a compiler-emitted thunk (register save/restore, etc.), not user C. The C stub will never byte-match. |
| `hand-written-assembly` | The original was inline asm or a library function we don't have source for. |
| `high-effort, partial-fix` | Improvable with substantial iteration on the C source style; document current best and move on. |
| `do-not-bit-field` | Tested converting a flat `& mask` to `_BIT.field` and it regressed — the ROM uses `btst`/`and` here, not `bld`/`bst`. Future bit-field sweeps should skip this site. |

Keep the **Reason:** line short — it shows up in greps. Put detail in the body.

---

## Conventions Summary

- C89: declare variables at the top of each block
- `#include "all_headers.h"` in every `.c` file; no other includes needed except `#include <machine.h>` for CCR/nop builtins
- Do not write `.h` files — they are auto-generated
- Do not rename functions or global variables
- Interrupt handlers: `__interrupt` before the function; `__noregsave` to skip register save/restore
- Inline assembly: `#pragma asm` / `#pragma endasm` blocks
