/*
 * time.c - C implementations for lib/time.h (all calendar times are UTC).
 *
 * Appended to the program by the compiler; only used functions are
 * compiled.  All private names start with __tm_ because this file shares
 * the program's macro namespace.
 *
 * Day counting uses Howard Hinnant's days_from_civil / civil_from_days
 * algorithms (proleptic Gregorian calendar).
 */
#include <time.h>

static const char *__tm_wday_name[7] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};
static const char *__tm_mon_name[12] = {
    "January", "February", "March", "April", "May", "June", "July",
    "August", "September", "October", "November", "December"
};

static struct tm __tm_buf;
static char __tm_asc[32];

/* floor division and modulo for possibly negative numerators */
static long __tm_fdiv(long __tm_a, long __tm_b) {
    long __tm_q = __tm_a / __tm_b;
    if ((__tm_a % __tm_b) != 0 && (__tm_a < 0) != (__tm_b < 0)) __tm_q--;
    return __tm_q;
}

static long __tm_fmod(long __tm_a, long __tm_b) {
    return __tm_a - __tm_fdiv(__tm_a, __tm_b) * __tm_b;
}

/* days since 1970-01-01 of year y, month m (1-12), day d (1-31) */
static long __tm_days_from_civil(long __tm_y, long __tm_m, long __tm_d) {
    long __tm_era, __tm_yoe, __tm_doy, __tm_doe;
    if (__tm_m <= 2) __tm_y--;
    __tm_era = __tm_fdiv(__tm_y, 400);
    __tm_yoe = __tm_y - __tm_era * 400;
    __tm_doy = (153 * (__tm_m + (__tm_m > 2 ? -3 : 9)) + 2) / 5 + __tm_d - 1;
    __tm_doe = __tm_yoe * 365 + __tm_yoe / 4 - __tm_yoe / 100 + __tm_doy;
    return __tm_era * 146097 + __tm_doe - 719468;
}

clock_t clock(void);

time_t time(time_t *__tm_t) {
    /* the simulator provides no calendar clock */
    if (__tm_t) *__tm_t = (time_t)-1;
    return (time_t)-1;
}

double difftime(time_t __tm_t1, time_t __tm_t0) {
    return (double)(__tm_t1 - __tm_t0);
}

struct tm *gmtime_r(const time_t *__tm_tp, struct tm *__tm_r) {
    long __tm_t = *__tm_tp;
    long __tm_days = __tm_fdiv(__tm_t, 86400);
    long __tm_secs = __tm_t - __tm_days * 86400;
    long __tm_z, __tm_era, __tm_doe, __tm_yoe, __tm_y, __tm_doy, __tm_mp, __tm_d, __tm_m;
    __tm_r->tm_hour = (int)(__tm_secs / 3600);
    __tm_r->tm_min = (int)(__tm_secs / 60 % 60);
    __tm_r->tm_sec = (int)(__tm_secs % 60);
    __tm_r->tm_wday = (int)__tm_fmod(__tm_days + 4, 7);     /* 1970-01-01: Thursday */
    __tm_z = __tm_days + 719468;
    __tm_era = __tm_fdiv(__tm_z, 146097);
    __tm_doe = __tm_z - __tm_era * 146097;
    __tm_yoe = (__tm_doe - __tm_doe / 1460 + __tm_doe / 36524 - __tm_doe / 146096) / 365;
    __tm_y = __tm_yoe + __tm_era * 400;
    __tm_doy = __tm_doe - (365 * __tm_yoe + __tm_yoe / 4 - __tm_yoe / 100);
    __tm_mp = (5 * __tm_doy + 2) / 153;
    __tm_d = __tm_doy - (153 * __tm_mp + 2) / 5 + 1;
    __tm_m = __tm_mp < 10 ? __tm_mp + 3 : __tm_mp - 9;
    if (__tm_m <= 2) __tm_y++;
    __tm_r->tm_mday = (int)__tm_d;
    __tm_r->tm_mon = (int)(__tm_m - 1);
    __tm_r->tm_year = (int)(__tm_y - 1900);
    __tm_r->tm_yday = (int)(__tm_days - __tm_days_from_civil(__tm_y, 1, 1));
    __tm_r->tm_isdst = 0;
    return __tm_r;
}

