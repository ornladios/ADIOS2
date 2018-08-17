#include "config.h"
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#define HAVE_IOVEC_DEFINE
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include "assert.h"
#include "ffs.h"
#include "fm_internal.h"
#include "ffs_internal.h"
#include "io_interface.h"

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

static int
unix_readv_func(conn, iov, icount, errno_p, result_p)
void *conn;
struct iovec *iov;
int icount;
int *errno_p;
char **result_p;
{
    int orig_icount = icount;
    int fd = (int) (long)conn;
    int iget;

    while (icount > 0) {
	iget = readv(fd, (struct iovec *) iov, icount);
	if (iget == 0) {
	    if (result_p) *result_p = "End of file";
	    if (errno_p) *errno_p = 0;
	    return 0;		/* end of file */
	} else if (iget == -1) {
	    if (errno_p) *errno_p = errno;
	    if ((errno != EWOULDBLOCK) &&
		(errno != EAGAIN) &&
		(errno != EINTR)) {
		/* serious error */
		return -1;
	    } else {
		if (errno_p) *errno_p = 0;
		iget = 0;
	    }
	}
	while (iget > 0) {
	    if (iov[0].iov_len <= iget) {
		iget -= iov[0].iov_len;
		iov++;
		icount--;
	    } else {
		iov[0].iov_len -= iget;
		iov[0].iov_base = ((char*)iov[0].iov_base) + iget;
		iget = 0;
	    }
	}
    }
    return orig_icount;
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
unix_writev_func(conn, iov, iovcnt, errno_p, result_p)
void *conn;
struct iovec *iov;
int iovcnt;
int *errno_p;
char **result_p;
{
    int fd = (int) (long) conn;
    int left = 0;
    int iget = 0;
    int iovleft, i;

    iovleft = iovcnt;

    /* sum lengths */
    for (i=0; i<iovcnt; i++) left += iov[i].iov_len;

    while (left > 0) {
	iget = writev(fd, (struct iovec *) &iov[iovcnt - iovleft], iovleft);
	if (iget == -1) {
	    if ((errno != EWOULDBLOCK) && (errno != EAGAIN)) {
		/* serious error */
		if (errno_p) *errno_p = errno;
		return (iovcnt - iovleft);
	    } else {
		iget = 0;
	    }
	}
	if (iget == left)
	    return iovcnt;

	left -= iget;
	while (iget > 0) {
	    iget -= iov[iovcnt - iovleft].iov_len;
	    iovleft--;
	}

	if (iget < 0) {
	    /*
	     * Only part of the last block was written.  Modify IO 
	     * vector to indicate the remaining block to be written.
	     */
	    /* restore iovleft and iget to cover remaining block*/
	    iovleft++;
	    iget += iov[iovcnt - iovleft].iov_len;

	    /* adjust count down and base up by number of bytes written */
	    iov[iovcnt-iovleft].iov_len -= iget;
	    iov[iovcnt-iovleft].iov_base = 
		(char*)(iov[iovcnt-iovleft].iov_base) + iget;
	}
    }
    return iovcnt;
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
    if (input) *input = 0;
    if (output) *output = 0;

    tmp_flags &= ~(O_TRUNC);
    tmp_flags &= ~(O_CREAT);
    if ((O_RDONLY == tmp_flags) ||
	(O_WRONLY == tmp_flags)) {
	 /* must be old style call */
	 flags = (long)flag_str;
	 if (input) *input = (O_RDONLY == (long)flag_str);
	 if (output) *output = (O_WRONLY & (long)flag_str);
    } else {
	 if (strcmp(flag_str, "r") == 0) {
	      flags = O_RDONLY;
	      if (input) *input = 1;
	 } else if (strcmp(flag_str, "w") == 0) {
	      flags = O_WRONLY | O_CREAT | O_TRUNC;
	      if (output) *output = 1;
	 } else if (strcmp(flag_str, "a") == 0) {
	     flags = O_RDWR;
	     if (output) *output = 1;
	     if (input) *input = 1;
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
    


IOinterface_func ffs_file_read_func = unix_read_func;
IOinterface_func ffs_file_write_func = unix_write_func;
IOinterface_funcv ffs_file_readv_func = unix_readv_func;
IOinterface_funcv ffs_file_writev_func = unix_writev_func;

IOinterface_func ffs_read_func = unix_read_func;
IOinterface_func ffs_write_func = unix_write_func;
IOinterface_funcv ffs_readv_func = unix_readv_func;
IOinterface_funcv ffs_writev_func = unix_writev_func;
#ifndef IOV_MAX
#define IOV_MAX 16
#endif
int ffs_max_iov = IOV_MAX;
IOinterface_close ffs_close_func = unix_close_func;
IOinterface_open ffs_file_open_func = unix_file_open_func;
IOinterface_func ffs_server_read_func = unix_read_func;
IOinterface_func ffs_server_write_func = unix_write_func;
IOinterface_init ffs_sockets_init_func = NULL;
