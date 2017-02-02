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
#include "helpers.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define INIT_SIZE 5L


flist_t *flist_new()
{
    flist_t *list = malloc(sizeof(flist_t));
    if (list == NULL)
        return NULL;

    list->items = calloc(INIT_SIZE, sizeof(file_t *));
    if (list->items == NULL) {
        free(list);
        return NULL;
    }

    list->count         = 0;
    list->count_f       = 0;
    list->size          = 0;
    list->bytes_done    = 0;
    list->arr_size      = INIT_SIZE;

    return list;
}

void flist_delete(flist_t *list)
{
    for (ulong i = 0; i < list->count; i++)
        f_delete(list->items[i]);

    free(list->items);
    free(list);
}

int flist_add(flist_t *list, file_t *file)
{
    if (list == NULL)
        return -1;

    /* expand if necessary (we grow with 2^x here) */
    if (list->count == list->arr_size) {
        ulong size_new = list->arr_size * 2L * sizeof(file_t *);
        list->items = realloc(list->items, size_new);
        if (list->items == NULL)
            return -1;
        list->arr_size *= 2L;
    }

    /* insert new item, update counters */
    list->items[list->count] = file;
    ulong temp_c = list->count;
    off_t temp_s = list->size;
    list->size += file->size;
    list->count++;
    if (list->count <= temp_c || list->size < temp_s) {
        /* overflow */
        list->count = temp_c;
        list->size = temp_s;
        list->items[list->count] = NULL;
        return -1;
    }

    if (file->type == RFILE)
        list->count_f++;

    return 0;
}

file_t *flist_search_src(flist_t *list, char *src)
{
    for (ulong i = 0; i < list->count; i++) {
        if (strcmp(list->items[i]->src, src) == 0 &&
                list->items[i]->type != SLINK) {
            return list->items[i];
        }
    }

    return NULL;
}

inline void flist_sort(flist_t *list)
{
    qsort(list->items, list->arr_size, sizeof(file_t *), f_cmpr_dst);
}

int flist_shrink(flist_t *list)
{
    list->items = realloc(list->items, list->count * sizeof(file_t *));
    if (list->items == NULL)
        return -1;

    list->arr_size = list->count;

    return 0;
}

void flist_print(flist_t *list, opts_t *opts)
{
    off_t size = 0;
    ulong count = 0;
    char mark = 0;

    for (ulong i = 0; i < list->count; i++) {

        file_t *item = list->items[i];
        if (item->done && !opts->delete)
            continue;

        if (item->type == RFILE) {
            printf(" [F] ");
            count++;
            size += item->size;
        } else if (item->type == RDIR) {
            printf(" [D] ");
        } else if (item->type == SLINK) {
            printf(" [S] ");
        }

        printf("%s --> %s", item->src, item->dst);

        if (item->done) {
            mark = 1;
            printf("(*)");
        } else if (item->type == RFILE) {
            printf(" (%s)", size_str(item->size));
        }

        putchar('\n');
    }

    putchar('\n');
    if (mark) {
        puts("(*) marked items already exist at their destination");
        puts("    and do not count for transfer size.\n");
    }
    printf("Total transfer size: %lu file(s), %s\n\n", count, size_str(size));
    fflush(stdout);

    return;
}

strlist_t *strlist_new()
{
    strlist_t *list = malloc(sizeof(strlist_t));
    if (list == NULL)
        return NULL;

    list->items = calloc(INIT_SIZE, sizeof(char *));
    if (list->items == NULL) {
        free(list);
        return NULL;
    }

    list->count     = 0;
    list->arr_size  = INIT_SIZE;

    return list;
}

void strlist_delete(strlist_t *list)
{
    for (ulong i = 0; i < list->count; i++)
        free(list->items[i]);

    free(list->items);
    free(list);
}

int strlist_add(strlist_t *list, char *item)
{
    if (list == NULL)
        return -1;

    /* expand if necessary (we grow with 2^x here) */
    if (list->count == list->arr_size) {
        ulong new_size = list->arr_size * 2L * sizeof(char *);
        list->items = realloc(list->items, new_size);
        if (list->items == NULL)
            return -1;
        list->arr_size *= 2L;
    }

    /* insert new item, update counter */
    list->items[list->count] = item;
    ulong temp = list->count++;
    if (list->count <= temp) {
        /* overflow */
        list->count--;
        return -1;
    }

    return 0;
}