struct tm *gmtime(const time_t *__tm_t) { return gmtime_r(__tm_t, &__tm_buf); }
struct tm *localtime_r(const time_t *__tm_t, struct tm *__tm_r) { return gmtime_r(__tm_t, __tm_r); }
struct tm *localtime(const time_t *__tm_t) { return gmtime_r(__tm_t, &__tm_buf); }

time_t timegm(struct tm *__tm_p) {
    /* normalize the month first, then count days and seconds */
    long __tm_mon = __tm_p->tm_mon;
    long __tm_year = __tm_p->tm_year + 1900L + __tm_fdiv(__tm_mon, 12);
    long __tm_days;
    time_t __tm_t;
    __tm_mon = __tm_fmod(__tm_mon, 12);
    __tm_days = __tm_days_from_civil(__tm_year, __tm_mon + 1, 1) + __tm_p->tm_mday - 1;
    __tm_t = __tm_days * 86400 + __tm_p->tm_hour * 3600L + __tm_p->tm_min * 60L + __tm_p->tm_sec;
    gmtime_r(&__tm_t, __tm_p);
    return __tm_t;
}

time_t mktime(struct tm *__tm_p) { return timegm(__tm_p); }

char *asctime(const struct tm *__tm_p) {
    sprintf(__tm_asc, "%.3s %.3s%3d %02d:%02d:%02d %d\n",
            __tm_wday_name[__tm_fmod(__tm_p->tm_wday, 7)],
            __tm_mon_name[__tm_fmod(__tm_p->tm_mon, 12)],
            __tm_p->tm_mday, __tm_p->tm_hour, __tm_p->tm_min, __tm_p->tm_sec,
            __tm_p->tm_year + 1900);
    return __tm_asc;
}

char *ctime(const time_t *__tm_t) { return asctime(localtime(__tm_t)); }

/* ---- strftime */
struct __tm_out { char *__tm_s; size_t __tm_max; size_t __tm_n; };

static void __tm_putc(struct __tm_out *__tm_o, int __tm_c) {
    if (__tm_o->__tm_n < __tm_o->__tm_max) __tm_o->__tm_s[__tm_o->__tm_n] = __tm_c;
    __tm_o->__tm_n++;
}

static void __tm_puts(struct __tm_out *__tm_o, const char *__tm_s, int __tm_len) {
    while (*__tm_s && __tm_len != 0) {
        __tm_putc(__tm_o, *__tm_s++);
        __tm_len--;
    }
}

/* number with minimum width; pad is '0', ' ' or 0 for none */
static void __tm_putn(struct __tm_out *__tm_o, long __tm_v, int __tm_w, int __tm_pad) {
    char __tm_d[24];
    int __tm_k = 0;
    if (__tm_v < 0) {
        __tm_putc(__tm_o, '-');
        __tm_v = -__tm_v;
        __tm_w--;
    }
    do {
        __tm_d[__tm_k++] = '0' + (int)(__tm_v % 10);
        __tm_v = __tm_v / 10;
    } while (__tm_v > 0);
    if (__tm_pad)
        while (__tm_w-- > __tm_k) __tm_putc(__tm_o, __tm_pad);
    while (__tm_k > 0) __tm_putc(__tm_o, __tm_d[--__tm_k]);
}

