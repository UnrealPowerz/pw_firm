#include "all_headers.h"

/* Backing storage for the B_RAM section (0xF780..0xFEEF, 1904 bytes).
 *
 * `struct b_ram_section` is defined in include/globals.h via the included
 * b_ram_layout.h file. The instance lives in its own `#pragma section _RAM`
 * which the linker anchors at 0xF780 (see Makefile's link.sub generation).
 *
 * The existing globals.h macros (g.sys_statusFlags, g.animTick, etc.) continue to
 * drive all RAM access via volatile-pointer casts — they bypass this
 * struct entirely. The struct's purpose right now is to give the linker a
 * single typed allocation it can place, so scripts/compare_data_layout.py
 * can verify each named main.mar symbol lands at its expected RAM address.
 *
 * Future work: convert the macros to `g.<name>` struct-member access. That
 * requires renaming either the macros or struct members to avoid the
 * preprocessor expanding a macro inside `g.macro_name`.
 */

#pragma section _RAM
volatile struct b_ram_section g;
