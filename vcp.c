/* Copyright lynix <lynix47@gmail.com>, 2009
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
	int 	argstart;
	
	/* initialize options, set umask */
	init_opts();
	umask(0);
	
	/* parse cmdline options */
	if ((argstart = parse_opts(argc, argv)) == -1) {
		exit(EXIT_FAILURE);
	}
	if (argc < 3) {
		print_error("insufficient arguments.\nTry -h for help.");
		exit(EXIT_FAILURE);
	}
	
	/* parse cmdline files, build list */
	if (opts.verbose) {
		printf("collecting file information...\n");
		fflush(stdout);
	}
	if (build_list(argc, argstart, argv) != 0) {
		print_error("failed to build file list, aborting.");
		exit(EXIT_FAILURE);
	}
	if (opts.verbose) {
		/* print summary */
		printf("will now copy %lu file(s), %s\n\n", file_list->count, 
				size_str(file_list->size));
	}
	if (opts.debug) {
		/* print copy list */
		for (long i=0; i<file_list->count; i++) {
			printf("%s --> %s\n", file_list->items[i]->src,
				file_list->items[i]->dst);
		}
		putchar('\n');
		fflush(stdout);
	}
	
	/* do the actual copy */
	if (do_copy() != 0) {
		print_error("copying failed");
		free(file_list);
		exit(EXIT_FAILURE);
	}
	free(file_list);
	
	exit(EXIT_SUCCESS);
}

int build_list(int argc, int start, char *argv[])
{
	struct file *f_dest;
	char *item, *dest, *dest_dir, *dest_base;
	
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
	
	/* logic checking (do not copy file over dir / dir over file) */
	if ((f_dest = get_file(dest)) != NULL) {
		if (f_dest->type == RFILE) {
			if ((argc-start-1) != 1) {
				print_error("unable to copy multiple items to one file");
				return -1;
			}
		}
		free(f_dest);
	} else {
		if ((argc-start-1) > 1) {
			print_error("destination directory '%s' does not exist", 
						argv[argc-1]);
			return -1;
		}
	}
	
	/* create new lists */
	if ((file_list = flist_init()) == NULL) {
		print_debug("failed to create file list");
		return -1;
	}
	if ((dir_list = flist_init()) == NULL) {
		print_debug("failed to create dir list");
		return -1;
	}
	
	/* iterate through cmdline and insert items */
	for (int i=start; i<(argc-1); i++) {
		/* clean item path */
		item = realpath(argv[i], NULL);
		item = realloc(item, strlen(item)+1);
		/* crawl item */
		if (crawl_files(item, dest) != 0) {
			free(file_list);
			free(dir_list);
			return -1;
		}
	}
	
	/* check if something left to copy at all */
	if (file_list->count == 0 && dir_list->count == 0) {
		printf("no items to copy.\n");
		free(file_list);
		free(dir_list);
		exit(EXIT_SUCCESS);
	}
	
	return 0;
}

int do_copy()
{
	/* works off the file lists and copies each item					*/
	
	time_t start;
	ullong total_done;
	int error;
	struct strlist *failed;
	struct file *item;
	
	time(&start);
	total_done=0;
	error=0;
	
	if ((failed = strlist_init()) == NULL) {
		return -1;
	}

	/* copy files */
	for (ulong i=0; i < file_list->count; i++) {
		item = file_list->items[i];
		if (copy_file(item, file_list->count, file_list->size,
				total_done,	start, failed) != 0) {
			error = 1;
		}
		total_done += item->size;
	}
	
	/* reverse-iterate through directories and set times, evtl. delete 	*/
	if (!error && dir_list->count > 0) {
		for (ulong i = (dir_list->count)-1; i >= 0; i--) {
			if (clone_attrs(dir_list->items[i]) != 0) {
				error = 1;
				error_append(failed, dir_list->items[i]->src,
				"unable to apply directory attributes", strerror(errno));
			} else {
				if (opts.delete) {
					if (rmdir(dir_list->items[i]->src) != 0) {
						error = 1;
						error_append(failed, dir_list->items[i]->src,
						 "unable to delete directory", strerror(errno));
					}
				}
			}
			if (i == 0) {
				break;
			}
		}
	}
	
	/* print list of failed items */
	if (error) {
		print_error("the following errors occured:");
		for (ulong i=0; i < failed->count; i++) {
			printf("   %s\n", failed->items[i]);
		}
		return -1;
	}
	
	return 0;
}