static void __tm_format(struct __tm_out *__tm_o, const char *__tm_f, const struct tm *__tm_p) {
    int __tm_h12;
    for (; *__tm_f; __tm_f++) {
        if (*__tm_f != '%') {
            __tm_putc(__tm_o, *__tm_f);
            continue;
        }
        __tm_f++;
        if (!*__tm_f) {
            __tm_putc(__tm_o, '%');
            return;
        }
        __tm_h12 = __tm_p->tm_hour % 12;
        if (__tm_h12 == 0) __tm_h12 = 12;
        switch (*__tm_f) {
        case 'a': __tm_puts(__tm_o, __tm_wday_name[__tm_fmod(__tm_p->tm_wday, 7)], 3); break;
        case 'A': __tm_puts(__tm_o, __tm_wday_name[__tm_fmod(__tm_p->tm_wday, 7)], -1); break;
        case 'b':
        case 'h': __tm_puts(__tm_o, __tm_mon_name[__tm_fmod(__tm_p->tm_mon, 12)], 3); break;
        case 'B': __tm_puts(__tm_o, __tm_mon_name[__tm_fmod(__tm_p->tm_mon, 12)], -1); break;
        case 'c': __tm_format(__tm_o, "%a %b %e %H:%M:%S %Y", __tm_p); break;
        case 'C': __tm_putn(__tm_o, (__tm_p->tm_year + 1900) / 100, 2, '0'); break;
        case 'd': __tm_putn(__tm_o, __tm_p->tm_mday, 2, '0'); break;
        case 'D': __tm_format(__tm_o, "%m/%d/%y", __tm_p); break;
        case 'e': __tm_putn(__tm_o, __tm_p->tm_mday, 2, ' '); break;
        case 'F': __tm_format(__tm_o, "%Y-%m-%d", __tm_p); break;
        case 'H': __tm_putn(__tm_o, __tm_p->tm_hour, 2, '0'); break;
        case 'I': __tm_putn(__tm_o, __tm_h12, 2, '0'); break;
        case 'j': __tm_putn(__tm_o, __tm_p->tm_yday + 1, 3, '0'); break;
        case 'm': __tm_putn(__tm_o, __tm_p->tm_mon + 1, 2, '0'); break;
        case 'M': __tm_putn(__tm_o, __tm_p->tm_min, 2, '0'); break;
        case 'n': __tm_putc(__tm_o, '\n'); break;
        case 'p': __tm_puts(__tm_o, __tm_p->tm_hour < 12 ? "AM" : "PM", -1); break;
        case 'r': __tm_format(__tm_o, "%I:%M:%S %p", __tm_p); break;
        case 'R': __tm_format(__tm_o, "%H:%M", __tm_p); break;
        case 'S': __tm_putn(__tm_o, __tm_p->tm_sec, 2, '0'); break;
        case 't': __tm_putc(__tm_o, '\t'); break;
        case 'T': __tm_format(__tm_o, "%H:%M:%S", __tm_p); break;
        case 'u': __tm_putn(__tm_o, __tm_p->tm_wday == 0 ? 7 : __tm_p->tm_wday, 1, 0); break;
        case 'U': __tm_putn(__tm_o, (__tm_p->tm_yday + 7 - __tm_p->tm_wday) / 7, 2, '0'); break;
        case 'w': __tm_putn(__tm_o, __tm_p->tm_wday, 1, 0); break;
        case 'W': __tm_putn(__tm_o, (__tm_p->tm_yday + 7 - (__tm_p->tm_wday + 6) % 7) / 7, 2, '0'); break;
        case 'x': __tm_format(__tm_o, "%m/%d/%y", __tm_p); break;
        case 'X': __tm_format(__tm_o, "%H:%M:%S", __tm_p); break;
        case 'y': __tm_putn(__tm_o, __tm_fmod(__tm_p->tm_year + 1900, 100), 2, '0'); break;
        case 'Y': __tm_putn(__tm_o, __tm_p->tm_year + 1900L, 1, 0); break;
        case 'z': __tm_puts(__tm_o, "+0000", -1); break;
        case 'Z': __tm_puts(__tm_o, "UTC", -1); break;
        case '%': __tm_putc(__tm_o, '%'); break;
        default:
            __tm_putc(__tm_o, '%');
            __tm_putc(__tm_o, *__tm_f);
            break;
        }
    }
}

size_t strftime(char *__tm_s, size_t __tm_max, const char *__tm_f, const struct tm *__tm_p) {
    struct __tm_out __tm_o;
    __tm_o.__tm_s = __tm_s;
    __tm_o.__tm_max = __tm_max;
    __tm_o.__tm_n = 0;
    __tm_format(&__tm_o, __tm_f, __tm_p);
    if (__tm_o.__tm_n >= __tm_max) {
        if (__tm_max > 0) __tm_s[0] = 0;
        return 0;
    }
    __tm_s[__tm_o.__tm_n] = 0;
    return __tm_o.__tm_n;
}
