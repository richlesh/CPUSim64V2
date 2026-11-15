/*
 * pp.c - the C++ preprocessor (ISO C++14 [cpp]).
 *
 * pp_begin(file) starts reading a file; pp_next() returns the next token of
 * the preprocessed program, K_EOF at the end.  Directives (#include, #define,
 * #undef, #if, #ifdef, #ifndef, #elif, #else, #endif, #line, #error,
 * #warning, #pragma) are executed when they are met; the tokens of skipped
 * groups are dropped.  Macros are expanded with the hide-set algorithm of
 * Dave Prosser (as in the C standard's description): every token carries the
 * set of macros it came from, so a macro is never expanded inside its own
 * expansion.  Supported: object-like and function-like macros, variadic
 * macros (__VA_ARGS__, and the GNU form "name..."), # and ##, the GNU
 * ", ## __VA_ARGS__", __FILE__, __LINE__, __COUNTER__, __INCLUDE_LEVEL__,
 * defined and __has_include in #if.
 *
 * Tokens are read from a stack of open files (Src).  Macro expansions and
 * tokens that were read ahead are pushed back on a token stack (pushback)
 * that has priority over the files.
 *
 * #line N "file" gives the following lines of the current file the
 * (presumed) line N and the file name "file": src_take() applies the offset
 * to the tokens it returns.  The tokens then carry the presumed file index
 * (add_file_name) and line, which is what the generated C names in #line
 * directives and what error messages show.
 */

struct Macro {
    Name *name;
    int is_func, variadic, nparams, builtin;
    Name **params;
    TokVec body;
    int file, line;
};

enum { BI_NONE = 0, BI_FILE, BI_LINE, BI_COUNTER, BI_INCLUDE_LEVEL };

typedef struct {
    int taken;           /* a group of this #if has been taken */
    int in_else;
    int file, line;
} Cond;

typedef struct {
    TokVec *toks;
    int pos;
    int file;            /* the file that was read */
    int pres;            /* presumed file (#line) */
    int delta;           /* presumed line = line + delta */
    int last_raw;        /* the line of the last token taken, without delta */
    char *dir;           /* directory of the file, with a trailing / (or "") */
    int ncond;           /* depth of the condition stack at entry */
} Src;

#define MAX_INCLUDE_DEPTH 200

static Src srcs[MAX_INCLUDE_DEPTH + 2];
static int nsrc = 0;
static TokVec pushback;
static Cond *conds = 0;
static int nconds = 0, conds_cap = 0;
static char **inc_dirs = 0;
static int ninc = 0;
static char **once_files = 0;
static int nonce = 0;
static int counter_value = 0;
static Name *nm_va_args, *nm_defined, *nm_has_include;

/* the library directory: the last include directory; its files are marked is_lib */
static char *lib_dir_prefix = 0;
int pp_c_include_count = 0;
const char **pp_c_includes = 0;

void pp_set_lib_dir(const char *dir) {
    size_t n = strlen(dir);
    lib_dir_prefix = xalloc(n + 2);
    strcpy(lib_dir_prefix, dir);
    if (n > 0 && dir[n - 1] != '/') strcat(lib_dir_prefix, "/");
    pp_add_include_dir(dir);
}

void pp_add_include_dir(const char *dir) {
    inc_dirs = xrealloc(inc_dirs, sizeof(char *) * (ninc + 1));
    inc_dirs[ninc++] = xstrdup(dir);
}

/* ------------------------------------------------------------ hide sets */

static Hide *hs_new(Name *n, Hide *next) {
    Hide *h = anew(sizeof(Hide));
    h->name = n;
    h->next = next;
    return h;
}

static int hs_has(Hide *h, Name *n) {
    for (; h; h = h->next)
        if (h->name == n) return 1;
    return 0;
}

/* a new set with the members of a and b */
static Hide *hs_union(Hide *a, Hide *b) {
    Hide *r = b;
    Hide *rev = 0, *p;
    if (!a) return b;
    if (!b) return a;
    for (p = a; p; p = p->next)
        if (!hs_has(b, p->name)) rev = hs_new(p->name, rev);
    for (p = rev; p; p = p->next) r = hs_new(p->name, r);
    return r;
}

static Hide *hs_intersect(Hide *a, Hide *b) {
    Hide *r = 0, *p;
    for (p = a; p; p = p->next)
        if (hs_has(b, p->name)) r = hs_new(p->name, r);
    return r;
}

/* ------------------------------------------------------------ the file stack */

static char *dir_of(const char *path) {
    const char *s = strrchr(path, '/');
    int n = s ? (int)(s - path) + 1 : 0;
    char *d = xalloc(n + 1);
    memcpy(d, path, n);
    return d;
}

static int file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static void push_src(int file) {
    Src *s;
    if (nsrc >= MAX_INCLUDE_DEPTH) error_at(file, 1, "#include nested too deeply");
    s = &srcs[nsrc++];
    s->toks = lex_file(file);
    s->pos = 0;
    s->file = file;
    s->pres = file;
    s->delta = 0;
    s->last_raw = 0;
    s->dir = dir_of(files[file].path);
    s->ncond = nconds;
}

/* the next token of the current file, or 0 at its end (the file stays open) */
static Token *src_peek(void) {
    Src *s;
    if (nsrc == 0) return 0;
    s = &srcs[nsrc - 1];
    return s->pos < s->toks->n ? s->toks->v[s->pos] : 0;
}

