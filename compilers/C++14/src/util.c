/*
 * util.c - memory, strings, source files and errors.
 */

void fatal(const char *msg) {
    fprintf(stderr, "cppc: %s\n", msg);
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

/* Zeroed memory that is never freed (tokens, names, hide sets).  Many small
   blocks from the C11 library's malloc cost much time and space on the
   simulator, so they are cut from big chunks instead. */
#define ARENA_CHUNK 32768
static char *arena_p = 0;
static size_t arena_left = 0;

void *anew(size_t n) {
    void *p;
    n = (n + 7) & ~(size_t)7;
    if (n > ARENA_CHUNK / 8) return xalloc(n);
    if (n > arena_left) {
        arena_p = xalloc(ARENA_CHUNK);
        arena_left = ARENA_CHUNK;
    }
    p = arena_p;
    arena_p += n;
    arena_left -= n;
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

/* the last component of a path */
const char *base_name(const char *p) {
    const char *s = strrchr(p, '/');
    return s ? s + 1 : p;
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

/* appends the formatted text, of any length */
void sb_vprintf(StrBuf *b, const char *fmt, va_list ap) {
    char buf[512];
    va_list aq;
    int n;
    va_copy(aq, ap);
    n = vsnprintf(buf, sizeof buf, fmt, aq);
    va_end(aq);
    if (n < 0) return;
    if ((size_t)n < sizeof buf) {
        sb_puts(b, buf);
    } else {
        char *big = xalloc((size_t)n + 1);
        vsnprintf(big, (size_t)n + 1, fmt, ap);
        sb_puts(b, big);
        free(big);
    }
}

void sb_printf(StrBuf *b, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    sb_vprintf(b, fmt, ap);
    va_end(ap);
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
    if (n >= 3 && b[0] == 0xEF && b[1] == 0xBB && b[2] == 0xBF) i = 3;   /* BOM */
    while (i < n) {
        int c = b[i], need = 0, cp = 0, j;
        if (c < 0x80) { t[k++] = c; i++; continue; }
        if ((c & 0xE0) == 0xC0) { need = 1; cp = c & 0x1F; }
        else if ((c & 0xF0) == 0xE0) { need = 2; cp = c & 0x0F; }
        else if ((c & 0xF8) == 0xF0) { need = 3; cp = c & 0x07; }
        else { t[k++] = 0xFFFD; i++; continue; }
        for (j = 1; j <= need; j++) {
            if (i + j >= n || (b[i + j] & 0xC0) != 0x80) break;
            cp = (cp << 6) | (b[i + j] & 0x3F);
        }
        if (j <= need) { t[k++] = 0xFFFD; i++; continue; }
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

/* a name that tokens can refer to (the file name of a #line directive) */
int add_file_name(const char *name) {
    SrcFile *sf;
    int i;
    for (i = 0; i < nfiles; i++)
        if (strcmp(files[i].path, name) == 0) return i;
    if (nfiles == files_cap) {
        files_cap = files_cap ? files_cap * 2 : 8;
        files = xrealloc(files, sizeof(SrcFile) * files_cap);
    }
    sf = &files[nfiles];
    sf->path = xstrdup(name);
    sf->disp = sf->path;
    sf->text = 0;
    sf->len = 0;
    return nfiles++;
}

/* prog.cpp -> prog.gen.c (the translator's output, compiled by the C11 compiler) */
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

void (*err_context_hook)(void) = 0;     /* prints the template instantiations that led to the error */
jmp_buf *err_trap = 0;                 /* set while a substitution is tried: an error returns there */
char err_trap_msg[600];

void verror_at(int file, int line, const char *fmt, va_list ap) {
    char buf[600];
    vsnprintf(buf, sizeof buf, fmt, ap);
    if (err_trap) {
        strcpy(err_trap_msg, buf);
        longjmp(*err_trap, 1);
    }
    if (err_context_hook) err_context_hook();
    if (file >= 0 && file < nfiles)
        fprintf(stderr, "%s:%d: error: %s\n", files[file].disp, line, buf);
    else
        fprintf(stderr, "cppc: error: %s\n", buf);
    exit(1);
}

void error_at(int file, int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(file, line, fmt, ap);
    va_end(ap);
}

void warning_at(int file, int line, const char *fmt, ...) {
    char buf[600];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (file >= 0 && file < nfiles)
        fprintf(stderr, "%s:%d: warning: %s\n", files[file].disp, line, buf);
    else
        fprintf(stderr, "cppc: warning: %s\n", buf);
}

/* ------------------------------------------------------------ names */

#define NAME_BUCKETS 4096
static Name *name_tab[NAME_BUCKETS];

Name *intern(const char *s, int len) {
    int h = 0, i;
    Name *n;
    for (i = 0; i < len; i++) h = (h * 31 + (s[i] & 0x1FFFFF)) & 0x3FFFFFF;
    for (n = name_tab[h % NAME_BUCKETS]; n; n = n->next)
        if (n->hash == h && n->len == len && memcmp(n->s, s, len) == 0) return n;
    n = anew(sizeof(Name));
    n->s = anew(len + 1);
    memcpy(n->s, s, len);
    n->len = len;
    n->hash = h;
    n->next = name_tab[h % NAME_BUCKETS];
    name_tab[h % NAME_BUCKETS] = n;
    return n;
}

Name *intern_str(const char *s) {
    return intern(s, (int)strlen(s));
}

/* ------------------------------------------------------------ tokens */

void tv_push(TokVec *t, Token *x) {
    if (t->n == t->cap) {
        t->cap = t->cap ? t->cap * 2 : 16;
        t->v = xrealloc(t->v, sizeof(Token *) * t->cap);
    }
    t->v[t->n++] = x;
}

Token *tok_new(int kind, const char *text, int file, int line) {
    Token *t = anew(sizeof(Token));
    t->kind = kind;
    t->text = text;
    t->file = file;
    t->line = line;
    return t;
}

Token *tok_copy(const Token *t) {
    Token *c = anew(sizeof(Token));
    *c = *t;
    return c;
}
