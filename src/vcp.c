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
flist_t         *copy_list;
strlist_t       *fail_list;
char            file_done_flag;
pthread_mutex_t file_bytes_lock;
off_t           file_bytes_done;

/* functions */
void    *copy_thread(void *p);
void    *progress_thread(void *p);
int     build_list(int argc, int start, char *argv[]);
int     work_list();
int     crawl(char *src, char *dst);


int main(int argc, char *argv[])
{
    /* initialize options, set umask */
    init_opts(&opts);
    umask(0);
    
    if (argc < 3) {
        print_error("insufficient arguments. Try -h for help.");
        exit(EXIT_FAILURE);
    }
    
    /* parse command line options */
    int argstart = parse_opts(&opts, argc, argv);
    if (argstart < 0)
        exit(EXIT_FAILURE);

    /* parse argument files, build copy list */
    if (opts.debug) {
        puts("Collecting file information...");
        fflush(stdout);
    }
    if (build_list(argc, argstart, argv) != 0) {
        print_error("failed to build file list, aborting.");
        exit(EXIT_FAILURE);
    }

    /* evtl. print summary, exit if in pretend mode */
    if (opts.verbose || opts.pretend)
        flist_print(copy_list, &opts);
    if (opts.pretend) {
        flist_delete(copy_list);
        exit(EXIT_SUCCESS);
    }
    
    /* process copy list */
    if (work_list() != 0) {
        flist_delete(copy_list);
        exit(EXIT_FAILURE);
    }
    
    flist_delete(copy_list);
    exit(EXIT_SUCCESS);
}

int build_list(int argc, int start, char *argv[])
{
    int     argfcount;
    file_t  *f_item, *f_item2;
    char    *dest, *dest_dir, *dest_base, *item;
    
    /* clean destination path */
    dest_dir = malloc(strlen(argv[argc-1])+1);
    dest_base = malloc(strlen(argv[argc-1])+1);
    strcpy(dest_dir, argv[argc-1]);
    strcpy(dest_base, argv[argc-1]);
    dest_dir = dirname(dest_dir);
    dest_base = basename(dest_base);
    dest_dir = realpath(dest_dir, NULL);
    if ((dest_dir = realloc(dest_dir, strlen(dest_dir)+1)) == NULL) {
        return -1;
    }
    dest = path_str(dest_dir, dest_base);
    
    /* logic checking (do not copy file/dir over dir/file) */
    argfcount = argc-start-1;
    if ((f_item = f_new(dest)) != NULL) {
        if (f_item->type == RFILE) {
            if (argfcount != 1) {
                print_error("unable to copy multiple items to one file");
                return -1;
            }
        }
        f_delete(f_item);
    } else {
        errno = 0;
        if (argfcount > 1) {
            print_error("destination directory '%s' does not exist", 
                        dest);
            return -1;
        }
    }
    
    /* create copy list */
    if ((copy_list = flist_new()) == NULL) {
        print_debug("failed to create copy list");
        return -1;
    }
    
    /* iterate through cmdline and insert items */
    if ((argfcount == 1) && (f_item == NULL || f_item->type == RFILE)) {
        item = realpath(argv[start], NULL);
        item = realloc(item, strlen(item)+1);
        if (crawl(item, dest) != 0) {
            flist_delete(copy_list);
            return -1;
        }
    } else {
        for (int i=start; i<start+argfcount; i++) {
            /* clean item path */
            item = realpath(argv[i], NULL);
            item = realloc(item, strlen(item)+1);
            /* crawl item */
            if (crawl(item, path_str(dest, basename(item))) != 0) {
                flist_delete(copy_list);
                return -1;
            }
        }
    }

    /* update symlinks if linked file is to be copied */
    for (ulong i=0; i<copy_list->count; i++) {
        f_item = copy_list->items[i];
        if (f_item->type != SLINK) {
            continue;
        }
        if ((f_item2 = flist_search_src(copy_list, f_item)) != NULL) {
            f_item->src = f_item2->dst;
        }
    }
    
    /* check if something left to copy at all */
    if (copy_list->count == 0) {
        printf("vcp: no items to copy.\n");
        flist_delete(copy_list);
        exit(EXIT_SUCCESS);
    }

    /* finally sort list by destination */
    flist_shrink(copy_list);
    flist_sort(copy_list);
    
    return 0;
}

