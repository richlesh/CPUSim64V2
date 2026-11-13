/*
 * stdio.c - C implementation of lib/stdio.h.
 *
 * Appended to the program by the compiler; only used functions are
 * compiled.  All private names start with __sio_ because this file shares
 * the program's macro namespace.
 *
 * Operating-system access goes through these assembly primitives (runtime
 * library); they take C strings (0-terminated word arrays) and convert to
 * CPUSim64 packed strings internally:
 */
#include <stdio.h>

long __io_open(const char *__sio_name, long __sio_mode);   /* -> port or -1 */
long __io_close(long __sio_port);
long __io_getc(long __sio_port);                /* codepoint or -1 */
long __io_getb(long __sio_port);                /* byte or -1 */
long __io_putc(long __sio_port, long __sio_c);
long __io_putb(long __sio_port, long __sio_b);
long __io_write(long __sio_port, const char *__sio_buf, long __sio_n);
long __io_flush(long __sio_port);
long __io_remove(const char *__sio_name);       /* 0 or -1 */
long __io_exists(const char *__sio_name);       /* 1 if a regular file */
long __io_tmpname(char *__sio_buf);             /* 1 on success */
double __atof_exact(const char *__sio_s);
void *malloc(size_t __sio_n);
void *realloc(void *__sio_p, size_t __sio_n);
void free(void *__sio_p);

/* mode bits (FILE.__flags) */
#define __SIO_RD    1
#define __SIO_WR    2
#define __SIO_APP   4
#define __SIO_BIN   8
#define __SIO_UPD   16
#define __SIO_TMP   32
#define __SIO_STD   64

FILE __sio_std[3] = {
    { 0, __SIO_RD | __SIO_STD, EOF, 0, 0, 0, -1, 1, 0 },
    { 1, __SIO_WR | __SIO_STD, EOF, 0, 0, 0, -1, 2, 0 },
    { 2, __SIO_WR | __SIO_STD, EOF, 0, 0, 0, -1, 2, 0 }
};

static long __sio_strlen(const char *__sio_s) {
    long __sio_n = 0;
    while (__sio_s[__sio_n]) __sio_n++;
    return __sio_n;
}

static char *__sio_strdup(const char *__sio_s) {
    long __sio_n = __sio_strlen(__sio_s), __sio_i;
    char *__sio_d = malloc(__sio_n + 1);
    if (!__sio_d) return 0;
    for (__sio_i = 0; __sio_i <= __sio_n; __sio_i++) __sio_d[__sio_i] = __sio_s[__sio_i];
    return __sio_d;
}

/* ===================================================================== */
/* streams                                                               */
/* ===================================================================== */

/* opens the port for the stream's current file in direction dir (1 read,
   2 write) positioned at __pos.  Returns 0 on failure. */
static int __sio_reopen(FILE *__sio_f, int __sio_dir) {
    long __sio_bin = (__sio_f->__flags & __SIO_BIN) ? 3 : 0;
    long __sio_p, __sio_i;
    if (__sio_f->__port >= 0) __io_close(__sio_f->__port);
    __sio_f->__port = -1;
    __sio_f->__dir = 0;
    if (__sio_dir == 1) {
        __sio_p = __io_open(__sio_f->__name, __sio_bin);
        if (__sio_p < 0) return 0;
        __sio_f->__port = __sio_p;
        __sio_f->__dir = 1;
        for (__sio_i = 0; __sio_i < __sio_f->__pos; __sio_i++) {
            long __sio_c = __sio_bin ? __io_getb(__sio_p) : __io_getc(__sio_p);
            if (__sio_c < 0) {
                __sio_f->__pos = __sio_i;
                __sio_f->__len = __sio_i;
                break;
            }
        }
        return 1;
    }
    /* writing is only possible at the end of the file (append) */
    __sio_p = __io_open(__sio_f->__name, __sio_bin + 2);
    if (__sio_p < 0) return 0;
    __sio_f->__port = __sio_p;
    __sio_f->__dir = 2;
    return 1;
}

/* makes the stream ready to read (dir 1) or write (dir 2) */
static int __sio_ready(FILE *__sio_f, int __sio_dir) {
    if (__sio_f->__port < 0 && __sio_f->__dir == 0 && !__sio_f->__name) {
        __sio_f->__err = 1;
        return 0;
    }
    if (__sio_dir == 1 && !(__sio_f->__flags & __SIO_RD)) { __sio_f->__err = 1; return 0; }
    if (__sio_dir == 2 && !(__sio_f->__flags & __SIO_WR)) { __sio_f->__err = 1; return 0; }
    if (__sio_f->__dir == __sio_dir) return 1;
    if (__sio_f->__flags & __SIO_STD) return 1;
    if (__sio_dir == 2) {
        /* append mode always writes at the end; otherwise the position
           must be the end of the file */
        if (!(__sio_f->__flags & __SIO_APP)
                && (__sio_f->__len < 0 || __sio_f->__pos != __sio_f->__len)) {
            __sio_f->__err = 1;
            return 0;
        }
    }
    if (!__sio_reopen(__sio_f, __sio_dir)) {
        __sio_f->__err = 1;
        return 0;
    }
    if (__sio_dir == 2 && __sio_f->__len >= 0) __sio_f->__pos = __sio_f->__len;
    return 1;
}

/* parses an fopen mode; returns the flags or 0 if invalid */
static long __sio_mode(const char *__sio_m) {
    long __sio_fl;
    if (*__sio_m == 'r') __sio_fl = __SIO_RD;
    else if (*__sio_m == 'w') __sio_fl = __SIO_WR;
    else if (*__sio_m == 'a') __sio_fl = __SIO_WR | __SIO_APP;
    else return 0;
    for (__sio_m++; *__sio_m; __sio_m++) {
        if (*__sio_m == 'b') __sio_fl = __sio_fl | __SIO_BIN;
        else if (*__sio_m == '+') __sio_fl = __sio_fl | __SIO_RD | __SIO_WR | __SIO_UPD;
        else if (*__sio_m == 'x') __sio_fl = __sio_fl | 128;
        else if (*__sio_m == 't') ;
        else return 0;
    }
    return __sio_fl;
}

/* opens name with mode into *f; returns f or 0 */
static FILE *__sio_open(FILE *__sio_f, const char *__sio_name, const char *__sio_mstr) {
    long __sio_fl = __sio_mode(__sio_mstr);
    long __sio_bin, __sio_p, __sio_how;
    if (!__sio_fl) return 0;
    __sio_bin = (__sio_fl & __SIO_BIN) ? 3 : 0;
    if (*__sio_mstr == 'r') {
        if (!__io_exists(__sio_name)) return 0;
        __sio_how = 0;
    } else if (*__sio_mstr == 'w') {
        if ((__sio_fl & 128) && __io_exists(__sio_name)) return 0;
        __sio_how = 1;
    } else {
        __sio_how = 2;
    }
    __sio_p = __io_open(__sio_name, __sio_bin + __sio_how);
    if (__sio_p < 0) return 0;
    __sio_f->__port = __sio_p;
    __sio_f->__flags = __sio_fl & ~128L;
    __sio_f->__unget = EOF;
    __sio_f->__eof = 0;
    __sio_f->__err = 0;
    __sio_f->__pos = 0;
    __sio_f->__len = __sio_how == 1 ? 0 : -1;
    __sio_f->__dir = __sio_how == 0 ? 1 : 2;
    __sio_f->__name = __sio_strdup(__sio_name);
    if (__sio_how == 2) {
        /* the length (and position) of an appended file is found lazily */
        __sio_f->__pos = -1;
    }
    return __sio_f;
}

/* finds the length of an appended file the first time it is needed */
static void __sio_fixpos(FILE *__sio_f) {
    long __sio_n = 0, __sio_p, __sio_c;
    if (__sio_f->__pos >= 0) return;
    __sio_p = __io_open(__sio_f->__name, (__sio_f->__flags & __SIO_BIN) ? 3 : 0);
    if (__sio_p >= 0) {
        for (;;) {
            __sio_c = (__sio_f->__flags & __SIO_BIN) ? __io_getb(__sio_p) : __io_getc(__sio_p);
            if (__sio_c < 0) break;
            __sio_n++;
        }
        __io_close(__sio_p);
    }
    __sio_f->__pos = __sio_n;
    __sio_f->__len = __sio_n;
}

