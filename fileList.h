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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * A simple implementation of a double-linked list of files with common
 * attributes and a destination field. This is neither complete nor
 * elegant, it just fits my needs. 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdlib.h>			/* realpath(), and others	*/
#include <sys/stat.h>		/* file stats 				*/
#include <libgen.h>			/* basename() 				*/
#include <sys/types.h>		/* uid_t, gid_t, etc		*/
#include <utime.h>			/* struct utimbuf			*/
#include <unistd.h>			/* F_OK						*/

#define S_IFDIR __S_IFDIR	/* regular directory		*/
#define S_IFREG	__S_IFREG	/* regular file				*/

typedef long long 		llong;
typedef unsigned long 	ulong;

enum ftype { RFILE, RDIR };

struct file {
	char	*filename;
	char	*src;
	char	*dst;
	enum	ftype type;
	llong	size;
	uid_t	uid;
	gid_t	gid;
	mode_t	mode;
	struct	utimbuf	times;
	
	struct file	*prev;
	struct file	*next;
};

struct fileList {
	long 	count;
	llong 	size;
	struct	file *head;
	struct	file *pointer;
};

struct 	file *get_file(char *fname);
struct 	file *fl_next(struct fileList *list);
struct 	file *fl_prev(struct fileList *list);
struct 	fileList *fl_create();
int		fl_add(struct fileList *list, struct file *item);
void 	fl_rewind(struct fileList *list);
void 	fl_forward(struct fileList *list);
