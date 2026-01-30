#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define pid_t intptr_t
#include <process.h>
/* inet_aton is not available on Windows, provide our own */
struct in_addr;
extern int inet_aton(const char* cp, struct in_addr* addr);
#endif

#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
/* Define waitpid-style macros for Windows */
#ifndef WIFEXITED
#define WIFEXITED(s)    (((s) & 0x7f) == 0)
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(s)  (((s) >> 8) & 0xff)
#endif
#ifndef WIFSIGNALED
#define WIFSIGNALED(s)  (((s) & 0x7f) != 0 && ((s) & 0x7f) != 0x7f)
#endif
#ifndef WTERMSIG
#define WTERMSIG(s)     ((s) & 0x7f)
#endif
#ifndef WNOHANG
#define WNOHANG 1
#endif
#define kill(x,y) TerminateProcess((HANDLE)(x), y)
#else
#include <sys/wait.h>
#endif

/* Variables used by PARSE_ARGS - defined in support.c */
extern int quiet;
extern int regression;
extern char *transport;
extern char *control;
extern char *argv0;
extern int no_fork;
extern void (*on_exit_handler)(void);

/* Function prototypes */
extern void usage(void);
extern pid_t run_subprocess(char **args);
extern pid_t wait_for_subprocess(pid_t proc, int *exit_state, int block);

/* DFG test support - for tests that fork multiple children */
struct _CManager;  /* forward declaration */
typedef struct _delay_struct {
    char **list;
    char *master_contact;
} delay_struct;

extern void test_fork_children(char **list, char *master_contact);
extern void delayed_fork_children(struct _CManager *cm, char **list, char *master_contact, int delay_seconds);
extern int wait_for_children(char **list);
extern void set_subproc_args(int argc, char **argv);

#ifndef PARSE_EXTRA_ARGS
#define PARSE_EXTRA_ARGS
#endif

#define PARSE_ARGS() \
    argv0 = argv[0];\
    while (argv[1] && (argv[1][0] == '-')) {\
	if (strcmp(&argv[1][1], "control") == 0) {\
	    control = argv[2];\
	    argv++;\
	    argc--;\
	} else if (argv[1][1] == 'c') {\
	    regression_master = 0;\
	} else if (strcmp(&argv[1][1], "ssh") == 0) {\
	    char *destination_host;\
	    char *first_colon, *second_colon;\
	    char *ssh_port = NULL;\
	    if (!argv[2]) {\
	        printf("Missing --ssh destination\n");\
		usage();\
	    }\
	    first_colon = strchr(argv[2], ':');\
	    if (first_colon) {\
	        *first_colon = 0;\
		second_colon = strchr(first_colon+1, ':');\
	    } else {\
	        second_colon = NULL;\
	    }\
	    destination_host = strdup(argv[2]);\
	    if (first_colon) {\
	        int ssh_port_int;\
		if (second_colon) *second_colon = 0;\
		if (sscanf(first_colon+1, "%d", &ssh_port_int) != 1) {\
		    second_colon = first_colon;\
		}  else {\
		    ssh_port = first_colon + 1;\
		}\
	    }\
	    if (second_colon) {\
	        set_remote_directory(second_colon+1);\
	    }\
	    if (strlen(SSH_PATH) == 0) {\
		printf("SSH_PATH in config.h is empty!  Can't run ssh\n");\
		exit(1);\
	    }\
	    set_ssh_args(destination_host, ssh_port);\
	    argv++; argc--;\
	PARSE_EXTRA_ARGS\
	} else if (argv[1][1] == 's') {\
	    regression_master = 0;\
	} else if (argv[1][1] == 'q') {\
	    quiet++;\
	} else if (argv[1][1] == 'v') {\
	    quiet--;\
	} else if (argv[1][1] == 'n') {\
	    regression = 0;\
	    no_fork = 1;\
	    quiet = -1;\
	} else if (argv[1][1] == 't') {\
	    transport = argv[2];\
	    argv++;\
	    argc--;\
	}\
	argv++;\
	argc--;\
    }

/* Helper functions for SSH setup used by PARSE_ARGS macro */
extern void set_ssh_args(char *destination_host, char *ssh_port);
extern void set_remote_directory(const char *dir);

#endif /* TEST_SUPPORT_H */