FILE *fopen(const char *__sio_name, const char *__sio_m) {
    FILE *__sio_f = malloc(sizeof(FILE));
    if (!__sio_f) return 0;
    __sio_f->__port = -1;
    __sio_f->__name = 0;
    if (!__sio_open(__sio_f, __sio_name, __sio_m)) {
        free(__sio_f);
        return 0;
    }
    return __sio_f;
}

static void __sio_release(FILE *__sio_f) {
    if (__sio_f->__port > 2) __io_close(__sio_f->__port);
    __sio_f->__port = -1;
    __sio_f->__dir = 0;
    if (__sio_f->__name) {
        if (__sio_f->__flags & __SIO_TMP) __io_remove(__sio_f->__name);
        free(__sio_f->__name);
        __sio_f->__name = 0;
    }
}

FILE *freopen(const char *__sio_name, const char *__sio_m, FILE *__sio_f) {
    long __sio_std = __sio_f->__flags & __SIO_STD;
    if (!__sio_name) return __sio_f;
    if (!__sio_std) __sio_release(__sio_f);
    else __sio_f->__port = -1;
    if (!__sio_open(__sio_f, __sio_name, __sio_m)) return 0;
    return __sio_f;
}

int fclose(FILE *__sio_f) {
    if (__sio_f->__flags & __SIO_STD) {
        if (__sio_f->__port > 2) __io_close(__sio_f->__port);
        __sio_f->__port = -1;
        __sio_f->__dir = 0;
        __sio_f->__flags = __SIO_STD;
        return 0;
    }
    __sio_release(__sio_f);
    free(__sio_f);
    return 0;
}

int fflush(FILE *__sio_f) {
    if (!__sio_f) {
        __io_flush(1);
        __io_flush(2);
        return 0;
    }
    if (__sio_f->__port >= 0 && __sio_f->__dir == 2) __io_flush(__sio_f->__port);
    return 0;
}

void setbuf(FILE *__sio_f, char *__sio_b) { }

int setvbuf(FILE *__sio_f, char *__sio_b, int __sio_mode, size_t __sio_n) { return 0; }

int fileno(FILE *__sio_f) { return (int)__sio_f->__port; }

int remove(const char *__sio_name) { return (int)__io_remove(__sio_name); }

int rename(const char *__sio_from, const char *__sio_to) {
    long __sio_in, __sio_out, __sio_c;
    if (!__io_exists(__sio_from)) return -1;
    __sio_in = __io_open(__sio_from, 3);
    if (__sio_in < 0) return -1;
    __sio_out = __io_open(__sio_to, 4);
    if (__sio_out < 0) {
        __io_close(__sio_in);
        return -1;
    }
    for (;;) {
        __sio_c = __io_getb(__sio_in);
        if (__sio_c < 0) break;
        __io_putb(__sio_out, __sio_c);
    }
    __io_close(__sio_in);
    __io_close(__sio_out);
    return (int)__io_remove(__sio_from);
}

static char __sio_tmpbuf[L_tmpnam];

char *tmpnam(char *__sio_buf) {
    if (!__sio_buf) __sio_buf = __sio_tmpbuf;
    if (!__io_tmpname(__sio_buf)) return 0;
    return __sio_buf;
}

FILE *tmpfile(void) {
    char __sio_name[L_tmpnam];
    FILE *__sio_f;
    if (!tmpnam(__sio_name)) return 0;
    __sio_f = fopen(__sio_name, "w+b");
    if (__sio_f) __sio_f->__flags = __sio_f->__flags | __SIO_TMP;
    return __sio_f;
}

/* ===================================================================== */
/* character I/O                                                         */
/* ===================================================================== */

int fgetc(FILE *__sio_f) {
    long __sio_c;
    if (__sio_f->__unget != EOF) {
        __sio_c = __sio_f->__unget;
        __sio_f->__unget = EOF;
        __sio_f->__pos++;
        return (int)__sio_c;
    }
    if (__sio_f->__eof) return EOF;
    if (!__sio_ready(__sio_f, 1)) return EOF;
    if (__sio_f->__flags & __SIO_BIN) __sio_c = __io_getb(__sio_f->__port);
    else __sio_c = __io_getc(__sio_f->__port);
    if (__sio_c < 0) {
        __sio_f->__eof = 1;
        if (!(__sio_f->__flags & __SIO_STD)) __sio_f->__len = __sio_f->__pos;
        return EOF;
    }
    __sio_f->__pos++;
    return (int)__sio_c;
}

int getc(FILE *__sio_f) { return fgetc(__sio_f); }

int getchar(void) { return fgetc(stdin); }

int ungetc(int __sio_c, FILE *__sio_f) {
    if (__sio_c == EOF || __sio_f->__unget != EOF) return EOF;
    __sio_f->__unget = __sio_c;
    __sio_f->__eof = 0;
    __sio_f->__pos--;
    return __sio_c;
}

/* writes n chars; returns n or -1 */
static long __sio_write(FILE *__sio_f, const char *__sio_s, long __sio_n) {
    long __sio_i;
    if (__sio_n <= 0) return 0;
    if (!__sio_ready(__sio_f, 2)) return -1;
    if (__sio_f->__pos < 0) __sio_fixpos(__sio_f);
    if (__sio_f->__flags & __SIO_BIN) {
        for (__sio_i = 0; __sio_i < __sio_n; __sio_i++)
            __io_putb(__sio_f->__port, __sio_s[__sio_i] & 255);
    } else if (__sio_n == 1) {
        __io_putc(__sio_f->__port, __sio_s[0]);
    } else {
        __io_write(__sio_f->__port, __sio_s, __sio_n);
    }
    __sio_f->__pos = __sio_f->__pos + __sio_n;
    if (__sio_f->__len >= 0 && __sio_f->__pos > __sio_f->__len) __sio_f->__len = __sio_f->__pos;
    return __sio_n;
}

int fputc(int __sio_c, FILE *__sio_f) {
    char __sio_b[1];
    __sio_b[0] = __sio_c;
    if (__sio_write(__sio_f, __sio_b, 1) < 0) return EOF;
    return __sio_c;
}

int putc(int __sio_c, FILE *__sio_f) { return fputc(__sio_c, __sio_f); }

int putchar(int __sio_c) { return fputc(__sio_c, stdout); }

int fputs(const char *__sio_s, FILE *__sio_f) {
    if (__sio_write(__sio_f, __sio_s, __sio_strlen(__sio_s)) < 0) return EOF;
    return 0;
}

int puts(const char *__sio_s) {
    if (fputs(__sio_s, stdout) == EOF) return EOF;
    if (fputc('\n', stdout) == EOF) return EOF;
    return 0;
}

char *fgets(char *__sio_buf, int __sio_n, FILE *__sio_f) {
    int __sio_i = 0, __sio_c;
    if (__sio_n <= 0) return 0;
    while (__sio_i < __sio_n - 1) {
        __sio_c = fgetc(__sio_f);
        if (__sio_c == EOF) break;
        __sio_buf[__sio_i++] = __sio_c;
        if (__sio_c == '\n') break;
    }
    if (__sio_i == 0) return 0;
    __sio_buf[__sio_i] = 0;
    return __sio_buf;
}

ssize_t getdelim(char **__sio_lp, size_t *__sio_np, int __sio_delim, FILE *__sio_f) {
    size_t __sio_i = 0;
    int __sio_c;
    if (!__sio_lp || !__sio_np) return -1;
    if (!*__sio_lp || *__sio_np == 0) {
        *__sio_np = 120;
        *__sio_lp = malloc(*__sio_np);
        if (!*__sio_lp) return -1;
    }
    for (;;) {
        __sio_c = fgetc(__sio_f);
        if (__sio_c == EOF) break;
        if (__sio_i + 1 >= *__sio_np) {
            char *__sio_nb = realloc(*__sio_lp, *__sio_np * 2);
            if (!__sio_nb) return -1;
            *__sio_lp = __sio_nb;
            *__sio_np = *__sio_np * 2;
        }
        (*__sio_lp)[__sio_i++] = __sio_c;
        if (__sio_c == __sio_delim) break;
    }
    (*__sio_lp)[__sio_i] = 0;
    if (__sio_i == 0) return -1;
    return (ssize_t)__sio_i;
}

ssize_t getline(char **__sio_lp, size_t *__sio_np, FILE *__sio_f) {
    return getdelim(__sio_lp, __sio_np, '\n', __sio_f);
}

