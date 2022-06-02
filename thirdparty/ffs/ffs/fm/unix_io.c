#include "config.h"
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#define HAVE_IOVEC_DEFINE
#endif
#include <stdio.h>
#include "fm.h"
#include "fm_internal.h"
#include "assert.h"

#ifndef SELECT_DEFINED
extern int select(int width, fd_set *readfds, fd_set *writefds, 
		  fd_set *exceptfds, struct timeval *timeout);
#endif

static int
unix_read_func(conn, buffer, length, errno_p, result_p)
void *conn;
void *buffer;
int length;
int *errno_p;
char **result_p;
{
    int left = length;
    int iget;
    int fd = (int) (long) conn;

    iget = read(fd, (char *) buffer, length);
    if (iget == 0) {
	if (result_p) *result_p = "End of file";
	if (errno_p) *errno_p = 0;
	return 0;		/* end of file */
    } else if (iget == -1) {
	int lerrno = errno;
	if (errno_p) *errno_p = lerrno;
	if ((lerrno != EWOULDBLOCK) &&
	    (lerrno != EAGAIN) &&
	    (lerrno != EINTR)) {
	    /* serious error */
	    return -1;
	} else {
	    if (errno_p) *errno_p = 0;
	    iget = 0;
	}
    }
    left = length - iget;
    while (left > 0) {
	iget = read(fd, (char *) buffer + length - left, left);
	if (iget == 0) {
	    if (result_p) *result_p = "End of file";
	    if (errno_p) *errno_p = 0;
	    return length - left;	/* end of file */
	} else if (iget == -1) {
	    int lerrno = errno;
	    if (errno_p) *errno_p = errno;
	    if ((lerrno != EWOULDBLOCK) &&
		(lerrno != EAGAIN) &&
		(lerrno != EINTR)) {
		/* serious error */
		return (length - left);
	    } else {
		if (errno_p) *errno_p = 0;
		iget = 0;
	    }
	}
	left -= iget;
    }
    return length;
}

int
unix_timeout_read_func(conn, buffer, length, errno_p, result_p)
void *conn;
void *buffer;
int length;
int *errno_p;
char **result_p;
{
    int left = length;
    int count = 2;
    int iget;
    int fd = (int) (long) conn;
    int fdflags = fcntl(fd, F_GETFL, 0);

    fdflags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, fdflags) == -1) 
	perror("fcntl block");
    iget = read(fd, (char *) buffer, length);
    if (iget == 0) {
	if (result_p) *result_p = "End of file";
	if (errno_p) *errno_p = 0;
	fdflags &= ~O_NONBLOCK;
	if (fcntl(fd, F_SETFL, fdflags) == -1) 
	    perror("fcntl nonblock");
	return 0;		/* end of file */
    } else if (iget == -1) {
	int lerrno = errno;
	if (errno_p) *errno_p = lerrno;
	if ((lerrno != EWOULDBLOCK) &&
	    (lerrno != EAGAIN) &&
	    (lerrno != EINTR)) {
	    /* serious error */
	    fdflags &= ~O_NONBLOCK;
	    if (fcntl(fd, F_SETFL, fdflags) == -1) 
		perror("fcntl nonblock");
	    return -1;
	} else {
	    if (errno_p) *errno_p = 0;
	    iget = 0;
	}
    }
    left = length - iget;
    while (left > 0) {
	count--;
	if (count <= 0) {
	    fdflags &= ~O_NONBLOCK;
	    if (fcntl(fd, F_SETFL, fdflags) == -1) 
		perror("fcntl nonblock");
	    return -1;
	}
	sleep(1);
	iget = read(fd, (char *) buffer + length - left, left);
	if (iget == 0) {
	    if (result_p) *result_p = "End of file";
	    if (errno_p) *errno_p = 0;
	    fdflags &= ~O_NONBLOCK;
	    if (fcntl(fd, F_SETFL, fdflags) == -1) 
		perror("fcntl nonblock");
	    return length - left;	/* end of file */
	} else if (iget == -1) {
	    int lerrno = errno;
	    if (errno_p) *errno_p = errno;
	    if ((lerrno != EWOULDBLOCK) &&
		(lerrno != EAGAIN) &&
		(lerrno != EINTR)) {
		/* serious error */
		fdflags &= ~O_NONBLOCK;
		if (fcntl(fd, F_SETFL, fdflags) == -1) 
		    perror("fcntl nonblock");
		return (length - left);
	    } else {
		if (errno_p) *errno_p = 0;
		iget = 0;
	    }
	}
	left -= iget;
    }
    fdflags &= ~O_NONBLOCK;
    if (fcntl(fd, F_SETFL, fdflags) == -1) 
	perror("fcntl nonblock");
    return length;
}

