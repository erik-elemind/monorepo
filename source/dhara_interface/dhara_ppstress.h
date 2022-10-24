/* Dhara portable pseudo-random stress test
 * Copyright (C) 2022 Igor Institute LLC
 *
 * Author: Daniel Beer <daniel.beer@igorinstitute.com>
 */

#ifndef DHARA_PPSTRESS_H_
#define DHARA_PPSTRESS_H_

#include "config.h"

/* Define ENABLE_DHARA_PPSTRESS to enable stress-testing and inspection
 * commands. These add ~5kB of RAM use. You shouldn't need them unless
 * you suspect that something is wrong with either the FTL or the FTL's
 * NAND interface.
 *
 * Before running any of these commands, you should terminate any task
 * that uses storage, and issue a "dhara_clear" command to reset the
 * FTL. After running a NAND stress test, you should run a safe erase of
 * the chip (not the eraseall command provided here, which discards
 * bad-block marks). This is done with "nand_safe_wipe_all_blocks"
 * followed by a reset.
 *
 * Following a high-level stress test (ppstress), or to quickly drop all
 * flash contents, you can issue the following sequence:
 *
 *    dhara_clear
 *    dhara_sync
 *    reset
 *
 * This is preferable to a full erase, but it works only if the FTL is
 * in a valid state.
 */
#if (defined(ENABLE_DHARA_PPSTRESS) && (ENABLE_DHARA_PPSTRESS > 0U))

/* General diagnostics */
void dhara_cmd_stat(int argc, char **argv);

/* Randomized stress testing */
void dhara_cmd_ppstress(int argc, char **argv);
void dhara_cmd_ppnand(int argc, char **argv);

/* Bad-block debugging */
void dhara_cmd_bbmap(int argc, char **argv);
void dhara_cmd_blkerase(int argc, char **argv);
void dhara_cmd_blkmark(int argc, char **argv);
void dhara_cmd_eraseall(int argc, char **argv);

#endif // (defined(ENABLE_DHARA_PPSTRESS) && (ENABLE_DHARA_PPSTRESS > 0U))

#endif