size_t fread(void *__sio_ptr, size_t __sio_size, size_t __sio_cnt, FILE *__sio_f) {
    char *__sio_p = __sio_ptr;
    size_t __sio_total = __sio_size * __sio_cnt, __sio_i;
    int __sio_c;
    if (__sio_size == 0) return 0;
    for (__sio_i = 0; __sio_i < __sio_total; __sio_i++) {
        __sio_c = fgetc(__sio_f);
        if (__sio_c == EOF) break;
        __sio_p[__sio_i] = __sio_c;
    }
    return __sio_i / __sio_size;
}

size_t fwrite(const void *__sio_ptr, size_t __sio_size, size_t __sio_cnt, FILE *__sio_f) {
    if (__sio_size == 0 || __sio_cnt == 0) return 0;
    if (__sio_write(__sio_f, __sio_ptr, (long)(__sio_size * __sio_cnt)) < 0) return 0;
    return __sio_cnt;
}

/* ===================================================================== */
/* positioning                                                           */
/* ===================================================================== */

long ftell(FILE *__sio_f) {
    if (__sio_f->__flags & __SIO_STD) return -1;
    if (__sio_f->__pos < 0) __sio_fixpos(__sio_f);
    return __sio_f->__pos;
}

/* length of the file, reading it if necessary */
static long __sio_length(FILE *__sio_f) {
    if (__sio_f->__len >= 0) return __sio_f->__len;
    if (__sio_f->__dir == 2 && __sio_f->__port >= 0) __io_flush(__sio_f->__port);
    {
        long __sio_save = __sio_f->__pos;
        __sio_f->__pos = -1;
        __sio_fixpos(__sio_f);
        __sio_f->__pos = __sio_save;
    }
    return __sio_f->__len;
}

int fseek(FILE *__sio_f, long __sio_off, int __sio_whence) {
    long __sio_target, __sio_cur;
    if (__sio_f->__flags & __SIO_STD) return -1;
    __sio_cur = ftell(__sio_f);
    if (__sio_whence == SEEK_SET) __sio_target = __sio_off;
    else if (__sio_whence == SEEK_CUR) __sio_target = __sio_cur + __sio_off;
    else if (__sio_whence == SEEK_END) __sio_target = __sio_length(__sio_f) + __sio_off;
    else return -1;
    if (__sio_target < 0) return -1;
    __sio_f->__unget = EOF;
    __sio_f->__eof = 0;
    if (__sio_f->__dir == 1 && __sio_target >= __sio_f->__pos) {
        /* read forward */
        while (__sio_f->__pos < __sio_target) {
            if (fgetc(__sio_f) == EOF) {
                __sio_f->__eof = 0;
                break;
            }
        }
        return 0;
    }
    if (__sio_f->__dir == 2) {
        if (__sio_target == __sio_f->__pos) return 0;
        __io_flush(__sio_f->__port);
    }
    /* reposition: the next read or write reopens the file */
    if (__sio_f->__port >= 0) __io_close(__sio_f->__port);
    __sio_f->__port = -1;
    __sio_f->__dir = 0;
    __sio_f->__pos = __sio_target;
    return 0;
}

void rewind(FILE *__sio_f) {
    fseek(__sio_f, 0, SEEK_SET);
    __sio_f->__err = 0;
}

int fgetpos(FILE *__sio_f, fpos_t *__sio_p) {
    long __sio_t = ftell(__sio_f);
    if (__sio_t < 0) return -1;
    *__sio_p = __sio_t;
    return 0;
}

int fsetpos(FILE *__sio_f, const fpos_t *__sio_p) {
    return fseek(__sio_f, *__sio_p, SEEK_SET);
}

void clearerr(FILE *__sio_f) {
    __sio_f->__eof = 0;
    __sio_f->__err = 0;
}

int feof(FILE *__sio_f) { return __sio_f->__eof != 0; }

int ferror(FILE *__sio_f) { return __sio_f->__err != 0; }

void perror(const char *__sio_s) {
    if (__sio_s && *__sio_s) {
        fputs(__sio_s, stderr);
        fputs(": ", stderr);
    }
    fputs("Error\n", stderr);
}

/* ===================================================================== */
/* formatted output                                                      */
/* ===================================================================== */

/* output sink: a word buffer that starts in caller-provided storage and
   moves to the heap when it grows */
struct __sio_out {
    char *__sio_b;
    long __sio_n;
    long __sio_cap;
    long __sio_heap;
};

static void __sio_grow(struct __sio_out *__sio_o) {
    long __sio_nc = __sio_o->__sio_cap * 2 + 64, __sio_i;
    char *__sio_nb;
    if (__sio_o->__sio_heap) {
        __sio_nb = realloc(__sio_o->__sio_b, __sio_nc);
    } else {
        __sio_nb = malloc(__sio_nc);
        if (__sio_nb)
            for (__sio_i = 0; __sio_i < __sio_o->__sio_n; __sio_i++) __sio_nb[__sio_i] = __sio_o->__sio_b[__sio_i];
    }
    if (!__sio_nb) return;
    __sio_o->__sio_b = __sio_nb;
    __sio_o->__sio_cap = __sio_nc;
    __sio_o->__sio_heap = 1;
}

static void __sio_put(struct __sio_out *__sio_o, long __sio_c) {
    if (__sio_o->__sio_n >= __sio_o->__sio_cap) {
        __sio_grow(__sio_o);
        if (__sio_o->__sio_n >= __sio_o->__sio_cap) return;
    }
    __sio_o->__sio_b[__sio_o->__sio_n++] = __sio_c;
}

static void __sio_puts(struct __sio_out *__sio_o, const char *__sio_s, long __sio_n) {
    long __sio_i;
    for (__sio_i = 0; __sio_i < __sio_n; __sio_i++) __sio_put(__sio_o, __sio_s[__sio_i]);
}

static void __sio_rep(struct __sio_out *__sio_o, long __sio_c, long __sio_n) {
    while (__sio_n-- > 0) __sio_put(__sio_o, __sio_c);
}

/* conversion specification */
struct __sio_spec {
    long __sio_minus, __sio_plus, __sio_space, __sio_zero, __sio_alt;
    long __sio_width, __sio_prec, __sio_len, __sio_conv;
};

/* writes prefix and body padded to the field width; zero padding goes
   between prefix and body when allowed */
static void __sio_field(struct __sio_out *__sio_o, struct __sio_spec *__sio_sp,
                        const char *__sio_pre, long __sio_npre,
                        const char *__sio_body, long __sio_nbody, long __sio_zok) {
    long __sio_pad = __sio_sp->__sio_width - __sio_npre - __sio_nbody;
    if (__sio_pad < 0) __sio_pad = 0;
    if (__sio_sp->__sio_minus) {
        __sio_puts(__sio_o, __sio_pre, __sio_npre);
        __sio_puts(__sio_o, __sio_body, __sio_nbody);
        __sio_rep(__sio_o, ' ', __sio_pad);
    } else if (__sio_sp->__sio_zero && __sio_zok) {
        __sio_puts(__sio_o, __sio_pre, __sio_npre);
        __sio_rep(__sio_o, '0', __sio_pad);
        __sio_puts(__sio_o, __sio_body, __sio_nbody);
    } else {
        __sio_rep(__sio_o, ' ', __sio_pad);
        __sio_puts(__sio_o, __sio_pre, __sio_npre);
        __sio_puts(__sio_o, __sio_body, __sio_nbody);
    }
}

