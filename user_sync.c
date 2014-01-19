/*
 * The startup routines, including main(), for SNAPSync.
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <error.h>
#include <fcntl.h>

#include "user_sync.h"
#include "inotify.h"


#define line_size 100

char *src=NULL;
char *dest=NULL;
char *detach=NULL;
int tsec=10;
int status_err=-1;
int fd_update=-1;
int fd_delete=-1;
int infd;
char *prefix[1024];

const uint32_t standard_event_mask=
		IN_ATTRIB 	|
		IN_CLOSE_WRITE 	|
		IN_CREATE	|
		IN_DELETE	|
		IN_DELETE_SELF	|
		IN_MODIFY	|
		IN_MOVED_FROM	|
		IN_MOVED_TO;

pthread_t thread_notify;

/* function prototypes */
static void parse_file(char *);
static void main1(void);
void print_err(char *);
static void pull(void);
static void open_file_desc(void);
static void signal_initializer(void);

char files_f[200], delete_files[200],
	ful[100], fdl[100], vpath[1024];

void cleanup(void)
{
	print_err("Got an instruction to terminate...\n");
	print_err("pthread_cancel()ing created thread...\n");
	pthread_cancel(thread_notify);
	print_err("pthread_cancel()ing done\n");
}

void init_inotify(void)
{
	
	infd = inotify_init();
        if(infd < 0)
        {
                print_err("inotify_init() error\n");
        }
	wd_src = inotify_add_watch(infd, src, standard_event_mask);
	prefix[wd_src]=(char *)calloc(strlen(src), sizeof(char *));
	strcpy(prefix[wd_src], src);
	traverse_main(src);
}

static void signal_initializer(void)
{
	if(signal(SIGALRM,timeout_handler)==SIG_ERR)
	{
		printf("error handling SIGALRM - exiting\n");
		exit(1);
	}
	/*if(signal(SIGTERM,cleanup)==SIG_ERR)
	{
		printf("error handling SIGTERM - exiting\n");
		exit(1);
	}
	if(signal(SIGINT,cleanup)==SIG_ERR)
	{
		printf("error handling SIGINT - exiting\n");
		exit(1);
	}	
	if(signal(SIGQUIT,cleanup)==SIG_ERR)
	{
		printf("error handling SIGQUIT - exiting\n");
		exit(1);
	}	
	if(signal(SIGHUP,cleanup)==SIG_ERR)
	{
		printf("error handling SIGHUP - exiting\n");
		exit(1);
	}	
	if(signal(SIGABRT,cleanup)==SIG_ERR)
	{
		printf("error handling SIGABRT - exiting\n");
		exit(1);
	}*/	
return;
}

static void pull(void)
{
	int argc, retval;
	char *argv[10], sr[100], dt[100];
	pid_t pid;
	strcpy(sr,src);
	strcpy(dt,dest);
	pid=fork();
	if(pid<0)
	{
		print_err("error fork() for pull - exiting..\n");
		exit(EXIT_FAILURE);
	}
	else if(pid==0)
	{
		argv[0]="rsync";
		argv[1]="-av";
		argv[2]="--timeout=60";
		argv[3]="--del";
		argv[4]=dt;
		argv[5]=sr;
		argc=6;
label:
		retval=sync_main(argc,argv);
		//printf("%d\n",retval);
		switch(retval)
		{
			case 0:print_err("Updates pull()'ed successfully\n");
				break;

			case -17:print_err("failed to establish connection, nothing done - trying again...\n");
				//erase_data(fll);
				goto label;
	
			//case -19:print_err("Connection lost during the transmission - checking files...\n");
			//	check_file();
		}
		exit(EXIT_SUCCESS);
	}
	wait(NULL);
	return;
}

