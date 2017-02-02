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

#ifndef _OPTIONS_H
#define _OPTIONS_H

#define BUFFS 1048576       /* 1MiB buffer for read() and write()       */
#define BUFFM 10            /* buffer multiplier, see work_list()       */
#define BAR_WIDTH 20        /* progress bar width (characters)          */
#define MAX_SIZE_L 15       /* maximum length of size string, numbers   */


typedef struct {
    unsigned int bars            : 1;
    unsigned int force           : 1;
    unsigned int filenames       : 1;
    unsigned int sync            : 1;
    unsigned int delete          : 1;
    unsigned int keep            : 1;
    unsigned int quiet           : 1;
    unsigned int update          : 1;
    unsigned int verbose         : 1;
    unsigned int pretend         : 1;
    unsigned int debug           : 1;
    unsigned int ignore_uid_err  : 1;
} opts_t;


// initialize given options structure
void init_opts(opts_t *opts);

// parse command line, fill options structure
int parse_opts(opts_t *opts, int argc, char *argv[]);

#endif
