#include "config.h"
#include "support.h"
#include "simple_rec.h"
#include <atl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* winsock2.h and ws2tcpip.h are included via support.h for Windows */

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_WINDOWS_H
#define drand48() (((double)rand())/((double)RAND_MAX))
#define lrand48() rand()
#define srand48(x)
#endif

/* Global variables */
int quiet = 1;
int regression = 1;
char *transport = NULL;
char *control = NULL;
char *argv0 = NULL;
int no_fork = 0;
void (*on_exit_handler)(void) = NULL;

static char *ssh_args[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
static char remote_directory[1024] = "";

/* DFG test support variables */
static pid_t *pid_list = NULL;
static int start_subproc_arg_count = 0;
static char **subproc_args = NULL;
static int cur_subproc_arg = 0;

void
set_ssh_args(char *destination_host, char *ssh_port)
{
    ssh_args[0] = strdup(SSH_PATH);
    ssh_args[1] = destination_host;
    ssh_args[2] = NULL;
    if (ssh_port != NULL) {
	ssh_args[2] = "-p";
	ssh_args[3] = ssh_port;
	ssh_args[4] = NULL;
    }
}

void
set_remote_directory(const char *dir)
{
    strcpy(remote_directory, dir);
}

void
usage(void)
{
    printf("Usage:  %s <options> \n", argv0);
    printf("  Options:\n");
    printf("\t-q  quiet\n");
    printf("\t-v  verbose\n");
    printf("\t-n  No regression test.  I.E. just run the master and print \n\t\twhat command would have been run for client.\n");
    printf("\t-ssh <hostname>:<ssh_port>:<remote directory>  parameters to use for remote client via ssh.\n");
    printf("\t-ssh <hostname>:<remote directory>  parameters to use for remote client via ssh.\n");

    exit(1);
}

#ifdef _MSC_VER
int inet_aton(const char* cp, struct in_addr* addr)
{
    addr->s_addr = inet_addr(cp);
    return (addr->s_addr == INADDR_NONE) ? 0 : 1;
}
#endif

pid_t
run_subprocess(char **args)
{
#ifdef HAVE_WINDOWS_H
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char comm_line[8191];

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );
    char module[MAX_PATH];
    GetModuleFileName(NULL, &module[0], MAX_PATH);
    int i = 1;
    strcpy(comm_line, module);
    strcat(comm_line, " ");
    while (args[i] != NULL) {
      strcat(comm_line, args[i]);
      strcat(comm_line, " ");
      i++;

    }
    if (quiet <= 0) {
        printf("Subproc arguments are: %s\n", comm_line);
    }
    if (!CreateProcess(module,
		       comm_line,
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
		       &pi )
    )
    {
        printf( "CreateProcess failed (%lu).\n", GetLastError() );
	printf("Args were argv[0] = %s\n", args[0]);
	printf("Args were argv[1] = %s, argv[2] = %s\n", args[1], args[2]);
        return 0;
    }
    return (intptr_t) pi.hProcess;
#else
    pid_t child;
    char **run_args = args;
    if (quiet <=0) {printf("Forking subprocess\n");}
    if (ssh_args[0] != NULL) {
        int i=0, j=0;
        int count = 0;
	while(args[count] != NULL) count++;
	count+= 6; /* enough */
	run_args = malloc(count * sizeof(run_args[0]));
	while(ssh_args[i]) {
	    run_args[i] = ssh_args[i];
	    i++;
	}
	if (remote_directory[0] != 0) {
	  if (strrchr(argv0, '/')) argv0 = strrchr(argv0, '/') + 1;
	  run_args[i] = malloc(strlen(remote_directory) +
			       strlen(argv0) + 4);
	  strcpy(run_args[i], remote_directory);
	  if (remote_directory[strlen(remote_directory)-1] != '/')
	    strcat(run_args[i], "/");
	  strcat(run_args[i], argv0);
	} else {
	  run_args[i] = argv0;
	}
	i++;
	while(args[j+1]) {
	    run_args[i] = args[j+1];
	    i++; j++;
	}
	run_args[i] = NULL;
    } else {
        run_args[0] = argv0;
    }
    if (quiet <= 0) {
        int i=0;
	printf("Subproc arguments are: ");
	while(run_args[i]) {
	    printf("%s ", run_args[i++]);
	}
	printf("\n");
    }
    if (no_fork) {
	child = -1;
    } else {
        child = fork();
	if (child == 0) {
	    /* I'm the child */
	    execv(run_args[0], run_args);
	}
    }
    return child;
#endif
}