/* ---- integers */
static void __sio_fmt_int(struct __sio_out *__sio_o, struct __sio_spec *__sio_sp,
                          unsigned long __sio_v, long __sio_neg) {
    char __sio_dig[80], __sio_pre[4], __sio_body[160];
    long __sio_nd = 0, __sio_np = 0, __sio_nb = 0, __sio_i;
    long __sio_c = __sio_sp->__sio_conv;
    unsigned long __sio_t = __sio_v;
    const char *__sio_hex = __sio_c == 'X' ? "0123456789ABCDEF" : "0123456789abcdef";
    if (__sio_c == 'o') {
        while (__sio_t) { __sio_dig[__sio_nd++] = '0' + (long)(__sio_t & 7); __sio_t = __sio_t >> 3; }
    } else if (__sio_c == 'x' || __sio_c == 'X' || __sio_c == 'p') {
        while (__sio_t) { __sio_dig[__sio_nd++] = __sio_hex[__sio_t & 15]; __sio_t = __sio_t >> 4; }
    } else {
        while (__sio_t) { __sio_dig[__sio_nd++] = '0' + (long)(__sio_t % 10); __sio_t = __sio_t / 10; }
    }
    /* precision: minimum number of digits (default 1) */
    if (__sio_sp->__sio_prec < 0) {
        if (__sio_nd == 0) __sio_dig[__sio_nd++] = '0';
    } else {
        while (__sio_nd < __sio_sp->__sio_prec && __sio_nd < 79) __sio_dig[__sio_nd++] = '0';
    }
    if (__sio_c == 'o' && __sio_sp->__sio_alt && (__sio_nd == 0 || __sio_dig[__sio_nd - 1] != '0'))
        __sio_dig[__sio_nd++] = '0';
    if (__sio_c == 'd' || __sio_c == 'i') {
        if (__sio_neg) __sio_pre[__sio_np++] = '-';
        else if (__sio_sp->__sio_plus) __sio_pre[__sio_np++] = '+';
        else if (__sio_sp->__sio_space) __sio_pre[__sio_np++] = ' ';
    }
    if ((__sio_c == 'x' || __sio_c == 'X') && __sio_sp->__sio_alt && __sio_v != 0) {
        __sio_pre[__sio_np++] = '0';
        __sio_pre[__sio_np++] = __sio_c;
    }
    if (__sio_c == 'p') {
        __sio_pre[__sio_np++] = '0';
        __sio_pre[__sio_np++] = 'x';
    }
    for (__sio_i = __sio_nd - 1; __sio_i >= 0; __sio_i--) __sio_body[__sio_nb++] = __sio_dig[__sio_i];
    __sio_field(__sio_o, __sio_sp, __sio_pre, __sio_np, __sio_body, __sio_nb, __sio_sp->__sio_prec < 0);
}

/* ---- floating point */
union __sio_bits { double __sio_d; unsigned long __sio_u; };

static unsigned long __sio_dbits(double __sio_x) {
    union __sio_bits __sio_b;
    __sio_b.__sio_d = __sio_x;
    return __sio_b.__sio_u;
}

/* exact decimal value of a non-negative double: 0.d[0]d[1]...d[n-1] * 10^exp
   (no trailing zero digits; n == 0 means zero) */
#define __SIO_NDIG 800
struct __sio_dec {
    long __sio_n;
    long __sio_exp;
    char __sio_d[__SIO_NDIG];
};

static long __sio_mul(long *__sio_l, long __sio_nl, long __sio_f) {
    long __sio_i, __sio_t, __sio_carry = 0;
    for (__sio_i = 0; __sio_i < __sio_nl; __sio_i++) {
        __sio_t = __sio_l[__sio_i] * __sio_f + __sio_carry;
        __sio_l[__sio_i] = __sio_t % 1000000000;
        __sio_carry = __sio_t / 1000000000;
    }
    while (__sio_carry) {
        __sio_l[__sio_nl++] = __sio_carry % 1000000000;
        __sio_carry = __sio_carry / 1000000000;
    }
    return __sio_nl;
}

static void __sio_exact(double __sio_x, struct __sio_dec *__sio_r) {
    unsigned long __sio_u = __sio_dbits(__sio_x) & 0x7fffffffffffffffUL;
    long __sio_ef = (long)(__sio_u >> 52);
    long __sio_m = (long)(__sio_u & 0xfffffffffffffUL);
    long __sio_e, __sio_k, __sio_s, __sio_nl, __sio_i, __sio_j, __sio_v, __sio_n = 0;
    long __sio_limb[100];
    char __sio_g[9];
    if (__sio_u == 0) {
        __sio_r->__sio_n = 0;
        __sio_r->__sio_exp = 1;
        return;
    }
    if (__sio_ef == 0) __sio_e = -1074;
    else {
        __sio_m = __sio_m | 0x10000000000000L;
        __sio_e = __sio_ef - 1075;
    }
    while ((__sio_m & 1) == 0) {
        __sio_m = __sio_m >> 1;
        __sio_e++;
    }
    __sio_limb[0] = __sio_m % 1000000000;
    __sio_limb[1] = __sio_m / 1000000000;
    __sio_nl = __sio_limb[1] ? 2 : 1;
    if (__sio_e > 0) {
        for (__sio_k = __sio_e; __sio_k > 0; __sio_k = __sio_k - __sio_s) {
            __sio_s = __sio_k > 30 ? 30 : __sio_k;
            __sio_nl = __sio_mul(__sio_limb, __sio_nl, 1L << __sio_s);
        }
    } else if (__sio_e < 0) {
        for (__sio_k = -__sio_e; __sio_k > 0; __sio_k = __sio_k - __sio_s) {
            long __sio_f = 1;
            __sio_s = __sio_k > 13 ? 13 : __sio_k;
            for (__sio_j = 0; __sio_j < __sio_s; __sio_j++) __sio_f = __sio_f * 5;
            __sio_nl = __sio_mul(__sio_limb, __sio_nl, __sio_f);
        }
    }
    /* limbs -> digits, most significant first */
    for (__sio_i = __sio_nl - 1; __sio_i >= 0; __sio_i--) {
        __sio_v = __sio_limb[__sio_i];
        for (__sio_j = 8; __sio_j >= 0; __sio_j--) {
            __sio_g[__sio_j] = __sio_v % 10;
            __sio_v = __sio_v / 10;
        }
        for (__sio_j = 0; __sio_j < 9; __sio_j++) {
            if (__sio_n == 0 && __sio_g[__sio_j] == 0) continue;
            __sio_r->__sio_d[__sio_n++] = __sio_g[__sio_j];
        }
    }
    __sio_r->__sio_exp = __sio_n + (__sio_e < 0 ? __sio_e : 0);
    while (__sio_n > 0 && __sio_r->__sio_d[__sio_n - 1] == 0) __sio_n--;
    __sio_r->__sio_n = __sio_n;
}

/* rounds to k significant digits (k may be <= 0), half to even */
static void __sio_round(struct __sio_dec *__sio_r, long __sio_k) {
    long __sio_n = __sio_r->__sio_n, __sio_dk, __sio_up, __sio_prev, __sio_i;
    if (__sio_k >= __sio_n) return;
    if (__sio_k < 0) {
        __sio_r->__sio_n = 0;
        return;
    }
    __sio_dk = __sio_r->__sio_d[__sio_k];
    __sio_prev = __sio_k > 0 ? __sio_r->__sio_d[__sio_k - 1] : 0;
    __sio_up = __sio_dk > 5 || (__sio_dk == 5 && (__sio_n > __sio_k + 1 || (__sio_prev & 1)));
    __sio_n = __sio_k;
    if (__sio_up) {
        __sio_i = __sio_k - 1;
        while (__sio_i >= 0 && __sio_r->__sio_d[__sio_i] == 9) {
            __sio_r->__sio_d[__sio_i] = 0;
            __sio_i--;
        }
        if (__sio_i < 0) {
            __sio_r->__sio_d[0] = 1;
            __sio_n = 1;
            __sio_r->__sio_exp++;
        } else {
            __sio_r->__sio_d[__sio_i]++;
        }
    }
    while (__sio_n > 0 && __sio_r->__sio_d[__sio_n - 1] == 0) __sio_n--;
    __sio_r->__sio_n = __sio_n;
}

static long __sio_digit(struct __sio_dec *__sio_r, long __sio_j) {
    if (__sio_j < 0 || __sio_j >= __sio_r->__sio_n) return 0;
    return __sio_r->__sio_d[__sio_j];
}

/* %f body (r already rounded or exact) with p decimals */
static void __sio_body_f(struct __sio_out *__sio_o, struct __sio_dec *__sio_r, long __sio_p, long __sio_alt) {
    long __sio_j, __sio_ex = __sio_r->__sio_n == 0 ? 0 : __sio_r->__sio_exp;
    if (__sio_ex <= 0) __sio_put(__sio_o, '0');
    else
        for (__sio_j = 0; __sio_j < __sio_ex; __sio_j++) __sio_put(__sio_o, '0' + __sio_digit(__sio_r, __sio_j));
    if (__sio_p > 0 || __sio_alt) __sio_put(__sio_o, '.');
    for (__sio_j = __sio_ex; __sio_j < __sio_ex + __sio_p; __sio_j++)
        __sio_put(__sio_o, '0' + __sio_digit(__sio_r, __sio_j));
}

