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

#ifndef _FILE_H
#define _FILE_H

#define _XOPEN_SOURCE 500

#include <sys/types.h>                  // uid_t, gid_t, etc.
#include <utime.h>                      // struct utimbuf

typedef enum { RFILE, RDIR, SLINK } ftype_t;

typedef struct {
    char    *fname;
    char    *src;
    char    *dst;
    char    *ldst;
    ftype_t type;
    off_t   size;
    uid_t   uid;
    gid_t   gid;
    mode_t  mode;
    struct  utimbuf times;
    char    done;
} file_t;


// create file_t struct from given source and destination paths
file_t *f_new(char *src, char *dst);

// delete given file_t struct
void    f_delete(file_t *file);

// compare given files regarding size, owner and timestamps
int     f_equal(file_t *a, file_t *b);

// transfer file attributes from source to destination on given item
int     f_clone_attrs(file_t *item);

// file_t comparator implementation for qsort()
int     f_cmpr_dst(const void *a, const void *b);

#endif