static Token *src_take(void) {
    Src *s;
    Token *t;
    if (nsrc == 0) return 0;
    s = &srcs[nsrc - 1];
    if (s->pos >= s->toks->n) return 0;
    t = s->toks->v[s->pos++];
    s->last_raw = t->line;
    t->line += s->delta;
    t->file = s->pres;
    return t;
}

static void pop_src(void) {
    Src *s = &srcs[nsrc - 1];
    if (nconds > s->ncond) {
        Cond *c = &conds[nconds - 1];
        error_at(c->file, c->line, "unterminated conditional directive");
    }
    nsrc--;
}

/* the next token from the pushed-back tokens or the files, or 0 at the end */
static Token *raw_next(void) {
    for (;;) {
        Token *t;
        if (pushback.n > 0) return pushback.v[--pushback.n];
        t = src_take();
        if (t) return t;
        if (nsrc == 0) return 0;
        pop_src();
        if (nsrc == 0) return 0;
    }
}

static void unget(Token *t) {
    tv_push(&pushback, t);
}

/* ------------------------------------------------------------ macro table */

static Macro *find_macro(Name *n) {
    return n->macro;
}

static void define_builtin(const char *name, int bi) {
    Macro *m = xalloc(sizeof(Macro));
    m->name = intern_str(name);
    m->builtin = bi;
    m->name->macro = m;
}

/* #define NAME VALUE from the command line or the predefined macros */
void pp_define(const char *name, const char *value) {
    Macro *m = xalloc(sizeof(Macro));
    int n;
    int *cps = native_to_cps(value, &n);
    int i;
    m->name = intern_str(name);
    lex_text(cps, n, -1, 1, &m->body);
    for (i = 0; i < m->body.n; i++) m->body.v[i]->flags &= ~TF_BOL;
    if (m->body.n > 0) m->body.v[0]->flags &= ~TF_SPACE;
    m->file = -1;
    m->name->macro = m;
    free(cps);
}

void pp_init(void) {
    lex_init();
    nm_va_args = intern_str("__VA_ARGS__");
    nm_defined = intern_str("defined");
    nm_has_include = intern_str("__has_include");
    define_builtin("__FILE__", BI_FILE);
    define_builtin("__LINE__", BI_LINE);
    define_builtin("__COUNTER__", BI_COUNTER);
    define_builtin("__INCLUDE_LEVEL__", BI_INCLUDE_LEVEL);
    pp_define("__cplusplus", "201402L");
    pp_define("__STDC_HOSTED__", "1");
    pp_define("__CPUSIM64__", "1");
    pp_define("__CPPC__", "1");
    pp_define("__DATE__", "\"??? ?? ????\"");
    pp_define("__TIME__", "\"??:??:??\"");
}

/* ------------------------------------------------------------ token helpers */

static int is_punct(Token *t, int id) {
    return t && t->kind == K_PUNCT && t->id == id;
}

static void err_tok(Token *t, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(t->file, t->line, fmt, ap);
    va_end(ap);
}

/* the tokens up to the end of the line of the directive being processed */
static TokVec *rest_of_line(void) {
    TokVec *v = xalloc(sizeof(TokVec));
    Token *t;
    for (;;) {
        t = src_peek();
        if (!t || (t->flags & TF_BOL)) break;
        tv_push(v, src_take());
    }
    return v;
}

/* the spelling of token k..: used for #include <...> and stringizing */
static void put_spelling(StrBuf *sb, Token *t) {
    sb_puts(sb, t->text);
}

/* ------------------------------------------------------------ macro definition */

static int param_index(Macro *m, Name *n) {
    int i;
    for (i = 0; i < m->nparams; i++)
        if (m->params[i] == n) return i;
    return -1;
}

static void do_define(TokVec *line) {
    Macro *m;
    Token *nt, *t;
    int i, n = line->n;
    if (n < 2 || line->v[1]->kind != K_IDENT) {
        err_tok(line->v[0], "macro names must be identifiers");
    }
    nt = line->v[1];
    if (nt->name == nm_defined) err_tok(nt, "\"defined\" cannot be used as a macro name");
    if (nt->name->macro && nt->name->macro->builtin)
        err_tok(nt, "\"%s\" cannot be redefined", nt->text);
    m = xalloc(sizeof(Macro));
    m->name = nt->name;
    m->file = nt->file;
    m->line = nt->line;
    i = 2;
    if (i < n && is_punct(line->v[i], P_LPAREN) && !(line->v[i]->flags & TF_SPACE)) {
        m->is_func = 1;
        i++;
        if (i < n && is_punct(line->v[i], P_RPAREN)) {
            i++;
        } else {
            for (;;) {
                if (i >= n) err_tok(nt, "missing ')' in macro parameter list");
                t = line->v[i];
                if (is_punct(t, P_ELLIPSIS)) {
                    m->variadic = 1;
                    m->params = xrealloc(m->params, sizeof(Name *) * (m->nparams + 1));
                    m->params[m->nparams++] = nm_va_args;
                    i++;
                    if (i >= n || !is_punct(line->v[i], P_RPAREN))
                        err_tok(t, "missing ')' in macro parameter list");
                    i++;
                    break;
                }
                if (t->kind != K_IDENT) err_tok(t, "expected a parameter name");
                if (param_index(m, t->name) >= 0) err_tok(t, "duplicate macro parameter \"%s\"", t->text);
                m->params = xrealloc(m->params, sizeof(Name *) * (m->nparams + 1));
                m->params[m->nparams++] = t->name;
                i++;
                if (i < n && is_punct(line->v[i], P_ELLIPSIS)) {   /* GNU: name... */
                    m->variadic = 1;
                    i++;
                    if (i >= n || !is_punct(line->v[i], P_RPAREN))
                        err_tok(t, "missing ')' in macro parameter list");
                    i++;
                    break;
                }
                if (i < n && is_punct(line->v[i], P_RPAREN)) {
                    i++;
                    break;
                }
                if (i < n && is_punct(line->v[i], P_COMMA)) {
                    i++;
                    continue;
                }
                err_tok(nt, "expected ',' or ')' in macro parameter list");
            }
        }
    }
    for (; i < n; i++) {
        Token *c = tok_copy(line->v[i]);
        c->flags &= ~TF_BOL;
        tv_push(&m->body, c);
    }
    if (m->body.n > 0) m->body.v[0]->flags &= ~TF_SPACE;
    /* checks: # must be followed by a parameter in a function-like macro; ## not at the ends */
    for (i = 0; i < m->body.n; i++) {
        t = m->body.v[i];
        if (is_punct(t, P_HASH) && m->is_func) {
            if (i + 1 >= m->body.n || m->body.v[i + 1]->kind != K_IDENT
                || param_index(m, m->body.v[i + 1]->name) < 0)
                err_tok(t, "'#' is not followed by a macro parameter");
        }
        if (is_punct(t, P_HASHHASH) && (i == 0 || i == m->body.n - 1))
            err_tok(t, "'##' cannot appear at either end of a macro expansion");
    }
    nt->name->macro = m;
}

