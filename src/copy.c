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


extern strlist_t       *fail_list;
extern flist_t         *copy_list;
extern char            file_done_flag;
extern pthread_mutex_t file_bytes_lock;
extern off_t           file_bytes_done;
extern opts_t          opts;

void    *copy_thread(void *p);
void    *progress_thread(void *p);


void copy_file(file_t *file)
{
    /* initialize globals */
    if (pthread_mutex_init(&file_bytes_lock, NULL) != 0) {
        fail_append(fail_list, file->dst,
                "failed to initialize bytes_done mutex");
        return;
    }
    file_bytes_done = 0;
    file_done_flag = 0;

    pthread_t copy_thrd, progr_thrd;
    char join_progr = 0;

    /* spawn copy worker thread */
    if (pthread_create(&copy_thrd, NULL, copy_thread, file) != 0) {
        fail_append(fail_list, file->dst, "unable to spawn copy-thread");
        pthread_mutex_destroy(&file_bytes_lock);
        return;
    }

    /* spawn progress thread */
    if (!opts.quiet && file->size > BUFFS*BUFFM) {
        if (pthread_create(&progr_thrd, NULL, progress_thread, file) != 0) {
            print_error("unable to spawn progress-thread (quiet copy)");
        } else {
            join_progr = 1;
        }
    }

    pthread_join(copy_thrd, NULL);
    if (join_progr) {
        /* signal progress thread */
        file_done_flag = 1;
        pthread_join(progr_thrd, NULL);
    }

    /* account global progress */
    copy_list->bytes_done += file->size;

    /* clean up */
    pthread_mutex_destroy(&file_bytes_lock);

    return;
}

void copy_link(file_t *file)
{
    /* remove evtl. existing one */
    if (access(file->dst, F_OK) == 0) {
        if (remove(file->dst) != 0) {
            fail_append(fail_list, file->dst, "unable to delete link");
        }
    }
    /* create new link */
    if (symlink(file->src, file->dst) != 0) {
        fail_append(fail_list, file->dst, "unable to create symlink");
        file->done = 0;
    } else {
        file->done = 1;
    }
}

void copy_dir(file_t *file)
{
    if (mkdir(file->dst, file->mode) != 0) {
        fail_append(fail_list, file->dst, "unable to create directory");
        file->done = 0;
    } else {
        file->done = 1;
    }

    return;
}
