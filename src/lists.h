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

#define LIST_START 5L

#include "file.h"

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

flist_t *flist_init();
file_t  *flist_search_src(flist_t *list, file_t *item);
int     flist_add(flist_t *list, file_t *item);
int     flist_shrink(flist_t *list);
void    flist_sort_dst(flist_t *list);

typedef struct {
    ulong   count;
    ulong   arr_size;
    char    **items;
} strlist_t;

strlist_t   *strlist_init();
int         strlist_add(strlist_t *list, char *item);

#endif