/* ------------------------------------------------------------ expansion */

static TokVec *expand_tokens(TokVec *in);

static Token *placemarker(Token *like) {
    Token *t = tok_new(K_PLACE, "", like->file, like->line);
    return t;
}

/* "..." for the tokens of an argument (the # operator) */
static Token *stringize(TokVec *arg, Token *like) {
    StrBuf sb;
    int i;
    Token *t;
    sb_init(&sb);
    sb_putc(&sb, '"');
    for (i = 0; i < arg->n; i++) {
        Token *a = arg->v[i];
        const char *p;
        if (i > 0 && (a->flags & TF_SPACE)) sb_putc(&sb, ' ');
        for (p = a->text; *p; p++) {
            if ((a->kind == K_STR || a->kind == K_CHAR) && (*p == '"' || *p == '\\')) sb_putc(&sb, '\\');
            sb_putc(&sb, *p);
        }
    }
    sb_putc(&sb, '"');
    t = tok_new(K_STR, sb.s, like->file, like->line);
    t->flags = like->flags & TF_SPACE;
    return t;
}

/* the ## operator: pastes the spellings of two tokens and lexes the result */
static Token *paste(Token *l, Token *r) {
    StrBuf sb;
    TokVec tv;
    int n, *cps;
    Token *t;
    if (l->kind == K_PLACE) return tok_copy(r);
    if (r->kind == K_PLACE) return tok_copy(l);
    sb_init(&sb);
    sb_puts(&sb, l->text);
    sb_puts(&sb, r->text);
    cps = native_to_cps(sb.s, &n);
    memset(&tv, 0, sizeof tv);
    lex_text(cps, n, l->file, l->line, &tv);
    if (tv.n != 1 || tv.v[0]->kind == K_OTHER)
        err_tok(l, "pasting \"%s\" and \"%s\" does not give a valid preprocessing token", l->text, r->text);
    t = tv.v[0];
    t->file = l->file;
    t->line = l->line;
    t->flags = l->flags & TF_SPACE;
    t->hide = hs_intersect(l->hide, r->hide);
    free(cps);
    return t;
}

/* the arguments of an invocation of m; the '(' has been read.  *rp = the ')'. */
static TokVec *collect_args(Macro *m, Token *call, int *nargs, Token **rp) {
    TokVec *args = xalloc(sizeof(TokVec) * (m->nparams + 1));
    int depth = 0, n = 0;
    Token *t;
    for (;;) {
        t = raw_next();
        if (!t) err_tok(call, "unterminated argument list invoking macro \"%s\"", m->name->s);
        t->flags &= ~TF_BOL;
        if (is_punct(t, P_LPAREN)) depth++;
        if (is_punct(t, P_RPAREN)) {
            if (depth == 0) break;
            depth--;
        }
        if (depth == 0 && is_punct(t, P_COMMA) && !(m->variadic && n >= m->nparams - 1)) {
            n++;
            continue;
        }
        if (n < m->nparams + 1) tv_push(&args[n], t);
    }
    n++;
    /* f() has no arguments, unless the macro has one parameter */
    if (n == 1 && args[0].n == 0 && m->nparams == 0) n = 0;
    *nargs = n;
    *rp = t;
    return args;
}

