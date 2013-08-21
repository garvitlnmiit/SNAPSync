#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "user_sync.h"

void timeout_handler(int);
static int update_check(int);
static int copy_to_del(void);
static int copy_to_upd(void);
static void erase_data(char *);
static void check_file(int);
static void get_fname(char *, int);
static int if_contain(char *);
static int if_exists(int, int);
static void get_data(int);

char log_ct[200][200];
int ind=0;

static int if_exists(int inx, int val)
{
	FILE *fp;
	char c,buf[100];
	int j=0;
	switch(val)
	{
		case 0: 
			fp=fopen(ful,"r");
			while((c=fgetc(fp))!=EOF)
			{
				if(c=='\n')
				{
					buf[j]='\n';
					buf[j+1]='\0';
					if(strcmp(buf,u_files[inx])==0)
						return 1;
					j=0;
					buf[0]='\0';
				}
				else
					buf[j++]=c;
			}
			fclose(fp);
			break;
		case 1:
			fp=fopen(fdl,"r");
			while((c=fgetc(fp))!=EOF)
			{
				if(c=='\n')
				{
					buf[j]='\n';
					buf[j+1]='\0';
					if(strcmp(buf,d_files[inx])==0)
						return 1;
					j=0;
					buf[0]='\0';
				}
				else
					buf[j++]=c;
			}
			fclose(fp);		
	}
return 0;
}

static void erase_data(char *filename)
{
	int ret;
	ret=open(filename,O_RDWR | O_TRUNC);
	if(ret<0)
	{
		print_err("error in flushing the content of update.log or delete.log - exiting...\n");
		exit(1);
	}
	return;
}

static int copy_to_del()
{
	int i,ret;
	for(i=0;i<d_cnt;i++)
	{
		if(if_exists(i,1)!=0)
			continue;	
		ret=write(fd_delete,d_files[i],strlen(d_files[i]));
		if(ret==-1)
		{
			print_err("failed to write in delete.log\n");
			exit(1);
		}
		d_files[i][0]='\0';		
	}
	d_cnt=0;
	return 1;	
}

static int copy_to_upd()
{
	int i,ret;
	for(i=0;i<u_cnt;i++)
	{
		if(if_exists(i,0)!=0)
			continue;	
		ret=write(fd_update,u_files[i],strlen(u_files[i]));
		if(ret==-1)
		{
			print_err("failed to write in update.log\n");
			exit(1);
		}
		u_files[i][0]='\0';
	}
	u_cnt=0;
	return 1;	
}


static int update_check(int val)
{
	char tmp[50];
	switch(val)
	{
		// update
		case 0: if(u_cnt>0)
			return copy_to_upd();
			lseek(fd_update,0,SEEK_SET);
			if(read(fd_update,tmp,50)>0)
			return 1;
			break;
		// delete
		case 1: if(d_cnt>0)
			return copy_to_del();
			lseek(fd_delete,0,SEEK_SET);
			if(read(fd_delete,tmp,50)>0)
			return 1;
			break;
	}


return 0;
}

static void get_fname(char line[], int len)
{
	int k=0;
	//len=strlen(line);
	while(line[--len]!=' ');
	while(line[++len]!='\0')
	{
		log_ct[ind][k++]=line[len];
	}
	log_ct[ind][k]='\0';
	printf(">> %s\n", log_ct[ind]);
	ind++;
}

static void get_data(int val)
{
	char c, buf[200];
	int j=0;
	FILE *fp;
	fp=fopen(fll,"r");
	
	switch(val)
	{
	case 0:
	while((c=fgetc(fp))!=EOF)
	{
		if(c=='\n')
		{
			buf[j]='\0';
			if(strstr(buf,"<f++++")!=NULL)
				get_fname(buf,strlen(buf));
			j=0;
			buf[0]='\0';
		}
		else
		buf[j++]=c;
	}
	fclose(fp);
	break;
	
	case 1:
	while((c=fgetc(fp))!=EOF)
	{
		if(c=='\n')
		{
			buf[j]='\0';
			if(strstr(buf,"*deleting")!=NULL)
				get_fname(buf,strlen(buf));
			j=0;
			buf[0]='\0';
		}
		else
		buf[j++]=c;
	}
	fclose(fp);
	}
}

static int if_contain(char *buf)
{
	int i, flag=0;
	for(i=0;i<ind;i++)
		if(strcmp(buf,log_ct[i])==0)
		{
			//log_ct[i][0]='\0';
			flag=1;
			break;
		}
return flag;
}

