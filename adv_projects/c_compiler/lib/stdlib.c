/*
 * stdlib.c - C implementations for lib/stdlib.h.
 *
 * Appended to the program by the compiler; only used functions are
 * compiled.  All private names start with __sl_ because this file shares
 * the program's macro namespace.
 */
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

double __atof_exact(const char *__sl_s);   /* [rt] */

/* ---- program termination */
#define __SL_ATEXIT_MAX 32
static void (*__sl_atexit_fn[__SL_ATEXIT_MAX])(void);
static int __sl_atexit_cnt;

int atexit(void (*__sl_f)(void)) {
    /* referencing exit() makes the compiler include it, so that returning
       from main also runs the handlers */
    void (*__sl_e)(int) = exit;
    if (__sl_atexit_cnt >= __SL_ATEXIT_MAX) return -1;
    __sl_atexit_fn[__sl_atexit_cnt++] = __sl_f;
    return 0;
}

void exit(int __sl_status) {
    while (__sl_atexit_cnt > 0) {
        __sl_atexit_cnt--;
        __sl_atexit_fn[__sl_atexit_cnt]();
    }
    _Exit(__sl_status);
}

char *getenv(const char *__sl_name) { return 0; }

/* ---- integer arithmetic (division truncates toward zero, as in C99) */
int abs(int __sl_n) { return __sl_n < 0 ? -__sl_n : __sl_n; }
long labs(long __sl_n) { return __sl_n < 0 ? -__sl_n : __sl_n; }
long long llabs(long long __sl_n) { return __sl_n < 0 ? -__sl_n : __sl_n; }

div_t div(int __sl_n, int __sl_d) {
    div_t __sl_r;
    __sl_r.quot = __sl_n / __sl_d;
    __sl_r.rem = __sl_n % __sl_d;
    return __sl_r;
}

ldiv_t ldiv(long __sl_n, long __sl_d) {
    ldiv_t __sl_r;
    __sl_r.quot = __sl_n / __sl_d;
    __sl_r.rem = __sl_n % __sl_d;
    return __sl_r;
}

lldiv_t lldiv(long long __sl_n, long long __sl_d) {
    lldiv_t __sl_r;
    __sl_r.quot = __sl_n / __sl_d;
    __sl_r.rem = __sl_n % __sl_d;
    return __sl_r;
}

/* ---- numeric conversion */
static int __sl_digit(int __sl_c) {
    if (__sl_c >= '0' && __sl_c <= '9') return __sl_c - '0';
    if (__sl_c >= 'a' && __sl_c <= 'z') return __sl_c - 'a' + 10;
    if (__sl_c >= 'A' && __sl_c <= 'Z') return __sl_c - 'A' + 10;
    return 99;
}

/* Parses the magnitude of an integer.  *__sl_neg receives the sign and
   *__sl_ovf is set if the magnitude does not fit in 64 unsigned bits.
   Unsigned division is avoided: ULONG_MAX / base is derived from
   LONG_MAX / base. */
static unsigned long __sl_strtoux(const char *__sl_s, char **__sl_end,
                                  int __sl_base, int *__sl_neg, int *__sl_ovf) {
    const char *__sl_p = __sl_s;
    unsigned long __sl_v = 0;
    int __sl_any = 0;
    *__sl_neg = 0;
    *__sl_ovf = 0;
    while (isspace(*__sl_p)) __sl_p++;
    if (*__sl_p == '-') { *__sl_neg = 1; __sl_p++; }
    else if (*__sl_p == '+') __sl_p++;
    if ((__sl_base == 0 || __sl_base == 16) && __sl_p[0] == '0'
            && (__sl_p[1] == 'x' || __sl_p[1] == 'X') && isxdigit(__sl_p[2])) {
        __sl_p += 2;
        __sl_base = 16;
    } else if (__sl_base == 0) {
        __sl_base = __sl_p[0] == '0' ? 8 : 10;
    }
    if (__sl_base < 2 || __sl_base > 36) {
        if (__sl_end) *__sl_end = (char *)__sl_s;
        return 0;
    }
    long __sl_q = LONG_MAX / __sl_base;
    long __sl_r = LONG_MAX % __sl_base;
    unsigned long __sl_lim = (unsigned long)(2 * __sl_q + (2 * __sl_r + 1) / __sl_base);
    int __sl_limd = (int)((2 * __sl_r + 1) % __sl_base);
    for (;;) {
        int __sl_d = __sl_digit(*__sl_p);
        if (__sl_d >= __sl_base) break;
        __sl_any = 1;
        if (__sl_v > __sl_lim || (__sl_v == __sl_lim && __sl_d > __sl_limd))
            *__sl_ovf = 1;
        __sl_v = __sl_v * __sl_base + __sl_d;
        __sl_p++;
    }
    if (__sl_end) *__sl_end = (char *)(__sl_any ? __sl_p : __sl_s);
    return __sl_v;
}

