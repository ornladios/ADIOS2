#define FD_SETSIZE 1024
#include "winsock2.h"
#include <Windows.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>
#include <time.h>
#define getpid() _getpid()
#define read(fd, buf, len) recv(fd, (buf), (len), 0)
#define write(fd, buf, len) send(fd, buf, (len), 0)
#define close(x) closesocket(x)
#define INST_ADDRSTRLEN 50

#define pthread_mutex_t SRWLOCK
#define pthread_thread_t HANDLE
#define pthread_thread_id DWORD
#define pthread_cond_t CONDITION_VARIABLE
#define pthread_self() GetCurrentThreadId()
#define pthread_mutex_init(m, x) InitializeSRWLock(m)
#define pthread_mutex_lock(m) AcquireSRWLockExclusive(m)
#define pthread_mutex_unlock(m) ReleaseSRWLockExclusive(m)
#define pthread_cond_init(c, x) InitializeConditionVariable(c)
#define pthread_cond_signal(c) WakeConditionVariable(c)
#define pthread_cond_wait(c, m) SleepConditionVariableSRW(c, m, INFINITE, 0)
#define PTHREAD_MUTEX_INITIALIZER SRWLOCK_INIT

#define strdup _strdup
#define fileno _fileno
#define getpid _getpid
#define unlink _unlink
#ifndef PATH_MAX
#define PATH_MAX 400
#endif
#define realpath(N, R) _fullpath((R), (N), PATH_MAX)
#define close _close
#define sleep(x) Sleep((x)*1000)
#define usleep(x) Sleep((x) >> 10)
#define read _read
#define lseek _lseek
#define open _open
static void gettimeofday(struct timeval *now, void *tz)
{
    /* GSE...  No gettimeofday on windows.
     * Must use _ftime, get millisec time, convert to usec.  Bleh.
     */
    struct _timeb nowb;
    _ftime(&nowb);
    now->tv_sec = (long)nowb.time;
    now->tv_usec = nowb.millitm * 1000;
}
#define timersub(a, b, result)                                                                     \
    do                                                                                             \
    {                                                                                              \
        (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                                              \
        (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                                           \
        if ((result)->tv_usec < 0)                                                                 \
        {                                                                                          \
            --(result)->tv_sec;                                                                    \
            (result)->tv_usec += 1000000;                                                          \
        }                                                                                          \
    } while (0)