int crawl(char *src, char *dst)
{
    /* recursively scans directory and adds files to transfer list        */
    
    DIR    *src_dir;
    struct dirent *src_dirp;
    file_t *f_src;
    file_t *f_dst;
    
    /* check source access, prepare file struct */
    if ((f_src = f_new(src)) == NULL) {
        print_error("failed to open '%s': %s", src, strerror(errno));
        return -1;
    }
    f_src->dst = dst;

    /* collision handling */
    if ((f_dst = f_new(dst)) != NULL) {
        if (f_src->type != RDIR && f_dst->type != RDIR) {
            if (opts.keep) {
                f_src->done = 1;
            } else if (opts.update && f_equal(f_src, f_dst)) {
                f_src->done = 1;
            } else if (!opts.force) {
                if (!ask_overwrite(f_dst, f_src)) {
                    f_src->done = 1;
                }
            }
        } else if (f_src->type == RDIR && f_dst->type == RDIR) {
            f_src->done = 1;
        } else {
            print_error("%s: type mismatch (file/dir -> dir/file)",
                        dst);
        }
    }

    /* add to copy list */
    if (flist_add(copy_list, f_src) != 0) {
        print_error("unable to add to copy list");
        return -1;
    }

    /* crawl contents if src is a directory */
    if (f_src != NULL) {
        if (f_src->type == RDIR) {
            if ((src_dir = opendir(src)) == NULL) {
                print_error("failed to open directory '%s': %s", src,
                            strerror(errno));
                return -1;
            }
            while ((src_dirp = readdir(src_dir)) != NULL) {
                /* IMPORTANT: skip references to . and .. */
                if (strcmp(src_dirp->d_name, "..") == 0 || 
                    strcmp(src_dirp->d_name, ".") == 0) {
                    continue;
                }
                /* recursively crawl directory contents */
                if (crawl(path_str(src, src_dirp->d_name),
                        path_str(dst, src_dirp->d_name)) != 0) {
                    return -1;
                }
            }
            closedir(src_dir);
        }
    }

    return 0;
}

int work_list()
{
    /* works off the file list and copies each item                        */

    file_t *item;
    char *tempstr;

    /* initialize fail-list */
    if ((fail_list = strlist_new()) == NULL) {
        print_error("failed to create fail_list");
        return -1;
    }

    /* work off the list */
    for (ulong i=0; i < copy_list->count; i++) {
        item = copy_list->items[i];
        /* skip symlinks and existing items */
        if (item->done == 1 || item->type == SLINK) {
            continue;
        }
        if (opts.filenames) {
            printf("%s\n", item->src);
        }
        /* directories */
        if (item->type == RDIR) {
            copy_dir(item);
            continue;
        }
        /* files */
        if (item->type == RFILE) {
            copy_file(item);
            continue;
        }
    }

    /* create symlinks */
    for (ulong i=0; i < copy_list->count; i++) {
        item = copy_list->items[i];
        if (item->type == SLINK && item->done == 0) {
            copy_link(item);
        }
    }
    
    /* reverse re-iterate: update directory-attrs, evtl. delete items */
    for (ulong i=copy_list->count-1; i < copy_list->count; i--) {
        item = copy_list->items[i];
        if (item->done != 1) {
            print_debug("skipping failed item '%s'", item->fname);
            continue;
        }
        if (item->type == RDIR) {
            if (f_clone_attrs(item) != 0) {
                if (!opts.ignore_uid_err) {
                    fail_append(fail_list, item->dst,
                            "unable to set attributes");
                    item->done = 0;
                    continue;
                }
            }
        }
        if (opts.delete) {
            if (item->type == SLINK) {
                tempstr = item->dst;
            } else {
                tempstr = item->src;
            }
            if (remove(tempstr) != 0) {
                fail_append(fail_list, tempstr, "unable to delete");
                item->done = 0;
            }
        }
    }
    
    /* print list of failed items */
    if (fail_list->count > 0) {
        print_error("the following errors occured:");
        for (ulong i=0; i < fail_list->count; i++) {
            printf("   %s\n", fail_list->items[i]);
        }
        return -1;
    }
    
    return 0;
}

