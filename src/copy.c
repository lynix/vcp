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

#include "copy.h"
#include "helpers.h"

#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h>

/* inter-thread globals */
char            progress_alive;
pthread_mutex_t progress_lock;
size_t          progress_bytes;
opts_t          *options;
flist_t         *file_list;


int copy_file(file_t *file, flist_t *flist, strlist_t *fail_list, opts_t *opts,
        char *buffer, unsigned int buff_size)
{
    /* open files */
    FILE *src = fopen(file->src, "r");
    FILE *dst = fopen(file->dst, "w");
    if (src == NULL) {
        fail_append(fail_list, file->src, "unable to open for reading");
        return -1;
    }
    if (dst == NULL) {
        fail_append(fail_list, file->dst, "unable to open for writing");
        return -1;
    }

    /* initialize stats, spawn progress thread if reasonable */
    size_t n_read, n_writt;
    pthread_t prg_thread = NULL;
    progress_alive = 0;
    if (!opts->quiet && file->size > BUFFS*BUFFM) {
        if (pthread_mutex_init(&progress_lock, NULL) != 0) {
            fail_append(fail_list, file->dst, "failed to initialize mutex");
            return -1;
        }
        progress_alive = 1;
        progress_bytes = 0;
        file_list = flist;
        options = opts;
        if (pthread_create(&prg_thread, NULL, progress_thread, file) != 0) {
            print_error("failed to spawn progress thread, doing silent copy");
            progress_alive = 0;
        }
    }

    /* perform actual file I/O */
    do {
        n_read  = fread(buffer, 1, buff_size, src);
        n_writt = fwrite(buffer, 1, n_read, dst);
        if (progress_alive) {
            pthread_mutex_lock(&progress_lock);
                progress_bytes += n_read;
            pthread_mutex_unlock(&progress_lock);
        }
    } while (n_read > 0 && n_writt == n_read);

    if (progress_alive) {
        progress_alive = 0;
        pthread_join(prg_thread, NULL);
        pthread_mutex_destroy(&progress_lock);
    }

    /* error handling */
    if (ferror(src) || ferror(dst)) {
        fail_append(fail_list, ferror(src) ? file->src : file->dst,
                ferror(src) ? "I/O error while reading" :
                        "unable to open for writing");
        fclose(src);
        fclose(dst);
        if (remove(file->dst) != 0)
            fail_append(fail_list, file->dst, "failed to remove partial file");
        return -1;
    }

    /* fsync if requested */
    if (opts->sync) {
        if (fsync(fileno(dst)) != 0) {
            fail_append(fail_list, file->dst, "failed to fsync() file to disk");
            fclose(src);
            fclose(dst);
            return -1;
        }
    }

    fclose(src);
    fclose(dst);

    /* clone attributes */
    if (f_clone_attrs(file) && !opts->ignore_uid_err) {
        fail_append(fail_list, file->dst, "failed to apply attributes");
        return -1;
    }

    return 0;
}

int copy_link(file_t *file, strlist_t *fail_list)
{
    /* remove evtl. existing one */
    if ((access(file->dst, F_OK) == 0) && (remove(file->dst) != 0)) {
        fail_append(fail_list, file->dst, "unable to delete link");
        return -1;
    }

    /* create new link */
    if (symlink(file->ldst, file->dst) != 0) {
        fail_append(fail_list, file->dst, "unable to create symlink");
        return -1;
    }

    return 0;
}

int copy_dir(file_t *file, opts_t *opts, strlist_t *fail_list)
{
    /* create destination directory if not existing */
    if (access(file->dst, F_OK) != 0 && mkdir(file->dst, file->mode) != 0) {
        fail_append(fail_list, file->dst, "unable to create directory");
        return -1;
    }

    /* clone attributes */
    if (f_clone_attrs(file) && !opts->ignore_uid_err) {
        fail_append(fail_list, file->dst, "failed to apply attributes");
        return -1;
    }

    return 0;
}

void *progress_thread(void *arg)
{
    file_t *item = (file_t *)arg;
    time_t start, now, elapsed;
    char perc_t, perc_f, *speed;
    off_t bytes_per_sec;
    size_t bytes_written;
    int remaining_s;
    char eta_h, eta_m, eta_s;

    perc_t = (long double)file_list->bytes_done / file_list->size * 100;
    bytes_written = 0;
    time(&start);

    /* initial output */
    if (file_list->count_f > 1) {
        if (options->bars) {
            print_progr_bm(0, perc_t, "0b", 0, 0, 0);
        } else {
            print_progr_pm(0, perc_t, size_str(item->size),
                           size_str(file_list->size), "0b", 0, 0, 0);
        }
    } else {
        if (options->bars) {
            print_progr_bs(0, "0b", 0, 0, 0);
        } else {
            print_progr_ps(0, size_str(item->size), "0b", 0, 0, 0);
        }
    }

    /* wait for worker thread to start copying */
    while (progress_alive) {
        sleep(1);
        pthread_mutex_lock(&progress_lock);
            bytes_written = progress_bytes;
        pthread_mutex_unlock(&progress_lock);
        if (bytes_written > 0) {
            break;
        }
    }

    while (progress_alive) {
        /* get written bytes, calculate speed */
        pthread_mutex_lock(&progress_lock);
            bytes_written = progress_bytes;
            time(&now);
        pthread_mutex_unlock(&progress_lock);
        elapsed = now - start;
        if (elapsed <= 0) {
            elapsed = 1;
        }
        bytes_per_sec = (float)bytes_written / elapsed;
        speed = size_str(bytes_per_sec);
        /* calculate percentage, ETA */
        perc_f = (float)bytes_written / item->size * 100;
        perc_t = (float)(file_list->bytes_done + bytes_written) /
                 file_list->size * 100;
        remaining_s = (file_list->size - (file_list->bytes_done +
                      bytes_written)) / bytes_per_sec;
        eta_s = remaining_s % 60;
        eta_m = (remaining_s % 3600) / 60;
        eta_h = remaining_s / 3600;
        if (eta_h > 99 || eta_h < 0) {
            eta_h = 99;
        }
        /* print beautiful progress information */
        if (file_list->count_f > 1) {
            if (options->bars) {
                print_progr_bm(perc_f, perc_t, speed, eta_s, eta_m, eta_h);
            } else {
                print_progr_pm(perc_f, perc_t, size_str(item->size),
                               size_str(file_list->size), speed, eta_s, eta_m, eta_h);
            }
        } else {
            if (options->bars) {
                print_progr_bs(perc_f, speed, eta_s, eta_m, eta_h);
            } else {
                print_progr_ps(perc_f, size_str(item->size), speed, eta_s, eta_m, eta_h);
            }
        }
        fflush(stdout);
        /* check termination */
        if (!progress_alive) {
            // TODO: additional output for 100%
            break;
        }
        sleep(1);
    }

    //TODO: clear whole terminal line
    putchar('\r');
    putchar('\n');

    return NULL;
}
