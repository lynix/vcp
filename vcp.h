/* TODO:	- -l switch that prints limits (filenum, sizes,...)
 * 			- fix that goddam hell of var-types (llong, ulong, double,...)
 * 			- progress: filesize, re-sort
 * 			- new progress format (like rsync):
 * 			  	/path/some/where/file.ext
 * 			  	File: [.......]		Total: [.......]
 * 			- code BEAUTIFYing (should go along anyway)
 * 			- signal handling (delete incomplete on interrupt)
 * 			- rewrite error messages (filenames, shortening)
 * 			- overflow protection for large vars (size, etc.)
 * 			- free space check?
 * 			- directory attributes: dropped?
 * 			- last: feature-list :-)
 * */

/* FEATURES:
 * 			- no abort on failed file: tries its best to copy all the others,
 * 			  so you don't have to start the whole thing again just because
 * 			  one single file didn't ant to cooperate. prints a list of failed
 * 			  files after finishing its list. so after fixing the problem
 * 			  (encoding/whatever) you just start again with "-k" and you're done
 * 
 * */

/* LIMITS:
 * 			- only regular files and directories, no symlinks, nodes or sth.
 * 			- max. ??? files, max. filesize ??? Bytes, max. list size ??? Bytes
 * */

#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <libgen.h>			/* basename(), dirname() */
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>			/* realpath() */
#include <stdlib.h>			/* realpath() */
#include <openssl/md5.h>	/* MD5() */
#include <errno.h>			/* errno, strerror() */

#include "fileList.h"		/* my own humble implementation */

#define BUFFS 1048576		/* 1MiB buffer size for read() and write() */
#define MAX_SIZE_L 30		/* maximum length of size string, numbers */
#define SPEED_N 5			/* speed middle calculation */
#define FNAME_W 25			/* filename width in progress output */
#define _POSIX_SOURCE		/* fsync() */

struct options {
	int force;
	int sync;
	int delete;
	int keep;
	int quiet;
	int verbose;
	int debug;
};

# if defined __STRICT_ANSI__
	char 	*realpath (const char *, char *);
	int 	fsync(int);
# endif

void 	print_usage();
void 	init_opts();
void 	print_error(char *string, ...);
void	print_debug(char *string, ...);
void 	progress(double t_perc, long num, long t_num, char *fname, 
									  double perc, ulong bps, long eta);
int 	copy_file(struct file *item, long filenum, long total_filenum, 
				llong total_size, llong total_done, time_t total_start);
int 	parse_opts(int argc, char *argv[]);
int 	crawl_files(struct fileList *list, char *item, char *dest);
int 	do_copy(struct fileList *files);
int 	clone_attrs(struct file *src, char *dst);
int 	f_equal(struct file *a, struct file *b);
char 	*strccat(char *a, char *b);
char 	*path_str(char *path, char *sub);
char 	ask_overwrite(char *src, char *src_size, char *dst, char *dst_size);
char 	*size_str(llong bytes);
ulong 	speed(ulong spd);
struct 	fileList *build_list(int argc, int start, char *argv[]);
