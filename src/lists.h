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

#ifndef _LISTS_H
#define _LISTS_H

#include "file.h"
#include "options.h"

#include <sys/types.h>

typedef unsigned long ulong;

typedef struct {
    ulong   count;
    ulong   count_f;
    ulong   arr_size;
    off_t   size;
    off_t   bytes_done;
    file_t  **items;
} flist_t;

// create new file list
flist_t *flist_new();

// delete given file list
void    flist_delete(flist_t *list);

// find file item by given source path in given file list
file_t  *flist_search_src(flist_t *list, char *item);

// add given file item to given file list
int     flist_add(flist_t *list, file_t *item);

// shrink given file list structure allocation to required size
int     flist_shrink(flist_t *list);

// sort given file list (by destination path)
void    flist_sort(flist_t *list);

// print textual representation of given list to stdout
void    flist_print(flist_t *list, opts_t *opts);


typedef struct {
    ulong   count;
    ulong   arr_size;
    char    **items;
} strlist_t;

// create new string list
strlist_t   *strlist_new();

// delete given string list
void        strlist_delete(strlist_t *list);

// add given string to given string list
int         strlist_add(strlist_t *list, char *item);

#endif
