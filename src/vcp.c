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
#include <limits.h>         /* realpath()                               */
#include <stdlib.h>         /* realpath()                               */
#include <errno.h>          /* errno, strerror()                        */
#include <sys/ioctl.h>      /* ioctl(), get terminal width              */
#include <pthread.h>        /* threads, what else                       */
#include <sys/stat.h>
#include <string.h>

#include "file.h"
#include "lists.h"          /* my list implementations                  */
#include "helpers.h"        /* little helper functions                  */
#include "options.h"        /* global options, options struct           */
#include "copy.h"

/* globals */
opts_t          opts;

/* functions */
flist_t *build_list(int argc, int start, char *argv[]);
int     work_list(flist_t *list);
int     crawl(flist_t *file_list, char *src, char *dst);


int main(int argc, char *argv[])
{
    /* initialize options, set umask */
    init_opts(&opts);
    umask(0);
    
    /* parse command line options */
    int argstart = parse_opts(&opts, argc, argv);
    if (argstart < 0)
        exit(EXIT_FAILURE);

    if (argc < 3) {
        print_error("insufficient arguments. Try -h for help.");
        exit(EXIT_FAILURE);
    }

    /* parse argument files, build copy list */
    if (opts.debug) {
        puts("Collecting file information...");
        fflush(stdout);
    }
    flist_t *copy_list = build_list(argc, argstart, argv);
    if (copy_list == NULL) {
        print_error("failed to build file list, aborting.");
        exit(EXIT_FAILURE);
    }
    /* check if something left to copy at all */
    if (copy_list->count == 0) {
        printf("vcp: no items to copy.\n");
        flist_delete(copy_list);
        exit(EXIT_SUCCESS);
    }

    /* evtl. print summary, exit if in pretend mode */
    if (opts.verbose || opts.pretend)
        flist_print(copy_list, &opts);
    if (opts.pretend) {
        flist_delete(copy_list);
        exit(EXIT_SUCCESS);
    }
    
    /* process copy list */
    if (work_list(copy_list) != 0) {
        flist_delete(copy_list);
        exit(EXIT_FAILURE);
    }
    
    flist_delete(copy_list);
    exit(EXIT_SUCCESS);
}

flist_t *build_list(int argc, int start, char *argv[])
{
    /* clean destination path */
    char *dest = clean_path(argv[argc-1]);

    /* logic checking (do not copy file/dir over dir/file) */
    int num_src = argc - start - 1;
    struct stat dest_stat;
    dest_stat.st_mode = 0;
    if (stat(dest, &dest_stat) == 0) {
        if (!S_ISDIR(dest_stat.st_mode) && num_src > 1) {
            print_error("unable to copy multiple items to one file");
            return NULL;
        }
    } else {
        errno = 0;
        if (num_src > 1) {
            print_error("destination directory '%s' does not exist", dest);
            return NULL;
        }
    }

    /* create copy list */
    flist_t *file_list = flist_new();
    if (file_list == NULL) {
        print_debug("failed to create copy list");
        return NULL;
    }
    
    /* iterate over command line and crawl items, recursively */
    for (int i=start; i<start+num_src; i++) {
        char *src = argv[i];
        char *new_dest = dest;
        if (S_ISDIR(dest_stat.st_mode))
            new_dest = path_str(new_dest, path_base(src));

        if (crawl(file_list, src, new_dest) != 0) {
            flist_delete(file_list);
            free(dest);
            return NULL;
        }

        if (S_ISDIR(dest_stat.st_mode))
            free(new_dest);
    }

    /* shrink and sort list by destination */
    flist_shrink(file_list);
    if (file_list->count > 0)
        flist_sort(file_list);
    
    free(dest);

    return file_list;
}

