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

#define _POSIX_SOURCE		/* fsync() */

#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <libgen.h>			/* basename(), dirname() */
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>			/* realpath() 								*/
#include <stdlib.h>			/* realpath() 								*/
#include <errno.h>			/* errno, strerror() 						*/
#include <sys/ioctl.h>		/* ioctl(), get terminal width 				*/

#include "lists.h"			/* my list implementations 					*/

#define BUFFS 1048576		/* 1MiB buffer for read() and write() 		*/
#define MAX_SIZE_L 30		/* maximum length of size string, numbers 	*/
#define SPEED_N 5			/* speed middle calculation 				*/
#define BAR_WIDTH 20		/* progress bar width (characters) 			*/

struct options {
	int bars;
	int force;
	int sync;
	int delete;
	int keep;
	int quiet;
	int update;
	int verbose;
	int debug;
};

struct	options opts;
struct	flist *file_list;
struct	flist *dir_list;
ulong	speeds[SPEED_N];

# if defined __STRICT_ANSI__
	char 	*realpath (const char *, char *);
	int 	fsync(int);
# endif

void 	print_usage();
void 	init_opts();
void 	print_error(char *string, ...);
void	print_debug(char *string, ...);
void	print_limits();
void	progress(double t_perc, ulong t_num, double perc, ulong bps,
				long eta, ullong fsize);
void 	error_append(struct strlist *list, char *fname, char *error,
					 char * reason);
int		build_list(int argc, int start, char *argv[]);
int 	copy_file(struct file *item, ulong total_filenum, 
				 ullong total_size, ullong total_done, time_t total_start,
				 struct strlist *failed);
int 	parse_opts(int argc, char *argv[]);
int 	crawl_files(char *item, char *dest);
int 	do_copy();
int 	clone_attrs(struct file *src);
int 	f_equal(struct file *a, struct file *b);
char 	*strccat(char *a, char *b);
char 	*path_str(char *path, char *sub);
char 	ask_overwrite(char *src, char *src_size, char *dst, char *dst_size);
char 	*size_str(ullong bytes);
char	*prog_bar(double percent);
ulong 	speed(ulong spd);

