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

#include <stdlib.h>                     /* realpath(), and others   */
#include <sys/stat.h>                   /* file attributes          */
#include <libgen.h>                     /* basename()               */
#include <sys/types.h>                  /* uid_t, gid_t, etc.       */
#include <utime.h>                      /* struct utimbuf           */
#include <unistd.h>                     /* F_OK                     */
#include <string.h>                     /* strcmp()                 */
#include <errno.h>                      /* clear errno if !F_OK     */

#include "helpers.h"

enum ftype { RFILE, RDIR, SLINK };

struct file {
    char    *fname;
    char    *src;
    char    *dst;
    enum    ftype type;
    off_t   size;
    uid_t   uid;
    gid_t   gid;
    mode_t  mode;
    struct  utimbuf times;
    char    done;
};

struct  file *f_get(char *fname);
int     f_equal(struct file *a, struct file *b);
int     f_exists(char *fname);
int     f_clone_attrs(struct file *item);
int     f_cmpr_dst(const void *a, const void *b);
