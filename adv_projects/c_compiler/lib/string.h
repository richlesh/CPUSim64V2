/*
 * string.h - string and memory functions for the CPUSim64 C compiler.
 *
 * Implemented in C in lib/string.c; only used functions are compiled.
 * A char is one 64-bit word holding a Unicode codepoint, and every length
 * and count is in chars (= words).
 */
#ifndef _CPUSIM64_STRING_H
#define _CPUSIM64_STRING_H

/* ---- copying */
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
char *strdup(const char *s);
char *strndup(const char *s, size_t n);

/* ---- concatenation */
char *strcat(char *dst, const char *src);
char *strncat(char *dst, const char *src, size_t n);

/* ---- comparison */
int memcmp(const void *a, const void *b, size_t n);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
int strcoll(const char *a, const char *b);
size_t strxfrm(char *dst, const char *src, size_t n);

/* ---- searching */
void *memchr(const void *s, int c, size_t n);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char *strpbrk(const char *s, const char *accept);
char *strstr(const char *haystack, const char *needle);
char *strtok(char *s, const char *delim);
char *strtok_r(char *s, const char *delim, char **save);

/* ---- miscellaneous */
void *memset(void *s, int c, size_t n);
size_t strlen(const char *s);
size_t strnlen(const char *s, size_t max);
char *strerror(int errnum);

#endif