static void open_file_desc(void)
{
	char fse[100];
	
	strcpy(fll,vpath);
	strcat(fll,log_file);
	strcat(log_f, fll);

	fse[0]='\0';
	strcat(fse,vpath);
	strcat(fse,status_err_file);

	ful[0]='\0';		
	strcat(ful,vpath);
	strcat(ful,upd_file);
	strcat(files_f,ful);

	fdl[0]='\0';		
	strcat(fdl,vpath);
	strcat(fdl,del_file);
	strcat(delete_files,fdl);

	status_err=open(fse,O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);
	if(status_err<0)
	{printf("\nerror opening status_error.log\n"); exit(1);}
	
	fd_update=open(ful,O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
	if(fd_update<0)
	{printf("\nerror opening update.log\n"); exit(1);}

	fd_delete=open(fdl,O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
	if(fd_delete<0)
	{printf("\nerror opening delete.log\n"); exit(1);}

	fd_log=open(fll,O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
	if(fd_log<0)
	{printf("\nerror opening log.log\n"); exit(1);}
}

void print_err(char *msg)
{
	int len,ret;
	len=strlen(msg);
	ret=write(status_err,msg,len);
	if(ret==-1)
	exit(1);
}

static void parse_file(char *config_file)
{
	int i=0,j=0;
	char c,*line[20],*str,*delim="=",*token;
	FILE *fp;
	fp=fopen(config_file,"r");

	if(fp==NULL)
	{
		printf("fopen() error on %s\n",config_file);
		exit(EXIT_FAILURE);
	}			

	str=(char *)malloc(100*sizeof(char));

	while((c=fgetc(fp))!=EOF)
	{
		if(c=='\n')
		{
			*(str+j)='\0';
			line[i++]=str;
			j=0;
			str=(char *)malloc(100*sizeof(char));
		}
		else
		*(str+j++)=c;
	}
	
	fclose(fp);

	for(j=0;j<i;j++)
	{
		token=strtok(line[j],delim);
		if(strcmp(token,"time")==0)
		{
			tsec=atoi(strtok(NULL,delim));
		}
		else if(strcmp(token,"daemon")==0)	
		{
			detach=strtok(NULL,delim);
			if(detach==NULL)
			{perror("give either true or false\n"); exit(EXIT_FAILURE);}
		}
		else if(strcmp(token,"destination")==0)	
		{
			dest=strtok(NULL,delim);
			//arg[k++]=dest;
			if(dest==NULL)
			{perror("invalid path\n"); exit(EXIT_FAILURE);}
		}
		else if(strcmp(token, "source")==0)	
		{
			src=(char *)malloc(100*sizeof(char));
			src=strtok(NULL,delim);
			//arg[3]=src;
			if(src==NULL)
			{perror("invalid path\n"); exit(EXIT_FAILURE);}
		}
		else
		{
			perror("Invalid Attribute\n"); 		
			exit(EXIT_FAILURE);
		}
		
	}
	//arg[k]=(char *)0;
	free(str);
	for(j=0;j<i;j++)
	free(line[j]);
}

static void main1(void)
{
	int th_status,join_status;
	void *res;	

	//pull();		//pull content from the server for the first time.
	init_inotify();
	printf("DONE\n");	
	alarm(tsec);
        /*creating thread for the inotify*/
	th_status=pthread_create(&thread_notify,NULL,inotify_main,NULL);

	if(th_status!=0)
	{
		/*Thread creation failure*/
		print_err("thread creation failure for inotify - exiting..\n");
		exit(EXIT_FAILURE);
	}

	/*waiting for the complete execution of the inotify_thread*/
	join_status=pthread_join(thread_notify,&res);
	
	if(join_status!=0)
	{
		/*Log the pthread_join failure status*/
		print_err("pthread_join 'inotify' failure status\n");
	}

	if(res==PTHREAD_CANCELED)
	print_err("Thread cancellation was successful\n");
	else
	print_err("Thread cancellation was NOT successful\n");
	
	print_err("End of the program - return\n");	
		
	return;
}

int main(int argc,char *argv[])
{
	pid_t pid, xpid;
	int c,fd, x_status; //i = 0,t,c,index,digit_optind=0,length;	
	char *config_file=NULL, onetimepass[100];
	opterr=0;
	strcpy(files_f, "--files-from=");
	strcpy(delete_files, "--include-from=");
	strcpy(log_f,"--log-file=");	
	
	strcpy(onetimepass, vpath);
	strcpy(onetimepass, "onetimepass.sh");		

	signal_initializer();	
	
	if(getcwd(vpath, sizeof(vpath)) == NULL) {
		perror("Error getting CWD");
		exit(EXIT_FAILURE);
	}
	strcat(vpath,"/");	
	printf("\n>>> %s\n",vpath);
	open_file_desc();

	while(1)
	{
		//int this_option_optind=optind?optind:1;
		int option_index=0;
		static struct option long_options[]={{"config.file",1,0,0},{0,0,0,0}};

		c=getopt_long(argc,argv,"abt:",long_options,&option_index);
		
		if(c==-1)
		break;		
	
		switch(c)
		{
			case 0: printf("option %s ",long_options[option_index].name);
				if(optarg)
				{
					config_file=optarg;
					printf("with arguments %s\n",optarg);
				}				
				else
				{
				printf("\nerror: Option requires argument\n");
				exit(EXIT_FAILURE);
				}
				break;
			default:
				fprintf(stderr, "Usage: %s --config.file=path_to_conf_file\n", argv[0]);
	                   	exit(EXIT_FAILURE);				
		}
	}
	
	if(optind<argc)
	{
		printf("Non-option arguments are also present\n");
		exit(EXIT_FAILURE);
	}
	
	if(config_file==NULL)
	{
		fprintf(stderr, "Usage: %s --config.file=path_to_conf_file\n", argv[0]);
		exit(EXIT_FAILURE);						
	}
	
	src=(char *)malloc(50*sizeof(char));
	dest=(char *)malloc(50*sizeof(char));
	detach=(char *)malloc(10*sizeof(char));	

	parse_file(config_file); // extracting data from configuration file

	printf("time=%dsec, source=%s, dest=%s, detach=%s\n",tsec,src,dest,detach);	
/*
	xpid=fork();

	if(xpid<0)
	{
		
		fprintf(stderr,"error: fork() for ssh-keygen failure.\n");
		exit(EXIT_FAILURE);	
	}
	else if(xpid==0)
	{
		printf("\n******One time password*********\n");
		printf("Public key creation.\n");
		printf("After that copy the public key to the server's '.ssh/authorized_key'.\n");
		sleep(2);
		if(execl(onetimepass,"./onetimepass.sh",(char *)0)==-1)
		{
			fprintf(stderr,"error: exec() failure check the filename or path.\n");
		}	
		sleep(2);	
		exit(20);
	}

	waitpid(-1,&x_status,0);	//wait for the child to terminate.
*/	
	if(strcmp(detach,"true")==0) // if detach=true, then make the process as daemon
	{
		pid=fork();
		if(pid<0)
		{printf("error: daemon failure.\n"); exit(EXIT_FAILURE);}
		else if(pid>0)
		{
			printf("Now going to be a daemon... PID is %d\n",pid);
			exit(EXIT_SUCCESS);
		}

		umask(0);
		pid_t sid=setsid();
		if(sid<0)
		{
			print_err("error setsid() - exiting..\n");
			exit(EXIT_FAILURE);
		}
		
		if(chdir("/")<0)
		{
			print_err("error changing to '/' - exiting..\n");
			exit(EXIT_FAILURE);
		}
		
		if((fd=open("/dev/null",O_WRONLY))==-1)
		{
			print_err("error opening /dev/null - exiting\n");
			exit(EXIT_FAILURE);
		}		
			
		fflush(stdout);

		if(dup2(fd,1)==-1)
		{
			print_err("error in dup2() - exiting\n");
			exit(EXIT_FAILURE);			
		}

		close(STDIN_FILENO);
		//close(STDOUT_FILENO);
		close(STDERR_FILENO);
		main1(); // main extension		
	}
	else
	{	
		main1(); // main extension
	}

return 0;	
}
