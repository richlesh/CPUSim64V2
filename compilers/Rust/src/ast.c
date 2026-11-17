/*
 * ast.c - AST nodes: arena, names, node construction and the text dump.
 * The format is documented at the top of ast.h.
 */

/* ------------------------------------------------------------ memory */

#define AST_CHUNK 262144

static char *ast_arena_p = 0;
static long ast_arena_left = 0;

void *ast_alloc(size_t n) {
    void *p;
    n = (n + 7) & ~(size_t)7;
    if (n > AST_CHUNK / 8) return xalloc(n);
    if ((long)n > ast_arena_left) {
        ast_arena_p = xalloc(AST_CHUNK);
        ast_arena_left = AST_CHUNK;
    }
    p = ast_arena_p;
    ast_arena_p += n;
    ast_arena_left -= (long)n;
    return p;
}

/* ------------------------------------------------------------ names */

#define NAME_BUCKETS 1024

static Name *name_tab[NAME_BUCKETS];

Name *name_intern(const char *s, long len) {
    long h = 5381, i;
    Name *n;
    int b;
    for (i = 0; i < len; i++) h = (h * 33 + (s[i] & 0x1FFFFF)) & 0x7FFFFFFF;
    b = (int)(h % NAME_BUCKETS);
    for (n = name_tab[b]; n; n = n->next) {
        if (n->hash == h && n->len == len && memcmp(n->s, s, (size_t)len * sizeof(char)) == 0) return n;
    }
    n = ast_alloc(sizeof(Name));
    n->s = ast_alloc((size_t)(len + 1) * sizeof(char));
    memcpy(n->s, s, (size_t)len * sizeof(char));
    n->s[len] = 0;
    n->len = len;
    n->hash = h;
    n->next = name_tab[b];
    name_tab[b] = n;
    return n;
}

Name *name_intern_str(const char *s) {
    return name_intern(s, (long)strlen(s));
}

/* ------------------------------------------------------------ tables */

static const char *node_names[] = {
    "",
#define N(x) #x,
#include "nodes.inc"
#undef N
    ""
};

static const char *op_names[] = {
    "",
#define OP(n, s) s,
#include "nodes.inc"
#undef OP
    ""
};

typedef struct {
    long bit;
    const char *name;
} FlagName;

static FlagName flag_names[] = {
#define F(n) { F_##n, #n },
#include "nodes.inc"
#undef F
    { 0, 0 }
};

const char *node_kind_name(int kind) {
    if (kind < 0 || kind >= N__COUNT) return "";
    return node_names[kind];
}

const char *op_spelling(int op) {
    if (op < 0 || op >= OP__COUNT) return "";
    return op_names[op];
}

/* ------------------------------------------------------------ construction */

int ast_cur_file = 0;
void (*ast_tok_text)(FILE *f, const struct Token *toks, long ntoks) = 0;

Node *mk(int kind, int line, int col) {
    Node *n = ast_alloc(sizeof(Node));
    n->kind = kind;
    n->line = line;
    n->col = col;
    n->file = ast_cur_file;
    return n;
}

void list_push(NodeList *l, Node *n) {
    if (!n) return;
    if (l->n == l->cap) {
        long ncap = l->cap ? l->cap * 2 : 4;
        Node **nv = ast_alloc((size_t)ncap * sizeof(Node *));
        if (l->n) memcpy(nv, l->v, (size_t)l->n * sizeof(Node *));
        l->v = nv;
        l->cap = ncap;
    }
    l->v[l->n++] = n;
}

void node_set_str(Node *n, const char *bytes, long len) {
    long i;
    char *p = ast_alloc((size_t)(len + 1) * sizeof(char));
    for (i = 0; i < len; i++) p[i] = (char)(bytes[i] & 0xFF);
    p[len] = 0;
    n->str = p;
    n->slen = len;
}

/* ------------------------------------------------------------ dump */

static void dump_put(FILE *f, const char *s) {
#ifdef __CPUSIM64__
    while (*s) fputc(*s++, f);       /* one codepoint per char */
#else
    fputs(s, f);
#endif
}

static void dump_indent(FILE *f, int depth) {
    int i;
    for (i = 0; i < depth; i++) fputs("  ", f);
}

