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

#include "options.h"
#include "helpers.h"

#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>

void init_opts(opts_t *opts)
{
    opts->bars = 0;
    opts->force = 0;
    opts->filenames = 1;
    opts->sync = 0;
    opts->delete = 0;
    opts->keep = 0;
    opts->quiet = 0;
    opts->verbose = 0;
    opts->update = 0;
    opts->pretend = 0;
    opts->debug = 0;
    opts->ignore_uid_err = 0;

    return;
}

int parse_opts(opts_t *opts, int argc, char *argv[])
{
    char c;
    extern int optind, optopt, opterr;

    opterr = 0;

    while ((c = getopt(argc, argv, "bdfhkpqstuvDBQ")) != -1) {
        switch (c) {
            case 'b':
                opts->bars = 1;
                break;
            case 'B':
                opts->bars = 1;
                opts->filenames = 0;
                break;
            case 'f':
                if (opts->keep == 0) {
                    opts->force = 1;
                } else {
                    print_error("no -f and -k at the same time\n");
                    return -1;
                }
                break;
            case 'k':
                if (opts->force == 0) {
                    opts->keep = 1;
                } else {
                    print_error("no -f and -k at the same time\n");
                    return -1;
                }
                break;
            case 's':
                opts->sync = 1;
                break;
            case 't':
                opts->ignore_uid_err = 1;
                break;
            case 'p':
                opts->pretend = 1;
                break;
            case 'q':
                opts->quiet = 1;
                break;
            case 'Q':
                opts->quiet = 1;
                opts->filenames = 0;
                break;
            case 'd':
                opts->delete = 1;
                break;
            case 'h':
                print_usage();
                exit(EXIT_SUCCESS);
            case 'u':
                opts->update = 1;
                break;
            case 'v':
                opts->verbose = 1;
                break;
            case 'D':
                opts->debug = 1;
                break;
            case '?':
                if (isprint(optopt)) {
                    print_error("unknown option \"-%c\".\nTry -h for help.",
                                optopt);
                } else {
                    print_error("unknown option character \"\\x%x\".\nTry -h for help.",
                                optopt);
                }
                return -1;
            default:
                return -1;
        }
    }

    return optind;
}
