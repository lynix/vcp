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

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "options.h"

void print_usage();
void print_error(char *string, ...);
void print_debug(char *string, ...);

void print_progr_bs(char perc, char *bps, char eta_s, char eta_m,
                           char eta_h);
void print_progr_bm(char perc_f, char perc_t, char *bps, char eta_s,
                    char eta_m, char eta_h);
void print_progr_ps(char perc, char *size, char *bps, char eta_s,
                    char eta_m, char eta_h);
void print_progr_pm(char perc_f, char perc_t, char *size_f, char *size_t,
                    char *bps, char eta_s, char eta_m, char eta_h);

char *strccat(char *a, char *b);
char *path_str(char *path, char *sub);
char *size_str(off_t bytes);
char *bar_str(char percent);