int copy_file(struct file *f_src, ulong t_num, ullong t_size,
			 ullong t_done, time_t t_start, struct strlist *failed)
{
	int 	fd_src, fd_dst, error, stats;
	char 	*buffer, *dest;
	long  	eta, buffsize, bytes_r, bytes_w;
	ullong 	bytes_d;
	struct 	file *f_dest;
	struct	winsize ws;
	double	t_perc, perc, spd;
	time_t 	timer;
	
	/* check if destination exists */
	dest = f_src->dst;
	if ((f_dest = get_file(dest)) != NULL) {
		/* overwrite (remove and recreate), we know the user wants it */
		if (remove(dest) != 0) {
			error_append(failed, f_src->src, "unable to delete existing destination",
						strerror(errno));
			return -1;
		}
		free(f_dest);
	}
	
	/* open source (true, true ;) ) */
	if ((fd_src = open64(f_src->src, O_RDONLY)) == -1) {
		error_append(failed, f_src->src, "unable to open for reading",
					strerror(errno));
		return -1;
	}
	
	/* open destination */
	if ((fd_dst = open64(dest, O_CREAT|O_EXCL|O_WRONLY, (mode_t)0600)) == -1) {
		error_append(failed, dest, "unable to open for writing",
					strerror(errno));
		close(fd_src);
		return -1;
	}
	
	/* set buffer size, allocate buffer */
	if (f_src->size < BUFFS) {
		buffsize = f_src->size;
		stats = 0;
	} else {
		buffsize = BUFFS;
		stats = 1;
	}
	if ((buffer = malloc(buffsize)) == NULL) {
		print_error("failed to allocate %s for buffer",
					size_str(buffsize));
		error_append(failed, f_src->src, "failed allocating buffer", NULL);
		close(fd_src);
		close(fd_dst);
		return -1;
	}
	
	/* initialize counters */
	bytes_d = 0;
	bytes_r = 0;
	bytes_w = 0;
	error = 0;
	time(&timer);

	/* print source filename */
	printf("%s\n", f_src->src);
	
	/* the part you are looking for: read from source... */
	while ((bytes_r = read(fd_src, buffer, buffsize))) {
		if (bytes_r == -1) {
			print_error("\rI/O error while reading: %s", strerror(errno));
			error_append(failed, f_src->src, "I/O reading error",
						strerror(errno));
			error = 1;
			break;
		}
		/* ... and write to destination */
		bytes_w = write(fd_dst, buffer, bytes_r);
		if (bytes_w != bytes_r) {
			print_error("\rI/O error writing to '%s': %s", dest,
						strerror(errno));
			printf("read: %ld, written: %ld, fd_dst: %d, fd_src: %d\n", bytes_r, bytes_w, fd_dst, fd_src);
			error_append(failed, f_src->src, "I/O writing error",
						strerror(errno));
			error = 1;
			break;
		}
		
		/* account written data */
		bytes_d += (ullong)bytes_w;
		t_done += (ullong)bytes_w;
			
		/* print beautiful progress information */
		if (!opts.quiet && stats && (time(NULL) > timer)) {
			time(&timer);
			t_perc = (long double)t_done / t_size * 100;
			perc = (long double)bytes_d / f_src->size * 100;
			spd = speed(t_done / (time(NULL)-t_start));
			eta = (t_size - t_done) / spd;
			progress(t_perc, t_num, perc, spd,	eta, f_src->size);
		}
	}
	/* clear progress information line for next filename*/
	if (!opts.quiet && stats) {
		putchar('\r');
		ioctl(0, TIOCGWINSZ, &ws);
		for (int n=0; n < ws.ws_col; n++) {
			putchar(' ');
		}
		putchar('\r');
	}

	if (!error) {
		/* fsync if requested */
		if (opts.sync) {
			if (fsync(fd_dst) != 0) {
				error_append(failed, dest, "fsync failed",
							strerror(errno));
				error=1;
			}
		}
	
		/* clone attributes */
		if (clone_attrs(f_src) != 0) {
			error_append(failed, dest, "failed to apply attributes",
						strerror(errno));
			error = 1;
		}

		/* remove source if requested */
		if (opts.delete && !error) {
			if (remove(f_src->src) != 0) {
				print_error("failed to remove '%s': %s", f_src->src,
							strerror(errno));
				error_append(failed, f_src->src, "failed removing file",
							strerror(errno));
				error = 1;
			}
		}
	}
	
	/* remove incomplete transfers */
	if (bytes_d != f_src->size) {
		print_debug("transfer check negative, removing '%s'", dest);
		if (remove(dest) != 0) {
			error_append(failed, dest, "failed to remove broken destination",
						strerror(errno));
		}
		error = 1;
	}
	
	/* clean up */
	free(buffer);
	close(fd_src);
	close(fd_dst);

	
	if (error) {
		return -1;
	}
	
	return 0;
}

