/* Copyright lynix <lynix47@gmail.com>, 2009, 2010, 2014
 * 
 * This file is part of vcp (verbose cp).
 *
 * vcp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vcp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vcp. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _HELPERS_H
#define _HELPERS_H

#define _XOPEN_SOURCE 500

#include "file.h"
#include "lists.h"

#include <sys/types.h>                  // off_t


// print usage information on stdout
void print_usage();

// print error message to stderr
void print_error(char *string, ...);

// print debugging information to stdout
void print_debug(char *string, ...);

// print progress information in various formats
void print_progr_bs(char perc, char *bps, char eta_s, char eta_m, char eta_h);
void print_progr_bm(char perc_f, char perc_t, char *bps, char eta_s, char eta_m,
        char eta_h);
void print_progr_ps(char perc, char *size, char *bps, char eta_s, char eta_m,
        char eta_h);
void print_progr_pm(char perc_f, char perc_t, char *size_f, char *size_t,
        char *bps, char eta_s, char eta_m, char eta_h);

// copy a string including allocation using malloc()
char *strccat(char *a, char *b);

// construct path string from components
char *path_str(char *path, char *sub);

// convert byte number to human readable representation (IEC format)
char *size_str(off_t bytes);

// print progress bar for given percentage value
char *bar_str(char percent);

// query file overwriting
int ask_overwrite(file_t *old, file_t *new);

// append textual representation of failed file to given string list
void fail_append(strlist_t *fail_list, char *fname, char *error);

#endif
