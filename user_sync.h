#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>

#include "main.h"
#include "traverse_dir.h"

#define vpath "./"
#define file_name_size 100
#define total_files 100

#define upd_file "update.log"
#define del_file "delete.log"
#define status_err_file "status_err.log"
#define log_file "log.log"

char files_f[200], delete_files[200],log_f[200],
	ful[100], fdl[100],fll[100];

/* parameter needed by the software program */

char *src, //for source directory path
     *dest, //for destination directory path
     *detach;	//to make it daemon(true) or a process(false)
int tsec; // after each tsec check for any updates and transfer that to the server.

char u_files[file_name_size][total_files],
	d_files[file_name_size][total_files];

int u_cnt,
	d_cnt;

/* end */

//char *log_file, 
//	*files_from
//;

/* file descriptors need to be opened */ 

int status_err, //for storing status, errors and crash messages. Used by developers in debugging.
    fd_update,  // for storing information related to creation and modification of files. To be used by software only.
    fd_delete,  // for storing information related to deletion of files. To be used by software only.	 		
    fd_log	 
;
/* end */

void print_err(char *);		//for logging status and error messages
void timeout_handler(int); //for timeout handler
