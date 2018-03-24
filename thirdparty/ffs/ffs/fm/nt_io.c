#include "config.h"
#include <sys/types.h>
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#define HAVE_IOVEC_DEFINE
#endif
#define FD_SETSIZE 1024
#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include "ffs.h"
#include "io_interface.h"
#include "ffs_internal.h"

static int
nt_file_read_func(conn, buffer, length, errno_p, result_p)
void *conn;
void *buffer;
int length;
int *errno_p;
char **result_p;
{
    int left = length;
    DWORD iget;
    BOOL bResult;
    bResult = ReadFile(conn, (char *) buffer, length, &iget, NULL);

    if (iget == 0) {
	*result_p = "End of file";
	*errno_p = 0;
	return 0;		/* end of file */
    } else {
	if (!bResult) {
	    *errno_p = GetLastError();
	    if ((*errno_p != WSAEWOULDBLOCK) &&
		(*errno_p != WSAEINPROGRESS) &&
		(*errno_p != WSAEINTR)) {
		/* serious error */
		return -1;
	    } else {
		*errno_p = 0;
		iget = 0;
	    }
	}
    }

    left = length - iget;
    while (left > 0) {
	bResult = ReadFile((HANDLE) conn, (char *) buffer + length - left,
			   left, &iget, NULL);
	if (iget == 0) {
	    *result_p = "End of file";
	    *errno_p = 0;
	    return length - left;	/* end of file */
	} else {
	    if (!bResult) {
		*errno_p = GetLastError();
		if ((*errno_p != WSAEWOULDBLOCK) &&
		    (*errno_p != WSAEINPROGRESS) &&
		    (*errno_p != WSAEINTR)) {
		    /* serious error */
		    return (length - left);
		} else {
		    *errno_p = 0;
		    iget = 0;
		}
	    }
	}
	left -= iget;
    }
    return length;
}

static int
nt_socket_read_func(conn, buffer, length, errno_p, result_p)
void *conn;
void *buffer;
int length;
int *errno_p;
char **result_p;
{
    int left = length;
    int iget;
    iget = recv((unsigned int) conn, (char *) buffer + length - left, left, 0);
    if (iget == 0) {
	*result_p = NULL;
	*errno_p = 0;
	return 0;		/* No more socket data */
    } else if (iget == SOCKET_ERROR) {
	*errno_p = WSAGetLastError();
	if ((*errno_p != WSAEWOULDBLOCK) &&
	    (*errno_p != WSAEINPROGRESS) &&
		(*errno_p != WSAECONNRESET)  &&
	    (*errno_p != WSAEINTR)) {
	    /* serious error */
	    fprintf(stderr, "WINSOCK ERROR during receive, %i on socket %i\n",
		    *errno_p, conn);
	    return -1;
	} else {
		if (*errno_p == WSAECONNRESET)
			return -1;
	    *errno_p = 0;
	    iget = 0;
	}
    }
    left = length - iget;
    while (left > 0) {
	iget = recv((unsigned int) conn, (char *) buffer + length - left,
		    left, 0);
	if (iget == 0) {
	    *result_p = NULL;
	    *errno_p = 0;
	    return length - left;	/* no more socket data */
	} else {
	    if (iget == SOCKET_ERROR) {
		*errno_p = WSAGetLastError();
		if ((*errno_p != WSAEWOULDBLOCK) &&
		    (*errno_p != WSAEINPROGRESS) &&
			(*errno_p != WSAECONNRESET)  &&
		    (*errno_p != WSAEINTR)) {

		    /* serious error */
		    fprintf(stderr, "WINSOCK ERROR during receive2, %i on socket %i\n",
			    *errno_p, conn);
		    return (length - left);
		} else {
			if (*errno_p == WSAECONNRESET)
				return -1;
		    *errno_p = 0;
		    iget = 0;
		}
	    }
	}
	left -= iget;
    }

    return length;
}


static int
nt_file_write_func(conn, buffer, length, errno_p, result_p)
void *conn;
void *buffer;
int length;
int *errno_p;
char **result_p;
{
    int left = length;
    int iget = 0;
    BOOL bResult;

    while (left > 0) {
	bResult = WriteFile((HANDLE) conn, (char *) buffer + length - left, 
			    left, &iget, NULL);
	if (!bResult) {
	    *errno_p = GetLastError();
	    if ((*errno_p != WSAEWOULDBLOCK) &&
		(*errno_p != WSAEINPROGRESS) &&
		(*errno_p != WSAEINTR)) {
		/* serious error */
		return (length - left);
	    } else {
		*errno_p = 0;
		iget = 0;
	    }
	}
	left -= iget;
    }
    return length;
}


