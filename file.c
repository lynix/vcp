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

#include "file.h"

struct file *get_file(char *fname)
{
	struct file *new_item;
	struct stat64 filestat;
	
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
	if (stat64(fname, &filestat) != 0) {
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
		/* directory -> set zero-size */
		new_item->type = RDIR;
		new_item->size = 0;
	} else {
		/* unknown object, abort */
		free(new_item);
		return NULL;
	}
	
	return new_item;
}
