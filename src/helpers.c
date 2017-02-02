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

#include "helpers.h"
#include "options.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#define BAR_STEP (100.0/(BAR_WIDTH-2))


void print_error(char *msg, ...)
{
    va_list argpointer;

    va_start(argpointer, msg);
    fprintf(stderr, "vcp: ");
    vfprintf(stderr, msg, argpointer);
    fprintf(stderr, "\n");
    fflush(stderr);

    return;
}

void print_debug(char *msg, ...)
{
    extern opts_t opts;
    if (!opts.debug)
        return;

    va_list argpointer;

    va_start(argpointer, msg);
    printf("vcp (debug): ");
    vprintf(msg, argpointer);
    printf("\n");
    fflush(stdout);

    return;
}

void print_usage()
{
    puts("vcp  Copyright (C) 2009, 2010, 2014  lynix <lynix47@gmail.com>\n");

    puts("This program comes with ABSOLUTELY NO WARRANTY, use at own risk.");
    puts("is free software, and you are welcome to redistribute it under the");
    puts("terms of the GNU General Public License as published by the Free");
    puts("Software Foundation, either version 3 of the License, or (at your");
    puts("option) any later version.\n");

    puts("Usage:    vcp [OPTIONS] SOURCE(S) DESTINATION\n");

    puts("Behaviour:");
    puts("  -d  delete source(s) on success (like `mv`)");
    puts("  -f  overwrite existing files (default: ask)");
    puts("  -k  skip existing files (default: ask)");
    puts("  -p  no real action, only show what would be done");
    puts("  -u  skip identical existing files");
    puts("  -s  ensure each file is synched to disk after write");
    puts("  -t  ignore errors on preserving uid/gid");
    puts("Output control:");
    puts("  -b  display progress bars and file names (default: text)");
    puts("  -B  display progress bars only, no file names");
    puts("  -q  do not print progress indicators");
    puts("  -Q  do not print progress indicators nor file names");
    puts("General options:");
    puts("  -h  print usage and license information");
    puts("  -v  be verbose");
    puts("  -D  print debugging messages\n");

    printf("This version of vcp was built on %s %s.\n", __DATE__, __TIME__);

    return;
}

char *strccat(char *a, char *b)
{
    if (a == NULL && b == NULL)
        return NULL;

    int len_a = (a == NULL) ? 0 : strlen(a);
    int len_b = (b == NULL) ? 0 : strlen(b);

    char *result = malloc(len_a + len_b + 1);
    if (result == NULL)
        return NULL;

    memcpy(result, a, len_a);
    memcpy(result + len_a, b, len_b);
    result[len_a + len_b] = '\0';

    return result;
}

char *path_str(char *path, char *sub)
{
    int len_p = strlen(path);
    int len_s = strlen(sub);

    char *result = malloc(len_p + len_s + 2);
    memcpy(result, path, len_p);
    memcpy(result + len_p + 1, sub, len_s);

    result[len_p] = '/';
    result[len_p + len_s + 1] = '\0';

    return result;
}

char *clean_path(char *path)
{
    /* remove trailing '/' */
    size_t len = strlen(path);
    if (path[len - 1] == '/')
        path[len - 1] = '\0';

    /* find start of last path component */
    char *base = path_base(path);

    /* if none found: dir is CWD, otherwise use everything left */
    char *dir;
    if (base == path) {
        dir = ".";
    } else {
        dir = path;
        *(base - 1) = '\0';
    }
    dir = realpath(dir, NULL);

    char *result = path_str(dir, base);
    free(dir);

    return result;
}

char *path_base(char *path)
{
    char *ret = path;
    for (char *p = path; *p != '\0'; p++)
        if (*p == '/')
            ret = p + 1;
    if (*ret == '\0')
        ret = path;

    return ret;
}

