#include "config.h"
#include "stdlib.h"
#include "chr_time.h"

#include <sys/time.h>

extern void
chr_get_time( chr_time *time)
{
    gettimeofday((struct timeval *) time, NULL);
}

extern void
chr_timer_start( chr_time *time)
{
    chr_get_time(time);
}

extern void
chr_timer_stop( chr_time *time)
{
    struct timeval now;
    struct timeval duration;

    gettimeofday(&now, NULL);
    chr_timer_diff((chr_time*)&duration, (chr_time*)&now, time);
    *((struct timeval *) time) = duration;
}

extern int
chr_timer_eq_zero (chr_time *time)
{
    struct timeval *t = (struct timeval *) time; 
    return ((t->tv_sec == 0) && (t->tv_usec == 0));
}

extern void
chr_timer_diff( chr_time *diff, chr_time *src1, chr_time *src2)
{
    struct timeval d;
    struct timeval *s1 = (struct timeval *)src1;
    struct timeval *s2 = (struct timeval *)src2;
    d.tv_sec = s1->tv_sec - s2->tv_sec;
    d.tv_usec = s1->tv_usec - s2->tv_usec;
    if (d.tv_usec < 0) {
	d.tv_usec += 1000000;
	d.tv_sec--;
    }
    *((struct timeval*)diff) = d;
}

extern void
chr_timer_sum( chr_time *sum, chr_time *src1, chr_time *src2)
{
    struct timeval s;
    struct timeval *s1 = (struct timeval *)src1;
    struct timeval *s2 = (struct timeval *)src2;
    s.tv_sec = s1->tv_sec + s2->tv_sec;
    s.tv_usec = s1->tv_usec + s2->tv_usec;
    if (s.tv_usec > 1000000) {
	s.tv_usec -= 1000000;
	s.tv_sec++;
    }
    *((struct timeval*)sum) = s;
}


extern double
chr_time_to_secs(chr_time *time)
{
    return (double)((struct timeval*)time)->tv_sec + 
	((double)((struct timeval*)time)->tv_usec)/1000000.0;
}

extern double
chr_time_to_millisecs(chr_time *time)
{
    return ((double)((struct timeval*)time)->tv_sec)*1000.0 + 
	((double)((struct timeval*)time)->tv_usec)/1000.0;
}

extern double
chr_time_to_microsecs(chr_time *time)
{
    return ((double)((struct timeval*)time)->tv_sec)*1000000.0 + 
	((double)((struct timeval*)time)->tv_usec);
}

extern double
chr_time_to_nanosecs(chr_time *time)
{
    return ((double)((struct timeval*)time)->tv_sec)*1000000000.0 + 
	((double)((struct timeval*)time)->tv_usec*1000.0);
}

extern double
chr_approx_resolution()
{
    struct timeval start, stop, diff;
    gettimeofday(&start, NULL);
    gettimeofday(&stop, NULL);
    while(start.tv_usec == stop.tv_usec) {
	gettimeofday(&stop, NULL);
    }
    chr_timer_diff((chr_time*)&diff, (chr_time*)&stop, (chr_time*)&start);
    return chr_time_to_secs((chr_time*)&diff);
}
