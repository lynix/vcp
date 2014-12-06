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


file_t *f_get(char *fname)
{
    file_t *new_item;
    struct stat filestat;
    
    /* check existance */
    if (!f_exists(fname)) {
        return NULL;
    }
    
    /* create new item */
    new_item = malloc(sizeof(file_t));
    if (new_item == NULL) {
        return NULL;
    }
    new_item->dst = NULL;
    new_item->size = 0;
    new_item->uid = 0;
    new_item->gid = 0;
    new_item->mode = 0;
    new_item->done = 0;
    
    /* set source names */
    new_item->src = fname;
    new_item->fname = basename(fname);

    /* determine file type, collect attributes */
    if (lstat(fname, &filestat) != 0) {
        free(new_item);
        return NULL;
    }
    if (S_ISLNK(filestat.st_mode)) {
        /* symlink */
        new_item->type = SLINK;
        new_item->dst = new_item->src;
        new_item->src = malloc(filestat.st_size + 1);
        if (readlink(fname, new_item->src, filestat.st_size) !=
                        filestat.st_size) {
            free(new_item);
            return NULL;
        }
        new_item->src[filestat.st_size] = '\0';
    } else {
        /* POSIX: uid, modes etc. not defined by lstat(), use stat()    */
        if (stat(fname, &filestat) != 0) {
            free(new_item);
            return NULL;
        }
        new_item->uid = filestat.st_uid;
        new_item->gid = filestat.st_gid;
        new_item->mode = filestat.st_mode;
        new_item->times.actime = filestat.st_atime;
        new_item->times.modtime = filestat.st_mtime;
        if (S_ISREG(filestat.st_mode)) {
            /* regular file */
            new_item->type = RFILE;
            new_item->size = filestat.st_size;
        } else if (S_ISDIR(filestat.st_mode)) {
            /* directory */
            new_item->type = RDIR;
        } else {
            /* unknown object, abort */
            free(new_item);
            return NULL;
        }
    }
    
    return new_item;
}

int f_equal(file_t *a, file_t *b)
{
    /* compares two given files regarding their size, owner and times   */

    /* compare filetype */
    if (a->type != b->type) {
        return 0;
    }
    /* compare filesize */
    if (a->size != b->size) {
        return 0;
    }
    /* compare owner */
    if (a->uid != b->uid || a->gid != b->gid) {
        return 0;
    }
    /* compare modification times */
    if (a->times.modtime != b->times.modtime) {
        return 0;
    }
    /* compare link destination */
    if (a->type == SLINK) {
        if (strcmp(a->src, b->src) != 0) {
            return 0;
        }
    }
    
    return 1;
}

int f_clone_attrs(file_t *item)
{
    /* applies the attributes of a given file to another                */
    
    int retval=0;
    
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

int f_exists(char *fname)
{
    if (access(fname, F_OK) != 0) {
        errno = 0;
        return 0;
    }
    
    return 1;
}

int f_cmpr_dst(const void *a, const void *b)
{
    char *str_a;
    char *str_b;

    str_a = (*((file_t **)a))->dst;
    str_b = (*((file_t **)b))->dst;
    
    return strcmp(str_a, str_b);
}