int crawl(flist_t *file_list, char *src, char *dst)
{
    /* check source access, prepare file struct */
    file_t *f_src = f_new(src, dst);
    if (f_src == NULL) {
        print_error("failed to open '%s': %s", src, strerror(errno));
        return -1;
    }

    /* collision handling */
    if (access(dst, F_OK) == 0) {
        file_t *f_dst = f_new(dst, dst);
        if (f_dst == NULL) {
            print_error("failed to read '%s': %s", dst, strerror(errno));
            f_delete(f_src);
            f_delete(f_dst);
            return -1;
        }

        if ((f_src->type == RDIR) != (f_dst->type == RDIR)) {
            print_error("type mismatch while trying to replace file with directory or vice versa: '%s', '%s'",
                    src, dst);
            f_delete(f_src);
            f_delete(f_dst);
            return -1;
        }

        if (opts.keep || (opts.update && f_equal(f_src, f_dst)))
            f_src->done = 1;
        else if (!opts.force && !ask_overwrite(f_dst, f_src))
            f_src->done = 1;

        f_delete(f_dst);
    }

    /* add to copy list */
    if (!f_src->done && (flist_add(file_list, f_src) != 0)) {
        print_error("unable to add to copy list");
        f_delete(f_src);
        return -1;
    }

    /* advance to recursion only if src is a directory */
    if (f_src->type != RDIR)
        return 0;

    DIR *src_dir = opendir(src);
    if (src_dir == NULL) {
        print_error("failed to open directory '%s': %s", src, strerror(errno));
        return -1;
    }
    struct dirent *src_dirp;
    while ((src_dirp = readdir(src_dir)) != NULL) {
        /* IMPORTANT: skip '.' and '..' */
        char *n = src_dirp->d_name;
        if (n[0] == '.' && (n[1] == '\0' || (n[1] == '.' && n[2] == '\0')))
            continue;

        /* recursively crawl directory contents */
        char *sub_src = path_str(src, src_dirp->d_name);
        char *sub_dst = path_str(dst, src_dirp->d_name);
        if (crawl(file_list, sub_src, sub_dst) != 0) {
            free(sub_src);
            free(sub_dst);
            closedir(src_dir);
            return -1;
        }
        free(sub_src);
        free(sub_dst);
    }
    closedir(src_dir);

    return 0;
}

int work_list(flist_t *list)
{
    /* initialize fail-list */
    strlist_t *fail_list = strlist_new();
    if (fail_list == NULL) {
        print_error("failed to create fail_list");
        return -1;
    }

    /* allocate I/O buffer */
    char *buffer = malloc(BUFFS);
    if (buffer == NULL) {
        print_error("failed to allocate I/O buffer");
        strlist_delete(fail_list);
        return -1;
    }

    /* work off the list */
    for (ulong i=0; i<list->count; i++) {
        file_t *item = list->items[i];

        /* skip processed items */
        if (item->done == 1)
            continue;

        if (opts.verbose)
            printf("%s\n", item->src);

        if (item->type == RDIR) {
            if (copy_dir(item, &opts, fail_list) == 0)
                item->done = 1;
        } else if (item->type == SLINK) {
            if (copy_link(item, fail_list) == 0)
                item->done = 1;
        } else if (item->type == RFILE) {
            if (copy_file(item, list, fail_list, &opts, buffer, BUFFS) == 0) {
                item->done = 1;
                list->bytes_done += item->size;
            }
        }
    }

    /* clear I/O buffer */
    free(buffer);

    /* re-iterate: update directory attributes, delete items if requested */
    for (ulong i=list->count-1; i < list->count; i--) {
        file_t *item = list->items[i];

        if (item->done != 1) {
            print_debug("skipping failed item '%s'", item->fname);
            continue;
        }

        if (item->type == RDIR) {
            if (f_clone_attrs(item) != 0 && !opts.ignore_uid_err) {
                fail_append(fail_list, item->dst, "unable to set attributes");
                item->done = 0;
                continue;
            }
        }

        if (opts.delete && (remove(item->src) != 0)) {
            fail_append(fail_list, item->src, "failed to delete");
            item->done = 0;
        }
    }
    
    /* print list of failed items */
    if (fail_list->count > 0) {
        print_error("the following errors occured:");
        for (ulong i=0; i < fail_list->count; i++) {
            printf("   %s\n", fail_list->items[i]);
        }
        strlist_delete(fail_list);
        return -1;
    }
    
    strlist_delete(fail_list);

    return 0;
}