/* the expansion of an invocation: pushes the tokens back */
static void expand_function(Macro *m, Token *call, TokVec *args, int nargs, Token *rparen) {
    TokVec out, **expanded;
    Hide *hs;
    int i, k;
    memset(&out, 0, sizeof out);
    if (m->variadic) {
        if (nargs == m->nparams - 1) nargs = m->nparams;       /* no variadic argument at all */
        if (nargs < m->nparams)
            err_tok(call, "macro \"%s\" requires %d arguments, but only %d given", m->name->s, m->nparams - 1, nargs);
    } else if (nargs != m->nparams) {
        err_tok(call, "macro \"%s\" requires %d arguments, but %d given", m->name->s, m->nparams, nargs);
    }
    expanded = xalloc(sizeof(TokVec *) * (m->nparams + 1));
    for (i = 0; i < m->body.n; i++) {
        Token *t = m->body.v[i];
        int pi = t->kind == K_IDENT ? param_index(m, t->name) : -1;
        if (is_punct(t, P_HASH) && i + 1 < m->body.n) {
            Token *pt = m->body.v[i + 1];
            k = param_index(m, pt->name);
            if (k >= 0) {
                tv_push(&out, stringize(&args[k], t));
                i++;
                continue;
            }
        }
        if (is_punct(t, P_HASHHASH) && i + 1 < m->body.n) {
            Token *rhs = m->body.v[i + 1];
            Token *lhs;
            int rk = rhs->kind == K_IDENT ? param_index(m, rhs->name) : -1;
            if (out.n == 0) err_tok(t, "'##' cannot appear at either end of a macro expansion");
            lhs = out.v[--out.n];
            if (rk >= 0) {
                TokVec *a = &args[rk];
                if (is_punct(lhs, P_COMMA) && m->variadic && rk == m->nparams - 1) {
                    /* GNU: , ## __VA_ARGS__ drops the comma when there are no variadic arguments */
                    if (a->n == 0) {
                        i++;
                        continue;
                    }
                    tv_push(&out, lhs);
                    for (k = 0; k < a->n; k++) tv_push(&out, tok_copy(a->v[k]));
                } else if (a->n == 0) {
                    tv_push(&out, lhs);
                } else {
                    tv_push(&out, paste(lhs, a->v[0]));
                    for (k = 1; k < a->n; k++) tv_push(&out, tok_copy(a->v[k]));
                }
            } else {
                tv_push(&out, paste(lhs, tok_copy(rhs)));
            }
            i++;
            continue;
        }
        if (pi >= 0) {
            int next_paste = i + 1 < m->body.n && is_punct(m->body.v[i + 1], P_HASHHASH);
            TokVec *a = &args[pi];
            if (next_paste) {
                if (a->n == 0) tv_push(&out, placemarker(t));
                else for (k = 0; k < a->n; k++) tv_push(&out, tok_copy(a->v[k]));
            } else {
                if (!expanded[pi]) expanded[pi] = expand_tokens(a);
                for (k = 0; k < expanded[pi]->n; k++) tv_push(&out, tok_copy(expanded[pi]->v[k]));
            }
            continue;
        }
        tv_push(&out, tok_copy(t));
    }
    /* hide set: (HS of the name ∩ HS of the ')') ∪ {name} */
    hs = hs_new(m->name, hs_intersect(call->hide, rparen->hide));
    {
        int j = 0;
        for (i = 0; i < out.n; i++) {
            Token *t = out.v[i];
            if (t->kind == K_PLACE) continue;
            t->hide = hs_union(t->hide, hs);
            t->flags &= ~TF_BOL;
            t->file = call->file;
            t->line = call->line;
            out.v[j++] = t;
        }
        out.n = j;
    }
    if (out.n > 0) {
        out.v[0]->flags = (out.v[0]->flags & ~TF_SPACE) | (call->flags & TF_SPACE);
    }
    for (i = out.n - 1; i >= 0; i--) unget(out.v[i]);
    for (i = 0; i < m->nparams + 1; i++) {
        free(args[i].v);
        if (expanded[i]) {
            free(expanded[i]->v);
            free(expanded[i]);
        }
    }
    free(args);
    free(expanded);
    free(out.v);
}

static void expand_object(Macro *m, Token *call) {
    Hide *hs = hs_new(m->name, call->hide);
    int i;
    for (i = m->body.n - 1; i >= 0; i--) {
        Token *t = tok_copy(m->body.v[i]);
        t->hide = hs_union(t->hide, hs);
        t->file = call->file;
        t->line = call->line;
        t->flags &= ~TF_BOL;
        if (i == 0) t->flags = (t->flags & ~TF_SPACE) | (call->flags & TF_SPACE);
        unget(t);
    }
}

static Token *make_number(Token *like, i64 v) {
    char buf[40];
    Token *t;
    sprintf(buf, "%lld", v);
    t = tok_new(K_NUM, xstrdup(buf), like->file, like->line);
    t->flags = like->flags & TF_SPACE;
    return t;
}

static Token *expand_builtin(Macro *m, Token *call) {
    Token *t;
    if (m->builtin == BI_LINE) return make_number(call, call->line);
    if (m->builtin == BI_COUNTER) return make_number(call, counter_value++);
    if (m->builtin == BI_INCLUDE_LEVEL) return make_number(call, nsrc > 0 ? nsrc - 1 : 0);
    /* __FILE__ */
    {
        StrBuf sb;
        const char *p;
        sb_init(&sb);
        sb_putc(&sb, '"');
        for (p = files[call->file].disp; *p; p++) {
            if (*p == '"' || *p == '\\') sb_putc(&sb, '\\');
            sb_putc(&sb, *p);
        }
        sb_putc(&sb, '"');
        t = tok_new(K_STR, sb.s, call->file, call->line);
        t->flags = call->flags & TF_SPACE;
    }
    return t;
}

static void directive(Token *hash);