/* %e body (r rounded to p + 1 digits) */
static void __sio_body_e(struct __sio_out *__sio_o, struct __sio_dec *__sio_r, long __sio_p,
                         long __sio_alt, long __sio_upper) {
    long __sio_j, __sio_x = __sio_r->__sio_n == 0 ? 0 : __sio_r->__sio_exp - 1;
    char __sio_xd[8];
    long __sio_nx = 0;
    __sio_put(__sio_o, '0' + __sio_digit(__sio_r, 0));
    if (__sio_p > 0 || __sio_alt) __sio_put(__sio_o, '.');
    for (__sio_j = 1; __sio_j <= __sio_p; __sio_j++) __sio_put(__sio_o, '0' + __sio_digit(__sio_r, __sio_j));
    __sio_put(__sio_o, __sio_upper ? 'E' : 'e');
    __sio_put(__sio_o, __sio_x < 0 ? '-' : '+');
    if (__sio_x < 0) __sio_x = -__sio_x;
    do {
        __sio_xd[__sio_nx++] = '0' + __sio_x % 10;
        __sio_x = __sio_x / 10;
    } while (__sio_x);
    if (__sio_nx < 2) __sio_xd[__sio_nx++] = '0';
    while (__sio_nx > 0) __sio_put(__sio_o, __sio_xd[--__sio_nx]);
}

/* %a body: hexadecimal floating point */
static void __sio_body_a(struct __sio_out *__sio_o, double __sio_x, long __sio_p,
                         long __sio_alt, long __sio_upper) {
    unsigned long __sio_u = __sio_dbits(__sio_x) & 0x7fffffffffffffffUL;
    long __sio_ef = (long)(__sio_u >> 52), __sio_e, __sio_nd, __sio_i, __sio_lead, __sio_extra;
    unsigned long __sio_m = __sio_u & 0xfffffffffffffUL;
    const char *__sio_hex = __sio_upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char __sio_xd[8];
    long __sio_nx = 0;
    if (__sio_u == 0) {
        __sio_lead = 0;
        __sio_e = 0;
    } else if (__sio_ef == 0) {
        __sio_lead = 0;
        __sio_e = -1022;
    } else {
        __sio_lead = 1;
        __sio_e = __sio_ef - 1023;
    }
    /* 13 hex digits of fraction; round to p digits (half to even) */
    if (__sio_p >= 0 && __sio_p < 13) {
        long __sio_drop = (13 - __sio_p) * 4;
        unsigned long __sio_rem = __sio_m & ((1UL << __sio_drop) - 1);
        unsigned long __sio_half = 1UL << (__sio_drop - 1);
        __sio_m = __sio_m >> __sio_drop;
        if (__sio_rem > __sio_half || (__sio_rem == __sio_half && (__sio_m & 1))) {
            __sio_m++;
            if (__sio_m >> (__sio_p * 4)) {
                __sio_m = __sio_m & ((1UL << (__sio_p * 4)) - 1);
                __sio_lead++;
            }
        }
        __sio_nd = __sio_p;
        __sio_extra = 0;
    } else {
        __sio_nd = 13;
        __sio_extra = __sio_p > 13 ? __sio_p - 13 : 0;
        if (__sio_p < 0)
            while (__sio_nd > 0 && (__sio_m & 15) == 0) {
                __sio_m = __sio_m >> 4;
                __sio_nd--;
            }
    }
    __sio_put(__sio_o, '0' + __sio_lead);
    if (__sio_nd > 0 || __sio_extra > 0 || __sio_alt) __sio_put(__sio_o, '.');
    for (__sio_i = __sio_nd - 1; __sio_i >= 0; __sio_i--)
        __sio_put(__sio_o, __sio_hex[(__sio_m >> (__sio_i * 4)) & 15]);
    __sio_rep(__sio_o, '0', __sio_extra);
    __sio_put(__sio_o, __sio_upper ? 'P' : 'p');
    __sio_put(__sio_o, __sio_e < 0 ? '-' : '+');
    if (__sio_e < 0) __sio_e = -__sio_e;
    do {
        __sio_xd[__sio_nx++] = '0' + __sio_e % 10;
        __sio_e = __sio_e / 10;
    } while (__sio_e);
    while (__sio_nx > 0) __sio_put(__sio_o, __sio_xd[--__sio_nx]);
}

static void __sio_fmt_float(struct __sio_out *__sio_o, struct __sio_spec *__sio_sp, double __sio_x) {
    unsigned long __sio_u = __sio_dbits(__sio_x);
    long __sio_neg = (__sio_u >> 63) != 0;
    long __sio_c = __sio_sp->__sio_conv;
    long __sio_upper = __sio_c == 'E' || __sio_c == 'F' || __sio_c == 'G' || __sio_c == 'A';
    long __sio_p = __sio_sp->__sio_prec;
    char __sio_pre[4], __sio_lbuf[64];
    long __sio_np = 0;
    struct __sio_out __sio_t;
    struct __sio_dec __sio_r;
    if (__sio_neg) __sio_pre[__sio_np++] = '-';
    else if (__sio_sp->__sio_plus) __sio_pre[__sio_np++] = '+';
    else if (__sio_sp->__sio_space) __sio_pre[__sio_np++] = ' ';
    if ((__sio_u & 0x7ff0000000000000UL) == 0x7ff0000000000000UL) {
        long __sio_nan = (__sio_u & 0xfffffffffffffUL) != 0;
        const char *__sio_w = __sio_nan ? (__sio_upper ? "NAN" : "nan") : (__sio_upper ? "INF" : "inf");
        __sio_field(__sio_o, __sio_sp, __sio_pre, __sio_np, __sio_w, 3, 0);
        return;
    }
    __sio_t.__sio_b = __sio_lbuf;
    __sio_t.__sio_n = 0;
    __sio_t.__sio_cap = 64;
    __sio_t.__sio_heap = 0;
    if (__sio_c == 'a' || __sio_c == 'A') {
        __sio_pre[__sio_np++] = '0';
        __sio_pre[__sio_np++] = __sio_upper ? 'X' : 'x';
        __sio_body_a(&__sio_t, __sio_x, __sio_p, __sio_sp->__sio_alt, __sio_upper);
    } else {
        if (__sio_p < 0) __sio_p = 6;
        __sio_exact(__sio_x, &__sio_r);
        if (__sio_c == 'f' || __sio_c == 'F') {
            __sio_round(&__sio_r, __sio_r.__sio_exp + __sio_p);
            __sio_body_f(&__sio_t, &__sio_r, __sio_p, __sio_sp->__sio_alt);
        } else if (__sio_c == 'e' || __sio_c == 'E') {
            __sio_round(&__sio_r, __sio_p + 1);
            __sio_body_e(&__sio_t, &__sio_r, __sio_p, __sio_sp->__sio_alt, __sio_upper);
        } else {
            /* %g: style e if X < -4 or X >= P, else style f */
            long __sio_P = __sio_p == 0 ? 1 : __sio_p, __sio_X, __sio_i, __sio_dot = -1, __sio_end;
            __sio_round(&__sio_r, __sio_P);
            __sio_X = __sio_r.__sio_n == 0 ? 0 : __sio_r.__sio_exp - 1;
            if (__sio_P > __sio_X && __sio_X >= -4)
                __sio_body_f(&__sio_t, &__sio_r, __sio_P - 1 - __sio_X, __sio_sp->__sio_alt);
            else
                __sio_body_e(&__sio_t, &__sio_r, __sio_P - 1, __sio_sp->__sio_alt, __sio_upper);
            if (!__sio_sp->__sio_alt) {
                /* remove trailing zeros of the fraction */
                __sio_end = __sio_t.__sio_n;
                for (__sio_i = 0; __sio_i < __sio_t.__sio_n; __sio_i++) {
                    if (__sio_t.__sio_b[__sio_i] == '.') __sio_dot = __sio_i;
                    if (__sio_t.__sio_b[__sio_i] == 'e' || __sio_t.__sio_b[__sio_i] == 'E') {
                        __sio_end = __sio_i;
                        break;
                    }
                }
                if (__sio_dot >= 0) {
                    long __sio_k = __sio_end;
                    while (__sio_k > __sio_dot + 1 && __sio_t.__sio_b[__sio_k - 1] == '0') __sio_k--;
                    if (__sio_k == __sio_dot + 1) __sio_k = __sio_dot;
                    if (__sio_k < __sio_end) {
                        long __sio_shift = __sio_end - __sio_k;
                        for (__sio_i = __sio_end; __sio_i < __sio_t.__sio_n; __sio_i++)
                            __sio_t.__sio_b[__sio_i - __sio_shift] = __sio_t.__sio_b[__sio_i];
                        __sio_t.__sio_n = __sio_t.__sio_n - __sio_shift;
                    }
                }
            }
        }
    }
    __sio_field(__sio_o, __sio_sp, __sio_pre, __sio_np, __sio_t.__sio_b, __sio_t.__sio_n, 1);
    if (__sio_t.__sio_heap) free(__sio_t.__sio_b);
}