static void check_file(int val)
{
	char c, buf[100],temp[100][100];
	int tcn=0,i,j=0,len_t;
	FILE *fp;
	ind=0;
	get_data(val);

	switch(val)
	{
	case 0:	
	fp=fopen(ful,"r");

	while((c=fgetc(fp))!=EOF)
	{
		if(c=='\n')
		{
			buf[j]='\0';
			if(if_contain(buf)==0) // file not transferred, storing it in a temp;
				strcpy(temp[tcn++], buf);
			j=0;
			buf[0]='\0';
		}
		else
		buf[j++]=c;
	}
	fclose(fp);
	erase_data(ful);
	erase_data(fll);
	//printf("erasing done!! %d\n",tcn);
	for(i=0;i<tcn;i++)
	{
		len_t=strlen(temp[i]);
		temp[i][len_t]='\n';
		temp[i][len_t+1]='\0';
		if(write(fd_update,temp[i],len_t+1)<0)
		print_err("error in writing not updated files to update.log\n");		
	}

	for(i=0;i<ind;i++)
		log_ct[i][0]='\0';
	break;
	
	case 1:
	fp=fopen(fdl,"r");

	while((c=fgetc(fp))!=EOF)
	{
		if(c=='\n')
		{
			buf[j]='\0';
			if(if_contain(buf)==0) // file not transferred, storing it in a temp;
				strcpy(temp[tcn++], buf);
			j=0;
			buf[0]='\0';
		}
		else
		buf[j++]=c;
	}
	fclose(fp);
	erase_data(ful);
	erase_data(fll);
	//printf("erasing done!! %d\n",tcn);
	for(i=0;i<tcn;i++)
	{
		len_t=strlen(temp[i]);
		temp[i][len_t]='\n';
		temp[i][len_t+1]='\0';
		if(write(fd_delete,temp[i],len_t+1)<0)
		print_err("error in writing not updated files to update.log\n");		
	}

	for(i=0;i<ind;i++)
		log_ct[i][0]='\0';		
	}
	//printf("complete\n");
}

void timeout_handler(int signo)
{	
	if(signo!=SIGALRM)
		return;

	int argc,retval;
	char *argv[10], sr[100], dt[100];
	pid_t pid;
	strcpy(sr,src);
	strcpy(dt,dest);

	int cpos1=0,cpos2=0;

	cpos1=update_check(0);
	cpos2=update_check(1);
	
	//printf("printing...\n");
	
	if(cpos1==0 && cpos2==0)
	{
		print_err("rsync: no updates and no deletes. - returning..\n");
		alarm(tsec);	
		return;
	}

	if(cpos1!=0)
	{
	pid=fork();
	if(pid==0)
	{
		argv[0]="rsync";
		argv[1]="-av";
		argv[2]="--timeout=60";
		argv[3]=log_f;
		argv[4]=files_f;
		argv[5]=sr;
		argv[6]=dt;
		argc=7;

		retval=sync_main(argc,argv);

		switch(retval)
		{
			case 13:print_err("Updates transferred successfully - flushing update.log...\n");
				erase_data(ful);
				erase_data(fll);	
				break;

			case -17:print_err("failed to establish connection, nothing done - returning...\n");
				erase_data(fll);
				break;
	
			case -19:print_err("Connection lost during the transmission - checking files...\n");
				check_file(0);
		}
		exit(12);
	}
	wait(NULL);
	}
	/**/
	if(cpos2!=0)
	{
	pid=fork();
	if(pid==0)
	{
		argv[0]="rsync";
		argv[1]="-av";
		argv[2]="--timeout=60";
		argv[3]="--delete";
		argv[4]=delete_files;
		argv[5]=log_f;
		argv[6]="--exclude=*";
		argv[7]=sr;
		argv[8]=dt;
		argc=9;

		retval=sync_main(argc,argv);

		switch(retval)
		{
			case 13:print_err("Deleted successfully - flushing delete.log...\n");
				erase_data(fdl);
				erase_data(fll);	
				break;

			case -17:print_err("failed to establish connection, nothing done - returning...\n");
				erase_data(fll);
				break;
	
			case -19:print_err("Connection lost during the transmission - checking files...\n");
				check_file(1);
		}
		exit(12);
	}
	wait(NULL);
	}
	/**/
	alarm(tsec);

return;	
}