/* the next token after macro expansion; the directives are executed on the way */
static Token *expand_next(void) {
    for (;;) {
        Token *t = raw_next();
        Macro *m;
        if (!t) return 0;
        if (t->kind == K_PUNCT && t->id == P_HASH && (t->flags & TF_BOL) && nsrc > 0) {
            directive(t);
            continue;
        }
        if (t->kind != K_IDENT) return t;
        m = find_macro(t->name);
        if (!m || (t->flags & TF_NOEXP)) return t;
        if (hs_has(t->hide, t->name)) {
            t->flags |= TF_NOEXP;
            return t;
        }
        if (m->builtin) {
            Token *r = expand_builtin(m, t);
            return r;
        }
        if (m->is_func) {
            Token *p = raw_next();
            TokVec *args;
            int nargs;
            Token *rparen;
            if (!p || !is_punct(p, P_LPAREN)) {
                if (p) unget(p);
                return t;
            }
            args = collect_args(m, t, &nargs, &rparen);
            expand_function(m, t, args, nargs, rparen);
        } else {
            expand_object(m, t);
        }
    }
}

/* fully expands a list of tokens (macro arguments, #if expressions) */
static TokVec *expand_tokens(TokVec *in) {
    TokVec *res = xalloc(sizeof(TokVec));
    TokVec saved = pushback;
    Token *sent = tok_new(K_EOF, "", 0, 0);
    int i;
    memset(&pushback, 0, sizeof pushback);
    tv_push(&pushback, sent);
    for (i = in->n - 1; i >= 0; i--) {
        Token *c = tok_copy(in->v[i]);
        c->flags &= ~TF_BOL;
        tv_push(&pushback, c);
    }
    for (;;) {
        Token *t = expand_next();
        if (!t || t == sent) break;
        tv_push(res, t);
    }
    free(pushback.v);
    pushback = saved;
    return res;
}

/* ------------------------------------------------------------ conditions */

static Cond *push_cond(int file, int line) {
    Cond *c;
    if (nconds == conds_cap) {
        conds_cap = conds_cap ? conds_cap * 2 : 16;
        conds = xrealloc(conds, sizeof(Cond) * conds_cap);
    }
    c = &conds[nconds++];
    memset(c, 0, sizeof *c);
    c->file = file;
    c->line = line;
    return c;
}

static int eval_if(TokVec *line, int first, Token *at);

/* skips tokens until the #elif, #else or #endif that ends the current group;
   returns that directive's name token, with the rest of its line in *rest */
static Token *skip_group(TokVec *rest) {
    int depth = 0;
    Cond *c = &conds[nconds - 1];
    for (;;) {
        Token *t = src_take(), *d;
        if (!t) error_at(c->file, c->line, "unterminated conditional directive");
        if (!(is_punct(t, P_HASH) && (t->flags & TF_BOL))) continue;
        {
            TokVec *line = rest_of_line();
            if (line->n == 0 || line->v[0]->kind != K_IDENT) continue;
            d = line->v[0];
            if (strcmp(d->text, "if") == 0 || strcmp(d->text, "ifdef") == 0 || strcmp(d->text, "ifndef") == 0) {
                depth++;
            } else if (strcmp(d->text, "endif") == 0) {
                if (depth == 0) {
                    *rest = *line;
                    return d;
                }
                depth--;
            } else if (depth == 0 && (strcmp(d->text, "else") == 0 || strcmp(d->text, "elif") == 0)) {
                *rest = *line;
                return d;
            }
        }
    }
}

/* the current group of the innermost #if is not taken: skip to the next group to take */
static void skip_to_active(void) {
    Cond *c = &conds[nconds - 1];
    for (;;) {
        TokVec rest;
        Token *d;
        memset(&rest, 0, sizeof rest);
        d = skip_group(&rest);
        if (strcmp(d->text, "endif") == 0) {
            nconds--;
            return;
        }
        if (strcmp(d->text, "else") == 0) {
            if (c->in_else) err_tok(d, "#else after #else");
            c->in_else = 1;
            if (!c->taken) {
                c->taken = 1;
                return;
            }
            continue;
        }
        /* #elif */
        if (c->in_else) err_tok(d, "#elif after #else");
        if (!c->taken && eval_if(&rest, 1, d)) {
            c->taken = 1;
            return;
        }
    }
}

/* ------------------------------------------------------------ #include */

static int find_include(const char *name, int quoted, const char *curdir, char **out) {
    char *path;
    int i;
    if (name[0] == '/') {
        if (file_exists(name)) { *out = xstrdup(name); return 1; }
        return 0;
    }
    if (quoted) {
        path = xalloc(strlen(curdir) + strlen(name) + 2);
        sprintf(path, "%s%s", curdir, name);
        if (file_exists(path)) { *out = path; return 1; }
        free(path);
    }
    for (i = 0; i < ninc; i++) {
        path = xalloc(strlen(inc_dirs[i]) + strlen(name) + 2);
        sprintf(path, "%s/%s", inc_dirs[i], name);
        if (file_exists(path)) { *out = path; return 1; }
        free(path);
    }
    return 0;
}

/* the header name of "name" or <name> (the tokens of an #include or __has_include argument) */
static char *header_name(TokVec *t, int first, int *quoted, Token *at) {
    StrBuf sb;
    int i;
    Token *a;
    if (first >= t->n) err_tok(at, "#include expects \"FILENAME\" or <FILENAME>");
    a = t->v[first];
    if (a->kind == K_STR && a->text[0] == '"') {
        int len = (int)strlen(a->text);
        char *s = xalloc(len);
        memcpy(s, a->text + 1, len - 2);
        s[len - 2] = 0;
        *quoted = 1;
        return s;
    }
    if (is_punct(a, P_LT)) {
        sb_init(&sb);
        for (i = first + 1; i < t->n; i++) {
            if (is_punct(t->v[i], P_GT)) {
                *quoted = 0;
                return sb.s;
            }
            if (i > first + 1 && (t->v[i]->flags & TF_SPACE)) sb_putc(&sb, ' ');
            put_spelling(&sb, t->v[i]);
        }
        err_tok(at, "missing terminating > character");
    }
    err_tok(at, "#include expects \"FILENAME\" or <FILENAME>");
    return 0;
}