static int
unix_write_func(conn, buffer, length, errno_p, result_p)
void *conn;
void *buffer;
int length;
int *errno_p;
char **result_p;
{
    int left = length;
    int iget = 0;
    int fd = (int) (long)conn;

    while (left > 0) {
	iget = write(fd, (char *) buffer + length - left, left);
	if (iget == -1) {
	    int lerrno = errno;
	    if (errno_p) *errno_p = errno;
	    if ((lerrno != EWOULDBLOCK) &&
		(lerrno != EAGAIN) &&
		(lerrno != EINTR)) {
		/* serious error */
		return (length - left);
	    } else {
		if (errno_p) *errno_p = 0;
		iget = 0;
	    }
	}
	left -= iget;
    }
    return length;
}

static int
unix_close_func(conn)
void *conn;
{
     return close((int) (long) conn);
}

static void *
unix_file_open_func(path, flag_str, input, output)
const char *path;
const char *flag_str;
int *input;
int *output;
{
    int flags;
    int file_id;
    long tmp_flags = (long)flag_str;
    *input = *output = 0;

    tmp_flags &= ~(O_TRUNC);
    tmp_flags &= ~(O_CREAT);
    if ((O_RDONLY == tmp_flags) ||
	(O_WRONLY == tmp_flags)) {
	 /* must be old style call */
	 flags = (long)flag_str;
	 *input = (O_RDONLY == (long)flag_str);
	 *output = (O_WRONLY & (long)flag_str);
    } else {
	 if (strcmp(flag_str, "r") == 0) {
	      flags = O_RDONLY;
	      *input = 1;
	 } else if (strcmp(flag_str, "w") == 0) {
	      flags = O_WRONLY | O_CREAT | O_TRUNC;
	      *output = 1;
	 } else {
	      fprintf(stderr, "Open flags value not understood for file \"%s\"\n",
		      path);
	      return NULL;
	 }
    }
    file_id = open(path, flags, 0777);
    if (file_id == -1) {
	 return NULL;
    } else {
	 return (void*) (long) file_id;
    }
}
    
static int
unix_poll_func(conn)
void *conn;
{
    int fd = (int) (long) conn;
    struct timeval time;
    fd_set read_fds;
    int ret_val;

    time.tv_sec = time.tv_usec = 0;
#ifdef __NVCOMPILER
#pragma diag_suppress 550
#endif
    FD_ZERO(&read_fds);
#ifdef __NVCOMPILER
#pragma diag_default 550
#endif
    FD_SET(fd, &read_fds);
    ret_val = select(FD_SETSIZE, &read_fds, NULL, NULL, &time);
    return (ret_val > 0);
}

IOinterface_func os_file_read_func = unix_read_func;
IOinterface_func os_file_write_func = unix_write_func;

IOinterface_func os_read_func = unix_read_func;
IOinterface_func os_write_func = unix_write_func;

#ifndef IOV_MAX
#define IOV_MAX 16
#endif
int os_max_iov = IOV_MAX;
IOinterface_close os_close_func = unix_close_func;
IOinterface_poll  os_poll_func = unix_poll_func;
IOinterface_open os_file_open_func = unix_file_open_func;
IOinterface_func os_server_read_func = unix_read_func;
IOinterface_func os_server_write_func = unix_write_func;
IOinterface_init os_sockets_init_func = NULL;
