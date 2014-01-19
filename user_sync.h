/*
 * The header of user_sync.c, declaring constants and defining function prototype and variables
 * , for SNAPSync.
 *
 * Copyright (C) 2013 Garvit Sharma <garvits45@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, visit the http://fsf.org website.

*/


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>

#include "main.h"
#include "traverse_dir.h"

#define file_name_size 100
#define total_files 100

#define upd_file "update.log"
#define del_file "delete.log"
#define status_err_file "status_err.log"
#define log_file "log.log"

char files_f[200], delete_files[200],log_f[200],
	ful[100], fdl[100],fll[100], vpath[1024];

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