long strtol(const char *__sl_s, char **__sl_end, int __sl_base) {
    int __sl_neg, __sl_ovf;
    unsigned long __sl_m = __sl_strtoux(__sl_s, __sl_end, __sl_base, &__sl_neg, &__sl_ovf);
    if (__sl_neg) {
        if (__sl_ovf || __sl_m > (unsigned long)LONG_MAX + 1) return LONG_MIN;
        return -(long)__sl_m;
    }
    if (__sl_ovf || __sl_m > (unsigned long)LONG_MAX) return LONG_MAX;
    return (long)__sl_m;
}

long long strtoll(const char *__sl_s, char **__sl_end, int __sl_base) {
    return strtol(__sl_s, __sl_end, __sl_base);
}

unsigned long strtoul(const char *__sl_s, char **__sl_end, int __sl_base) {
    int __sl_neg, __sl_ovf;
    unsigned long __sl_m = __sl_strtoux(__sl_s, __sl_end, __sl_base, &__sl_neg, &__sl_ovf);
    if (__sl_ovf) return ULONG_MAX;
    return __sl_neg ? -__sl_m : __sl_m;
}

unsigned long long strtoull(const char *__sl_s, char **__sl_end, int __sl_base) {
    return strtoul(__sl_s, __sl_end, __sl_base);
}

int atoi(const char *__sl_s) { return (int)strtol(__sl_s, 0, 10); }
long atol(const char *__sl_s) { return strtol(__sl_s, 0, 10); }
long long atoll(const char *__sl_s) { return strtol(__sl_s, 0, 10); }

/* case-insensitive prefix match of a lowercase word */
static int __sl_word(const char *__sl_p, const char *__sl_w) {
    int __sl_n = 0;
    while (__sl_w[__sl_n]) {
        if (tolower(__sl_p[__sl_n]) != __sl_w[__sl_n]) return 0;
        __sl_n++;
    }
    return __sl_n;
}

double strtod(const char *__sl_s, char **__sl_end) {
    char __sl_buf[64];
    int __sl_n = 0, __sl_dig = 0, __sl_neg = 0, __sl_k;
    const char *__sl_p = __sl_s;
    while (isspace(*__sl_p)) __sl_p++;
    if (*__sl_p == '+' || *__sl_p == '-') {
        __sl_neg = *__sl_p == '-';
        __sl_buf[__sl_n++] = *__sl_p++;
    }
    /* inf, infinity, nan */
    if ((__sl_k = __sl_word(__sl_p, "infinity")) != 0 || (__sl_k = __sl_word(__sl_p, "inf")) != 0) {
        if (__sl_end) *__sl_end = (char *)(__sl_p + __sl_k);
        return __sl_neg ? -1.0 / 0.0 : 1.0 / 0.0;
    }
    if ((__sl_k = __sl_word(__sl_p, "nan")) != 0) {
        if (__sl_end) *__sl_end = (char *)(__sl_p + __sl_k);
        return 0.0 / 0.0;
    }
    while (isdigit(*__sl_p) && __sl_n < 60) { __sl_buf[__sl_n++] = *__sl_p++; __sl_dig = 1; }
    if (*__sl_p == '.') {
        __sl_buf[__sl_n++] = *__sl_p++;
        while (isdigit(*__sl_p) && __sl_n < 60) { __sl_buf[__sl_n++] = *__sl_p++; __sl_dig = 1; }
    }
    if (__sl_dig && (*__sl_p == 'e' || *__sl_p == 'E')) {
        const char *__sl_q = __sl_p + 1;
        if (*__sl_q == '+' || *__sl_q == '-') __sl_q++;
        if (isdigit(*__sl_q)) {
            __sl_buf[__sl_n++] = *__sl_p++;
            if (*__sl_p == '+' || *__sl_p == '-') __sl_buf[__sl_n++] = *__sl_p++;
            while (isdigit(*__sl_p) && __sl_n < 62) __sl_buf[__sl_n++] = *__sl_p++;
        }
    }
    __sl_buf[__sl_n] = 0;
    if (!__sl_dig) {
        if (__sl_end) *__sl_end = (char *)__sl_s;
        return 0.0;
    }
    if (__sl_end) *__sl_end = (char *)__sl_p;
    return __atof_exact(__sl_buf);
}