void *copy_thread(void *p)
{
    int   src, dst;
    char  *buffer;
    long  buffsize, bytes_r, bytes_w;
    file_t *item;

    item = (file_t *)p;
    
    /* open source (true, true ;)) */
    if ((src = open(item->src, O_RDONLY)) == -1) {
        fail_append(fail_list, item->src, "unable to open for reading");
        return NULL;
    }
    
    /* open destination */
    if ((dst = open(item->dst, O_CREAT|O_WRONLY|O_TRUNC, (mode_t)0600))
                        == -1) {
        fail_append(fail_list, item->dst, "unable to open for writing");
        close(src);
        return NULL;
    }
    
    /* set buffer size, allocate buffer */
    buffsize = (item->size > BUFFS) ? BUFFS : item->size;
    if ((buffer = malloc(buffsize)) == NULL) {
        print_error("failed to allocate %s for buffer",
                    size_str(buffsize));
        fail_append(fail_list, item->src, "failed to allocate buffer");
        close(src);
        close(dst);
        return NULL;
    }
    
    /* initialize counters */
    bytes_r = 0;
    bytes_w = 0;

    /* the part you are looking for: read from source... */
    while ((bytes_r = read(src, buffer, buffsize))) {
        if (bytes_r == -1) {
            fail_append(fail_list, item->src, "I/O error while reading");
            break;
        }
        /* ... and write to destination */
        bytes_w = write(dst, buffer, bytes_r);
        if (bytes_w != bytes_r) {
            fail_append(fail_list, item->src, "I/O error while writing");
            break;
        }
        /* account written data, thread-safe */
        pthread_mutex_lock(&file_bytes_lock);
        file_bytes_done += (off_t)bytes_w;
        pthread_mutex_unlock(&file_bytes_lock);
    }

    /* check result */
    if (file_bytes_done == item->size) {
        /* mark as done */
        item->done = 1;
        /* fsync if requested */
        if (opts.sync) {
            if (fsync(dst) != 0) {
                fail_append(fail_list, item->dst, "fsync failed");
                item->done = 0;
            }
        }
        
        /* clone attributes */
        if (f_clone_attrs(item) != 0) {
            if (!opts.ignore_uid_err) {
                fail_append(fail_list, item->dst, "failed to apply attributes");
                item->done = 0;
            }
        }
    } else {
        /* remove incomplete transfers */
        if (remove(item->dst) != 0) {
            fail_append(fail_list, item->dst, "failed to remove corrupt dest");
        }
    }
    
    /* clean up */
    free(buffer);
    close(dst);
    close(src);

    return NULL;
}

void *progress_thread(void *p)
{
    file_t *item = (file_t *)p;
    time_t start, now, elapsed;
    char perc_t, perc_f, *speed;
    off_t bytes_per_sec, bytes_written;
    int remaining_s;
    char eta_h, eta_m, eta_s;

    perc_t = (float)copy_list->bytes_done / copy_list->size * 100;
    bytes_written = 0;
    time(&start);

    /* initial output */
    if (copy_list->count_f > 1) {
        if (opts.bars) {
            print_progr_bm(0, perc_t, "0b", 0, 0, 0);
        } else {
            print_progr_pm(0, perc_t, size_str(item->size),
                           size_str(copy_list->size), "0b", 0, 0, 0);
        }
    } else {
        if (opts.bars) {
            print_progr_bs(0, "0b", 0, 0, 0);
        } else {
            print_progr_ps(0, size_str(item->size), "0b", 0, 0, 0);
        }
    }

    /* wait for worker thread to start copying */
    while (!file_done_flag) {
        sleep(1);
        pthread_mutex_lock(&file_bytes_lock);
        bytes_written = file_bytes_done;
        pthread_mutex_unlock(&file_bytes_lock);
        if (bytes_written > 0) {
            break;
        }
    }
    
    while (!file_done_flag) {
        /* get written bytes, calculate speed */
        pthread_mutex_lock(&file_bytes_lock);
        bytes_written = file_bytes_done;
        time(&now);
        pthread_mutex_unlock(&file_bytes_lock);
        elapsed = now - start;
        if (elapsed <= 0) {
            elapsed = 1;
        }
        bytes_per_sec = (float)bytes_written / elapsed;
        speed = size_str(bytes_per_sec);
        /* calculate percentage, ETA */
        perc_f = (float)bytes_written / item->size * 100;
        perc_t = (float)(copy_list->bytes_done + bytes_written) /
                 copy_list->size * 100;
        remaining_s = (copy_list->size - (copy_list->bytes_done +
                      bytes_written)) / bytes_per_sec;
        eta_s = remaining_s % 60;
        eta_m = (remaining_s % 3600) / 60;
        eta_h = remaining_s / 3600;
        if (eta_h > 99 || eta_h < 0) {
            eta_h = 99;
        }
        /* print beautiful progress information */
        if (copy_list->count_f > 1) {
            if (opts.bars) {
                print_progr_bm(perc_f, perc_t, speed, eta_s, eta_m, eta_h);
            } else {
                print_progr_pm(perc_f, perc_t, size_str(item->size),
                               size_str(copy_list->size), speed, eta_s, eta_m, eta_h);
            }
        } else {
            if (opts.bars) {
                print_progr_bs(perc_f, speed, eta_s, eta_m, eta_h);
            } else {
                print_progr_ps(perc_f, size_str(item->size), speed, eta_s, eta_m, eta_h);
            }
        }
        fflush(stdout);
        /* check termination */
        if (file_done_flag != 0) {
            break;
        }
        sleep(1);
    }

    //TODO: clear whole terminal line
    putchar('\r');
    putchar('\n');
    
    return NULL;
}
