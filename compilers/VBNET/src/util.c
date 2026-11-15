/*
 * util.c - memory, strings, source files, errors and name interning.
 */

void fatal(const char *msg) {
    fprintf(stderr, "vbc: %s\n", msg);
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

void sb_put_cp(StrBuf *b, int cp) {
#ifdef __CPUSIM64__
    sb_putc(b, cp);
#else
    if (cp < 0x80) sb_putc(b, cp);
    else if (cp < 0x800) {
        sb_putc(b, 0xC0 | (cp >> 6));
        sb_putc(b, 0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        sb_putc(b, 0xE0 | (cp >> 12));
        sb_putc(b, 0x80 | ((cp >> 6) & 0x3F));
        sb_putc(b, 0x80 | (cp & 0x3F));
    } else {
        sb_putc(b, 0xF0 | (cp >> 18));
        sb_putc(b, 0x80 | ((cp >> 12) & 0x3F));
        sb_putc(b, 0x80 | ((cp >> 6) & 0x3F));
        sb_putc(b, 0x80 | (cp & 0x3F));
    }
#endif
}

void sb_puts(StrBuf *b, const char *s) {
    while (*s) sb_putc(b, *s++);
}

void sb_printf(StrBuf *b, const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    sb_puts(b, buf);
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

/* ------------------------------------------------------------ errors */

void verror_at(int file, int line, const char *fmt, va_list ap) {
    char buf[600];
    vsnprintf(buf, sizeof buf, fmt, ap);
    if (file >= 0 && file < nfiles)
        fprintf(stderr, "%s:%d: error: %s\n", files[file].disp, line, buf);
    else
        fprintf(stderr, "vbc: error: %s\n", buf);
    exit(1);
}

void error_at(int file, int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(file, line, fmt, ap);
    va_end(ap);
}

/* ------------------------------------------------------------ names */

#define NAME_BUCKETS 2048
static Name *name_tab[NAME_BUCKETS];

/* case folding for identifiers: ASCII, Latin-1, Latin Extended-A, Greek and
   Cyrillic capitals map to their small letters */
int fold_cp(int cp) {
    if (cp >= 'A' && cp <= 'Z') return cp + 32;
    if (cp < 0xC0) return cp;
    if (cp <= 0xDE && cp != 0xD7) return cp + 32;
    if (cp >= 0x100 && cp <= 0x137 && (cp & 1) == 0) return cp + 1;
    if (cp >= 0x139 && cp <= 0x148 && (cp & 1) == 1) return cp + 1;
    if (cp >= 0x14A && cp <= 0x177 && (cp & 1) == 0) return cp + 1;
    if (cp >= 0x391 && cp <= 0x3AB && cp != 0x3A2) return cp + 32;
    if (cp >= 0x410 && cp <= 0x42F) return cp + 32;
    if (cp >= 0x400 && cp <= 0x40F) return cp + 80;
    return cp;
}

static unsigned hash_str(const char *s) {
    u64 h = 5381;
    while (*s) h = (h * 33 + (u64)(unsigned char)*s++) & 0xFFFFFFFFULL;
    return (unsigned)(h % NAME_BUCKETS);
}

Name *intern(const char *key) {
    unsigned h = hash_str(key);
    Name *n;
    for (n = name_tab[h]; n; n = n->next)
        if (strcmp(n->key, key) == 0) return n;
    n = xalloc(sizeof(Name));
    n->key = xstrdup(key);
    n->kw = 0;
    n->next = name_tab[h];
    name_tab[h] = n;
    return n;
}

Name *intern_word(const char *spelling) {
    StrBuf b;
    Name *n;
    sb_init(&b);
    while (*spelling) {
        int c = *spelling++;
        sb_putc(&b, (c >= 'A' && c <= 'Z') ? c + 32 : c);
    }
    n = intern(b.s);
    free(b.s);
    return n;
}
