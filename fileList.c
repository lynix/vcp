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
 * along with vcp.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fileList.h"

struct fileList *fl_create()
{
	struct fileList *list;
	
	/* create new list */
	if ((list = malloc(sizeof(struct fileList))) == NULL) {
		return NULL;
	}
	list->count = 0;
	list->head = NULL;
	list->pointer = NULL;
	
	return list;
}

struct file *fl_next(struct fileList *list)
{	
	struct file *retval;
	
	retval = list->pointer;
	if (list->pointer != NULL) {
		list->pointer = list->pointer->next;
	}
	
	return retval;
}

struct file *fl_prev(struct fileList *list)
{	
	struct file *retval;
	
	retval = list->pointer;
	if (list->pointer != NULL) {
		list->pointer = list->pointer->prev;
	}
	
	return retval;
}

int fl_add(struct fileList *list, struct file *item)
{
	
	struct file *pointer;
	long count;
	llong size;
	
	if (item == NULL) {
		return -1;
	}
	
	/* clean file links, if inherited from another list */
	item->prev = NULL;
	item->next = NULL;
	
	/* store old list dimensions, for overflow check */
	count = list->count;
	size = list->size;
	
	/* seek destination */
	pointer = list->head;
	if (pointer != NULL) {
		/* elements present, append */
		while (pointer->next != NULL) {
			pointer = pointer->next;
		}
		pointer->next = item;
		item->prev = pointer;
		list->size += item->size;
	} else {
		/* list empty, initialize */
		list->head = item;
		list->pointer = list->head;
		list->size = item->size;
	}
	(list->count)++;
	
	/* overflow check for list dimensions */
	if (list->size < size || list->count < count) {
		return -1;
	}
	
	return 0;
}

struct file *get_file(char *fname)
{
	struct file *new_item;
	struct stat filestat;
	
	/* check existance */
	if (access(fname, F_OK) != 0) {
		return NULL;
	}
	
	/* create new item */
	new_item = malloc(sizeof(struct file));
	if (new_item == NULL) {
		return NULL;
	}
	
	/* collect file properties */
	new_item->src = fname;
	new_item->filename = basename(fname);
	if (stat(fname, &filestat) != 0) {
		free(new_item);
		return NULL;
	}
	new_item->uid = filestat.st_uid;
	new_item->gid = filestat.st_gid;
	new_item->mode = filestat.st_mode;
	new_item->times.actime = filestat.st_atime;
	new_item->times.modtime = filestat.st_mtime;
	
	if (filestat.st_mode & S_IFREG) {
		/* regular file */
		new_item->type = RFILE;
		new_item->size = filestat.st_size;
	} else if (filestat.st_mode & S_IFDIR) {
		/* directory -> set zero-size, important for do_copy() */
		new_item->type = RDIR;
		new_item->size = 0;
	} else {
		/* unknown object, abort */
		free(new_item);
		return NULL;
	}
	new_item->next = NULL;
	new_item->prev = NULL;
	
	return new_item;
}

void fl_rewind(struct fileList *list)
{
	list->pointer = list->head;
	
	return;
}

void fl_forward(struct fileList *list)
{
	if (list->pointer == NULL) {
		return;
	}
	
	while (list->pointer->next != NULL) {
		list->pointer = list->pointer->next;
	}
	
	return;
}
