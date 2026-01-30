#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "evpath.h"
#include "test_support.h"

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#define getpid() _getpid()
/* srand48 is defined in simple_rec.h */
#endif

static pid_t subproc_proc = 0;

static void
fail_and_die(int signal)
{
    (void) signal;
    fprintf(stderr, "EVPath DFG test failed to complete in reasonable time\n");
    if (on_exit_handler) on_exit_handler();
    if (subproc_proc != 0) {
        kill(subproc_proc, 9);
    }
    exit(1);
}

int
main(int argc, char **argv)
{
    int regression_master = 1;
    int ret = 0;

    argv0 = argv[0];

    while (argv[1] && (argv[1][0] == '-')) {
        if (argv[1][1] == 'c') {
            regression_master = 0;
        } else if (argv[1][1] == 'q') {
            quiet++;
        } else if (argv[1][1] == 'v') {
            quiet--;
        } else if (argv[1][1] == 'n') {
            regression = 0;
            quiet = -1;
            no_fork = 1;
        } else if (strcmp(&argv[1][1], "ssh") == 0) {
            char *destination_host;
            char *first_colon, *second_colon;
            char *ssh_port = NULL;
            if (!argv[2]) {
                printf("Missing --ssh destination\n");
                usage();
            }
            first_colon = strchr(argv[2], ':');
            if (first_colon) {
                *first_colon = 0;
                second_colon = strchr(first_colon+1, ':');
            } else {
                second_colon = NULL;
            }
            destination_host = strdup(argv[2]);
            if (first_colon) {
                int ssh_port_int;
                if (second_colon) *second_colon = 0;
                if (sscanf(first_colon+1, "%d", &ssh_port_int) != 1) {
                    second_colon = first_colon;
                } else {
                    ssh_port = first_colon + 1;
                }
            }
            if (second_colon) {
                set_remote_directory(second_colon+1);
            }
            if (strlen(SSH_PATH) == 0) {
                printf("SSH_PATH in config.h is empty!  Can't run ssh\n");
                exit(1);
            }
            set_ssh_args(destination_host, ssh_port);
            argv++; argc--;
        } else if (argv[1][1] == '-') {
            argv++;
            argc--;
            break;
        }
        argv++;
        argc--;
    }

    set_subproc_args(argc, argv);
    srand48(getpid());

#ifdef HAVE_WINDOWS_H
    SetTimer(NULL, 5, 60000, (TIMERPROC) fail_and_die);
#else
    {
        struct sigaction sigact;
        sigact.sa_flags = 0;
        sigact.sa_handler = fail_and_die;
        sigemptyset(&sigact.sa_mask);
        sigaddset(&sigact.sa_mask, SIGALRM);
        sigaction(SIGALRM, &sigact, NULL);
        if (regression == 0) {
            alarm(600);
        } else {
            alarm(60);
        }
    }
#endif

    if (!regression_master) {
        ret = be_test_child(argc, argv);
    } else {
        ret = be_test_master(argc-1, &argv[1]);
    }
    return ret;
}
