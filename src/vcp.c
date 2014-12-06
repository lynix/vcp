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

#include "vcp.h"

int main(int argc, char *argv[])
{
    int argstart;
    
    /* initialize options, set umask */
    init_opts();
    umask(0);
    
    /* parse cmdline options */
    if ((argstart = parse_opts(argc, argv)) == -1) {
        exit(EXIT_FAILURE);
    }
    if (argc < 3) {
        print_error("insufficient arguments. Try -h for help.");
        exit(EXIT_FAILURE);
    }
    
    /* parse cmdline files, build list */
    if (opts.debug) {
        printf("Collecting file information...\n");
        fflush(stdout);
    }
    if (build_list(argc, argstart, argv) != 0) {
        print_error("failed to build file list, aborting.");
        exit(EXIT_FAILURE);
    }

    /* evtl. print summary, exit if in pretend mode */
    if (opts.verbose || opts.pretend) {
        list_show();
    }
    if (opts.pretend) {
        free(copy_list);
        exit(EXIT_SUCCESS);
    }
    
    /* work off the list */
    if (work_list() != 0) {
        free(copy_list);
        exit(EXIT_FAILURE);
    }
    
    free(copy_list);
    exit(EXIT_SUCCESS);
}

int build_list(int argc, int start, char *argv[])
{
    int     argfcount;
    struct  file *f_item, *f_item2;
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
    if ((f_item = f_get(dest)) != NULL) {
        if (f_item->type == RFILE) {
            if (argfcount != 1) {
                print_error("unable to copy multiple items to one file");
                return -1;
            }
        }
        free(f_item);
    } else {
        errno = 0;
        if (argfcount > 1) {
            print_error("destination directory '%s' does not exist", 
                        dest);
            return -1;
        }
    }
    
    /* create copy list */
    if ((copy_list = flist_init()) == NULL) {
        print_debug("failed to create copy list");
        return -1;
    }
    
    /* iterate through cmdline and insert items */
    if ((argfcount == 1) && (f_item == NULL || f_item->type == RFILE)) {
        item = realpath(argv[start], NULL);
        item = realloc(item, strlen(item)+1);
        if (crawl(item, dest) != 0) {
            free(copy_list);
            return -1;
        }
    } else {
        for (int i=start; i<start+argfcount; i++) {
            /* clean item path */
            item = realpath(argv[i], NULL);
            item = realloc(item, strlen(item)+1);
            /* crawl item */
            if (crawl(item, path_str(dest, basename(item))) != 0) {
                free(copy_list);
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
        free(copy_list);
        exit(EXIT_SUCCESS);
    }

    /* finally sort list by destination */
    flist_sort_dst(copy_list);
    
    return 0;
}

void list_show()
{
    struct file *item;
    off_t  size;
    ulong  count;
    char   mark;

    mark = 0;
    size = 0;
    count = 0;
    
    for (ulong i=0; i<copy_list->count; i++) {
        item = copy_list->items[i];
        if (item->done && !opts.delete) {
            continue;
        }
        if (item->type == RFILE) {
            printf(" [F] ");
            count++;
            size += item->size;
        } else if (item->type == RDIR) {
            printf(" [D] ");
        } else if (item->type == SLINK) {
            printf(" [S] ");
        }
        printf("%s --> %s", item->src, item->dst);
        if (item->done) {
            mark = 1;
            printf("(*)");
        } else if (item->type == RFILE) {
            printf(" (%s)", size_str(item->size));
        }
        putchar('\n');
    }
    putchar('\n');
    if (mark) {
        printf("(*) marked items already exist at their destination\n");
        printf("    and do not count for transfer size.\n\n");
    }
    printf("Total transfer size: %lu file(s), %s\n\n", count,
            size_str(size));
    fflush(stdout);

    return;
}

int crawl(char *src, char *dst)
{
    /* recursively scans directory and adds files to transfer list        */
    
    DIR    *src_dir;
    struct dirent *src_dirp;
    struct file *f_src;
    struct file *f_dst;
    
    /* check source access, prepare file struct */
    if ((f_src = f_get(src)) == NULL) {
        print_error("failed to open '%s': %s", src, strerror(errno));
        return -1;
    }
    f_src->dst = dst;

    /* collision handling */
    if ((f_dst = f_get(dst)) != NULL) {
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

    struct file *item;
    char *tempstr;

    /* initialize fail-list */
    if ((fail_list = strlist_init()) == NULL) {
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
                    fail_append(item->dst, "unable to set attributes");
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
                fail_append(tempstr, "unable to delete");
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

void *do_copy(void *p)
{
    int   src, dst;
    char  *buffer;
    long  buffsize, bytes_r, bytes_w;
    struct file *item;

    item = (struct file *)p;
    
    /* open source (true, true ;)) */
    if ((src = open(item->src, O_RDONLY)) == -1) {
        fail_append(item->src, "unable to open for reading");
        return NULL;
    }
    
    /* open destination */
    if ((dst = open(item->dst, O_CREAT|O_WRONLY|O_TRUNC, (mode_t)0600))
                        == -1) {
        fail_append(item->dst, "unable to open for writing");
        close(src);
        return NULL;
    }
    
    /* set buffer size, allocate buffer */
    buffsize = (item->size > BUFFS) ? BUFFS : item->size;
    if ((buffer = malloc(buffsize)) == NULL) {
        print_error("failed to allocate %s for buffer",
                    size_str(buffsize));
        fail_append(item->src, "failed to allocate buffer");
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
            fail_append(item->src, "I/O error while reading");
            break;
        }
        /* ... and write to destination */
        bytes_w = write(dst, buffer, bytes_r);
        if (bytes_w != bytes_r) {
            fail_append(item->src, "I/O error while writing");
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
                fail_append(item->dst, "fsync failed");
                item->done = 0;
            }
        }
        
        /* clone attributes */
        if (f_clone_attrs(item) != 0) {
            if (!opts.ignore_uid_err) {
                fail_append(item->dst, "failed to apply attributes");
                item->done = 0;
            }
        }
    } else {
        /* remove incomplete transfers */
        if (remove(item->dst) != 0) {
            fail_append(item->dst, "failed to remove corrupt dest");
        }
    }
    
    /* clean up */
    free(buffer);
    close(dst);
    close(src);

    return NULL;
}

void *progress(void *p)
{
    struct file *item;
    time_t start, now, elapsed;
    char perc_t, perc_f, *speed;
    off_t bytes_per_sec, bytes_written;
    int remaining_s;
    char eta_h, eta_m, eta_s;

    item = (struct file *)p;
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

int ask_overwrite(struct file *old, struct file *new)
{
    /* prints a confirmation dialog for file overwriting                */
    
    char answer;
    
    do {
        printf("overwrite ");
        if (old->type != SLINK) {
            printf("%s (%s)", old->src, size_str(old->size));
        } else {
            printf("%s (symlink to %s)", old->dst, old->src);
        }
        printf(" with ");
        if (new->type != SLINK) {
            printf("%s (%s)", new->src, size_str(new->size));
        } else {
            printf(" symlink to %s", new->src);
        }
        printf(" (Y/n)? ");
        fflush(stdout);
        
        answer = getchar();
        if (answer != '\n') {
            while (getchar() != '\n') {
                /* ignore additional chars */
            }
        }
    } while (answer != 'Y' && answer != 'y' && answer != 'n' &&
                answer != '\n');
    
    if (answer == '\n' || answer == 'Y' || answer == 'y') {
        return 1;
    }
        
    return 0;
}

void init_opts()
{
    /* initializes the global options structure                            */
    
    opts.bars = 0;
    opts.force = 0;
    opts.filenames = 1;
    opts.sync = 0;
    opts.delete = 0;
    opts.keep = 0;
    opts.quiet = 0;
    opts.verbose = 0;
    opts.update = 0;
    opts.pretend = 0;
    opts.debug = 0;
    opts.ignore_uid_err = 0;

    return;
}

int parse_opts(int argc, char *argv[])
{
    /* parses command-line options and update opts structure            */
    
    char c;
    extern int optind, optopt, opterr;
    
    opterr = 0;
    
    while ((c = getopt(argc, argv, "bdfhkpqstuvDBQ")) != -1) {
        switch (c) {
            case 'b':
                opts.bars = 1;
                break;
            case 'B':
                opts.bars = 1;
                opts.filenames = 0;
                break;
            case 'f':
                if (opts.keep == 0) {
                    opts.force = 1;
                } else {
                    print_error("no -f and -k at the same time\n");
                    return -1;
                }
                break;
            case 'k':
                if (opts.force == 0) {
                    opts.keep = 1;
                } else {
                    print_error("no -f and -k at the same time\n");
                    return -1;
                }
                break;
            case 's':
                opts.sync = 1;
                break;
            case 't':
                opts.ignore_uid_err = 1;
                break;
            case 'p':
                opts.pretend = 1;
                break;
            case 'q':
                opts.quiet = 1;
                break;
            case 'Q':
                opts.quiet = 1;
                opts.filenames = 0;
                break;
            case 'd':
                opts.delete = 1;
                break;
            case 'h':
                print_usage();
                exit(EXIT_SUCCESS);
            case 'u':
                opts.update = 1;
                break;
            case 'v':
                opts.verbose = 1;
                break;
            case 'D':
                opts.debug = 1;
                break;
            case '?':
                if (isprint(optopt)) {
                    print_error("unknown option \"-%c\".\nTry -h for help.",
                                optopt);
                } else {
                    print_error("unknown option character \"\\x%x\".\nTry -h for help.",
                                optopt);
                }
                return -1;
            default:
                return -1;
        }
    }
          
    return optind;
}

off_t speed_avg(off_t spd)
{
    /* takes new current speed, adds it to the global array and            *
     * calculates the arithmetic average                                */
    
    /*ulong sum=0;
    
    for (int i=0; i<SPEED_N-1; i++) {
        speeds[i] = speeds[i+1];
        sum += speeds[i];
    }
    speeds[SPEED_N-1] = spd;
    sum += spd;
    
    return sum / SPEED_N;*/
    return 0;
}

void fail_append(char *fname, char *error)
{
    char *errmsg;

    errmsg = strccat(fname, ": ");
    errmsg = strccat(errmsg, error);
    if (errno != 0) {
        errmsg = strccat(errmsg, " (");
        errmsg = strccat(errmsg, strerror(errno));
        errmsg = strccat(errmsg, ")");
    }
    
    if (strlist_add(fail_list, errmsg) != 0) {
        print_debug("failed to add to fail-list:");
        print_error(errmsg);
    }

    return;
}

void copy_dir(struct file *item)
{
    if (mkdir(item->dst, item->mode) != 0) {
        fail_append(item->dst, "unable to create directory");
        item->done = 0;
    } else {
        item->done = 1;
    }

    return;
}

void copy_file(struct file *item)
{
    pthread_t copy_thread, progr_thread;
    char join_progr;

    /* initialize mutex */
    if (pthread_mutex_init(&file_bytes_lock, NULL) != 0) {
        fail_append(item->dst, "failed to initialize bytes_done mutex");
        return;
    }

    file_bytes_done = 0;
    file_done_flag = 0;
    join_progr = 0;
    
    if (pthread_create(&copy_thread, NULL, do_copy, item) != 0) {
        fail_append(item->dst, "unable to spawn copy-thread");
        return;
    }
    if (!opts.quiet && item->size > BUFFS*BUFFM) {
        if (pthread_create(&progr_thread, NULL, progress, item) != 0) {
            print_error("unable to spawn progress-thread (quiet copy)");
        } else {
            join_progr = 1;
        }
    }
    pthread_join(copy_thread, NULL);
    if (join_progr) {
        /* signal progress thread */
        file_done_flag = 1;
        pthread_join(progr_thread, NULL);
    }
    /* account global progress */
    copy_list->bytes_done += item->size;

    /* clean up */
    pthread_mutex_destroy(&file_bytes_lock);
}

void copy_link(struct file *item)
{
    /* remove evtl. existing one */
    if (f_exists(item->dst)) {
        if (remove(item->dst) != 0) {
            fail_append(item->dst, "unable to delete link");
        }
    }
    /* create new link */
    if (symlink(item->src, item->dst) != 0) {
        fail_append(item->dst, "unable to create symlink");
        item->done = 0;
    } else {
        item->done = 1;
    }
}
