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

#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#include <stdlib.h>						/* realpath(), and others	*/
#include <sys/stat.h>					/* file attributes			*/
#include <libgen.h>						/* basename() 				*/
#include <sys/types.h>					/* uid_t, gid_t, etc		*/
#include <utime.h>						/* struct utimbuf			*/
#include <unistd.h>						/* F_OK						*/

#define S_IFDIR __S_IFDIR				/* regular directory		*/
#define S_IFREG	__S_IFREG				/* regular file				*/

enum ftype { RFILE, RDIR };
typedef unsigned long long ullong;

struct file {
	char	*filename;
	char	*src;
	char	*dst;
	enum	ftype type;
	ullong	size;
	uid_t	uid;
	gid_t	gid;
	mode_t	mode;
	struct	utimbuf	times;
};

struct file *get_file(char *fname);
