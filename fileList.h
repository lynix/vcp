/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * A simple implementation of a double-linked list of files with common
 * attributes and a destination field. This is neither complete nor
 * elegant, it just fits my needs. 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdlib.h>			/* realpath(), and others	*/
#include <sys/stat.h>		/* file stats 				*/
#include <libgen.h>			/* basename() 				*/
#include <sys/types.h>		/* uid_t, gid_t, etc		*/
#include <utime.h>			/* struct utimbuf			*/
#include <unistd.h>			/* F_OK						*/

#define S_IFDIR __S_IFDIR	/* regular directory		*/
#define S_IFREG	__S_IFREG	/* regular file				*/

typedef long long 		llong;
typedef unsigned long 	ulong;

enum ftype { RFILE, RDIR };

struct file {
	char	*filename;
	char	*src;
	char	*dst;
	enum	ftype type;
	llong	size;
	uid_t	uid;
	gid_t	gid;
	mode_t	mode;
	struct	utimbuf	times;
	
	struct file	*prev;
	struct file	*next;
};

struct fileList {
	long 	count;
	llong 	size;
	struct	file *head;
	struct	file *pointer;
};

struct 	file *get_file(char *fname);
struct 	file *fl_next(struct fileList *list);
struct 	file *fl_prev(struct fileList *list);
struct 	fileList *fl_create();
int		fl_add(struct fileList *list, struct file *item);
void 	fl_rewind(struct fileList *list);
void 	fl_forward(struct fileList *list);