/*
 * Cross-platform wait for subprocess.
 * Returns: pid on success, 0 if non-blocking and child not exited, -1 on error.
 * exit_state is encoded like waitpid - use WIFEXITED/WEXITSTATUS/etc to analyze.
 */
pid_t
wait_for_subprocess(pid_t proc, int *exit_state, int block)
{
#ifdef HAVE_WINDOWS_H
    DWORD timeout = block ? INFINITE : 0;
    DWORD wait_result = WaitForSingleObject((HANDLE)proc, timeout);

    if (wait_result == WAIT_OBJECT_0) {
	DWORD child_exit_code;
	GetExitCodeProcess((HANDLE)proc, &child_exit_code);
	CloseHandle((HANDLE)proc);
	/* Encode exit status like Linux: exit_code in bits 8-15, 0 in bits 0-7 */
	*exit_state = (int)(child_exit_code << 8);
	return proc;
    } else if (wait_result == WAIT_TIMEOUT) {
	/* Non-blocking and child still running */
	return 0;
    } else {
	/* WAIT_FAILED or other error */
	return -1;
    }
#else
    return waitpid(proc, exit_state, block ? 0 : WNOHANG);
#endif
}

int
verify_simple_record(simple_rec_ptr event)
{
    long sum = 0;
    sum += event->integer_field % 100;
    sum += event->short_field % 100;
    sum += event->long_field % 100;
    sum += ((int) (event->nested_field.item.r * 100.0)) % 100;
    sum += ((int) (event->nested_field.item.i * 100.0)) % 100;
    sum += ((int) (event->double_field * 100.0)) % 100;
    sum += event->char_field;
    sum = sum % 100;
    return (sum == event->scan_sum);
}

extern int
checksum_simple_record(simple_rec_ptr event, attr_list attrs, int quiet_level)
{
    long sum = 0, scan_sum = 0;
    sum += event->integer_field % 100;
    sum += event->short_field % 100;
    sum += event->long_field % 100;
    sum += ((int) (event->nested_field.item.r * 100.0)) % 100;
    sum += ((int) (event->nested_field.item.i * 100.0)) % 100;
    sum += ((int) (event->double_field * 100.0)) % 100;
    sum += event->char_field;
    sum = sum % 100;
    scan_sum = event->scan_sum;
    if (sum != scan_sum) {
        printf("Received record checksum does not match. expected %d, got %d\n",
               (int) sum, (int) scan_sum);
    }
    if ((quiet_level <= 0) || (sum != scan_sum)) {
        printf("In the handler, event data is :\n");
        printf("	integer_field = %d\n", event->integer_field);
        printf("	short_field = %d\n", event->short_field);
        printf("	long_field = %ld\n", event->long_field);
        printf("	double_field = %g\n", event->double_field);
        printf("	char_field = %c\n", event->char_field);
        printf("Data was received with attributes : \n");
        if (attrs) dump_attr_list(attrs);
    }
    return (sum == scan_sum);
}

FMField nested_field_list[] =
{
    {"item", "complex", sizeof(complex), FMOffset(nested_ptr, item)},
    {NULL, NULL, 0, 0}
};

FMField complex_field_list[] =
{
    {"r", "double", sizeof(double), FMOffset(complex_ptr, r)},
    {"i", "double", sizeof(double), FMOffset(complex_ptr, i)},
    {NULL, NULL, 0, 0}
};

FMField simple_field_list[] =
{
    {"integer_field", "integer",
     sizeof(int), FMOffset(simple_rec_ptr, integer_field)},
    {"short_field", "integer",
     sizeof(short), FMOffset(simple_rec_ptr, short_field)},
    {"long_field", "integer",
     sizeof(long), FMOffset(simple_rec_ptr, long_field)},
    {"nested_field", "nested",
     sizeof(nested), FMOffset(simple_rec_ptr, nested_field)},
    {"double_field", "float",
     sizeof(double), FMOffset(simple_rec_ptr, double_field)},
    {"char_field", "char",
     sizeof(char), FMOffset(simple_rec_ptr, char_field)},
    {"scan_sum", "integer",
     sizeof(int), FMOffset(simple_rec_ptr, scan_sum)},
    {NULL, NULL, 0, 0}
};

