/*
 * string.c - C implementations for lib/string.h.
 *
 * Appended to the program by the compiler; only used functions are
 * compiled.  All private names start with __st_ because this file shares
 * the program's macro namespace.
 */
#include <string.h>
#include <stdlib.h>

/* ---- copying */
void *memcpy(void *__st_d, const void *__st_s, size_t __st_n) {
    char *__st_a = __st_d;
    const char *__st_b = __st_s;
    while (__st_n--) *__st_a++ = *__st_b++;
    return __st_d;
}

void *memmove(void *__st_d, const void *__st_s, size_t __st_n) {
    char *__st_a = __st_d;
    const char *__st_b = __st_s;
    if (__st_a < __st_b) {
        while (__st_n--) *__st_a++ = *__st_b++;
    } else if (__st_a > __st_b) {
        __st_a = __st_a + __st_n;
        __st_b = __st_b + __st_n;
        while (__st_n--) *--__st_a = *--__st_b;
    }
    return __st_d;
}

char *strcpy(char *__st_d, const char *__st_s) {
    char *__st_r = __st_d;
    while ((*__st_d++ = *__st_s++) != 0)
        ;
    return __st_r;
}

char *strncpy(char *__st_d, const char *__st_s, size_t __st_n) {
    size_t __st_i = 0;
    for (; __st_i < __st_n && __st_s[__st_i]; __st_i++) __st_d[__st_i] = __st_s[__st_i];
    for (; __st_i < __st_n; __st_i++) __st_d[__st_i] = 0;
    return __st_d;
}

char *strdup(const char *__st_s) {
    size_t __st_n = strlen(__st_s) + 1;
    char *__st_d = malloc(__st_n);
    if (__st_d) memcpy(__st_d, __st_s, __st_n);
    return __st_d;
}

char *strndup(const char *__st_s, size_t __st_n) {
    size_t __st_len = strnlen(__st_s, __st_n);
    char *__st_d = malloc(__st_len + 1);
    if (__st_d) {
        memcpy(__st_d, __st_s, __st_len);
        __st_d[__st_len] = 0;
    }
    return __st_d;
}

/* ---- concatenation */
char *strcat(char *__st_d, const char *__st_s) {
    strcpy(__st_d + strlen(__st_d), __st_s);
    return __st_d;
}

char *strncat(char *__st_d, const char *__st_s, size_t __st_n) {
    char *__st_p = __st_d + strlen(__st_d);
    while (__st_n && *__st_s) {
        *__st_p++ = *__st_s++;
        __st_n--;
    }
    *__st_p = 0;
    return __st_d;
}

/* ---- comparison (characters are non-negative codepoints) */
int memcmp(const void *__st_a, const void *__st_b, size_t __st_n) {
    const char *__st_p = __st_a;
    const char *__st_q = __st_b;
    for (; __st_n; __st_n--, __st_p++, __st_q++)
        if (*__st_p != *__st_q) return *__st_p < *__st_q ? -1 : 1;
    return 0;
}

int strcmp(const char *__st_a, const char *__st_b) {
    while (*__st_a && *__st_a == *__st_b) { __st_a++; __st_b++; }
    if (*__st_a == *__st_b) return 0;
    return *__st_a < *__st_b ? -1 : 1;
}

int strncmp(const char *__st_a, const char *__st_b, size_t __st_n) {
    while (__st_n && *__st_a && *__st_a == *__st_b) { __st_a++; __st_b++; __st_n--; }
    if (__st_n == 0 || *__st_a == *__st_b) return 0;
    return *__st_a < *__st_b ? -1 : 1;
}

/* only the "C" locale exists */
int strcoll(const char *__st_a, const char *__st_b) { return strcmp(__st_a, __st_b); }

size_t strxfrm(char *__st_d, const char *__st_s, size_t __st_n) {
    size_t __st_len = strlen(__st_s);
    if (__st_len < __st_n) strcpy(__st_d, __st_s);
    return __st_len;
}

/* ---- searching */
void *memchr(const void *__st_s, int __st_c, size_t __st_n) {
    const char *__st_p = __st_s;
    for (; __st_n; __st_n--, __st_p++)
        if (*__st_p == __st_c) return (void *)__st_p;
    return 0;
}

char *strchr(const char *__st_s, int __st_c) {
    for (;; __st_s++) {
        if (*__st_s == __st_c) return (char *)__st_s;
        if (!*__st_s) return 0;
    }
}

char *strrchr(const char *__st_s, int __st_c) {
    const char *__st_r = 0;
    for (;; __st_s++) {
        if (*__st_s == __st_c) __st_r = __st_s;
        if (!*__st_s) return (char *)__st_r;
    }
}

size_t strspn(const char *__st_s, const char *__st_acc) {
    size_t __st_n = 0;
    while (__st_s[__st_n] && strchr(__st_acc, __st_s[__st_n])) __st_n++;
    return __st_n;
}

size_t strcspn(const char *__st_s, const char *__st_rej) {
    size_t __st_n = 0;
    while (__st_s[__st_n] && !strchr(__st_rej, __st_s[__st_n])) __st_n++;
    return __st_n;
}

char *strpbrk(const char *__st_s, const char *__st_acc) {
    __st_s = __st_s + strcspn(__st_s, __st_acc);
    return *__st_s ? (char *)__st_s : 0;
}

char *strstr(const char *__st_h, const char *__st_nd) {
    size_t __st_k = strlen(__st_nd);
    if (!__st_k) return (char *)__st_h;
    for (; *__st_h; __st_h++)
        if (*__st_h == *__st_nd && strncmp(__st_h, __st_nd, __st_k) == 0)
            return (char *)__st_h;
    return 0;
}

char *strtok_r(char *__st_s, const char *__st_delim, char **__st_save) {
    char *__st_end;
    if (!__st_s) __st_s = *__st_save;
    if (!__st_s) return 0;
    __st_s = __st_s + strspn(__st_s, __st_delim);
    if (!*__st_s) {
        *__st_save = 0;
        return 0;
    }
    __st_end = __st_s + strcspn(__st_s, __st_delim);
    if (*__st_end) {
        *__st_end = 0;
        *__st_save = __st_end + 1;
    } else {
        *__st_save = 0;
    }
    return __st_s;
}

static char *__st_tok_save;

char *strtok(char *__st_s, const char *__st_delim) {
    return strtok_r(__st_s, __st_delim, &__st_tok_save);
}

/* ---- miscellaneous */
void *memset(void *__st_s, int __st_c, size_t __st_n) {
    char *__st_q = __st_s;
    while (__st_n--) *__st_q++ = __st_c;
    return __st_s;
}

size_t strlen(const char *__st_s) {
    const char *__st_p = __st_s;
    while (*__st_p) __st_p++;
    return __st_p - __st_s;
}

size_t strnlen(const char *__st_s, size_t __st_max) {
    size_t __st_n = 0;
    while (__st_n < __st_max && __st_s[__st_n]) __st_n++;
    return __st_n;
}

char *strerror(int __st_e) {
    return __st_e == 0 ? "Success" : "Unknown error";
}