char *size_str(off_t bytes)
{

    double number = (double)bytes;
    char *unit = "B";

    if (number >= 1099511627776) {
        unit = "TiB";
        number = (double)bytes / 1099511627776;
    } else if (number >= 1073741824) {
        unit = "GiB";
        number = (double)bytes / 1073741824;
    } else if (number >= 1048576) {
        unit = "MiB";
        number = (double)bytes / 1048576;
    } else if (number >= 1024) {
        unit = "KiB";
        number = (double)bytes / 1024;
    }

    char *buffer = malloc(MAX_SIZE_L);
    if (buffer == NULL)
        return NULL;
    snprintf(buffer, MAX_SIZE_L, "%.2f %s", number, unit);

    return buffer;
}

char *bar_str(char percent)
{
    char *result = malloc(BAR_WIDTH + 1);
    if (result == NULL)
        return NULL;

    result[0] = '[';
    result[BAR_WIDTH - 1] = ']';
    result[BAR_WIDTH] = '\0';

    double temp = BAR_STEP;
    int i = 1;
    while ((temp <= percent) && (i < BAR_WIDTH - 1)) {
        result[i] = '#';
        i++;
        temp += BAR_STEP;
    }
    while (i < BAR_WIDTH - 1) {
        result[i] = '-';
        i++;
    }

    return result;
}

int ask_overwrite(file_t *old, file_t *new)
{
    char answer;

    do {
        printf("overwrite ");
        if (old->type != SLINK) {
            printf("%s (%s)\n", old->src, size_str(old->size));
        } else {
            printf("%s (symlink to %s)\n", old->dst, old->ldst);
        }
        printf("     with ");
        if (new->type != SLINK) {
            printf("%s (%s)\n", new->src, size_str(new->size));
        } else {
            printf(" symlink to %s\n", new->ldst);
        }
        printf(" (Y/n)? ");
        fflush(stdout);

        answer = getchar();
        if (answer != '\n') {
            while (getchar() != '\n') {
                /* ignore additional chars */
            }
        }
    } while (answer != 'Y' && answer != 'y' && answer != 'n' &&
             answer != '\n');

    if (answer == '\n' || answer == 'Y' || answer == 'y') {
        return 1;
    }

    return 0;
}

void fail_append(strlist_t *fail_list, char *fname, char *error)
{
    char *errmsg;

    errmsg = strccat(fname, ": ");
    errmsg = strccat(errmsg, error);
    if (errno != 0) {
        errmsg = strccat(errmsg, " (");
        errmsg = strccat(errmsg, strerror(errno));
        errmsg = strccat(errmsg, ")");
    }

    if (strlist_add(fail_list, errmsg) != 0) {
        print_debug("failed to add to fail-list:");
        print_error(errmsg);
    }

    return;
}

inline void print_progr_bs(char perc, char *bps, char eta_s, char eta_m,
                           char eta_h)
{
    printf("\r %s %3d%% @ %s/s ETA %02d:%02d:%02d   ", bar_str(perc), perc,
           bps, eta_h, eta_m, eta_s);

    return;
}

inline void print_progr_bm(char perc_f, char perc_t, char *bps, char eta_s,
                           char eta_m, char eta_h)
{
    printf("\rFile: %s %3d%% @ %s/s  |  Total: %s %3d%% ETA %02d:%02d:%02d   ",
           bar_str(perc_f), perc_f, bps, bar_str(perc_t), perc_t,
           eta_h, eta_m, eta_s);

    return;
}

inline void print_progr_ps(char perc, char *size, char *bps, char eta_s,
                           char eta_m, char eta_h)
{
    printf("\r %3d%% of %s @ %s/s ETA %02d:%02d:%02d   ", perc, size, bps,
           eta_h, eta_m, eta_s);

    return;
}

inline void print_progr_pm(char perc_f, char perc_t, char *size_f, char *size_t,
                           char *bps, char eta_s, char eta_m, char eta_h)
{
    printf("\rFile: %3d%% of %s @ %s/s  |  Total: %3d%% of %s ETA %02d:%02d:%02d   ",
           perc_f, size_f, bps, perc_t, size_t, eta_h, eta_m, eta_s);

    return;
}
