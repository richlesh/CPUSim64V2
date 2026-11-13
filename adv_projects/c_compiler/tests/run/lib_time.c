#include <stdio.h>
#include <string.h>
#include <time.h>

static void show(long t) {
    time_t tt = t;
    struct tm *p = gmtime(&tt);
    char buf[128];
    printf("%ld: %04d-%02d-%02d %02d:%02d:%02d wday=%d yday=%d\n", t,
           p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min,
           p->tm_sec, p->tm_wday, p->tm_yday);
    printf("  asctime: %s", asctime(p));
    strftime(buf, sizeof buf, "%a %A %b %B %d %e %j %U %W %u %w %y %C", p);
    printf("  %s\n", buf);
    strftime(buf, sizeof buf, "%F %T %D %R %r %I %p %c %x %X %%", p);
    printf("  %s\n", buf);
    if (timegm(p) != t) printf("  timegm round trip FAILED\n");
}

int main(void) {
    show(0);
    show(951782400);        /* 2000-02-29 */
    show(1700000000);
    show(-1);
    show(-2208988800L);     /* 1900-01-01 */
    show(4102444799L);      /* 2099-12-31 23:59:59 */
    show(253402300799L);    /* 9999-12-31 23:59:59 */
    show(-30610224000L);    /* 1000-01-01 (macOS timegm cannot round-trip this; dates checked with Python datetime) */

    /* mktime normalizes out-of-range fields (TZ=UTC for the native run) */
    struct tm tm;
    memset(&tm, 0, sizeof tm);
    tm.tm_year = 2023 - 1900;
    tm.tm_mon = 12;         /* month 13 -> January 2024 */
    tm.tm_mday = 32;        /* -> February 1 */
    tm.tm_hour = 25;        /* -> 01:00 next day */
    tm.tm_min = -30;
    tm.tm_sec = 3661;
    time_t t = mktime(&tm);
    printf("mktime %ld -> %04d-%02d-%02d %02d:%02d:%02d wday=%d yday=%d\n", (long)t,
           tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
           tm.tm_wday, tm.tm_yday);
    memset(&tm, 0, sizeof tm);
    tm.tm_year = 100;
    tm.tm_mon = -1;         /* December 1999 */
    tm.tm_mday = 0;         /* -> November 30 */
    t = timegm(&tm);
    printf("timegm %ld -> %04d-%02d-%02d\n", (long)t, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

    struct tm r;
    time_t t2 = 86399;
    localtime_r(&t2, &r);
    gmtime_r(&t2, &r);
    printf("gmtime_r %02d:%02d:%02d ctime: %s", r.tm_hour, r.tm_min, r.tm_sec, ctime(&t2));

    printf("difftime %.1f\n", difftime(1000, 250));
    char small[8];
    int n1 = (int)strftime(small, sizeof small, "%Y-%m-%d", &r);
    int n2 = (int)strftime(small, sizeof small, "%H%M", &r);
    printf("strftime small=%d full=%d [%s]\n", n1, n2, small);
    time_t now = time(NULL);
    printf("time ok=%d clock ok=%d\n", now == (time_t)-1 || now > 1000000000, clock() >= 0);
    return 0;
}