int crawl_files(char *src, char *dest)
{
	/* recursively scan directory and adds files to transfer list		*/
	
	DIR *d_src;
	struct dirent *dp_src;
	struct file *f_src;
	struct file *f_dest;
	
	/* check source access */
	if ((f_src = get_file(src)) == NULL) {
		print_error("failed to open '%s': %s", src, strerror(errno));
		return -1;
	}
	
	/* check source type */
	if (f_src->type == RDIR) {
		/* directory, check destination */
		if ((f_dest = get_file(dest)) != NULL) {
			if (f_dest->type == RFILE) {
				print_error("unable to copy dir '%s' over file '%s'", src,
							dest);
				free(f_src);
				free(f_dest);
				return -1;
			}
			free(f_dest);
			f_dest = NULL;
			/* extend destination path */
			dest = path_str(dest, f_src->filename);
		}
		/* check destination again (eventually new path) */
		if ((f_dest = get_file(dest)) == NULL) {
			/* destination directory does not exist, create it */
			if (mkdir(dest, f_src->mode) != 0) {
				print_error("failed to create directory '%s': %s", dest,
							strerror(errno));
				return -1;
			} else {
				f_src->dst = dest;
				if (flist_add(dir_list, f_src) != 0) {
					print_debug("failed to add '%s' to dir list", src);
					return -1;
				}
			}
		} else {
			/* destinatio exists, check type */
			if (f_dest->type == RFILE) {
				print_error("unable to copy dir '%s' over file '%s'", src,
							dest);
				free(f_src);
				free(f_dest);
				return -1;
			}
			f_src->dst = dest;
			if (flist_add(dir_list, f_src) != 0) {
				print_debug("failed to add '%s' to dir list", src);
				return -1;
			}
		}
		/* crawl source directory */
		if ((d_src = opendir(src)) == NULL) {
			print_error("failed to open directory '%s': %s", src,
						strerror(errno));
			return -1;
		}
		while ((dp_src = readdir(d_src)) != NULL) {
			/* IMPORTANT: skip references to . and .. */
			if (strcmp(dp_src->d_name, "..") == 0 || 
					strcmp(dp_src->d_name, ".") == 0) {
				continue;
			}
			/* recursively crawl directory contents */
			if (crawl_files(path_str(src, dp_src->d_name), dest) 
					!= 0) {
				return -1;
			}
		}
		closedir(d_src);
	} else {
		/* file, check destination */
		if ((f_dest = get_file(dest)) != NULL) {
			if (f_dest->type == RDIR) {
				dest = path_str(dest, f_src->filename);
			}
			free(f_dest);
			f_dest = NULL;
			/* check destination again (pot. new path) */
			if ((f_dest = get_file(dest)) != NULL) {
				if (f_dest->type == RDIR) {
					print_error("unable to replace dir '%s' with file '%s'", 
								dest, src);
					return -1;
				}
				/* decide whether to skip/overwrite */
				if (!opts.force) {
					if (opts.keep) {
						return 0;
					} else if (opts.update) {
						if (f_equal(f_src, f_dest)) {
							if (opts.delete) {
								if (remove(f_src->src) != 0) {
									print_error("unable to delete '%s': %s\n",
										f_src->src, strerror(errno));
								}
							}
							return 0;
						}
					} else if (ask_overwrite(f_src->filename, size_str(
								f_src->size), f_dest->filename, size_str(
								f_dest->size)) == 'n') {
						return 0;
					}
				}
			}
		}
		/* finally add file to file list */
		f_src->dst = dest;
		if (flist_add(file_list, f_src) != 0) {
			print_debug("failed to add '%s' to file list", src);
			return -1;
		}
	}
	
	return 0;
}

