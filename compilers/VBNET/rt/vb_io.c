/*
 * vb_io.c - files for System.IO (included by vb_rt.c): the functions behind
 * File, StreamReader and StreamWriter of lib/Collections.vb.  Text files are
 * UTF-8 on disk; the C library reads and writes one codepoint per char.  A
 * file handle is the FILE pointer as a Long.
 */

#define E_FILENOTFOUND 17
#define E_IO 18

/* a VB String as a C string (one codepoint per char) */
static char *c_path(long *s) {
    long n, i;
    char *p;
    if (!s) fail("ArgumentNullException", "Value cannot be null. (Parameter 'path')");
    n = s[0];
    if (n == 0) fail("ArgumentException", "The value cannot be an empty string. (Parameter 'path')");
    p = malloc(n + 1);
    for (i = 0; i < n; i++) p[i] = (char)s[1 + i];
    p[n] = 0;
    return p;
}

static void not_found(long *path) {
    char msg[400];
    long i, k = sprintf(msg, "Could not find file '");
    for (i = 0; i < path[0] && k < 380; i++) msg[k++] = (char)path[1 + i];
    sprintf(msg + k, "'.");
    vb_throw_obj(vb_make_exception(E_FILENOTFOUND, str_from_c(msg)));
}

static void io_error(long *path) {
    char msg[400];
    long i, k = sprintf(msg, "Could not open the file '");
    for (i = 0; i < path[0] && k < 380; i++) msg[k++] = (char)path[1 + i];
    sprintf(msg + k, "'.");
    vb_throw_obj(vb_make_exception(E_IO, str_from_c(msg)));
}

/* the codepoints read as a String (buf grows) */
typedef struct {
    long *v;
    long n, cap;
} Chars;

static void ch_put(Chars *c, long x) {
    if (c->n == c->cap) {
        c->cap = c->cap ? 2 * c->cap : 64;
        c->v = realloc(c->v, sizeof(long) * c->cap);
    }
    c->v[c->n++] = x;
}

static long *ch_str(Chars *c) {
    long *s = vbrt_str_new(c->n), i;
    for (i = 0; i < c->n; i++) s[1 + i] = c->v[i];
    free(c->v);
    return s;
}

long vbrt_file_open(long *path, long mode) {
    char *p = c_path(path);
    FILE *f = fopen(p, mode == 0 ? "r" : mode == 1 ? "w" : "a");
    free(p);
    if (!f) {
        if (mode == 0) not_found(path);
        io_error(path);
    }
    return (long)f;
}

static FILE *open_file(long f) {
    if (!f) fail("ObjectDisposedException", "Cannot access a closed file.");
    return (FILE *)f;
}

/* ReadLine: the line without its \n or \r\n; Nothing at the end */
long *vbrt_file_readline(long f) {
    FILE *fp = open_file(f);
    Chars c;
    int x = fgetc(fp);
    memset(&c, 0, sizeof c);
    if (x == EOF) return 0;
    while (x != EOF && x != '\n') {
        if (x == '\r') {
            int y = fgetc(fp);
            if (y != '\n' && y != EOF) ungetc(y, fp);
            break;
        }
        ch_put(&c, x);
        x = fgetc(fp);
    }
    return ch_str(&c);
}

long *vbrt_file_readtoend(long f) {
    FILE *fp = open_file(f);
    Chars c;
    int x;
    memset(&c, 0, sizeof c);
    while ((x = fgetc(fp)) != EOF) ch_put(&c, x);
    return ch_str(&c);
}

long vbrt_file_read(long f) {
    int x = fgetc(open_file(f));
    return x == EOF ? -1 : x;
}

long vbrt_file_peek(long f) {
    FILE *fp = open_file(f);
    int x = fgetc(fp);
    if (x == EOF) return -1;
    ungetc(x, fp);
    return x;
}

void vbrt_file_write(long f, long *s) {
    FILE *fp = open_file(f);
    long i;
    for (i = 0; s && i < s[0]; i++) fputc((int)s[1 + i], fp);
}

void vbrt_file_close(long f) {
    if (f) fclose((FILE *)f);
}

long *vbrt_file_readall(long *path) {
    long f = vbrt_file_open(path, 0);
    long *s = vbrt_file_readtoend(f);
    vbrt_file_close(f);
    return s;
}

void vbrt_file_writeall(long *path, long *text, long append) {
    long f = vbrt_file_open(path, append ? 2 : 1);
    vbrt_file_write(f, text);
    vbrt_file_close(f);
}

long vbrt_file_exists(long *path) {
    char *p;
    FILE *f;
    if (!path || path[0] == 0) return 0;
    p = c_path(path);
    f = fopen(p, "r");
    free(p);
    if (!f) return 0;
    fclose(f);
    return -1;
}

void vbrt_file_delete(long *path) {
    char *p = c_path(path);
    remove(p);
    free(p);
}
