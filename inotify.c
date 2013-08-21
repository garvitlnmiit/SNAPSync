#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "inotify.h"	/* for macros, prototype and other headers */
#include "user_sync.h"

char u_files[file_name_size][total_files],
	d_files[file_name_size][total_files];

int u_cnt=0,
	d_cnt=0;

static int if_exists(char *fname, int val)
{
	int i=0;

	switch(val)
	{
		case 0: for(i=0;i<u_cnt;i++)
				if(strcmp(u_files[i],fname)==0)
					return 1;
			break;
				
		case 1: for(i=0;i<d_cnt;i++)
				if(strcmp(d_files[i],fname)==0)
					return 1;
	}
	return 0;
}

static void filter(char *filename,char *operation)
{
	char file_m[100],buffer[100];
	int len;
	printf("%s %s\n",filename,operation);
	if(*(filename+0)!='.')
	{
		if(strstr(filename,"~")!=NULL && strcmp(operation,"CREATED")==0)
		{
			len=strlen(filename);
			*(filename+len-1)='\0';
			strcpy(buffer,filename);
			strcat(buffer,"\n");
			if(if_exists(buffer,0)==0)
			strcpy(u_files[u_cnt++],buffer);
			return;
		}
		strcpy(buffer,filename);
		strcat(buffer,"\n");

		if(strcmp(operation,"DELETED")==0)
		{
			if(if_exists(buffer,1)==0)
			strcpy(d_files[d_cnt++],buffer);
			return;
		}
		else
		{
			if(if_exists(buffer,0)==0)
			strcpy(u_files[u_cnt++],buffer);
			return;
		}
	}
	else
	{
		if(strstr(filename,"swp")!=NULL && strcmp(operation,"MODIFIED")==0)
		{
			sscanf(filename,".%[^.].swp",file_m);
			len=strlen(file_m);
			*(file_m+len)='\n';
			*(file_m+len+1)='\0';

			if(if_exists(file_m,0)==0)
			strcpy(u_files[u_cnt++],file_m);
		}
	}

return;
}

static char *dir_path(char *temp, char *parent, char *child)
{
	int len;
	temp=(char *)calloc(strlen(parent)+strlen(child)+5, sizeof(char *));
	strcpy(temp,parent);
	strcat(temp,child);
	len=strlen(temp);
	if(*(temp+len-1)!='/')
	{
		*(temp+len)='/';
		*(temp+len+1)='\0';
	}
	//printf("%s\n",temp);
	return temp;			
}

static char *relative_path_dir(char *temp, int len)
{
	int len_t,i;
	char *relp;	
	len_t=strlen(temp);
	relp=(char *)calloc(len_t-len+2, sizeof(char *));
	for(i=len;*(temp+i)!='\0';i++)
		*(relp+i-(len))=*(temp+i);
	*(relp+i-(len))='\0';
	return relp;
}

static char *relative_path_file(char *temp, int len, char *fname)
{
	int len_t,i, len_f;
	char *relp;	
	len_t=strlen(temp);
	len_f=strlen(fname);
	relp=(char *)calloc(len_t-len+len_f+2, sizeof(char *));
	for(i=len;*(temp+i)!='\0';i++)
		*(relp+i-(len))=*(temp+i);
	*(relp+i-(len))='\0';
	strcat(relp,fname);
	return relp;
}

void *inotify_main()
{
	fd_set rfds;
	int wd1,length;//i=0, retval,temp_len;	
	struct inotify_event *event;	
	char buffer[EVENT_BUF_LEN], *p, *temp=NULL, *rel_f, *rel_d;
	//arg=NULL;
	//arg="no_use";
	
	FD_ZERO(&rfds);

	while(1)       
	{
		length = read( infd, buffer, EVENT_BUF_LEN ); 

		/*checking for error*/
		if ( length < 0 ) 
		{
			print_err("read() inotify_init error - exiting\n");
			exit(EXIT_FAILURE);
		}
  
		for(p=buffer; p<buffer+length;)
		{		
			event = ( struct inotify_event * ) p;  
   			
			if (event->len) 
			{
				if (event->mask & IN_CREATE) 
				{
        				if ( event->mask & IN_ISDIR )
					{	
						temp=dir_path(temp,prefix[event->wd],event->name);
						wd1=inotify_add_watch(infd, temp, standard_event_mask);
						prefix[wd1]=temp;
						//printf("%s\n",temp);
						rel_d=relative_path_dir(temp, strlen(prefix[wd_src]));
						filter(rel_d,"CREATED");
					}
					else
					{
						rel_f=relative_path_file(prefix[event->wd], strlen(prefix[wd_src]), event->name);
						filter(rel_f,"CREATED");
					}
				}
				else if ( event->mask & IN_DELETE ) 
				{
					if ( event->mask & IN_ISDIR ) 
						filter(event->name,"DELETED");
        				else{
						rel_f=relative_path_file(prefix[event->wd], strlen(prefix[wd_src]), event->name); 
						filter(rel_f,"DELETED");
					}	
				}
				else if ( event->mask & IN_CLOSE_WRITE )
                	        {
                	                if ( event->mask & IN_ISDIR ) 
						filter(event->name,"MODIFIED");
                	                else{
						rel_f=relative_path_file(prefix[event->wd], strlen(prefix[wd_src]), event->name); 
						filter(rel_f,"MODIFIED");
					}
                	        }/**/		
				else if ( event->mask & IN_MOVED_FROM )
                	       	{
                	                if ( event->mask & IN_ISDIR )	
						filter(event->name,"DELETED");
                	                else{
						rel_f=relative_path_file(prefix[event->wd], strlen(prefix[wd_src]), event->name);
						filter(rel_f,"DELETED");
					}
                	        }
				else if ( event->mask & IN_MOVED_TO )
                	        {	
                	                if ( event->mask & IN_ISDIR ) 
						filter(event->name,"CREATED");
                	                else{
						rel_f=relative_path_file(prefix[event->wd], strlen(prefix[wd_src]), event->name); 
						filter(rel_f,"CREATED");
					}
                	        }		
			}	/**/
			p += EVENT_SIZE + event->len;
		}

		buffer[0]='\0';
	}
 
	inotify_rm_watch( infd, wd_src );

	/*closing the INOTIFY instance*/
	close(infd);
        
return 0;
}