/* ---- the format interpreter */
static void __sio_vformat(struct __sio_out *__sio_o, const char *__sio_f, va_list __sio_ap) {
    struct __sio_spec __sio_sp;
    long __sio_c, __sio_v;
    unsigned long __sio_uv;
    const char *__sio_s;
    for (; *__sio_f; __sio_f++) {
        if (*__sio_f != '%') {
            __sio_put(__sio_o, *__sio_f);
            continue;
        }
        __sio_f++;
        if (*__sio_f == '%') {
            __sio_put(__sio_o, '%');
            continue;
        }
        __sio_sp.__sio_minus = __sio_sp.__sio_plus = __sio_sp.__sio_space = 0;
        __sio_sp.__sio_zero = __sio_sp.__sio_alt = 0;
        __sio_sp.__sio_width = 0;
        __sio_sp.__sio_prec = -1;
        __sio_sp.__sio_len = 0;
        for (;; __sio_f++) {
            if (*__sio_f == '-') __sio_sp.__sio_minus = 1;
            else if (*__sio_f == '+') __sio_sp.__sio_plus = 1;
            else if (*__sio_f == ' ') __sio_sp.__sio_space = 1;
            else if (*__sio_f == '0') __sio_sp.__sio_zero = 1;
            else if (*__sio_f == '#') __sio_sp.__sio_alt = 1;
            else break;
        }
        if (*__sio_f == '*') {
            __sio_sp.__sio_width = va_arg(__sio_ap, int);
            if (__sio_sp.__sio_width < 0) {
                __sio_sp.__sio_minus = 1;
                __sio_sp.__sio_width = -__sio_sp.__sio_width;
            }
            __sio_f++;
        } else {
            while (*__sio_f >= '0' && *__sio_f <= '9')
                __sio_sp.__sio_width = __sio_sp.__sio_width * 10 + (*__sio_f++ - '0');
        }
        if (*__sio_f == '.') {
            __sio_f++;
            __sio_sp.__sio_prec = 0;
            if (*__sio_f == '*') {
                __sio_sp.__sio_prec = va_arg(__sio_ap, int);
                if (__sio_sp.__sio_prec < 0) __sio_sp.__sio_prec = -1;
                __sio_f++;
            } else {
                while (*__sio_f >= '0' && *__sio_f <= '9')
                    __sio_sp.__sio_prec = __sio_sp.__sio_prec * 10 + (*__sio_f++ - '0');
            }
        }
        /* length modifiers: h and hh narrow the value, the others are
           all 64 bits here */
        for (;; __sio_f++) {
            if (*__sio_f == 'h') __sio_sp.__sio_len = __sio_sp.__sio_len == 'h' ? 'H' : 'h';
            else if (*__sio_f == 'l' || *__sio_f == 'L' || *__sio_f == 'q' || *__sio_f == 'j'
                     || *__sio_f == 'z' || *__sio_f == 't') ;
            else break;
        }
        __sio_c = *__sio_f;
        if (!__sio_c) break;
        __sio_sp.__sio_conv = __sio_c;
        if (__sio_c == 'd' || __sio_c == 'i') {
            __sio_v = va_arg(__sio_ap, long);
            if (__sio_sp.__sio_len == 'h') __sio_v = (__sio_v << 48) >> 48;
            else if (__sio_sp.__sio_len == 'H') __sio_v = (__sio_v << 56) >> 56;
            if (__sio_v < 0) __sio_fmt_int(__sio_o, &__sio_sp, -(unsigned long)__sio_v, 1);
            else __sio_fmt_int(__sio_o, &__sio_sp, (unsigned long)__sio_v, 0);
        } else if (__sio_c == 'u' || __sio_c == 'o' || __sio_c == 'x' || __sio_c == 'X') {
            __sio_uv = va_arg(__sio_ap, unsigned long);
            if (__sio_sp.__sio_len == 'h') __sio_uv = __sio_uv & 0xffff;
            else if (__sio_sp.__sio_len == 'H') __sio_uv = __sio_uv & 0xff;
            __sio_fmt_int(__sio_o, &__sio_sp, __sio_uv, 0);
        } else if (__sio_c == 'p') {
            __sio_uv = (unsigned long)va_arg(__sio_ap, void *);
            __sio_sp.__sio_alt = 0;
            __sio_fmt_int(__sio_o, &__sio_sp, __sio_uv, 0);
        } else if (__sio_c == 'c') {
            char __sio_ch[1];
            __sio_ch[0] = va_arg(__sio_ap, int);
            __sio_field(__sio_o, &__sio_sp, "", 0, __sio_ch, 1, 0);
        } else if (__sio_c == 's') {
            long __sio_n = 0;
            __sio_s = va_arg(__sio_ap, const char *);
            if (!__sio_s) __sio_s = "(null)";
            while (__sio_s[__sio_n] && (__sio_sp.__sio_prec < 0 || __sio_n < __sio_sp.__sio_prec)) __sio_n++;
            __sio_field(__sio_o, &__sio_sp, "", 0, __sio_s, __sio_n, 0);
        } else if (__sio_c == 'n') {
            long *__sio_np = va_arg(__sio_ap, long *);
            *__sio_np = __sio_o->__sio_n;
        } else if (__sio_c == 'f' || __sio_c == 'F' || __sio_c == 'e' || __sio_c == 'E'
                   || __sio_c == 'g' || __sio_c == 'G' || __sio_c == 'a' || __sio_c == 'A') {
            __sio_fmt_float(__sio_o, &__sio_sp, va_arg(__sio_ap, double));
        } else if (__sio_c == '%') {
            __sio_field(__sio_o, &__sio_sp, "", 0, "%", 1, 0);
        } else {
            /* unknown conversion: print it as written */
            __sio_put(__sio_o, '%');
            __sio_put(__sio_o, __sio_c);
        }
    }
}

int vfprintf(FILE *__sio_f, const char *__sio_fmt, va_list __sio_ap) {
    char __sio_lbuf[256];
    struct __sio_out __sio_o;
    long __sio_n;
    __sio_o.__sio_b = __sio_lbuf;
    __sio_o.__sio_n = 0;
    __sio_o.__sio_cap = 256;
    __sio_o.__sio_heap = 0;
    __sio_vformat(&__sio_o, __sio_fmt, __sio_ap);
    __sio_n = __sio_write(__sio_f, __sio_o.__sio_b, __sio_o.__sio_n);
    if (__sio_o.__sio_heap) free(__sio_o.__sio_b);
    if (__sio_n < 0) return -1;
    return (int)__sio_o.__sio_n;
}

int vprintf(const char *__sio_fmt, va_list __sio_ap) {
    return vfprintf(stdout, __sio_fmt, __sio_ap);
}

int vsnprintf(char *__sio_buf, size_t __sio_size, const char *__sio_fmt, va_list __sio_ap) {
    char __sio_lbuf[256];
    struct __sio_out __sio_o;
    long __sio_i, __sio_m;
    __sio_o.__sio_b = __sio_lbuf;
    __sio_o.__sio_n = 0;
    __sio_o.__sio_cap = 256;
    __sio_o.__sio_heap = 0;
    __sio_vformat(&__sio_o, __sio_fmt, __sio_ap);
    if (__sio_size > 0) {
        __sio_m = __sio_o.__sio_n;
        if ((size_t)__sio_m > __sio_size - 1) __sio_m = (long)(__sio_size - 1);
        for (__sio_i = 0; __sio_i < __sio_m; __sio_i++) __sio_buf[__sio_i] = __sio_o.__sio_b[__sio_i];
        __sio_buf[__sio_m] = 0;
    }
    if (__sio_o.__sio_heap) free(__sio_o.__sio_b);
    return (int)__sio_o.__sio_n;
}

int vsprintf(char *__sio_buf, const char *__sio_fmt, va_list __sio_ap) {
    return vsnprintf(__sio_buf, 0x7fffffffffffffffUL, __sio_fmt, __sio_ap);
}