static void do_include(TokVec *line, Token *d) {
    int quoted;
    char *name, *path;
    int i, file;
    Src *cur = &srcs[nsrc - 1];
    TokVec *exp = 0;
    if (line->n < 2) err_tok(d, "#include expects \"FILENAME\" or <FILENAME>");
    {
        Token *a = line->v[1];
        if (!(a->kind == K_STR && a->text[0] == '"') && !is_punct(a, P_LT)) {
            TokVec tail;
            memset(&tail, 0, sizeof tail);
            for (i = 1; i < line->n; i++) tv_push(&tail, line->v[i]);
            exp = expand_tokens(&tail);
            name = header_name(exp, 0, &quoted, d);
        } else {
            name = header_name(line, 1, &quoted, d);
        }
    }
    if (!find_include(name, quoted, cur->dir, &path))
        err_tok(d, "cannot find include file \"%s\"", name);
    for (i = 0; i < nonce; i++)
        if (strcmp(once_files[i], path) == 0) return;
    file = load_file(path);
    if (file < 0) err_tok(d, "cannot open include file \"%s\"", name);
    if (lib_dir_prefix && strncmp(path, lib_dir_prefix, strlen(lib_dir_prefix)) == 0) files[file].is_lib = 1;
    push_src(file);
}

/* ------------------------------------------------------------ #if expressions */

static TokVec *ev_toks;
static int ev_i;
static Token *ev_at;

static i64 ev_cond(void);

static Token *ev_peek(void) {
    return ev_i < ev_toks->n ? ev_toks->v[ev_i] : 0;
}

static void ev_error(const char *msg) {
    err_tok(ev_at, "%s", msg);
}

