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

#include "lists.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


flist_t *flist_init() {
    flist_t *list;
    
    if ((list = malloc(sizeof(flist_t))) == NULL) {
        return NULL;
    }
    if ((list->items = calloc(LIST_START, sizeof(file_t *))) ==
            NULL) {
        free(list);
        return NULL;
    }
    list->count = 0;
    list->count_f = 0;
    list->size = 0;
    list->bytes_done = 0;
    list->arr_size = LIST_START;

    return list;
}

int flist_add(flist_t *list, file_t *item) {
    ulong  temp_c;
    off_t temp_s;
    
    if (list == NULL) {
        return -1;
    }
    
    /* check fill state, expand if necessary (we grow with 2^x here) */
    if (list->count == list->arr_size) {
        if ((list->items = realloc(list->items, list->arr_size * 2L * 
                sizeof(file_t *))) == NULL) {
            return -1;
        }
        list->arr_size *= 2L;
    }
    
    /* insert new item, update counters (with overflow check) */
    list->items[list->count] = item;
    temp_c = list->count;
    temp_s = list->size;
    list->size += item->size;
    list->count++;
    if (list->count <= temp_c || list->size < temp_s) {
        list->count = temp_c;
        list->size = temp_s;
        list->items[list->count] = NULL;
        return -1;
    }
    if (item->type == RFILE) {
        list->count_f++;
    }
    
    return 0;
}

file_t *flist_search_src(flist_t *list, file_t *item)
{
    for (ulong i=0; i < list->count; i++) {
        if (strcmp(list->items[i]->src, item->src) == 0 &&
                list->items[i]->type != SLINK) {
            return list->items[i];
        }
    }

    return NULL;
}

void flist_sort_dst(flist_t *list)
{
    if (flist_shrink(list) != 0) {
        return;
    }

    qsort(list->items, list->arr_size, sizeof(file_t *),
            f_cmpr_dst);

    return;
}

int flist_shrink(flist_t *list)
{
    if ((list->items = realloc(list->items, list->count *
            sizeof(file_t *))) == NULL) {
        return -1;
    }
    list->arr_size = list->count;

    return 0;
}

strlist_t *strlist_init() {
    strlist_t *list;
    
    if ((list = malloc(sizeof(strlist_t))) == NULL) {
        return NULL;
    }
    if ((list->items = calloc(LIST_START, sizeof(char *))) == NULL) {
        free(list);
        return NULL;
    }
    list->count = 0;
    list->arr_size = LIST_START;

    return list;
}

int strlist_add(strlist_t *list, char *item) {
    ulong temp;
    
    if (list == NULL) {
        return -1;
    }
    
    /* check fill state, expand if necessary (we grow with 2^x here) */
    if (list->count == list->arr_size) {
        if ((list->items = realloc(list->items, list->arr_size * 2L * 
                sizeof(char *))) == NULL) {
            return -1;
        }
        list->arr_size *= 2L;
    }
    
    /* insert new item, update counter (with overflow check) */
    list->items[list->count] = item;
    temp = list->count++;
    if (list->count <= temp) {
        list->count--;
        return -1;
    }
    
    return 0;
}