static void dump_str(FILE *f, const char *s, long len) {
    long i;
    fputc('"', f);
    for (i = 0; i < len; i++) {
        int c = s[i] & 0xFF;
        if (c == '\\') fputs("\\\\", f);
        else if (c == '"') fputs("\\\"", f);
        else if (c == '\n') fputs("\\n", f);
        else if (c == '\r') fputs("\\r", f);
        else if (c == '\t') fputs("\\t", f);
        else if (c < 0x20 || c > 0x7E) fprintf(f, "\\x%02X", c);
        else fputc(c, f);
    }
    fputc('"', f);
}

static int dump_always_ival(int kind) {
    return kind == N_E_LIT_INT || kind == N_E_LIT_CHAR || kind == N_E_LIT_BYTE ||
           kind == N_E_LIT_BOOL || kind == N_E_TUPLE_FIELD;
}

static void dump_node(FILE *f, Node *n, const char *label, int depth, int closers);

static void dump_node(FILE *f, Node *n, const char *label, int depth, int closers) {
    Node *kids[5];
    const char *labels[5];
    int nk = 0, i;
    long k;
    dump_indent(f, depth);
    fputs(label, f);
    fputc('(', f);
    fputs(node_kind_name(n->kind), f);
    if (n->name) {
        fputc(' ', f);
        dump_put(f, n->name->s);
    }
    if (n->name2) {
        fputs(" as:", f);
        dump_put(f, n->name2->s);
    }
    if (n->str) {
        fputc(' ', f);
        dump_str(f, n->str, n->slen);
    }
    if (n->ival != 0 || dump_always_ival(n->kind)) fprintf(f, " #%llu", (unsigned long long)n->ival);
    if (n->op) fprintf(f, " op:%s", op_spelling(n->op));
    if (n->flags) {
        int first = 1;
        FlagName *fl;
        for (fl = flag_names; fl->bit; fl++) {
            if (!(n->flags & fl->bit)) continue;
            if (first) fputc(' ', f);
            fputc(first ? '{' : ',', f);
            first = 0;
            for (i = 0; fl->name[i]; i++) fputc(fl->name[i] >= 'A' && fl->name[i] <= 'Z' ? fl->name[i] + 32 : fl->name[i], f);
        }
        if (!first) fputc('}', f);
    }
    if (n->toks || n->ntoks) {
        fprintf(f, " toks:%ld", n->ntoks);
        if (ast_tok_text && n->toks) {
            fputc(' ', f);
            ast_tok_text(f, n->toks, n->ntoks);
        }
    }
    if (n->vis) { kids[nk] = n->vis; labels[nk++] = "vis="; }
    if (n->a) { kids[nk] = n->a; labels[nk++] = "a="; }
    if (n->b) { kids[nk] = n->b; labels[nk++] = "b="; }
    if (n->c) { kids[nk] = n->c; labels[nk++] = "c="; }
    if (n->d) { kids[nk] = n->d; labels[nk++] = "d="; }
    if (nk == 0 && n->attrs.n == 0 && n->list.n == 0) {
        for (i = 0; i <= closers; i++) fputc(')', f);
        fputc('\n', f);
        return;
    }
    fputc('\n', f);
    /* order: vis, attrs, a b c d, list.  vis is kids[0] when set; print it, then attrs, then the rest */
    {
        int start = 0;
        int list_first = (n->kind == N_BLOCK || n->kind == N_E_CLOSURE);
        long total, idx = 0;
        total = (long)nk + n->attrs.n + n->list.n;
        if (n->vis) {
            dump_node(f, kids[0], labels[0], depth + 1, (++idx == total) ? closers + 1 : 0);
            start = 1;
        }
        for (k = 0; k < n->attrs.n; k++)
            dump_node(f, n->attrs.v[k], "@", depth + 1, (++idx == total) ? closers + 1 : 0);
        if (list_first) {
            for (k = 0; k < n->list.n; k++)
                dump_node(f, n->list.v[k], "", depth + 1, (++idx == total) ? closers + 1 : 0);
        }
        for (i = start; i < nk; i++)
            dump_node(f, kids[i], labels[i], depth + 1, (++idx == total) ? closers + 1 : 0);
        if (!list_first) {
            for (k = 0; k < n->list.n; k++)
                dump_node(f, n->list.v[k], "", depth + 1, (++idx == total) ? closers + 1 : 0);
        }
    }
}

void ast_dump(FILE *f, Node *root) {
    if (root) dump_node(f, root, "", 0, 0);
}
