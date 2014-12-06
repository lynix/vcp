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

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <time.h>           /* clock_gettime() */
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <libgen.h>         /* basename(), dirname() */
#include <unistd.h>         /* symlink(), and others */
#include <getopt.h>
#include <ctype.h>
#include <limits.h>         /* realpath()                               */
#include <stdlib.h>         /* realpath()                               */
#include <errno.h>          /* errno, strerror()                        */
#include <sys/ioctl.h>      /* ioctl(), get terminal width              */
#include <pthread.h>        /* threads, what else                       */

#include "lists.h"          /* my list implementations                  */
#include "helpers.h"        /* little helper functions                  */
#include "options.h"        /* global options, options struct           */

/* globals */
struct options  opts;
struct flist    *copy_list;
struct strlist  *fail_list;
char            file_done_flag;
pthread_mutex_t file_bytes_lock;
off_t           file_bytes_done;
off_t           speed_arr[SPEED_N];

/* functions */
void    init_opts();
void    fail_append(char *fname, char *error);
void    list_show();
void    *do_copy(void *p);
void    *progress(void *p);
int     build_list(int argc, int start, char *argv[]);
int     work_list();
int     parse_opts(int argc, char *argv[]);
int     crawl(char *src, char *dst);
int     ask_overwrite(struct file *old, struct file *new);
void    copy_file(struct file *item);
void    copy_dir(struct file *item);
void    copy_link(struct file *item);
off_t   speed_avg(off_t spd);