double atof(const char *__sl_s) { return strtod(__sl_s, 0); }

/* ---- pseudo-random numbers: 64-bit LCG (Knuth MMIX constants); the top
   31 bits are returned */
static unsigned long __sl_seed = 1;

int rand(void) {
    __sl_seed = __sl_seed * 6364136223846793005UL + 1442695040888963407UL;
    return (int)((__sl_seed >> 33) & 0x7fffffff);
}

void srand(unsigned __sl_s) { __sl_seed = __sl_s; }

/* ---- searching and sorting */
static void __sl_swap(char *__sl_a, char *__sl_b, size_t __sl_size) {
    while (__sl_size--) {
        char __sl_t = *__sl_a;
        *__sl_a++ = *__sl_b;
        *__sl_b++ = __sl_t;
    }
}

/* heapsort: O(n log n) worst case, no recursion, no extra memory */
static void __sl_sift(char *__sl_b, size_t __sl_root, size_t __sl_n, size_t __sl_size,
                      int (*__sl_cmp)(const void *, const void *)) {
    for (;;) {
        size_t __sl_child = 2 * __sl_root + 1;
        if (__sl_child >= __sl_n) return;
        if (__sl_child + 1 < __sl_n
                && __sl_cmp(__sl_b + __sl_child * __sl_size, __sl_b + (__sl_child + 1) * __sl_size) < 0)
            __sl_child++;
        if (__sl_cmp(__sl_b + __sl_root * __sl_size, __sl_b + __sl_child * __sl_size) >= 0)
            return;
        __sl_swap(__sl_b + __sl_root * __sl_size, __sl_b + __sl_child * __sl_size, __sl_size);
        __sl_root = __sl_child;
    }
}

void qsort(void *__sl_base, size_t __sl_n, size_t __sl_size,
           int (*__sl_cmp)(const void *, const void *)) {
    char *__sl_b = __sl_base;
    size_t __sl_i;
    if (__sl_n < 2) return;
    for (__sl_i = __sl_n / 2; __sl_i > 0; __sl_i--)
        __sl_sift(__sl_b, __sl_i - 1, __sl_n, __sl_size, __sl_cmp);
    for (__sl_i = __sl_n - 1; __sl_i > 0; __sl_i--) {
        __sl_swap(__sl_b, __sl_b + __sl_i * __sl_size, __sl_size);
        __sl_sift(__sl_b, 0, __sl_i, __sl_size, __sl_cmp);
    }
}

void *bsearch(const void *__sl_key, const void *__sl_base, size_t __sl_n, size_t __sl_size,
              int (*__sl_cmp)(const void *, const void *)) {
    const char *__sl_b = __sl_base;
    size_t __sl_lo = 0, __sl_hi = __sl_n;
    while (__sl_lo < __sl_hi) {
        size_t __sl_mid = __sl_lo + (__sl_hi - __sl_lo) / 2;
        int __sl_r = __sl_cmp(__sl_key, __sl_b + __sl_mid * __sl_size);
        if (__sl_r == 0) return (void *)(__sl_b + __sl_mid * __sl_size);
        if (__sl_r < 0) __sl_hi = __sl_mid;
        else __sl_lo = __sl_mid + 1;
    }
    return 0;
}
