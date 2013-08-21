#include <errno.h>
#include <sys/inotify.h>
#include <unistd.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

void *inotify_main();

int infd, //inotify filedesc
     wd_src; //watch filedesc	

char *prefix[1024];

const uint32_t standard_event_mask;
