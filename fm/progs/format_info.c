
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include <string.h>
#include "fm.h"
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#define sleep(x) Sleep(1000*x)
#else
#include <netinet/in.h>
#include <arpa/inet.h>
extern int sleep();
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "fm_internal.h"
extern int (*establish_server_connection_ptr)(FMContext fmc, action_t action);

static int
serverAtomicWrite(fd, buffer, length)
void* fd;
void *buffer;
int length;
{
     char *junk_result_str;
     int junk_errno;
     return os_server_write_func(fd, buffer, length, &junk_errno,
				&junk_result_str);
}

int
main(argc, argv)
int argc;
char **argv;
{
    FMContext context = create_FMcontext();
    FMFormat ioformat = NULL;
    char *colon;
    if (argc <= 1) {
	printf("Usage:   format_info <global_format_name>\n");
	return 1;
    }
    colon = strchr(argv[1], ':');
    if (colon) {
	int i, len = strlen(++colon);
	unsigned char *tmp = malloc(len/2);
	for (i=0; i< len/2; i++) {
	    int tmpi;
	    sscanf(&colon[i*2], "%02x", &tmpi);
	    tmp[i] = tmpi;
	}
	ioformat = FMformat_from_ID(context, (char*)tmp);
    }
    if (ioformat == NULL) {
	printf("Format_info:  \"%s\" is not an ioformat name\n",
	       argv[1]);
	return 1;
    }
    dump_FMFormat(ioformat);
    return 0;
}	
