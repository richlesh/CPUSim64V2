/*
 * util.c - memory, strings, source files and errors.
 */

void fatal(const char *msg) {
    fprintf(stderr, "rustc: %s\n", msg);
    exit(2);
}

void *xalloc(size_t n) {
    void *p = calloc(1, n ? n : 1);
    if (!p) fatal("out of memory");
    return p;
}

void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n ? n : 1);
    if (!q) fatal("out of memory");
    return q;
}

char *xstrdup(const char *s) {
    char *p = xalloc(strlen(s) + 1);
    strcpy(p, s);
    return p;
}

/* paths for messages: relative to the working directory when shorter */
const char *display_path(const char *p) {
    const char *cwd = getenv("PWD");
    size_t n;
    if (!cwd || p[0] != '/') return p;
    n = strlen(cwd);
    while (n > 1 && cwd[n - 1] == '/') n--;
    if (n > 1 && strncmp(p, cwd, n) == 0 && p[n] == '/') return p + n + 1;
    return p;
}

/* ------------------------------------------------------------ StrBuf */

void sb_init(StrBuf *b) {
    b->cap = 32;
    b->len = 0;
    b->s = xalloc(b->cap);
}

void sb_putc(StrBuf *b, int c) {
    if (b->len + 2 > b->cap) {
        b->cap *= 2;
        b->s = xrealloc(b->s, b->cap);
    }
    b->s[b->len++] = (char)c;
    b->s[b->len] = 0;
}

/* the native encoding of a codepoint: one char under C11, UTF-8 natively */
int cp_encode(char *dst, int cp) {
#ifdef __CPUSIM64__
    dst[0] = (char)cp;
    return 1;
#else
    if (cp < 0x80) {
        dst[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800) {
        dst[0] = (char)(0xC0 | (cp >> 6));
        dst[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        dst[0] = (char)(0xE0 | (cp >> 12));
        dst[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        dst[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    dst[0] = (char)(0xF0 | (cp >> 18));
    dst[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    dst[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    dst[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
#endif
}

void sb_put_cp(StrBuf *b, int cp) {
    char tmp[8];
    int i, n = cp_encode(tmp, cp);
    for (i = 0; i < n; i++) sb_putc(b, tmp[i]);
}

void sb_puts(StrBuf *b, const char *s) {
    while (*s) sb_putc(b, *s++);
}

char *sb_take(StrBuf *b) {
    char *s = b->s;
    sb_init(b);
    return s;
}

void out_cp(FILE *f, int cp) {
#ifdef __CPUSIM64__
    fputc(cp, f);
#else
    StrBuf b;
    sb_init(&b);
    sb_put_cp(&b, cp);
    fputs(b.s, f);
    free(b.s);
#endif
}

/* ------------------------------------------------------------ source files */

SrcFile *files = 0;
int nfiles = 0;
static int files_cap = 0;

/* decodes UTF-8 bytes into codepoints; bad sequences become U+FFFD */
static int *decode_utf8(const unsigned char *b, int n, int *outlen) {
    int *t = xalloc(sizeof(int) * (n + 1));
    int i = 0, k = 0;
    while (i < n) {
        int c = b[i], need = 0, cp = 0, j, good = 1;
        if (c < 0x80) {
            t[k++] = c;
            i++;
            continue;
        }
        if (c >= 0xC2 && c <= 0xDF) { need = 1; cp = c & 0x1F; }
        else if (c >= 0xE0 && c <= 0xEF) { need = 2; cp = c & 0x0F; }
        else if (c >= 0xF0 && c <= 0xF4) { need = 3; cp = c & 0x07; }
        else { t[k++] = 0xFFFD; i++; continue; }
        for (j = 1; j <= need; j++) {
            if (i + j >= n || (b[i + j] & 0xC0) != 0x80) { good = 0; break; }
            cp = (cp << 6) | (b[i + j] & 0x3F);
        }
        if (!good) { t[k++] = 0xFFFD; i++; continue; }
        t[k++] = cp;
        i += need + 1;
    }
    t[k] = 0;
    *outlen = k;
    return t;
}

int load_file(const char *path) {
    FILE *f = fopen(path, "rb");
    unsigned char *buf;
    int n = 0, cap = 4096, got;
    SrcFile *sf;
    if (!f) return -1;
    buf = xalloc(cap);
    for (;;) {
        if (n + 4096 > cap) {
            cap *= 2;
            buf = xrealloc(buf, cap);
        }
        got = (int)fread(buf + n, 1, 4096, f);
        if (got <= 0) break;
        n += got;
    }
    fclose(f);
    if (nfiles == files_cap) {
        files_cap = files_cap ? files_cap * 2 : 8;
        files = xrealloc(files, sizeof(SrcFile) * files_cap);
    }
    sf = &files[nfiles];
    sf->path = xstrdup(path);
    sf->disp = display_path(sf->path);
    sf->text = decode_utf8(buf, n, &sf->len);
    free(buf);
    return nfiles++;
}

/* prog.rs -> prog.gen.c (the translator's output, compiled by the C11 compiler) */
char *output_path(const char *src) {
    size_t n = strlen(src), slash = 0, dot = 0, i;
    char *out;
    for (i = 0; i < n; i++) {
        if (src[i] == '/') slash = i + 1;
    }
    for (i = slash; i < n; i++) {
        if (src[i] == '.') dot = i;
    }
    if (dot <= slash) dot = n;
    out = xalloc(dot + 8);
    memcpy(out, src, dot);
    strcpy(out + dot, ".gen.c");
    return out;
}

/* ------------------------------------------------------------ errors */

void error_at(int file, int line, const char *fmt, ...) {
    char buf[600];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (file >= 0 && file < nfiles)
        fprintf(stderr, "%s:%d: error: %s\n", files[file].disp, line, buf);
    else
        fprintf(stderr, "rustc: error: %s\n", buf);
    exit(1);
}
