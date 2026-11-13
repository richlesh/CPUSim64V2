/*
 * stdio.h - input/output for the CPUSim64 C compiler.
 *
 * Implemented in C in lib/stdio.c on top of a few assembly I/O primitives
 * (open, close, read/write one character, write a block).  Only the
 * functions a program uses are compiled.
 *
 * Characters and strings: a char is one 64-bit word holding a Unicode
 * codepoint and C strings are 0-terminated arrays of such words.  Text
 * streams read and write UTF-8 encoded files one codepoint per char;
 * binary streams ("b" in the fopen mode) read and write one byte per char.
 *
 * Files are CPUSim64 random-access ports (iOPEN_RANDOM_ACCESS_FILE), so every
 * mode supports reading, writing and fseek / ftell as in C.  Positions are
 * byte offsets in the file for text and binary streams alike (a non-ASCII
 * character takes 2-4 bytes in a text file), as on Unix systems.
 *
 * Streams are unbuffered: every output call reaches the operating system
 * before it returns, so nothing needs flushing at exit.
 *
 * printf and scanf are implemented here in C with C semantics (exact
 * floating-point conversion and rounding), not with the CPUSim64 formatter.
 */
#ifndef _CPUSIM64_STDIO_H
#define _CPUSIM64_STDIO_H

#include <stddef.h>
#include <stdarg.h>

#ifndef __SSIZE_T_DEFINED
#define __SSIZE_T_DEFINED
typedef long ssize_t;
#endif

#ifndef EOF
#define EOF (-1)
#endif

#define BUFSIZ          1024
#define FILENAME_MAX    4096
#define FOPEN_MAX       250
#define L_tmpnam        256
#define TMP_MAX         238328
#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2
#define _IOFBF          0
#define _IOLBF          1
#define _IONBF          2

typedef long fpos_t;

/* The members are private to the library. */
typedef struct __FILE {
    long __port;        /* CPUSim64 I/O port, -1 when closed */
    long __flags;       /* __SIO_* mode bits */
    long __unget;       /* pushed-back char, or EOF */
    long __eof;         /* end-of-file indicator */
    long __err;         /* error indicator */
    long __ungpos;      /* position before the pushed-back char */
    char *__name;       /* path (tmpfile removes it on close) */
} FILE;

extern FILE __sio_std[3];
#define stdin   (&__sio_std[0])
#define stdout  (&__sio_std[1])
#define stderr  (&__sio_std[2])

/* ---- files */
FILE *fopen(const char *name, const char *mode);
FILE *freopen(const char *name, const char *mode, FILE *stream);
int fclose(FILE *stream);
int fflush(FILE *stream);
void setbuf(FILE *stream, char *buf);                       /* no effect */
int setvbuf(FILE *stream, char *buf, int mode, size_t size); /* no effect */
int remove(const char *name);
int rename(const char *from, const char *to);
FILE *tmpfile(void);
char *tmpnam(char *buf);
int fileno(FILE *stream);

/* ---- formatted output */
int printf(const char *fmt, ...);
int fprintf(FILE *stream, const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int vfprintf(FILE *stream, const char *fmt, va_list ap);
int vsprintf(char *buf, const char *fmt, va_list ap);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

/* ---- formatted input */
int scanf(const char *fmt, ...);
int fscanf(FILE *stream, const char *fmt, ...);
int sscanf(const char *str, const char *fmt, ...);
int vscanf(const char *fmt, va_list ap);
int vfscanf(FILE *stream, const char *fmt, va_list ap);
int vsscanf(const char *str, const char *fmt, va_list ap);

/* ---- character and line I/O */
int fgetc(FILE *stream);
int getc(FILE *stream);
int getchar(void);
int ungetc(int c, FILE *stream);
char *fgets(char *buf, int size, FILE *stream);
ssize_t getline(char **line, size_t *size, FILE *stream);
ssize_t getdelim(char **line, size_t *size, int delim, FILE *stream);
int fputc(int c, FILE *stream);
int putc(int c, FILE *stream);
int putchar(int c);
int fputs(const char *s, FILE *stream);
int puts(const char *s);

/* ---- direct I/O (sizes in chars, i.e. words; binary streams keep one
        byte per char) */
size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);

/* ---- positioning */
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
void rewind(FILE *stream);
int fgetpos(FILE *stream, fpos_t *pos);
int fsetpos(FILE *stream, const fpos_t *pos);

/* ---- errors */
void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void perror(const char *s);

#endif
