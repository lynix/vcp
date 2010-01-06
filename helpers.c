/* Copyright lynix <lynix47@gmail.com>, 2009, 2010
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


void print_error(char *msg, ...)
{
    /* prints an error message to stderr                                */
    
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
    /* prints debugging messages to stdout if debug is set                */
    
    va_list argpointer;
    extern struct options opts;
    
    if (opts.debug) {
        va_start(argpointer, msg);
        printf("vcp (debug): ");
        vprintf(msg, argpointer);
        printf("\n");
        fflush(stdout);
    }
    
    return;
}

void print_usage()
{
    /* prints copyright and usage information                            */
    
    printf("vcp  Copyright (C) 2009, 2010  lynix <lynix47@gmail.com>\n\n");
    
    printf("This program comes with ABSOLUTELY NO WARRANTY, use at\n");
    printf("own risk. This is free software, and you are welcome to\n");
    printf("redistribute it under the terms of the GNU General\n");
    printf("Public License as published by the Free Software\n");
    printf("Foundation, either version 3 of the License, or (at your\n");
    printf("option) any later version.\n\n");
    
    printf("Usage:    vcp [OPTIONS] SOURCE(S) DESTINATION\n");
    printf("\n");
    printf("Behaviour:\n");
    printf("  -d  delete source(s) on success\n");
    printf("  -f  overwrite existing files (default: ask)\n");
    printf("  -k  skip all existing files (default: ask)\n");
    printf("  -p  only show what would be done\n");
    printf("  -u  skip identical existing files\n");
    printf("  -s  ensure each file is synched to disk after write\n");
    printf("  -t  ignore errors on preserving uid/gid\n");
    printf("Output control:\n");
    printf("  -b  display progress bars (default: text)\n");
    printf("  -B  display progress bars only, no filenames\n");
    printf("  -q  do not print progress indicators\n");
    printf("  -Q  do not print progress indicators nor filenames\n");
    printf("General options:\n");
    printf("  -h  print usage and license information\n");
    printf("  -v  be verbose\n");
    printf("  -D  print debugging messages\n");
    printf("\n");
    printf("This version of vcp was built on %s %s.\n", __DATE__,
            __TIME__);
    
    return;
}

char *strccat(char *a, char *b)
{
    /* like strcat() but does not overwrite first argument and takes    *
     * care of sufficient destination size                                */
    
    char *retval;
    int n,m;
    
    if (a == NULL && b == NULL) {
        return NULL;
    }
    
    if (a == NULL) {
        n = 0;
    } else {
        n = strlen(a);
    }
    if (b == NULL) {
        m = 0;
    } else {
        m = strlen(b);
    }
        
    if ((retval = malloc(n+m+1)) == NULL) {
        return NULL;
    }
    for (int i=0; i<n; i++) {
        retval[i] = a[i];
    }
    for (int i=0; i<m; i++) {
        retval[i+n] = b[i];
    }    
    retval[n+m] = '\0';

    return retval;
}

char *path_str(char *path, char *sub)
{
    /* builds a path string with separating '/'                         */
    
    char *retval;

    if (path[strlen(path)-1] != '/') {
        retval = strccat(path, "/");
    } else {
        retval = path;
    }
    retval = strccat(retval, sub);
    
    return retval;
}

char *size_str(off_t bytes)
{
    /* generates human readable size information; we stick to the 'new'    *
     * IEC standard, see http://en.wikipedia.org/wiki/Binary_prefix        */
    
    char *retval, *unit, *buffer;
    double number;
    
    buffer = malloc(MAX_SIZE_L);

    if (bytes >= 1099511627776) {
        unit = "TiB";
        number = (double)bytes / 1099511627776;
    } else if (bytes >= 1073741824) {
        unit = "GiB";
        number = (double)bytes / 1073741824;
    } else if (bytes >= 1048576) {
        unit = "MiB";
        number = (double)bytes / 1048576;
    } else if (bytes >= 1024) {
        unit = "KiB";
        number = (double)bytes / 1024;
    } else {
        unit = "B";
        number = (double)bytes;
    }
    
    sprintf(buffer, "%.2f ", number);
    retval = strccat(buffer, unit);
    free(buffer);
    
    return retval;
}

char *bar_str(char percent)
{
    /* returns a progress bar as string                                    */

    #define BAR_STEP 100.0/(BAR_WIDTH-2)

    char *retval;
    double temp;
    short i;

    if ((retval = malloc(BAR_WIDTH+1)) == NULL) {
        return NULL;
    }
    retval[0] = '[';
    retval[BAR_WIDTH-1] = ']';
    retval[BAR_WIDTH] = '\0';

    i = 1;
    temp = BAR_STEP;
    while ((temp <= percent) && (i < BAR_WIDTH-1)) {
        retval[i] = '#';
        i++;
        temp += BAR_STEP;
    }
    while (i < BAR_WIDTH-1) {
        retval[i] = '-';
        i++;
    }

    return retval;
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
    printf("\rFile: %s %3d%% @ %s/s  Total: %s %3d%% ETA %02d:%02d:%02d   ",
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
    printf("\rFile: %3d%% of %s @ %s/s  Total: %3d%% of %s ETA %02d:%02d:%02d   ",
            perc_f, size_f, bps, perc_t, size_t, eta_h, eta_m, eta_s);
    
    return;
}
