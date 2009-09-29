/* Copyright lynix <lynix47@gmail.com>, 2009
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

struct flist *flist_init() {
	struct flist *list;
	
	if ((list = malloc(sizeof(struct flist))) == NULL) {
		return NULL;
	}
	if ((list->items = calloc(LIST_START, sizeof(struct file *))) == 
			NULL) {
		free(list);
		return NULL;
	}
	list->count = 0;
	list->size = 0L;
	list->arr_size = LIST_START;

	return list;
}

int	flist_add(struct flist *list, struct file *item) {
	long temp_s, temp_c;
	
	if (list == NULL) {
		return -1;
	}
	
	/* check fill state, expand if necessary (we grow with 2^x here) */
	if (list->count == list->arr_size) {
		if ((list->items = realloc(list->items, list->arr_size * 2L * 
				sizeof(struct file *)))	== NULL) {
			return -1;
		}
		list->arr_size *= 2L;
	}
	
	/* insert new item, update counters (with overflow check) */
	list->items[list->count] = item;
	temp_c = list->count++;
	temp_s = list->size;
	list->size += item->size;
	if (list->count < temp_c || list->size < temp_s) {
		list->count--;
		list->size -= item->size;
		return -1;
	}
	
	return 0;
}

struct strlist *strlist_init() {
	struct strlist *list;
	
	if ((list = malloc(sizeof(struct strlist))) == NULL) {
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

int	strlist_add(struct strlist *list, char *item) {
	long temp;
	
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
	if (list->count < temp) {
		list->count--;
		return -1;
	}
	
	return 0;
}