static int ev_digit(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static i64 ev_number(Token *t) {
    const char *p = t->text;
    int base = 10, d, any = 0;
    u64 v = 0;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) { base = 16; p += 2; }
    else if (p[0] == '0' && (p[1] == 'b' || p[1] == 'B')) { base = 2; p += 2; }
    else if (p[0] == '0') base = 8;
    for (; *p; p++) {
        if (*p == '\'') continue;
        d = ev_digit(*p);
        if (d < 0 || d >= base) break;
        v = v * base + d;
        any = 1;
    }
    if (!any && base != 8 && base != 10) err_tok(t, "invalid integer constant in #if");
    for (; *p; p++) {
        if (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L') continue;
        if (*p == '.' || *p == 'e' || *p == 'E') err_tok(t, "floating constant in preprocessor expression");
        err_tok(t, "invalid suffix \"%s\" on integer constant", p);
    }
    return (i64)v;
}

/* the value of a character literal: its first character, escapes decoded */
static i64 ev_char(Token *t) {
    const char *p = t->text;
    i64 v = 0;
    while (*p && *p != '\'') p++;
    if (*p != '\'') return 0;
    p++;
    if (*p == '\\') {
        p++;
        switch (*p) {
        case 'n': return 10;
        case 't': return 9;
        case 'r': return 13;
        case 'a': return 7;
        case 'b': return 8;
        case 'f': return 12;
        case 'v': return 11;
        case 'x':
            p++;
            while (ev_digit(*p) >= 0) v = v * 16 + ev_digit(*p++);
            return v;
        default:
            if (*p >= '0' && *p <= '7') {
                int k = 0;
                while (k < 3 && *p >= '0' && *p <= '7') { v = v * 8 + (*p - '0'); p++; k++; }
                return v;
            }
            return *p;
        }
    }
    return (unsigned char)*p;
}

static i64 ev_primary(void) {
    Token *t = ev_peek();
    if (!t) ev_error("#if with no expression");
    ev_i++;
    if (t->kind == K_NUM) return ev_number(t);
    if (t->kind == K_CHAR) return ev_char(t);
    if (t->kind == K_IDENT) {
        if (t->id == KW_true) return 1;
        return 0;                                  /* false, and every other identifier */
    }
    if (is_punct(t, P_LPAREN)) {
        i64 v = ev_cond();
        Token *c = ev_peek();
        if (!is_punct(c, P_RPAREN)) ev_error("missing ')' in expression");
        ev_i++;
        return v;
    }
    if (is_punct(t, P_PLUS)) return ev_primary();
    if (is_punct(t, P_MINUS)) return -ev_primary();
    if (is_punct(t, P_NOT)) return !ev_primary();
    if (is_punct(t, P_TILDE)) return ~ev_primary();
    err_tok(t, "token \"%s\" is not valid in preprocessor expressions", t->text);
    return 0;
}

static int ev_prec(Token *t) {
    if (!t || t->kind != K_PUNCT) return 0;
    switch (t->id) {
    case P_OROR: return 1;
    case P_ANDAND: return 2;
    case P_PIPE: return 3;
    case P_CARET: return 4;
    case P_AMP: return 5;
    case P_EQ: case P_NE: return 6;
    case P_LT: case P_GT: case P_LE: case P_GE: return 7;
    case P_SHL: case P_SHR: return 8;
    case P_PLUS: case P_MINUS: return 9;
    case P_STAR: case P_SLASH: case P_PERCENT: return 10;
    }
    return 0;
}

static i64 ev_binary(int minp) {
    i64 l = ev_primary();
    for (;;) {
        Token *t = ev_peek();
        int p = ev_prec(t);
        i64 r;
        if (p == 0 || p < minp) return l;
        ev_i++;
        r = ev_binary(p + 1);
        switch (t->id) {
        case P_OROR: l = l || r; break;
        case P_ANDAND: l = l && r; break;
        case P_PIPE: l = l | r; break;
        case P_CARET: l = l ^ r; break;
        case P_AMP: l = l & r; break;
        case P_EQ: l = l == r; break;
        case P_NE: l = l != r; break;
        case P_LT: l = l < r; break;
        case P_GT: l = l > r; break;
        case P_LE: l = l <= r; break;
        case P_GE: l = l >= r; break;
        case P_SHL: l = (r < 0 || r > 63) ? 0 : (i64)((u64)l << r); break;
        case P_SHR: l = (r < 0 || r > 63) ? (l < 0 ? -1 : 0) : l >> r; break;
        case P_PLUS: l = l + r; break;
        case P_MINUS: l = l - r; break;
        case P_STAR: l = l * r; break;
        case P_SLASH:
            if (r == 0) err_tok(t, "division by zero in #if");
            l = l / r;
            break;
        case P_PERCENT:
            if (r == 0) err_tok(t, "division by zero in #if");
            l = l % r;
            break;
        }
    }
}

static i64 ev_cond(void) {
    i64 c = ev_binary(1);
    Token *t = ev_peek();
    if (is_punct(t, P_QUESTION)) {
        i64 a, b;
        ev_i++;
        a = ev_cond();
        if (!is_punct(ev_peek(), P_COLON)) ev_error("expected ':' in the #if conditional expression");
        ev_i++;
        b = ev_cond();
        return c ? a : b;
    }
    return c;
}

/* the value of the #if condition made of line[first..]: "defined X" and
   __has_include are replaced first, then the macros are expanded */
static int eval_if(TokVec *line, int first, Token *at) {
    TokVec pre, *exp;
    int i;
    i64 v;
    memset(&pre, 0, sizeof pre);
    for (i = first; i < line->n; i++) {
        Token *t = line->v[i];
        if (t->kind == K_IDENT && t->name == nm_defined) {
            int paren = 0;
            Token *n;
            i++;
            if (i < line->n && is_punct(line->v[i], P_LPAREN)) { paren = 1; i++; }
            if (i >= line->n || line->v[i]->kind != K_IDENT) err_tok(t, "operator \"defined\" requires an identifier");
            n = line->v[i];
            if (paren) {
                i++;
                if (i >= line->n || !is_punct(line->v[i], P_RPAREN)) err_tok(t, "missing ')' after \"defined\"");
            }
            tv_push(&pre, make_number(t, n->name->macro ? 1 : 0));
        } else if (t->kind == K_IDENT && t->name == nm_has_include) {
            TokVec arg;
            int depth = 0, quoted, found;
            char *name, *path;
            memset(&arg, 0, sizeof arg);
            i++;
            if (i >= line->n || !is_punct(line->v[i], P_LPAREN)) err_tok(t, "missing '(' after \"__has_include\"");
            for (i++; i < line->n; i++) {
                if (is_punct(line->v[i], P_LPAREN)) depth++;
                if (is_punct(line->v[i], P_RPAREN)) {
                    if (depth == 0) break;
                    depth--;
                }
                tv_push(&arg, line->v[i]);
            }
            if (i >= line->n) err_tok(t, "missing ')' after \"__has_include\"");
            name = header_name(&arg, 0, &quoted, t);
            found = find_include(name, quoted, srcs[nsrc - 1].dir, &path);
            tv_push(&pre, make_number(t, found ? 1 : 0));
        } else {
            tv_push(&pre, t);
        }
    }
    exp = expand_tokens(&pre);
    if (exp->n == 0) err_tok(at, "#if with no expression");
    ev_toks = exp;
    ev_i = 0;
    ev_at = at;
    v = ev_cond();
    if (ev_i < exp->n) err_tok(exp->v[ev_i], "token \"%s\" is not valid in preprocessor expressions", exp->v[ev_i]->text);
    return v != 0;
}

/* ------------------------------------------------------------ directives */

static void do_line(TokVec *line, Token *d) {
    TokVec tail, *exp;
    Src *s = &srcs[nsrc - 1];
    Token *a;
    int i, n;
    i64 num;
    memset(&tail, 0, sizeof tail);
    for (i = 1; i < line->n; i++) tv_push(&tail, line->v[i]);
    exp = expand_tokens(&tail);
    if (exp->n == 0 || exp->v[0]->kind != K_NUM) err_tok(d, "\"%s\" after #line is not a positive integer", exp->n ? exp->v[0]->text : "");
    a = exp->v[0];
    for (i = 0; a->text[i]; i++)
        if (a->text[i] < '0' || a->text[i] > '9') err_tok(a, "\"%s\" after #line is not a positive integer", a->text);
    num = ev_number(a);
    if (exp->n > 1) {
        Token *f = exp->v[1];
        char *name;
        int len;
        if (f->kind != K_STR || f->text[0] != '"') err_tok(f, "invalid filename \"%s\"", f->text);
        len = (int)strlen(f->text);
        name = xalloc(len);
        n = 0;
        for (i = 1; i < len - 1; i++) {
            if (f->text[i] == '\\' && i + 1 < len - 1) i++;
            name[n++] = f->text[i];
        }
        name[n] = 0;
        s->pres = add_file_name(name);
    }
    s->delta = (int)num - (s->last_raw + 1);
}

static void directive(Token *hash) {
    TokVec *line = rest_of_line();
    Token *d;
    const char *name;
    Src *s = &srcs[nsrc - 1];
    (void)hash;
    if (line->n == 0) return;                        /* the null directive */
    d = line->v[0];
    if (d->kind == K_NUM) {                          /* # 33 "file": a line marker */
        TokVec fake;
        int i;
        memset(&fake, 0, sizeof fake);
        tv_push(&fake, d);
        for (i = 0; i < line->n; i++) tv_push(&fake, line->v[i]);
        do_line(&fake, d);
        return;
    }
    if (d->kind != K_IDENT) err_tok(d, "invalid preprocessing directive");
    name = d->text;
    if (strcmp(name, "define") == 0) { do_define(line); return; }
    if (strcmp(name, "undef") == 0) {
        if (line->n < 2 || line->v[1]->kind != K_IDENT) err_tok(d, "macro names must be identifiers");
        if (line->v[1]->name->macro && line->v[1]->name->macro->builtin)
            err_tok(line->v[1], "\"%s\" cannot be undefined", line->v[1]->text);
        line->v[1]->name->macro = 0;
        return;
    }
    if (strcmp(name, "include") == 0) { do_include(line, d); return; }
    if (strcmp(name, "if") == 0 || strcmp(name, "ifdef") == 0 || strcmp(name, "ifndef") == 0) {
        Cond *c = push_cond(d->file, d->line);
        int cond;
        if (name[2] == 0) {
            cond = eval_if(line, 1, d);
        } else {
            if (line->n < 2 || line->v[1]->kind != K_IDENT) err_tok(d, "macro names must be identifiers");
            cond = line->v[1]->name->macro != 0;
            if (name[2] == 'n') cond = !cond;       /* ifndef */
        }
        c = &conds[nconds - 1];
        c->taken = cond;
        if (!cond) skip_to_active();
        return;
    }
    if (strcmp(name, "elif") == 0 || strcmp(name, "else") == 0) {
        Cond *c;
        if (nconds <= s->ncond) err_tok(d, "#%s without #if", name);
        c = &conds[nconds - 1];
        if (c->in_else) err_tok(d, "#%s after #else", name);
        if (name[2] == 's') c->in_else = 1;
        c->taken = 1;
        skip_to_active();
        return;
    }
    if (strcmp(name, "endif") == 0) {
        if (nconds <= s->ncond) err_tok(d, "#endif without #if");
        nconds--;
        return;
    }
    if (strcmp(name, "line") == 0) { do_line(line, d); return; }
    if (strcmp(name, "error") == 0 || strcmp(name, "warning") == 0) {
        StrBuf sb;
        int i;
        sb_init(&sb);
        for (i = 1; i < line->n; i++) {
            if (i > 1 && (line->v[i]->flags & TF_SPACE)) sb_putc(&sb, ' ');
            put_spelling(&sb, line->v[i]);
        }
        if (name[0] == 'e') err_tok(d, "#error %s", sb.s);
        warning_at(d->file, d->line, "#warning %s", sb.s);
        return;
    }
    if (strcmp(name, "pragma") == 0) {
        /* #pragma cppc c_include <stdio.h>: the generated C includes this C header */
        if (line->n >= 4 && line->v[1]->kind == K_IDENT && strcmp(line->v[1]->text, "cppc") == 0
            && strcmp(line->v[2]->text, "c_include") == 0 && is_punct(line->v[3], P_LT)) {
            StrBuf hb;
            int k, dup = 0;
            sb_init(&hb);
            for (k = 4; k < line->n && !is_punct(line->v[k], P_GT); k++) put_spelling(&hb, line->v[k]);
            for (k = 0; k < pp_c_include_count; k++)
                if (strcmp(pp_c_includes[k], hb.s) == 0) dup = 1;
            if (!dup) {
                pp_c_includes = xrealloc(pp_c_includes, sizeof(char *) * (pp_c_include_count + 1));
                pp_c_includes[pp_c_include_count++] = hb.s;
            }
            return;
        }
        if (line->n >= 2 && line->v[1]->kind == K_IDENT && strcmp(line->v[1]->text, "once") == 0) {
            once_files = xrealloc(once_files, sizeof(char *) * (nonce + 1));
            once_files[nonce++] = xstrdup(files[s->file].path);
        }
        return;
    }
    if (strcmp(name, "ident") == 0 || strcmp(name, "sccs") == 0) return;
    err_tok(d, "invalid preprocessing directive #%s", name);
}

/* ------------------------------------------------------------ the interface */

void pp_begin(int file) {
    push_src(file);
}

static int last_file = 0, last_line = 0;

Token *pp_next(void) {
    Token *t = expand_next();
    if (!t) return tok_new(K_EOF, "", last_file, last_line);
    last_file = t->file;
    last_line = t->line;
    if (t->kind == K_OTHER) err_tok(t, "stray '%s' in program", t->text);
    return t;
}

/* --preprocess: the tokens of the preprocessed program, one source line per line */
void dump_preprocessed(FILE *f, int file) {
    int line = 0, pfile = -1, first = 1;
    Token *t;
    pp_begin(file);
    for (;;) {
        t = pp_next();
        if (t->kind == K_EOF) break;
        if (!first && (t->line != line || t->file != pfile)) fputc('\n', f);
        else if (!first) fputc(' ', f);
        fputs(t->text, f);
        first = 0;
        line = t->line;
        pfile = t->file;
    }
    if (!first) fputc('\n', f);
}
