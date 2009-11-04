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

#include <string.h>
#include "file.h"

#define LIST_START 5L

typedef unsigned long ulong;

struct flist {
	ulong	count;
	ulong	arr_size;
	ullong	size;
	struct file **items;
};

struct	flist *flist_init();
int		flist_add(struct flist *list, struct file *item);
int		flist_search(struct flist *list, char *dest_str);


struct strlist {
	ulong	count;
	ulong 	arr_size;
	char	**items;
};

struct	strlist *strlist_init();
int		strlist_add(struct strlist *list, char *item);