int printf(const char *__sio_fmt, ...) {
    va_list __sio_ap;
    int __sio_n;
    va_start(__sio_ap, __sio_fmt);
    __sio_n = vfprintf(stdout, __sio_fmt, __sio_ap);
    va_end(__sio_ap);
    return __sio_n;
}

int fprintf(FILE *__sio_f, const char *__sio_fmt, ...) {
    va_list __sio_ap;
    int __sio_n;
    va_start(__sio_ap, __sio_fmt);
    __sio_n = vfprintf(__sio_f, __sio_fmt, __sio_ap);
    va_end(__sio_ap);
    return __sio_n;
}

int sprintf(char *__sio_buf, const char *__sio_fmt, ...) {
    va_list __sio_ap;
    int __sio_n;
    va_start(__sio_ap, __sio_fmt);
    __sio_n = vsnprintf(__sio_buf, 0x7fffffffffffffffUL, __sio_fmt, __sio_ap);
    va_end(__sio_ap);
    return __sio_n;
}

int snprintf(char *__sio_buf, size_t __sio_size, const char *__sio_fmt, ...) {
    va_list __sio_ap;
    int __sio_n;
    va_start(__sio_ap, __sio_fmt);
    __sio_n = vsnprintf(__sio_buf, __sio_size, __sio_fmt, __sio_ap);
    va_end(__sio_ap);
    return __sio_n;
}

/* ===================================================================== */
/* formatted input                                                       */
/* ===================================================================== */

/* input source: a stream or a C string */
struct __sio_in {
    FILE *__sio_f;
    const char *__sio_s;
    long __sio_i;
    long __sio_cnt;
};

static long __sio_get(struct __sio_in *__sio_in) {
    long __sio_c;
    if (__sio_in->__sio_f) {
        __sio_c = fgetc(__sio_in->__sio_f);
    } else {
        __sio_c = __sio_in->__sio_s[__sio_in->__sio_i];
        if (__sio_c == 0) __sio_c = EOF;
        else __sio_in->__sio_i++;
    }
    if (__sio_c != EOF) __sio_in->__sio_cnt++;
    return __sio_c;
}

static void __sio_unget(struct __sio_in *__sio_in, long __sio_c) {
    if (__sio_c == EOF) return;
    if (__sio_in->__sio_f) ungetc((int)__sio_c, __sio_in->__sio_f);
    else __sio_in->__sio_i--;
    __sio_in->__sio_cnt--;
}

static long __sio_isspace(long __sio_c) {
    return __sio_c == ' ' || (__sio_c >= 9 && __sio_c <= 13);
}

/* value of digit c in base, or -1 */
static long __sio_dval(long __sio_c, long __sio_base) {
    long __sio_d;
    if (__sio_c >= '0' && __sio_c <= '9') __sio_d = __sio_c - '0';
    else if (__sio_c >= 'a' && __sio_c <= 'z') __sio_d = __sio_c - 'a' + 10;
    else if (__sio_c >= 'A' && __sio_c <= 'Z') __sio_d = __sio_c - 'A' + 10;
    else return -1;
    return __sio_d < __sio_base ? __sio_d : -1;
}

/* is c in the scanset [set_start, set_end) ? */
static long __sio_inset(const char *__sio_a, const char *__sio_b, long __sio_c) {
    long __sio_neg = 0, __sio_hit = 0;
    if (__sio_a < __sio_b && *__sio_a == '^') {
        __sio_neg = 1;
        __sio_a++;
    }
    while (__sio_a < __sio_b) {
        if (__sio_a + 2 < __sio_b && __sio_a[1] == '-') {
            if (__sio_c >= __sio_a[0] && __sio_c <= __sio_a[2]) __sio_hit = 1;
            __sio_a = __sio_a + 3;
        } else {
            if (__sio_c == *__sio_a) __sio_hit = 1;
            __sio_a++;
        }
    }
    return __sio_neg ? !__sio_hit : __sio_hit;
}

/* lowercase letter match for inf / nan */
static long __sio_lower(long __sio_c) {
    return (__sio_c >= 'A' && __sio_c <= 'Z') ? __sio_c + 32 : __sio_c;
}

