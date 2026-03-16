# Pokewalker Firmware Decompilation

Byte-accurate C decompilation of the Pokewalker (Pokémon HeartGold/SoulSilver) firmware.

The goal is to reconstruct the original C source code as closely as possible — not just something that recompiles to equivalent behavior, but code that, when compiled with the original toolchain, produces output that matches the ROM byte-for-byte.

**Current status:** all function implemented to some extent, 68.7% average match score.
Global variables are not all there yet. The project compiles but is not at all ready to run on an emulator -- though I hope we get there some day.

---

## Hardware Overview

| Component | Details |
|-----------|---------|
| MCU | Renesas H8/38606R — H8/300H core, 3.6 MHz, 2 KiB RAM, 48 KiB ROM |
| Display | 96×64 4-shade grayscale LCD (SSD1854) |
| Input | 3 buttons |
| Sensors | Accelerometer (BMA150), IrDA transceiver |
| Storage | 64 KiB EEPROM (M95512) |
| Audio | Piezo buzzer |

---

## Repository Layout

```
.
├── src/
│   ├── drivers/        # Hardware drivers (ADC, buttons, EEPROM, IR, LCD, RTC, sound)
│   ├── engine/         # Mid-level subsystems (graphics, IR protocol, UI dispatch)
│   ├── game/           # Game logic (battle, pedometer, session, social, minigames)
│   ├── system/         # Startup, interrupts, power, save, utilities
│   ├── ui/             # UI screens and menus
│   ├── globals.s       # Hand-written assembly: global variable layout
│   └── romdata.s       # Hand-written assembly: ROM data tables
├── include/
│   ├── all_headers.h   # Convenience include (included by every .c file)
│   ├── globals.h       # Global variable declarations
│   ├── iodefine.h      # H8 peripheral register definitions
│   └── types.h         # Integer typedefs + clang compatibility stubs
├── scripts/            # Build and analysis tooling (see Tooling section)
├── setup/              # Docker image build files
├── main.mar            # Ground-truth disassembly (reference for all decompilation)
├── symbols.txt         # Symbol table for the firmware (can be imported into Ghidra using the import symbols script)
└── Makefile
```

---

## Prerequisites

- **Docker** — for the Renesas H8 C compiler toolchain
- **Python 3.9+** — for analysis scripts
- **libclang** Python bindings — for `annotate_src.py`

```bash
pip install libclang
```

### Building the Docker Image

The `h8` Docker image bundles multiple versions of the Renesas H8 C compiler toolchain under Wine, plus a GCC H8 cross-toolchain for binary utilities. The docker image provides an easy way to run these old Windows-only tools on Linux or Mac.

**You must obtain the Renesas H8 C compiler installers yourself.** Renesas distributes these as proprietary commercial software. The `setup/` directory contains `unpack.sh`, which extracts them once you have the installer files. Place the installer files in `setup/installers/` before building.

> The build system uses the Renesas ch38 compiler — a proprietary tool. Obtain it through legitimate channels.

Once you have the installers in place:

```bash
docker build -t h8 setup/
```

This takes a while. It builds binutils for H8, compiles ISx and unshield from source, extracts all toolchain versions, and sets up a Wine environment.

### Obtaining the Reference Binary

You need `COMPLETE_DUMP.bin` — a raw binary dump of the Pokewalker ROM — for the comparison scripts to work. This file is **not distributed** with the repository. Dump it from a physical Pokewalker using appropriate hardware. Place it in the project root.

The ground-truth disassembly (`main.mar`) is included in the repository. It is the primary reference for all decompilation work.

---

## IDE / Editor Setup (clangd)

The repository includes a `.clangd` config for LSP-based editors (VS Code, Neovim, etc.).

Before opening the project in an editor, run these two steps:

```bash
# 1. Extract H8 SDK headers into h8inc/ (needed for <machine.h> and friends)
make sdk-headers

# 2. Generate cross-file function prototypes into build/gen/
make headers
```

`types.h` contains `#ifdef __clang__` stubs for H8-specific keywords (`__noregsave`, `__interrupt`, etc.) so clangd does not choke on them.

The `.clangd` file references paths inside the Docker image. You may need to adjust the `CompileFlags.Add` entries to point to your local `h8inc/` directory.

---

## Quick Start