static int
nt_socket_write_func(conn, buffer, length, errno_p, result_p)
void *conn;
void *buffer;
int length;
int *errno_p;
char **result_p;
{
    int left = length;
    int iget = 0;

    while (left > 0) {
	iget = send((unsigned int) conn, (char *) buffer + length - left,
		    left, 0);
	if (iget == SOCKET_ERROR) {
	    *errno_p = GetLastError();
	    if ((*errno_p != WSAEWOULDBLOCK) &&
		(*errno_p != WSAEINPROGRESS) &&
		(*errno_p != WSAEINTR)) {
		/* serious error */
		return (length - left);
	    } else {
		*errno_p = 0;
		iget = 0;
	    }
	}
	left -= iget;
    }
    return length;
}


static int
nt_close_func(conn)
void *conn;
{
    DWORD status;
    /* make sure handle exists before we close it. *otherwise -- an * * *
     * access error occurs */
    if (GetHandleInformation(conn, &status)) {
	CloseHandle((HANDLE) conn);
	return 1;
    }
    return 0;
}

static void *
nt_file_open_func(path, flag_str, input, output)
const char *path;
const char *flag_str;
int *input;
int *output;
{

    void *file;
    long tmp_flags = (long)flag_str;
    tmp_flags &= ~(O_TRUNC);
    tmp_flags &= ~(O_CREAT);

    if ((O_RDONLY == tmp_flags) ||
	(O_WRONLY == tmp_flags)) {
	 /* must be old style call */
	*input = (O_RDONLY == (long) flag_str);
	*output = (O_WRONLY & (long) flag_str);
    } else {
	if (strcmp(flag_str, "r") == 0) {
	    *input = TRUE;
	} else if (strcmp(flag_str, "w") == 0) {
	    *output = TRUE;
	} else {
	    fprintf(stderr, "Open flags value not understood for file \"%s\"\n",
		    path);
	    return NULL;
	}
    }

    if (*input) {
	file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ,
		      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);

    } else {
	file = CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ,
		      NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL);
    }
    if (file == INVALID_HANDLE_VALUE) {
	return NULL;
    } else {
	return file;
    }
}


static int
nt_socket_readv_func(conn, iov, icount, errno_p, result_p)
void *conn;
struct iovec *iov;
int icount;
int *errno_p;
char **result_p;
{

    int i = 0;
    for (; i < icount; i++) {
	if (nt_socket_read_func(conn, iov[i].iov_base, iov[i].iov_len,
				errno_p, result_p) != iov[i].iov_len) {
	    return i;
	}
    }
    return icount;
}


static int
null_file_readv_func(conn, iov, icount, errno_p, result_p)
void *conn;
struct iovec *iov;
int icount;
int *errno_p;
char **result_p;
{

    int i = 0;
    for (; i < icount; i++) {
	if (nt_file_read_func(conn, iov[i].iov_base, iov[i].iov_len, errno_p,
			      result_p) != iov[i].iov_len) {
	    return i;
	}
    }
    return icount;
}


/* Winsock init stuff, ask for ver 1.1 */
static WORD wVersionRequested = MAKEWORD(1, 1);
static WSADATA wsaData;

static void
nt_socket_init_func()
{
    int nErrorStatus;
    nErrorStatus = WSAStartup(wVersionRequested, &wsaData);
    if (nErrorStatus != 0) {
	fprintf(stderr, "Could not initialize windows socket library!");
	WSACleanup();
	exit(-1);
    }
}


static int
nt_poll_func(conn)
void *conn;
{
    int fd = (int) (long) conn;
    struct timeval time;
    fd_set read_fds;
    int ret_val;

    time.tv_sec = time.tv_usec = 0;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    if (fd > FD_SETSIZE) {
	fprintf(stderr, "Internal Error, stupid WINSOCK large FD bug.\n");
	fprintf(stderr, "Increase FD_SETSIZE.  Item not added to fdset.\n");
    }
    ret_val = select(FD_SETSIZE, &read_fds, NULL, NULL, &time);
    return (ret_val > 0);
}

IOinterface_func os_file_read_func = (IOinterface_func)nt_file_read_func;
IOinterface_func os_file_write_func = (IOinterface_func)nt_file_write_func;
IOinterface_funcv os_file_readv_func = (IOinterface_funcv)null_file_readv_func;
IOinterface_funcv os_file_writev_func = NULL;


IOinterface_func os_read_func = (IOinterface_func)nt_socket_read_func;
IOinterface_func os_write_func = (IOinterface_func)nt_socket_write_func;
IOinterface_funcv os_readv_func = (IOinterface_funcv)nt_socket_readv_func;
IOinterface_funcv os_writev_func = NULL;
int os_max_iov = 1;


IOinterface_open os_file_open_func = (IOinterface_open)nt_file_open_func;
IOinterface_close os_close_func = (IOinterface_close) nt_close_func;
IOinterface_poll  os_poll_func = (IOinterface_poll)nt_poll_func;
IOinterface_func os_server_read_func = (IOinterface_func)nt_socket_read_func;
IOinterface_func os_server_write_func = (IOinterface_func)nt_socket_write_func;
IOinterface_init os_sockets_init_func = (IOinterface_init)nt_socket_init_func;
