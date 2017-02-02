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

#ifndef _COPY_H
#define _COPY_H

#include "file.h"
#include "lists.h"


// copy regular file given as file_t, use supplied buffer for I/O
int copy_file(file_t *file, flist_t *flist, strlist_t *fail_list, opts_t *opts,
              char *buffer, unsigned int buff_size);

// 'copy' directory given as file_t, i.e. create destination directory
int copy_dir(file_t *file, opts_t *opts, strlist_t *fail_list);

// copy symlink given as file_t
int copy_link(file_t *file, strlist_t *fail_list);

// display transfer progress for given file, threaded
void *progress_thread(void *arg);


#endif