void progress(double t_perc, ulong t_num, double perc, ulong bps,
			 long eta, ullong fsize)
{
	int eta_s, eta_m, eta_h;
	
	/* split ETA */
	eta_s = eta % 60;
	eta_m = (eta % 3600) / 60;
	eta_h = eta / 3600;
	if (eta_h > 99) {
		eta_h = 99;
	}
	
	/* select style (single/multi) and finally print */
	if (t_num > 1) {
		if (opts.bars) {
			printf("\rFile: %3.0f%% %s   Total: %3.0f%% %s  ETA %02d:%02d:%02d   ",
				perc, prog_bar(perc), t_perc, prog_bar(t_perc), eta_h, eta_m, eta_s);
		} else {
			printf("\rFile: %3.0f%% of %s | Total: %3.0f%% @ %s/s ETA %02d:%02d:%02d   ",
				perc, size_str(fsize), t_perc, size_str(bps), eta_h, eta_m, eta_s);
		}
	} else {
		if (opts.bars) {
			printf("\r%3.0f%%  %s  ETA %02d:%02d:%02d  ", perc, prog_bar(perc),
				eta_h, eta_m, eta_s);
		} else {
			printf("\r%3.0f%% of %s @ %s/s ETA %02d:%02d:%02d   ", perc,
				size_str(fsize), size_str(bps), eta_h, eta_m, eta_s);
		}
	}
	fflush(stdout);
	
	return;
}

void print_error(char *msg, ...)
{
	/* prints an error message to stderr								*/
	va_list argpointer;
	
	va_start(argpointer, msg);
	fprintf(stderr, "Error: ");
	vfprintf(stderr, msg, argpointer);
	fprintf(stderr, "\n");
	fflush(stderr);
	
	return;
}

void print_debug(char *msg, ...)
{
	/* prints debugging messages to stderr if debug is set				*/
	va_list argpointer;
	
	if (opts.debug) {
		va_start(argpointer, msg);
		fprintf(stderr, "Debug: ");
		vfprintf(stderr, msg, argpointer);
		fprintf(stderr, "\n");
		fflush(stderr);
	}
	
	return;
}

char ask_overwrite(char *old, char *old_size, char *new, char *new_size)
{
	/* prints a confirmation dialog for file overwriting				*/
	
	char answer;
	
	do {
		printf("overwrite %s (%s) with %s (%s) (Y/n)? ", old, 
				old_size, new, new_size);
		fflush(stdout);
		answer = getchar();
		if (answer != '\n') {
			while (getchar() != '\n') {
				/* ignore additional chars */
			}
		}
	} while (answer != 'Y' && answer != 'y' && answer != 'n' &&
				answer != '\n');
	
	if (answer == '\n' || answer == 'Y') {
		answer = 'y';
	}
		
	return answer;
}

int clone_attrs(struct file *src)
{
	/* applies the attributes of a given file to another				*/
	
	int retval=0;
	
	/* set owner uid/gid */
	if (chown(src->dst, src->uid, src->gid) != 0) {
		print_debug("failed to set uid/gid");
		retval = -1;
	}
	/* set mode */
	if (chmod(src->dst, src->mode) != 0) {
		print_debug("failed to set mode");
		retval = -1;
	}
	/* set atime/mtime */
	if (utime(src->dst, &(src->times)) != 0) {
		print_debug("failed to set atime/mtime");
		retval = -1;
	}
	
	return retval;
}
	
void print_usage()
{
	/* prints copyright and usage information							*/
	
	printf("vcp  Copyright (C) 2009  lynix <lynix47@gmail.com>\n\n");
	
    printf("This program comes with ABSOLUTELY NO WARRANTY, use at\n");
    printf("own risk. This is free software, and you are welcome to\n");
    printf("redistribute it under the terms of the GNU General\n");
    printf("Public License as published by the Free Software\n");
    printf("Foundation, either version 3 of the License, or (at your\n");
    printf("option) any later version.\n\n");
    
	printf("Usage:	vcp [OPTIONS] [SOURCE(S)] [DESTINATION]\n\n");
	printf("  -b  display progress bars instead of text\n");
	printf("  -d  delete source(s) on success\n");
	printf("  -f  overwrite existing files (default: ask)\n");
	printf("  -h  print usage and license information\n");
	printf("  -k  skip all existing files (default: ask)\n");
	printf("  -q  do not print progress information\n");
	printf("  -s  ensure each file is synched to disk on completion\n");
	printf("  -u  skip identical existing files, update changed ones\n");
	printf("  -v  be verbose\n");
	printf("  -D  print debugging messages\n\n");
	
	printf("This version of vcp was built on %s %s.\n", __DATE__,
			__TIME__);
	
	return;
}

void init_opts()
{
	/* initializes the global options structure							*/
	
	opts.bars = 0;
	opts.force = 0;
	opts.sync = 0;
	opts.delete = 0;
	opts.keep = 0;
	opts.quiet = 0;
	opts.verbose = 0;
	opts.update = 0;
	opts.debug = 0;

	return;
}

