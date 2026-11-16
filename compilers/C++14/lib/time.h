// time.h - the C library's time.h as the C++ translator sees it.
#ifndef _CPPC_TIME_H
#define _CPPC_TIME_H
#pragma cppc c_include <time.h>
#include <stddef.h>
#define CLOCKS_PER_SEC 1000000
#define TIME_UTC 1
extern "C" {
typedef long time_t;
typedef long clock_t;
struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};
struct timespec {
    time_t tv_sec;
    long tv_nsec;
};
int timespec_get(struct timespec *ts, int base);
clock_t clock(void);
time_t time(time_t *t);
double difftime(time_t t1, time_t t0);
time_t mktime(struct tm *tp);
struct tm *gmtime(const time_t *t);
struct tm *localtime(const time_t *t);
char *asctime(const struct tm *tp);
char *ctime(const time_t *t);
size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tp);
}
#endif
