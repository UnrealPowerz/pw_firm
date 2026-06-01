# Contributing

This document describes a suggested workflow for improving the decompilation. Since the compiled binary now runs end-to-end on the emulator, contributions fall into four broad categories — in rough order of leverage:

1. **[Hunting the compiler](#hunting-the-compiler)** — identifying the exact ch38 release + flags the original was built with. If this ever lands, the score gap closes wholesale.
2. **[Finding semantic bugs](#finding-semantic-bugs)** — using the emulator to catch functional regressions that `compare_bin.py` is blind to.
3. **[Globals and data references](#globals-and-data-references)** — cleaning up `DAT_f7xx` names and inconsistent global types. Pays off independently of the compiler hunt.
4. **[Raising match scores](#raising-match-scores)** — the original workflow. Still useful on functions stuck in the 50–70% range where the issue is C-level, not codegen.

Pick whichever motivates you. The score workflow is the most mechanical and is documented in the most detail below, but it's not the most valuable place to start anymore.

---

## Hunting the compiler

The single biggest unresolved problem. Average match score has plateaued at ~76% and the dominant cause is that we don't know the exact ch38 release the original was built with.

### What we know

- It's some release of Renesas ch38 (symbol mangling, calling-convention helpers `$sp_regsv$3` / `$spregld2$3`, register-save prologue patterns all match).
- ch38 v6.02.02 (current default in the Docker image) is the closest among ~half a dozen versions we've tried. None reproduce the ROM fully.
- Even with the right version, the build appears to have used **mixed `-regparam` per file**: `-regparam=3` helps several large functions but regresses many small ones; `-regparam=2` (default) is the inverse. Neither matches the ROM on net better than ~0.2pp from the other.

### Patterns that pinpoint codegen divergence

These are the most reliable "smoke" markers — if you find a compiler release that emits these the same way the ROM does, you've found it.

| ROM emits | All our ch38 versions emit | Where to see it |
|-----------|----------------------------|-----------------|
| `bset Rn, Rd` for `dst \|= 1 << bit` (variable bit) | `1 << var` via a shift loop ending in `or.b r1l, r0l` | Anywhere `_BIT.b<N> = 1` is used with a runtime bit index — there are a handful in `src/game/battle.c` and `src/ui/`. |
| `divxs.w` + dead-code-eliminated sign-extension | `extu.w` + `mulxu.w` then a full divide | Pedometer step accumulation; some Y-axis position math |
| Tighter prologue scheduling around `$sp_regsv$3` | Helper jsr issued before the spill that would have made it unnecessary | Most "Class: cannot-fix-without-compiler-change" annotations in source |

`scripts/compare_bin.py --func <name>` shows the instruction-level diff for any function — that's where to spot a new pattern.

### What to try

- **Other Renesas releases.** Renesas has shipped ch38 / cc38h under several SKUs (the "H8SX/H8S/H8 family" CC compiler package, the standalone H8/300H package, various SKU revisions). The version history goes back well over a decade. Anything pre-v6 we haven't tested.
- **Vendor-patched builds.** Pokémon HG/SS shipped in 2009. A toolchain that old likely had Nintendo or Game Freak patches applied. These won't be on public download pages.
- **Build-flag combinations we haven't tested.** The `optlnk` linker has more options than we currently use; the same goes for ch38 itself. Particularly anything around inlining policy, code reordering, and constant pooling.
- **Per-file `-regparam` discovery.** If we could identify *which* files used `=3` vs `=2` in the original, we could match per-file even without per-file pragmas — the Makefile can pass different flags per `src/foo.c`.

### What to ignore

- Versions of ch38 that produce wildly different code in trivial functions are obviously the wrong toolchain — don't bother running the full match score on those.
- Anything that isn't ch38 (gcc-h8, sdcc, the old AS38 standalone assembler). Symbol mangling and the runtime helpers rule them out fast.

---

## Finding semantic bugs

`compare_bin.py` is byte-level: two instructions look identical if their encoded bytes match. That makes it **blind** to:

- Calling the wrong function — a `jsr @0x1234` and `jsr @0x5678` have different operands but the same opcode, and a function with the wrong address baked in still encodes the same way as one with the right address.
- Swapped arguments to a function — both encode identically when the args are in registers, even though the runtime behavior is different.
- Inverted conditionals — `bcs` vs `bcc` is one bit and looks like a normal codegen difference.
- Off-by-one in table indexing.
- Calling-convention assumptions in the C source that don't match what the original asm relied on (a function with a "secret" side-effect-on-a-register that the original C exploited and we can't express in C).

These are the bugs the emulator finds. They typically score 100% on `compare_bin` and produce visible misbehavior the moment you exercise the affected code path.

### Workflow

```bash
# Build, then run side-by-side
make
scripts/run_emu.sh           # our compiled ROM
scripts/run_emu.sh --orig    # the reference ROM, same eeprom seed

# Capture an instruction trace while reproducing a bug
scripts/run_emu.sh 2>/tmp/pw_trace.log
```

Exercise the same feature in both. When they diverge visually, the divergence is a semantic bug in the C. Find the function responsible (use the trace if needed), then look at `main.mar` for the same function and compare *semantics* — not just the instruction encoding.

### Representative examples

These are real bugs we found this way that all scored well on `compare_bin`:

- **`gfx_draw_sprite_simple` — swapped `w` and `h` parameters.** Our C signature had `(x, y, w, h, buf)`. The asm puts the third arg in `r1` and the fourth in `e0`. The function body uses `r1` for the vertical-height calculation and `e0` for stride. So `(x, y, w, h, buf)` ended up using `w` as height and `h` as stride — sprites rendered with 4 pages instead of 3 and the wrong stride. Fix: swap the C signature to `(x, y, h, w, buf)`. Score didn't move; pokemon sprites stopped being garbled.
- **Battle name-selection inverted `subY` check.** Five different cases in `ui_render_battle` had `if (gCurSubstateY < 4) gfx_draw_special_poke_name(...) else gfx_draw_route_pokemon_name(...)`. The asm `bcc` after `cmp.b #H'4, r0l` branches when `subY >= 4`, not `< 4`. Our build called `gfx_draw_special_poke_name` (which reads `EEPROM 0xC6FC`, factory-empty) for the wild-encounter case where `subY = 1..3`. Result: wild-pokemon name renders as a solid black box. `compare_refs.py` would have flagged it (different EEPROM addresses), but `compare_bin` says 100% because the call instruction encodes the same way.
- **`gfx_add_font_border` calling-convention abuse.** The original is a "leaf-like" function that modifies the caller's `r6` (passed pointer) by 6 bytes as a side effect — the caller relies on this to walk through 10 digits in the font table. C has no way to express "modify the caller's pointer register", so calling it twice from C OR's the same 3 words repeatedly instead of advancing. The decomp had to inline the OR sequence in the caller. Found by noticing the home-screen status bar's last segment rendering at the wrong y.

When you find one of these: it's worth a sentence in the function's comment block explaining what the asm relied on, since `compare_bin` won't surface it again.

---

## Globals and data references

A large chunk of the decomp's globals are still `DAT_f7xx`-named with types inferred per-call-site and sometimes inconsistently. This is mechanically improvable work — it doesn't require a deep understanding of the function semantics, just patience and care with the rename tooling.

### Symptoms

- Globals named `DAT_<addr>` (e.g. `DAT_f7d8`) in `include/globals.h`. The address is the only useful thing about the name.
- A single byte at one address being accessed as `uint8_t` in some functions and `*((uint16_t*)&DAT_xxx)` in others. The disassembly tells you which is right.
- `compare_refs.py --file <foo>.c` reports `data:` differences — the C is reading from a different address than the asm.
- Two adjacent bytes that the original code treated as a single `uint16_t` but the decomp split into two `uint8_t` globals (or vice versa).

### Workflow

```bash
# Comprehensive map of every symbol and who touches it
python3 scripts/data_usage.py
$EDITOR build/data_usage.md

# Find data-ref divergence for one file
python3 scripts/compare_refs.py --file game/battle.c

# Rename a global across src/, include/, AND main.mar in one shot
python3 scripts/rename_data.py DAT_f7d8 active_item_id
```

The flow is:

1. Pick a `DAT_f7xx` global. Open `build/data_usage.md` and look at every reader/writer.
2. Read those call sites in `main.mar` to figure out the *actual* type and what the value represents.
3. If the type needs to change (e.g. `uint8_t` → `uint16_t`), update `include/globals.h` and verify every caller still does the right thing — particularly look for sites that cast through pointers; those usually become wrong once the underlying type changes.
4. Rename it. Use `rename_data.py` so `src/`, `include/`, *and* `main.mar` all stay in sync — the symbol map between C and the disassembly matters for `compare_bin.py` to work.
5. Rebuild and re-run `compare_refs.py` on affected files to confirm the data refs now agree.

### Cautions

- **`src/globals.s` is a hand-written assembly file mapping symbol names to RAM addresses.** Don't rename a symbol without also updating it there. `rename_data.py` handles this but check the diff before committing.
- **`src/globals.s.bak` is intentionally preserved as the pre-rename layout reference.** `rename_data.py` skips it by design — don't "fix" it.
- **Don't rename anything used by `iodefine.h`** — those names are tied to the hardware register layout.

---

## Raising match scores

This is the original workflow and is still useful for functions stuck at 50–70% where the issue is C-level. The text below is the long-form version of the workflow; some of it (steps 1–7) hasn't changed since before the emulator came online.

Before diving in: if you're not sure whether a function's problem is "C-level" or "compiler-version", run it in the emulator first. If it visibly misbehaves, the problem is semantic and you want [Finding semantic bugs](#finding-semantic-bugs). If it behaves correctly but scores below 80%, it's a candidate for score work.

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

## Known ceilings

Some divergence won't close until either (a) the [compiler hunt](#hunting-the-compiler) succeeds, or (b) the project explicitly drops byte-matching as a goal. Today these are ceilings, not bugs:

**Codegen patterns no ch38 release we have reproduces.** See [Hunting the compiler → Patterns that pinpoint codegen divergence](#patterns-that-pinpoint-codegen-divergence). Functions affected by these plateau below 100%; the comparison tool shows it as mismatched instructions in otherwise-correct code.

**RAM bit-field representation.** A subset of RAM bytes (status flags, settings flags) are accessed in the ROM via `bld/bst/bnot`. The decomp models them as plain `uint8_t` with `& 0xMASK` operations, so the compiler emits `mov+and+cmp` instead of bit instructions. Closing this gap is mechanically possible — define bit-field unions for those globals — but hasn't been done yet. This *is* fixable; it's just unbuilt work, parked under [Globals and data references](#globals-and-data-references).

**Hand-written assembly in the original.** A small number of routines in the ROM are custom asm, not compiler output. These are implemented in `src/globals.s` and `src/romdata.s` (or as library functions linked in unchanged). They are not expected to be decompiled to C and will never byte-match through a C path.

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