static int __sio_vscan(struct __sio_in *__sio_in, const char *__sio_f, va_list __sio_ap) {
    long __sio_assigned = 0, __sio_convs = 0, __sio_c, __sio_sup, __sio_w, __sio_conv;
    for (; *__sio_f; __sio_f++) {
        if (__sio_isspace(*__sio_f)) {
            do __sio_c = __sio_get(__sio_in); while (__sio_isspace(__sio_c));
            __sio_unget(__sio_in, __sio_c);
            continue;
        }
        if (*__sio_f != '%' || __sio_f[1] == '%') {
            if (*__sio_f == '%') {
                __sio_f++;
                do __sio_c = __sio_get(__sio_in); while (__sio_isspace(__sio_c));
            } else {
                __sio_c = __sio_get(__sio_in);
            }
            if (__sio_c != *__sio_f) {
                __sio_unget(__sio_in, __sio_c);
                if (__sio_c == EOF && __sio_convs == 0) return EOF;
                return (int)__sio_assigned;
            }
            continue;
        }
        __sio_f++;
        __sio_sup = 0;
        if (*__sio_f == '*') {
            __sio_sup = 1;
            __sio_f++;
        }
        __sio_w = 0;
        while (*__sio_f >= '0' && *__sio_f <= '9') __sio_w = __sio_w * 10 + (*__sio_f++ - '0');
        while (*__sio_f == 'h' || *__sio_f == 'l' || *__sio_f == 'L' || *__sio_f == 'q'
               || *__sio_f == 'j' || *__sio_f == 'z' || *__sio_f == 't')
            __sio_f++;
        __sio_conv = *__sio_f;
        if (!__sio_conv) break;
        if (__sio_conv == 'n') {
            if (!__sio_sup) *va_arg(__sio_ap, long *) = __sio_in->__sio_cnt;
            continue;
        }
        if (__sio_conv != 'c' && __sio_conv != '[') {
            do __sio_c = __sio_get(__sio_in); while (__sio_isspace(__sio_c));
            __sio_unget(__sio_in, __sio_c);
        }
        if (__sio_conv == 'c') {
            char *__sio_p = __sio_sup ? 0 : va_arg(__sio_ap, char *);
            long __sio_k;
            if (__sio_w == 0) __sio_w = 1;
            for (__sio_k = 0; __sio_k < __sio_w; __sio_k++) {
                __sio_c = __sio_get(__sio_in);
                if (__sio_c == EOF) break;
                if (__sio_p) __sio_p[__sio_k] = __sio_c;
            }
            if (__sio_k == 0) return __sio_convs == 0 ? EOF : (int)__sio_assigned;
        } else if (__sio_conv == 's' || __sio_conv == '[') {
            char *__sio_p = __sio_sup ? 0 : va_arg(__sio_ap, char *);
            const char *__sio_sa = 0, *__sio_sb = 0;
            long __sio_k = 0;
            if (__sio_conv == '[') {
                __sio_sa = __sio_f + 1;
                __sio_sb = __sio_sa;
                if (*__sio_sb == '^') __sio_sb++;
                if (*__sio_sb == ']') __sio_sb++;
                while (*__sio_sb && *__sio_sb != ']') __sio_sb++;
                __sio_f = __sio_sb;
                if (!*__sio_f) __sio_f--;
            }
            for (;;) {
                if (__sio_w && __sio_k >= __sio_w) break;
                __sio_c = __sio_get(__sio_in);
                if (__sio_c == EOF) break;
                if (__sio_conv == 's' ? __sio_isspace(__sio_c) : !__sio_inset(__sio_sa, __sio_sb, __sio_c)) {
                    __sio_unget(__sio_in, __sio_c);
                    break;
                }
                if (__sio_p) __sio_p[__sio_k] = __sio_c;
                __sio_k++;
            }
            if (__sio_k == 0) {
                if (__sio_c == EOF && __sio_convs == 0) return EOF;
                return (int)__sio_assigned;
            }
            if (__sio_p) __sio_p[__sio_k] = 0;
        } else if (__sio_conv == 'd' || __sio_conv == 'i' || __sio_conv == 'u' || __sio_conv == 'o'
                   || __sio_conv == 'x' || __sio_conv == 'X' || __sio_conv == 'p') {
            long __sio_base = 10, __sio_neg = 0, __sio_nd = 0, __sio_k = 0, __sio_d;
            unsigned long __sio_v = 0;
            if (__sio_conv == 'o') __sio_base = 8;
            else if (__sio_conv == 'x' || __sio_conv == 'X' || __sio_conv == 'p') __sio_base = 16;
            else if (__sio_conv == 'i') __sio_base = 0;
            if (__sio_w == 0) __sio_w = 0x7fffffffffffffffL;
            __sio_c = __sio_get(__sio_in);
            if (__sio_c == '-' || __sio_c == '+') {
                __sio_neg = __sio_c == '-';
                __sio_k++;
                __sio_c = __sio_k < __sio_w ? __sio_get(__sio_in) : EOF;
            }
            if ((__sio_base == 0 || __sio_base == 16) && __sio_c == '0' && __sio_k < __sio_w) {
                __sio_nd = 1;
                __sio_k++;
                __sio_c = __sio_k < __sio_w ? __sio_get(__sio_in) : EOF;
                if ((__sio_c == 'x' || __sio_c == 'X') && __sio_k < __sio_w) {
                    __sio_base = 16;
                    __sio_nd = 0;
                    __sio_k++;
                    __sio_c = __sio_k < __sio_w ? __sio_get(__sio_in) : EOF;
                } else if (__sio_base == 0) {
                    __sio_base = 8;
                }
            }
            if (__sio_base == 0) __sio_base = 10;
            while (__sio_k < __sio_w && (__sio_d = __sio_dval(__sio_c, __sio_base)) >= 0) {
                __sio_v = __sio_v * __sio_base + __sio_d;
                __sio_nd++;
                __sio_k++;
                __sio_c = __sio_k < __sio_w ? __sio_get(__sio_in) : EOF;
            }
            if (__sio_k < __sio_w || __sio_c != EOF) __sio_unget(__sio_in, __sio_c);
            if (__sio_nd == 0) {
                /* end of input before the first conversion is an input
                   failure, even after a sign */
                if (__sio_c == EOF && __sio_convs == 0 && __sio_k < __sio_w) return EOF;
                return (int)__sio_assigned;
            }
            if (__sio_neg) __sio_v = -__sio_v;
            if (!__sio_sup) {
                if (__sio_conv == 'p') *va_arg(__sio_ap, void **) = (void *)__sio_v;
                else *va_arg(__sio_ap, long *) = (long)__sio_v;
            }
        } else if (__sio_conv == 'f' || __sio_conv == 'e' || __sio_conv == 'g' || __sio_conv == 'a'
                   || __sio_conv == 'E' || __sio_conv == 'F' || __sio_conv == 'G' || __sio_conv == 'A') {
            char __sio_buf[80];
            long __sio_n = 0, __sio_dig = 0, __sio_k = 0, __sio_special = 0;
            double __sio_val;
            if (__sio_w == 0 || __sio_w > 78) __sio_w = 78;
            __sio_c = __sio_get(__sio_in);
            if (__sio_c == '-' || __sio_c == '+') {
                __sio_buf[__sio_n++] = __sio_c;
                __sio_k++;
                __sio_c = __sio_k < __sio_w ? __sio_get(__sio_in) : EOF;
            }
            if (__sio_lower(__sio_c) == 'i' || __sio_lower(__sio_c) == 'n') {
                /* inf, infinity, nan */
                const char *__sio_word = __sio_lower(__sio_c) == 'i' ? "infinity" : "nan";
                long __sio_m = 0;
                while (__sio_word[__sio_m] && __sio_k < __sio_w && __sio_lower(__sio_c) == __sio_word[__sio_m]) {
                    __sio_m++;
                    __sio_k++;
                    __sio_c = __sio_k < __sio_w ? __sio_get(__sio_in) : EOF;
                }
                __sio_unget(__sio_in, __sio_c);
                if (!(__sio_m == 3 || __sio_m == 8)) return __sio_convs == 0 && __sio_c == EOF ? EOF : (int)__sio_assigned;
                __sio_val = __sio_word[0] == 'n' ? 0.0 / 0.0 : 1.0 / 0.0;
                if (__sio_n > 0 && __sio_buf[0] == '-') __sio_val = -__sio_val;
                __sio_special = 1;
            } else {
                while (__sio_k < __sio_w && __sio_c >= '0' && __sio_c <= '9') {
                    __sio_buf[__sio_n++] = __sio_c;
                    __sio_dig = 1;
                    __sio_k++;
                    __sio_c = __sio_k < __sio_w ? __sio_get(__sio_in) : EOF;
                }
                if (__sio_c == '.' && __sio_k < __sio_w) {
                    __sio_buf[__sio_n++] = __sio_c;
                    __sio_k++;
                    __sio_c = __sio_k < __sio_w ? __sio_get(__sio_in) : EOF;
                    while (__sio_k < __sio_w && __sio_c >= '0' && __sio_c <= '9') {
                        __sio_buf[__sio_n++] = __sio_c;
                        __sio_dig = 1;
                        __sio_k++;
                        __sio_c = __sio_k < __sio_w ? __sio_get(__sio_in) : EOF;
                    }
                }
                if (__sio_dig && (__sio_c == 'e' || __sio_c == 'E') && __sio_k < __sio_w) {
                    long __sio_save = __sio_n;
                    __sio_buf[__sio_n++] = 'e';
                    __sio_k++;
                    __sio_c = __sio_k < __sio_w ? __sio_get(__sio_in) : EOF;
                    if ((__sio_c == '-' || __sio_c == '+') && __sio_k < __sio_w) {
                        __sio_buf[__sio_n++] = __sio_c;
                        __sio_k++;
                        __sio_c = __sio_k < __sio_w ? __sio_get(__sio_in) : EOF;
                    }
                    if (!(__sio_c >= '0' && __sio_c <= '9')) __sio_n = __sio_save;
                    while (__sio_k < __sio_w && __sio_c >= '0' && __sio_c <= '9') {
                        __sio_buf[__sio_n++] = __sio_c;
                        __sio_k++;
                        __sio_c = __sio_k < __sio_w ? __sio_get(__sio_in) : EOF;
                    }
                }
                __sio_unget(__sio_in, __sio_c);
                if (!__sio_dig) {
                    if (__sio_c == EOF && __sio_convs == 0) return EOF;
                    return (int)__sio_assigned;
                }
                __sio_buf[__sio_n] = 0;
                __sio_val = __atof_exact(__sio_buf);
            }
            if (!__sio_sup) *va_arg(__sio_ap, double *) = __sio_val;
        } else {
            return (int)__sio_assigned;
        }
        __sio_convs++;
        if (!__sio_sup) __sio_assigned++;
    }
    return (int)__sio_assigned;
}

int vfscanf(FILE *__sio_f, const char *__sio_fmt, va_list __sio_ap) {
    struct __sio_in __sio_in;
    __sio_in.__sio_f = __sio_f;
    __sio_in.__sio_s = 0;
    __sio_in.__sio_i = 0;
    __sio_in.__sio_cnt = 0;
    return __sio_vscan(&__sio_in, __sio_fmt, __sio_ap);
}

int vscanf(const char *__sio_fmt, va_list __sio_ap) {
    return vfscanf(stdin, __sio_fmt, __sio_ap);
}

int vsscanf(const char *__sio_s, const char *__sio_fmt, va_list __sio_ap) {
    struct __sio_in __sio_in;
    __sio_in.__sio_f = 0;
    __sio_in.__sio_s = __sio_s;
    __sio_in.__sio_i = 0;
    __sio_in.__sio_cnt = 0;
    return __sio_vscan(&__sio_in, __sio_fmt, __sio_ap);
}

int scanf(const char *__sio_fmt, ...) {
    va_list __sio_ap;
    int __sio_n;
    va_start(__sio_ap, __sio_fmt);
    __sio_n = vfscanf(stdin, __sio_fmt, __sio_ap);
    va_end(__sio_ap);
    return __sio_n;
}

int fscanf(FILE *__sio_f, const char *__sio_fmt, ...) {
    va_list __sio_ap;
    int __sio_n;
    va_start(__sio_ap, __sio_fmt);
    __sio_n = vfscanf(__sio_f, __sio_fmt, __sio_ap);
    va_end(__sio_ap);
    return __sio_n;
}

int sscanf(const char *__sio_s, const char *__sio_fmt, ...) {
    va_list __sio_ap;
    int __sio_n;
    va_start(__sio_ap, __sio_fmt);
    __sio_n = vsscanf(__sio_s, __sio_fmt, __sio_ap);
    va_end(__sio_ap);
    return __sio_n;
}