int parse_opts(int argc, char *argv[])
{
	/* parses command-line options and update opts structure			*/
	
	char c;
    extern int optind, optopt, opterr;
    
    opterr = 0;
	
	while ((c = getopt(argc, argv, "bdfhkqsluvD")) != -1) {
		switch (c) {
			case 'b':
				opts.bars = 1;
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
			case 'q':
				opts.quiet = 1;
				break;
			case 'd':
				opts.delete = 1;
				break;
			case 'h':
				print_usage();
				exit(EXIT_SUCCESS);
			case 'l':
				print_limits();
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

char *strccat(char *a, char *b)
{
	/* like strcat() but does not overwrite first argument and takes	*
	 * care of sufficient destination size								*/
	
	char *retval;
	int n,m;
	
	if (a == NULL && b == NULL) {
		return NULL;
	}
	
	if (a == NULL) {
		n = 0;
	} else {
		n = strlen(a);
	}
	if (b == NULL) {
		m = 0;
	} else {
		m = strlen(b);
	}
		
	if ((retval = malloc(n+m+1)) == NULL) {
		return NULL;
	}
	for (int i=0; i<n; i++) {
		retval[i] = a[i];
	}
	for (int i=0; i<m; i++) {
		retval[i+n] = b[i];
	}	
	retval[n+m] = '\0';

	return retval;
}

char *path_str(char *path, char *sub)
{
	/* builds a path string with separating '/' 						*/
	
	char *retval;

	if (path[strlen(path)-1] != '/') {
		retval = strccat(path, "/");
	} else {
		retval = path;
	}
	retval = strccat(retval, sub);
	
	return retval;
}

char *size_str(ullong bytes)
{
	/* generates human readable size information; we stick to the 'new'	*
	 * IEC standard, see http://en.wikipedia.org/wiki/Binary_prefix		*/
	
	char *retval, *unit, *buffer;
	double number;
	
	buffer = malloc(MAX_SIZE_L);
	
	if (bytes >= 1073741824) {
		unit = "GiB";
		number = (double)bytes / 1073741824;
	} else if (bytes >= 1048576) {
		unit = "MiB";
		number = (double)bytes / 1048576;
	} else if (bytes >= 1024) {
		unit = "KiB";
		number = (double)bytes / 1024;
	} else {
		unit = "B";
		number = (double)bytes;
	}
	
	sprintf(buffer, "%.2f ", number);
	retval = strccat(buffer, unit);
	free(buffer);
	
	return retval;
}

ulong speed(ulong spd)
{
	/* takes new current speed, adds it to the global array and			*
	 * calculates the arithmetic average								*/
	
	ullong sum=0;
	
	for (int i=0; i<SPEED_N-1; i++) {
		speeds[i] = speeds[i+1];
		sum += speeds[i];
	}
	speeds[SPEED_N-1] = spd;
	sum += spd;
	
	return sum / SPEED_N;
}

int f_equal(struct file *a, struct file *b)
{
	/* compares two given files regarding their size, owner and times 	*/
	
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
	
	return 1;
}

void error_append(struct strlist *list, char *fname, char *error,
				 char *reason)
{
	char *errmsg;

	errmsg = strccat(fname, ": ");
	errmsg = strccat(errmsg, error);
	if (reason != NULL) {
		errmsg = strccat(errmsg, " (");
		errmsg = strccat(errmsg, reason);
		errmsg = strccat(errmsg, ")");
	}
	
	if (strlist_add(list, errmsg) != 0) {
		print_debug("failed to add to failed list:");
		print_error(errmsg);
	}

	return;
}

char *prog_bar(double percent)
{
	/* returns a progress bar as string									*/

	#define BAR_STEP 100.0/(BAR_WIDTH-2)

	char *retval;
	double temp;
	short i;

	if ((retval = malloc(BAR_WIDTH+1)) == NULL) {
		return NULL;
	}
	retval[0] = '[';
	retval[BAR_WIDTH-1] = ']';
	retval[BAR_WIDTH] = '\0';

	i = 1;
	temp = BAR_STEP;
	while ((temp <= percent) && (i < BAR_WIDTH-1)) {
		retval[i] = '#';
		i++;
		temp += BAR_STEP;
	}
	while (i < BAR_WIDTH-1) {
		retval[i] = '-';
		i++;
	}

	return retval;
}

void print_limits()
{
	/* prints size, count and speed limits								*/
	
	ullong max_ull;
	ulong  max_ulong;

	max_ull = 0;
	max_ulong = 0;
	
	printf("Limits (on this architecture):\n\n");
	printf(" - number of files         %lu\n", --max_ulong);
	printf(" - speed                   %s/s\n", size_str(max_ulong));
    printf(" - total size / file size  %s\n", size_str(--max_ull));
	
	return;
}
