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

#include "file.h"
#include "helpers.h"

#include <stdlib.h>                     /* realpath(), and others   */
#include <sys/stat.h>                   /* file attributes          */
#include <libgen.h>                     /* basename()               */
#include <unistd.h>                     /* F_OK                     */
#include <string.h>                     /* strcmp()                 */
#include <errno.h>                      /* clear errno if !F_OK     */


file_t *f_new(char *fname)
{
    if (access(fname, F_OK) != 0)
        return NULL;
    
    /* determine file type, collect attributes */
    struct stat fstat;
    if (lstat(fname, &fstat) != 0)
        return NULL;

    /* create file_t object */
    file_t *f_item = malloc(sizeof(file_t));
    if (f_item == NULL)
        return NULL;

    /* fill type-independent fields */
    f_item->dst     = NULL;
    f_item->size    = 0;
    f_item->uid     = 0;
    f_item->gid     = 0;
    f_item->mode    = 0;
    f_item->done    = 0;
    f_item->src     = fname;
    f_item->fname   = basename(fname);

    /* fill type-dependent fields */
    if (S_ISLNK(fstat.st_mode)) {
        /* symlink */
        f_item->type    = SLINK;
        f_item->dst     = f_item->src;
        f_item->src     = malloc(fstat.st_size + 1);
        if (readlink(fname, f_item->src, fstat.st_size) != fstat.st_size) {
            free(f_item);
            return NULL;
        }
        f_item->src[fstat.st_size] = '\0';
    } else {
        /* POSIX: uid, modes etc. not defined by lstat(), use stat() */
        if (stat(fname, &fstat) != 0) {
            free(f_item);
            return NULL;
        }
        f_item->uid             = fstat.st_uid;
        f_item->gid             = fstat.st_gid;
        f_item->mode            = fstat.st_mode;
        f_item->times.actime    = fstat.st_atime;
        f_item->times.modtime   = fstat.st_mtime;
        if (S_ISREG(fstat.st_mode)) {
            /* regular file */
            f_item->type = RFILE;
            f_item->size = fstat.st_size;
        } else if (S_ISDIR(fstat.st_mode)) {
            /* directory */
            f_item->type = RDIR;
        } else {
            /* unknown object, abort */
            free(f_item);
            return NULL;
        }
    }
    
    return f_item;
}

void f_delete(file_t *file)
{
    if (file->type == SLINK)
        free(file->src);

    free(file);
}

int f_equal(file_t *a, file_t *b)
{
    /* compare file type */
    if (a->type != b->type)
        return 0;

    /* compare size */
    if (a->size != b->size)
        return 0;

    /* compare owner */
    if (a->uid != b->uid || a->gid != b->gid)
        return 0;

    /* compare modification times */
    if (a->times.modtime != b->times.modtime)
        return 0;

    /* if symlink: compare destination */
    if (a->type == SLINK) {
        if (strcmp(a->src, b->src) != 0)
            return 0;
    }
    
    return 1;
}

int f_clone_attrs(file_t *item)
{
    int retval = 0;
    
    /* set owner uid/gid */
    if (chown(item->dst, item->uid, item->gid) != 0) {
        print_debug("failed to set uid/gid");
        retval = -1;
    }

    /* set mode */
    if (chmod(item->dst, item->mode) != 0) {
        print_debug("failed to set mode");
        retval = -1;
    }

    /* set atime/mtime */
    if (utime(item->dst, &(item->times)) != 0) {
        print_debug("failed to set atime/mtime");
        retval = -1;
    }
    
    return retval;
}

int f_cmpr_dst(const void *a, const void *b)
{
    file_t *file_a = (file_t *)a;
    file_t *file_b = (file_t *)b;
    
    return strcmp(file_a->dst, file_b->dst);
}