FMStructDescRec simple_format_list[] =
{
    {"simple", simple_field_list, sizeof(simple_rec), NULL},
    {"complex", complex_field_list, sizeof(complex), NULL},
    {"nested", nested_field_list, sizeof(nested), NULL},
    {NULL, NULL}
};

void
generate_simple_record(simple_rec_ptr event)
{
    long sum = 0;
    memset(event, 0, sizeof(*event));
    event->integer_field = (int) lrand48() % 100;
    sum += event->integer_field % 100;
    event->short_field = ((short) lrand48());
    sum += event->short_field % 100;
    event->long_field = ((long) lrand48());
    sum += event->long_field % 100;

    event->nested_field.item.r = drand48();
    sum += ((int) (event->nested_field.item.r * 100.0)) % 100;
    event->nested_field.item.i = drand48();
    sum += ((int) (event->nested_field.item.i * 100.0)) % 100;

    event->double_field = drand48();
    sum += ((int) (event->double_field * 100.0)) % 100;
    event->char_field = lrand48() % 128;
    sum += event->char_field;
    sum = sum % 100;
    event->scan_sum = (int) sum;
}

/*
 * DFG test support functions
 */

void
set_subproc_args(int argc, char **argv)
{
    start_subproc_arg_count = 8; /* leave a few open at the beginning for ssh args */
    subproc_args = calloc((argc + start_subproc_arg_count + 8), sizeof(argv[0]));
    cur_subproc_arg = start_subproc_arg_count;

    /* Set up the initial subprocess command (argv0) */
    if (remote_directory[0] != 0) {
        char *base = argv0;
        if (strrchr(base, '/')) base = strrchr(base, '/') + 1;
        if (strrchr(base, '\\')) base = strrchr(base, '\\') + 1;
        subproc_args[cur_subproc_arg] = malloc(strlen(remote_directory) + strlen(base) + 4);
        strcpy(subproc_args[cur_subproc_arg], remote_directory);
        if (remote_directory[strlen(remote_directory)-1] != '/' &&
            remote_directory[strlen(remote_directory)-1] != '\\')
            strcat(subproc_args[cur_subproc_arg], "/");
        strcat(subproc_args[cur_subproc_arg], base);
    } else {
        subproc_args[cur_subproc_arg] = argv0;
    }
    subproc_args[cur_subproc_arg+1] = NULL;
    cur_subproc_arg++;
}

void
test_fork_children(char **list, char *master_contact)
{
    char *args[20];
    int i = 0;
    int list_index = 1;
    static int pid_index = 1;

    if (!subproc_args) return;

    /* assume that we are list[0] */
    while(subproc_args[start_subproc_arg_count + i] != NULL) {
        args[i] = subproc_args[start_subproc_arg_count+i];
        i++;
    }
    if (quiet < 1) {
        args[i++] = "-v";
    }
    args[i++] = "-c";
    args[i+1] = master_contact;
    args[i+2] = NULL;
    if (!pid_list) pid_list = malloc(sizeof(pid_list[0]));
    while(list[list_index] != NULL) {
        args[i] = list[list_index];
        pid_list[pid_index - 1] = run_subprocess(args);
        list_index++;
        pid_index++;
        pid_list = realloc(pid_list, sizeof(pid_list[0]) * pid_index);
    }
    pid_list[pid_index - 1] = 0;
}

typedef void (*CMPollFunc)(void *cm, void *client_data);
typedef void *CMTaskHandle;
extern CMTaskHandle CMadd_delayed_task(void *cm, int secs, int usecs, CMPollFunc func, void *client_data);

static void
delay_fork_wrapper(void *cm, void *client_data)
{
    delay_struct *str = (delay_struct*)client_data;
    (void) cm;
    test_fork_children(str->list, str->master_contact);
    free(str);
}

void
delayed_fork_children(struct _CManager *cm, char **list, char *master_contact, int delay_seconds)
{
    delay_struct *str = malloc(sizeof(delay_struct));
    str->list = list;
    str->master_contact = master_contact;
    CMTaskHandle handle = CMadd_delayed_task((void*)cm, delay_seconds, 0, delay_fork_wrapper, (void*) str);
    (void) handle;
}

int
wait_for_children(char **list)
{
    int i = 0, stat;
    (void)list;
    while(pid_list && pid_list[i] != 0) {
        wait_for_subprocess(pid_list[i], &stat, 1);
        i++;
    }
    free(pid_list);
    pid_list = NULL;
    return 0;
}
