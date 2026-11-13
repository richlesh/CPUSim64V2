/*
 * time.h - date and time functions for the CPUSim64 C compiler.
 *
 * clock() is provided by the assembly runtime; everything else is in C in
 * lib/time.c.  The CPUSim64 simulator has no calendar clock, so time()
 * returns (time_t)-1 ("calendar time not available", as the C standard
 * allows).  All calendar conversion is done in UTC: localtime() is the
 * same as gmtime() and mktime() is the same as timegm().
 */
#ifndef _CPUSIM64_TIME_H
#define _CPUSIM64_TIME_H

#include <stddef.h>

typedef long time_t;
typedef long clock_t;

#define CLOCKS_PER_SEC  1000000

struct tm {
    int tm_sec;     /* seconds after the minute [0, 60] */
    int tm_min;     /* minutes after the hour [0, 59] */
    int tm_hour;    /* hours since midnight [0, 23] */
    int tm_mday;    /* day of the month [1, 31] */
    int tm_mon;     /* months since January [0, 11] */
    int tm_year;    /* years since 1900 */
    int tm_wday;    /* days since Sunday [0, 6] */
    int tm_yday;    /* days since January 1 [0, 365] */
    int tm_isdst;   /* always 0 (UTC) */
};

clock_t clock(void);                        /* elapsed run time in us */
time_t time(time_t *t);                     /* always (time_t)-1 */
double difftime(time_t t1, time_t t0);

time_t mktime(struct tm *tp);               /* normalizes *tp */
time_t timegm(struct tm *tp);
struct tm *gmtime(const time_t *t);
struct tm *gmtime_r(const time_t *t, struct tm *result);
struct tm *localtime(const time_t *t);
struct tm *localtime_r(const time_t *t, struct tm *result);

char *asctime(const struct tm *tp);         /* "Sun Sep 16 01:03:52 1973\n" */
char *ctime(const time_t *t);
size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tp);

#endif