```bash
# Full build (compiles all C, links, disassembles)
make

# Overall match scores by unit (requires make + COMPLETE_DUMP.bin)
python3 scripts/compare_bin.py

# Inspect a specific function
python3 scripts/compare_bin.py --func <function_name>

# Compile a single file (faster iteration)
make asm SRC=src/drivers/adc.c

# Compare a single function by assembly code directly (no objdump, includes labels)
python3 scripts/compare.py build/asm/drivers/adc.s --func drv_adc_check_battery
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for the full decompilation workflow.

---

## Source Tree

| Directory | Contents |
|-----------|----------|
| `src/drivers/` | Hardware abstraction: ADC, buttons, EEPROM, IR PHY, LCD, RTC, sound, accelerometer |
| `src/engine/` | Graphics rendering, IR communication protocol, UI dispatch/event loop |
| `src/game/` | Battle system, pedometer, session management, social features, dowsing and radar minigames |
| `src/system/` | Startup code, interrupt vectors, power management, save system, libc glue |
| `src/ui/` | Menus, stats screen, trainer card, animations, debug screen |

---

## Tooling

| Script | Description |
|--------|-------------|
| `scripts/compare_bin.py` | Main comparison tool — match scores vs the reference ROM, by function or unit |
| `scripts/compare.py` | Another comparison tool — match scores vs the reference assembly code directly |
| `scripts/compare_refs.py` | Cross-reference diff — shows calls/data/IO refs present in ASM but missing from C |
| `scripts/verify_coverage.py` | Lists functions from `main.mar` not yet implemented in C |
| `scripts/annotate_src.py` | Inserts `// ROM: 0xABCD  nn.n%` comments above each C function |
| `scripts/generate_headers.py` | Auto-generates `build/gen/` prototype headers from source (run by `make headers`) |

### Build Targets

| Target | Description |
|--------|-------------|
| `make` | Full build: headers + compile all C + link + disassemble |
| `make asm SRC=src/path/file.c` | Compile one file to `build/asm/path/file.s` |
| `make headers` | Regenerate `build/gen/` headers |
| `make sdk-headers` | Extract H8 SDK headers from Docker into `h8inc/` |
| `make clean` | Remove build artifacts (preserves `lib3hn.lib`) |

---

## Technical Reference

### C Dialect

All source files use **C89**. Variables must be declared at the top of each block. Every `.c` file includes `include/all_headers.h` (which pulls in `types.h`, `globals.h`, `iodefine.h`, and all generated prototypes).

Do not write `.h` files by hand — they are auto-generated from the source by `scripts/generate_headers.py`. Running `make` or `make headers` regenerates them.

### Integer Types

The H8/300H data model: `char` = 8-bit, `int` = 16-bit, `long` = 32-bit.

```c
typedef unsigned char  uint8_t;   /*  8-bit */
typedef unsigned int   uint16_t;  /* 16-bit */
typedef unsigned long  uint32_t;  /* 32-bit */
```

### machine.h Builtins

Include with angle brackets: `#include <machine.h>`

| Builtin | Description |
|---------|-------------|
| `set_ccr(v)` | Set condition code register |
| `get_ccr()` | Read condition code register |
| `and_ccr(v)` | AND into CCR (e.g., clear I flag to enable interrupts) |
| `or_ccr(v)` | OR into CCR (e.g., set I flag to disable interrupts) |
| `nop()` | No-operation |

### Compiler Notes

The compiler version used for the original firmware is unknown. The Docker image provides ch38 **v6.02.02** (but also others!), which produces nearly identical output but does not reproduce some instruction patterns found in the ROM. Register allocation also sometimes differs. I am not sure at this point what causes these differences, because the project was clearly compiled with some version of ch38. All the other versions of ch38 I have tried produce very similar results though.

### Useful Links

- [Renesas CC Compiler Package for H8SX, H8S, H8 Family](https://www.renesas.com/en/software-tool/cc-compiler-package-h8sx-h8s-h8-family)
- [Renesas H8/300H Series CC Compiler Package Ver.700 Users Manual](https://www.renesas.com/en/document/mat/h8s-h8300-series-cc-compiler-package-ver700-users-manual)
- [Renesas CC Compiler Package for H8SX, H8S, H8 Family Software Component List](https://www.renesas.com/en/document/oth/cc-compiler-package-h8sx-h8s-h8-family-software-component-list)
- [Renesas H8/38602R Group Hardware Manual](https://www.renesas.com/en/document/mah/h838602r-group-hardware-manual)
- [PokéWalker hacking by Dmitry.GR](https://dmitry.gr/?r=05.Projects&proj=28.%20pokewalker)
- [picowalker](https://github.com/mamba2410/picowalker-core)
- [PoketWalker emulator](https://github.com/h4lfheart/PocketWalker)