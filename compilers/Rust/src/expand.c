/*
 * expand.c - macro expansion: step 1 of the pipeline in sema.h.
 *
 * expand_crate() walks the whole AST and replaces every N_MACRO_CALL by
 * ordinary AST: the built-in macros (println!, format!, vec!, assert!, ...,
 * see design-expand.md) and macro_rules! macros.  macro_rules! definitions
 * stay in the tree as N_ITEM_MACRO_RULES; later stages skip them.
 *
 * Method: an expansion is built as an array of tokens (snippets of Rust
 * source lexed with the real lexer, user argument tokens spliced in) and
 * re-parsed with the parser at the call's position (reparse_expr / _stmts /
 * _items / _type).  The result is walked again, so macros produced by an
 * expansion are expanded too (depth limit 128).  Tokens that a macro_rules!
 * transcriber contributes get the line and column of the outermost call
 * site, as rustc reports panic locations; tokens that come from the
 * arguments keep their own position.
 *
 * Everything here is prefixed ex_ / Ex (the translator is one translation
 * unit).  Written for the C subset of the CPUSim64 C11 compiler: no
 * bit-fields, no VLAs, no compound literals, declarations at block starts.
 */
#include "sema.h"

#define EX_DEPTH_LIMIT 128

/* ================================================================ basics */

typedef struct {
    Token *v;
    long n, cap;
} ExToks;

static void ex_tpush(ExToks *t, const Token *k) {
    if (t->n == t->cap) {
        t->cap = t->cap ? t->cap * 2 : 64;
        t->v = xrealloc(t->v, sizeof(Token) * (size_t)t->cap);
    }
    t->v[t->n++] = *k;
}

static void ex_tappend(ExToks *t, const Token *k, long n) {
    long i;
    for (i = 0; i < n; i++) ex_tpush(t, &k[i]);
}

/* the position that generated tokens get: the outermost call site */
static int ex_line, ex_col, ex_file;

static void ex_set_site(Node *call) {
    ex_line = call->line;
    ex_col = call->col;
    ex_file = call->file;
}

static void ex_stamp(Token *t) {
    t->line = ex_line;
    t->col = ex_col;
    t->file = ex_file;
}

static void ex_err(Node *call, const char *fmt, ...) {
    char buf[600];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    error_at(call->file, call->line, "%s", buf);
}

static int ex_is_digit(int c) {
    return c >= '0' && c <= '9';
}

static int ex_is_id_start(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0x80;
}

static int ex_is_id_char(int c) {
    return ex_is_id_start(c) || ex_is_digit(c);
}

static int ex_is_open(int k) {
    return k == TK_LPAREN || k == TK_LBRACK || k == TK_LBRACE;
}

static int ex_is_close(int k) {
    return k == TK_RPAREN || k == TK_RBRACK || k == TK_RBRACE;
}

/* index of the closer that matches the opener t[i], or -1 */
static long ex_close_of(const Token *t, long n, long i) {
    long depth = 0, j;
    for (j = i; j < n; j++) {
        if (ex_is_open(t[j].kind)) depth++;
        else if (ex_is_close(t[j].kind)) {
            depth--;
            if (depth == 0) return j;
        }
    }
    return -1;
}

/* an invisible-group parenthesis put around an expr fragment (raw == 2) */
static int ex_is_inv(const Token *t) {
    return (t->kind == TK_LPAREN || t->kind == TK_RPAREN) && t->raw == 2;
}

/* ---------------------------------------------------------------- bytes <-> native strings */

static void ex_put_utf8(StrBuf *b, long cp) {
    if (cp < 0x80) {
        sb_putc(b, (int)cp);
    } else if (cp < 0x800) {
        sb_putc(b, (int)(0xC0 | (cp >> 6)));
        sb_putc(b, (int)(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        sb_putc(b, (int)(0xE0 | (cp >> 12)));
        sb_putc(b, (int)(0x80 | ((cp >> 6) & 0x3F)));
        sb_putc(b, (int)(0x80 | (cp & 0x3F)));
    } else {
        sb_putc(b, (int)(0xF0 | (cp >> 18)));
        sb_putc(b, (int)(0x80 | ((cp >> 12) & 0x3F)));
        sb_putc(b, (int)(0x80 | ((cp >> 6) & 0x3F)));
        sb_putc(b, (int)(0x80 | (cp & 0x3F)));
    }
}

/* appends a native string (a name, a file name) as UTF-8 bytes */
static void ex_put_native(StrBuf *b, const char *s) {
#ifdef __CPUSIM64__
    while (*s) {
        ex_put_utf8(b, (long)(*s & 0x1FFFFF));
        s++;
    }
#else
    while (*s) {
        sb_putc(b, *s);
        s++;
    }
#endif
}

/* decodes the UTF-8 character at s[*i] (len bytes in all) */
static long ex_get_utf8(const char *s, long len, long *i) {
    int c = s[*i] & 0xFF;
    long cp, k;
    int need;
    (*i)++;
    if (c < 0xC0) return c;
    need = c >= 0xF0 ? 3 : (c >= 0xE0 ? 2 : 1);
    cp = c & (0x3F >> need);
    for (k = 0; k < need && *i < len; k++) {
        cp = (cp << 6) | (s[*i] & 0x3F);
        (*i)++;
    }
    return cp;
}

/* UTF-8 bytes -> an interned (native) name */
static Name *ex_name_from_bytes(const char *s, long len) {
    StrBuf b;
    long i = 0;
    Name *nm;
    sb_init(&b);
    while (i < len) sb_put_cp(&b, (int)ex_get_utf8(s, len, &i));
    nm = name_intern(b.s, b.len);
    free(b.s);
    return nm;
}

/* ---------------------------------------------------------------- tokens to source text */

/* "..." with the escapes of Rust source; s holds UTF-8 bytes */
static void ex_put_quoted(StrBuf *b, const char *s, long len, int bytes) {
    long i;
    char tmp[16];
    sb_putc(b, '"');
    for (i = 0; i < len; i++) {
        int c = s[i] & 0xFF;
        if (c == '"') sb_puts(b, "\\\"");
        else if (c == '\\') sb_puts(b, "\\\\");
        else if (c == '\n') sb_puts(b, "\\n");
        else if (c == '\r') sb_puts(b, "\\r");
        else if (c == '\t') sb_puts(b, "\\t");
        else if (c == 0) sb_puts(b, "\\0");
        else if (c < 0x20 || c == 0x7F || (bytes && c >= 0x80)) {
            sprintf(tmp, "\\x%02X", c);
            sb_puts(b, tmp);
        } else sb_putc(b, c);
    }
    sb_putc(b, '"');
}

static void ex_put_raw_quoted(StrBuf *b, const char *s, long len) {
    int hashes = 0, again = 1, h;
    long i, j;
    while (again) {
        again = 0;
        for (i = 0; i < len; i++) {
            if ((s[i] & 0xFF) == '"') {
                for (j = 0; j < hashes && i + 1 + j < len && (s[i + 1 + j] & 0xFF) == '#'; j++) {
                }
                if (j == hashes) {
                    hashes++;
                    again = 1;
                    break;
                }
            }
        }
    }
    sb_putc(b, 'r');
    for (h = 0; h < hashes; h++) sb_putc(b, '#');
    sb_putc(b, '"');
    for (i = 0; i < len; i++) sb_putc(b, s[i] & 0xFF);
    sb_putc(b, '"');
    for (h = 0; h < hashes; h++) sb_putc(b, '#');
}

/* the length of an identifier without the hygiene suffix `__hN` added by macro expansion */
static long ex_unhyg_len(const char *s, long len) {
    long k = len;
    while (k > 0 && ex_is_digit(s[k - 1] & 0xFF)) k--;
    if (k < len && k >= 3 && s[k - 1] == 'h' && s[k - 2] == '_' && s[k - 3] == '_' && k - 3 > 0) return k - 3;
    return len;
}

/* the token as source text, as UTF-8 bytes */
static void ex_tok_src(StrBuf *b, const Token *t) {
    char tmp[40];
    long v;
    switch (t->kind) {
    case TK_EOF:
        break;
    case TK_IDENT:
        if (t->raw) sb_puts(b, "r#");
        {
            StrBuf nb;
            long keep = ex_unhyg_len(t->text->s, t->text->len);
            sb_init(&nb);
            { long q; for (q = 0; q < keep; q++) sb_putc(&nb, t->text->s[q]); }
            ex_put_native(b, nb.s);
            free(nb.s);
        }
        break;
    case TK_LIFETIME:
        ex_put_native(b, t->text->s);
        break;
    case TK_INT:
    case TK_FLOAT:
        ex_put_native(b, t->text->s);
        if (t->suffix) ex_put_native(b, t->suffix->s);
        break;
    case TK_CHAR:
        v = t->value;
        sb_putc(b, '\'');
        if (v == '\n') sb_puts(b, "\\n");
        else if (v == '\r') sb_puts(b, "\\r");
        else if (v == '\t') sb_puts(b, "\\t");
        else if (v == '\\') sb_puts(b, "\\\\");
        else if (v == '\'') sb_puts(b, "\\'");
        else if (v == 0) sb_puts(b, "\\0");
        else if (v < 0x20 || v == 0x7F) {
            sprintf(tmp, "\\u{%lx}", v);
            sb_puts(b, tmp);
        } else ex_put_utf8(b, v);
        sb_putc(b, '\'');
        break;
    case TK_BYTE:
        sprintf(tmp, "b'\\x%02lX'", t->value & 0xFF);
        sb_puts(b, tmp);
        break;
    case TK_STR:
        if (t->raw) ex_put_raw_quoted(b, t->str, t->slen);
        else ex_put_quoted(b, t->str, t->slen, 0);
        break;
    case TK_BSTR:
        sb_putc(b, 'b');
        ex_put_quoted(b, t->str, t->slen, 1);
        break;
    default:
        sb_puts(b, tk_spelling(t->kind));
        break;
    }
}

/* a copy of the tokens without the invisible parentheses of expr fragments */
static Token *ex_visible(const Token *t, long n, long *outn) {
    Token *v = xalloc(sizeof(Token) * (size_t)(n + 1));
    long i, k = 0;
    for (i = 0; i < n; i++)
        if (!ex_is_inv(&t[i])) v[k++] = t[i];
    *outn = k;
    return v;
}

static void ex_pretty(StrBuf *b, const Token *t, long n);

/* stringify!: the tokens with the spacing of the source, runs of white space as one blank;
   an expr fragment (a group in invisible parentheses) is printed like rustc prints an expression */
static void ex_stringify(StrBuf *b, const Token *t, long n) {
    long i = 0, j, depth;
    int first = 1;
    while (i < n) {
        if (t[i].kind == TK_LPAREN && ex_is_inv(&t[i])) {
            depth = 0;
            for (j = i; j < n; j++) {
                if (ex_is_inv(&t[j])) {
                    depth += t[j].kind == TK_LPAREN ? 1 : -1;
                    if (depth == 0) break;
                }
            }
            if (j < n) {
                if (!first && i + 1 < j && t[i + 1].space_before) sb_putc(b, ' ');
                ex_pretty(b, t + i + 1, j - i - 1);
                first = 0;
                i = j + 1;
                continue;
            }
        }
        if (!ex_is_inv(&t[i])) {
            if (!first && t[i].space_before) sb_putc(b, ' ');
            ex_tok_src(b, &t[i]);
            first = 0;
        }
        i++;
    }
}

static int ex_operand_end(const Token *t) {
    switch (t->kind) {
    case TK_IDENT: case TK_INT: case TK_FLOAT: case TK_CHAR: case TK_BYTE: case TK_STR: case TK_BSTR:
    case TK_RPAREN: case TK_RBRACK: case TK_RBRACE: case TK_QUESTION:
    case TK_KW_true: case TK_KW_false: case TK_KW_self: case TK_KW_Self: case TK_KW_crate: case TK_KW_super:
        return 1;
    default:
        return 0;
    }
}

static int ex_upper_ident(const Token *t) {
    return t->kind == TK_IDENT && t->text->s[0] >= 'A' && t->text->s[0] <= 'Z';
}

/* assert!: the expression printed the way rustc's pretty printer does (normalized spacing) */
static void ex_pretty(StrBuf *b, const Token *t, long n) {
    long i, m;
    int gdepth = 0, prev_gen_open = 0, sp, c_gen_open, clo = 0, c_clo_open, c_clo_close;
    Token *v = ex_visible(t, n, &m);
    for (i = 0; i < m; i++) {
        Token *c = &v[i];
        Token *p = i > 0 ? &v[i - 1] : 0;
        int unary_p = 0;
        sp = 1;
        c_gen_open = 0;
        c_clo_open = 0;
        c_clo_close = 0;
        if (c->kind == TK_OR) {
            if (clo) c_clo_close = 1;
            else if (i == 0 || !ex_operand_end(p)) c_clo_open = 1;
        }
        if (p && (p->kind == TK_NOT || p->kind == TK_MINUS || p->kind == TK_STAR || p->kind == TK_AND ||
                  p->kind == TK_ANDAND))
            unary_p = (i - 1 == 0) || !ex_operand_end(&v[i - 2]);
        if (c->kind == TK_LT && p && (p->kind == TK_PATHSEP || (ex_upper_ident(p) && !c->space_before))) c_gen_open = 1;
        if (i == 0) sp = 0;
        else if (prev_gen_open) sp = 0;
        else if (c_clo_close) sp = 0;
        else if (p->kind == TK_LPAREN || p->kind == TK_LBRACK || p->kind == TK_DOT || p->kind == TK_PATHSEP ||
                 p->kind == TK_POUND || p->kind == TK_DOLLAR || p->kind == TK_DOTDOT || p->kind == TK_DOTDOTEQ)
            sp = 0;
        else if (unary_p) sp = 0;
        else if (p->kind == TK_NOT && i >= 2 && v[i - 2].kind == TK_IDENT && ex_is_open(c->kind)) sp = 0;
        else if (c->kind == TK_NOT && p->kind == TK_IDENT && i + 1 < m && ex_is_open(v[i + 1].kind)) sp = 0;
        else if (c->kind == TK_RPAREN || c->kind == TK_RBRACK || c->kind == TK_COMMA || c->kind == TK_SEMI ||
                 c->kind == TK_DOT || c->kind == TK_QUESTION || c->kind == TK_PATHSEP || c->kind == TK_DOTDOT ||
                 c->kind == TK_DOTDOTEQ || c->kind == TK_COLON)
            sp = 0;
        else if (c_gen_open) sp = 0;
        else if (gdepth > 0 && (c->kind == TK_GT || c->kind == TK_SHR)) sp = 0;
        else if ((c->kind == TK_LPAREN || c->kind == TK_LBRACK) &&
                 (p->kind == TK_IDENT || p->kind == TK_RPAREN || p->kind == TK_RBRACK || p->kind == TK_QUESTION ||
                  p->kind == TK_GT || p->kind == TK_KW_self || p->kind == TK_KW_Self || p->kind == TK_KW_crate ||
                  p->kind == TK_KW_super))
            sp = 0;
        if (sp) sb_putc(b, ' ');
        ex_tok_src(b, c);
        prev_gen_open = 0;
        if (c_clo_open) {
            clo = 1;
            prev_gen_open = 1;
        } else if (c_clo_close) clo = 0;
        if (c_gen_open) {
            gdepth++;
            prev_gen_open = 1;
        } else if (gdepth > 0 && c->kind == TK_GT) gdepth--;
        else if (gdepth > 0 && c->kind == TK_SHR) gdepth = gdepth > 1 ? gdepth - 2 : 0;
    }
}

/* ---------------------------------------------------------------- generated tokens */

/* lexes a snippet of Rust source with the real lexer; the tokens get the call site's position */
static void ex_snip(ExToks *out, const char *src) {
    SrcFile tmp;
    SrcFile *save = files;
    int nsave = nfiles, n, k;
    long len = (long)strlen(src), i;
    Token *tk;
    tmp.text = xalloc(sizeof(int) * (size_t)(len + 1));
    for (i = 0; i < len; i++) tmp.text[i] = (unsigned char)src[i];
    tmp.text[len] = 0;
    tmp.len = (int)len;
    tmp.path = "<expansion>";
    tmp.disp = "<expansion>";
    files = &tmp;
    nfiles = 1;
    tk = lex_all(0, &n);
    files = save;
    nfiles = nsave;
    for (k = 0; k < n; k++) {
        if (tk[k].kind == TK_EOF) break;
        ex_stamp(&tk[k]);
        ex_tpush(out, &tk[k]);
    }
    free(tmp.text);
}

static void ex_snipf(ExToks *out, const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    ex_snip(out, buf);
}

/* a string literal token holding these bytes */
static void ex_tstr(ExToks *out, const char *bytes, long len) {
    Token t;
    long i;
    char *s;
    memset(&t, 0, sizeof t);
    t.kind = TK_STR;
    t.space_before = 1;
    ex_stamp(&t);
    s = xalloc((size_t)(len + 1));
    for (i = 0; i < len; i++) s[i] = (char)(bytes[i] & 0xFF);
    s[len] = 0;
    t.str = s;
    t.slen = len;
    ex_tpush(out, &t);
}

/* the file name of the call site as a string literal */
static void ex_emit_file(ExToks *out) {
    StrBuf b;
    sb_init(&b);
    ex_put_native(&b, files[ex_file].disp);
    ex_tstr(out, b.s, b.len);
    free(b.s);
}

/* ---------------------------------------------------------------- re-parsing helpers */

static Token *ex_copy_eof(Node *call, const Token *t, long n) {
    Token *buf = xalloc(sizeof(Token) * (size_t)(n + 1));
    if (n > 0) memcpy(buf, t, sizeof(Token) * (size_t)n);
    if (n > 0) buf[n] = t[n - 1];
    else {
        buf[n].line = call->line;
        buf[n].col = call->col;
        buf[n].file = call->file;
    }
    buf[n].kind = TK_EOF;
    buf[n].text = 0;
    buf[n].str = 0;
    buf[n].slen = 0;
    return buf;
}

/* so that nodes made by the re-parse carry the call's file */
static void ex_prep(Node *call) {
    P.file = call->file;
    ast_cur_file = call->file;
}

typedef struct {
    long s, e;       /* token range of the expression [s, e) */
    Name *name;      /* `name = expr` */
} ExSpan;

/* splits a comma separated list of expressions (`name = expr` allowed) by parsing it */
static ExSpan *ex_split(Node *call, const Token *t, long n, int *count) {
    Token *buf = ex_copy_eof(call, t, n);
    PState sv = p_save();
    ExSpan *v = 0;
    int cnt = 0, cap = 0;
    p_begin(buf, (int)n + 1, call->file);
    while (!at(TK_EOF)) {
        ExSpan sp;
        sp.name = 0;
        if (cur()->kind == TK_IDENT && peek(1)->kind == TK_EQ) {
            sp.name = cur()->text;
            next();
            next();
        }
        sp.s = P.pos;
        parse_expr();
        sp.e = P.pos;
        if (cnt == cap) {
            cap = cap ? cap * 2 : 8;
            v = xrealloc(v, sizeof(ExSpan) * (size_t)cap);
        }
        v[cnt++] = sp;
        if (!accept(TK_COMMA)) break;
    }
    if (!at(TK_EOF)) perr(cur(), "expected `,`, found %s", tok_desc(cur()));
    p_restore(sv);
    *count = cnt;
    return v;
}

/* the index just after the first expression of t[start..n) */
static long ex_expr_end(Node *call, const Token *t, long n, long start) {
    Token *buf = ex_copy_eof(call, t, n);
    PState sv = p_save();
    long e;
    p_begin(buf, (int)n + 1, call->file);
    P.pos = (int)start;
    parse_expr();
    e = P.pos;
    p_restore(sv);
    return e;
}

static Node *ex_reparse_pat(Node *call, Token *t, long n) {
    Token *buf = ex_copy_eof(call, t, n);
    PState sv = p_save();
    Node *p;
    p_begin(buf, (int)n + 1, call->file);
    p = parse_pattern();
    if (!at(TK_EOF)) perr(cur(), "unexpected token in macro");
    p_restore(sv);
    return p;
}

/* ================================================================ format strings */

enum { EX_DISPLAY = 0, EX_DEBUG, EX_LHEX, EX_UHEX, EX_OCT, EX_BIN, EX_LEXP, EX_UEXP };

static const char *ex_trait_names[] = {"Display", "Debug", "LowerHex", "UpperHex",
                                       "Octal", "Binary", "LowerExp", "UpperExp"};

typedef struct {
    int is_lit;
    char *s;
    long len;
    int arg, trait, has_spec, align, sign, alt, zero;
    long fill;
    int wk, pk;           /* 0 none, 1 literal, 2 argument */
    long wv, pv;
} ExPiece;

typedef struct {
    const Token *toks;
    long n;
    Name *name;
    int used;
    int captured;
} ExArg;

typedef struct {
    Node *call;
    ExArg *args;
    int nargs, cargs;
    int nexplicit;
    ExPiece *pc;
    int npc, cpc;
    int next_arg;
    int implicit_count;
    int implicit_over;
} ExFmt;

static void ex_fmt_err(ExFmt *f, const char *msg) {
    ex_err(f->call, "invalid format string: %s", msg);
}

static void ex_fmt_err_found(ExFmt *f, const char *s, long len, long i, const char *expected) {
    char buf[200];
    if (i >= len) {
        sprintf(buf, "expected %s but string was terminated", expected);
    } else {
        long j = i;
        long cp = ex_get_utf8(s, len, &j);
        StrBuf b;
        Name *nm;
        sb_init(&b);
        sb_put_cp(&b, (int)cp);
        nm = name_intern(b.s, b.len);
        free(b.s);
        snprintf(buf, sizeof buf, "expected %s, found `%s`", expected, nm->s);
    }
    ex_fmt_err(f, buf);
}

static void ex_fmt_add(ExFmt *f, const ExPiece *p) {
    if (f->npc == f->cpc) {
        f->cpc = f->cpc ? f->cpc * 2 : 16;
        f->pc = xrealloc(f->pc, sizeof(ExPiece) * (size_t)f->cpc);
    }
    f->pc[f->npc++] = *p;
}

static void ex_fmt_add_lit(ExFmt *f, StrBuf *lit) {
    ExPiece p;
    if (lit->len == 0) return;
    memset(&p, 0, sizeof p);
    p.is_lit = 1;
    p.len = lit->len;
    p.s = sb_take(lit);
    ex_fmt_add(f, &p);
}

static int ex_arg_count_text(char *buf, int n) {
    if (n == 0) return sprintf(buf, "no arguments were given");
    if (n == 1) return sprintf(buf, "there is 1 argument");
    return sprintf(buf, "there are %d arguments", n);
}

static int ex_fmt_next(ExFmt *f) {
    int i = f->next_arg++;
    f->implicit_count++;
    if (i >= f->nexplicit) f->implicit_over = 1;
    else f->args[i].used = 1;
    return i;
}

static int ex_fmt_index(ExFmt *f, long n) {
    char buf[200], what[100];
    if (n >= f->nexplicit) {
        ex_arg_count_text(what, f->nexplicit);
        sprintf(buf, "invalid reference to positional argument %ld (%s)", n, what);
        ex_err(f->call, "%s", buf);
    }
    f->args[n].used = 1;
    return (int)n;
}

static int ex_fmt_name(ExFmt *f, Name *nm) {
    int i;
    Token t;
    for (i = 0; i < f->nargs; i++) {
        if (f->args[i].name == nm) {
            f->args[i].used = 1;
            return i;
        }
    }
    /* a captured variable: one more argument, the path `name` */
    memset(&t, 0, sizeof t);
    t.kind = strcmp(nm->s, "self") == 0 ? TK_KW_self : TK_IDENT;
    t.text = nm;
    ex_stamp(&t);
    if (f->nargs == f->cargs) {
        f->cargs = f->cargs ? f->cargs * 2 : 8;
        f->args = xrealloc(f->args, sizeof(ExArg) * (size_t)f->cargs);
    }
    {
        Token *tp = xalloc(sizeof(Token));
        *tp = t;
        f->args[f->nargs].toks = tp;
    }
    f->args[f->nargs].n = 1;
    f->args[f->nargs].name = nm;
    f->args[f->nargs].used = 1;
    f->args[f->nargs].captured = 1;
    return f->nargs++;
}

static long ex_read_int(const char *s, long len, long *i) {
    long v = 0;
    while (*i < len && ex_is_digit(s[*i] & 0xFF)) {
        v = v * 10 + (s[*i] - '0');
        (*i)++;
    }
    return v;
}

/* parses the spec after `:` up to (not including) the closing brace */
static long ex_fmt_spec(ExFmt *f, ExPiece *p, const char *s, long len, long i) {
    long j, save;
    int c, c2;
    p->fill = ' ';
    /* [[fill]align] */
    if (i < len) {
        j = i;
        p->fill = ex_get_utf8(s, len, &j);
        c2 = j < len ? (s[j] & 0xFF) : 0;
        if (c2 == '<' || c2 == '^' || c2 == '>') {
            p->align = c2 == '<' ? 1 : (c2 == '^' ? 2 : 3);
            i = j + 1;
            p->has_spec = 1;
        } else {
            p->fill = ' ';
            c = s[i] & 0xFF;
            if (c == '<' || c == '^' || c == '>') {
                p->align = c == '<' ? 1 : (c == '^' ? 2 : 3);
                i++;
                p->has_spec = 1;
            }
        }
    }
    if (i < len && s[i] == '+') {
        p->sign = 1;
        p->has_spec = 1;
        i++;
    } else if (i < len && s[i] == '-') {
        p->sign = 2;
        p->has_spec = 1;
        i++;
    }
    if (i < len && s[i] == '#') {
        p->alt = 1;
        p->has_spec = 1;
        i++;
    }
    if (i < len && s[i] == '0' && !(i + 1 < len && s[i + 1] == '$')) {
        p->zero = 1;
        p->has_spec = 1;
        i++;
    }
    /* width */
    if (i < len && ex_is_digit(s[i] & 0xFF)) {
        long v = ex_read_int(s, len, &i);
        p->has_spec = 1;
        if (i < len && s[i] == '$') {
            i++;
            p->wk = 2;
            p->wv = ex_fmt_index(f, v);
        } else {
            p->wk = 1;
            p->wv = v;
        }
    } else if (i < len && ex_is_id_start(s[i] & 0xFF)) {
        save = i;
        while (i < len && ex_is_id_char(s[i] & 0xFF)) i++;
        if (i < len && s[i] == '$') {
            p->has_spec = 1;
            p->wk = 2;
            p->wv = ex_fmt_name(f, ex_name_from_bytes(s + save, i - save));
            i++;
        } else i = save;
    }
    /* precision */
    if (i < len && s[i] == '.') {
        i++;
        p->has_spec = 1;
        if (i < len && s[i] == '*') {
            i++;
            p->pk = 2;
            p->pv = ex_fmt_next(f);
        } else if (i < len && ex_is_digit(s[i] & 0xFF)) {
            long v = ex_read_int(s, len, &i);
            if (i < len && s[i] == '$') {
                i++;
                p->pk = 2;
                p->pv = ex_fmt_index(f, v);
            } else {
                p->pk = 1;
                p->pv = v;
            }
        } else if (i < len && ex_is_id_start(s[i] & 0xFF)) {
            save = i;
            while (i < len && ex_is_id_char(s[i] & 0xFF)) i++;
            if (i < len && s[i] == '$') {
                p->pk = 2;
                p->pv = ex_fmt_name(f, ex_name_from_bytes(s + save, i - save));
                i++;
            } else ex_fmt_err_found(f, s, len, save, "`}`");
        } else ex_fmt_err_found(f, s, len, i, "`*` or an integer");
    }
    /* type */
    if (i < len && s[i] == '?') {
        p->trait = EX_DEBUG;
        i++;
    } else if (i < len && ex_is_id_start(s[i] & 0xFF)) {
        StrBuf w;
        const char *ws;
        sb_init(&w);
        save = i;
        while (i < len && ex_is_id_char(s[i] & 0xFF)) i++;
        for (j = save; j < i; j++) sb_putc(&w, s[j] & 0xFF);
        ws = w.s;
        if (strcmp(ws, "x") == 0) p->trait = EX_LHEX;
        else if (strcmp(ws, "X") == 0) p->trait = EX_UHEX;
        else if (strcmp(ws, "o") == 0) p->trait = EX_OCT;
        else if (strcmp(ws, "b") == 0) p->trait = EX_BIN;
        else if (strcmp(ws, "e") == 0) p->trait = EX_LEXP;
        else if (strcmp(ws, "E") == 0) p->trait = EX_UEXP;
        else {
            Name *nm = ex_name_from_bytes(w.s, w.len);
            ex_err(f->call, "unknown format trait `%s`", nm->s);
        }
        if ((p->trait == EX_LHEX || p->trait == EX_UHEX) && i < len && s[i] == '?') {
            p->trait = EX_DEBUG;       /* {:x?}: debug, hex flags not supported */
            i++;
        }
        free(w.s);
    }
    return i;
}

/* after `{`: parses up to and including `}` */
static long ex_fmt_placeholder(ExFmt *f, const char *s, long len, long i) {
    ExPiece p;
    int kind = 0;
    long idx = 0, save;
    Name *nm = 0;
    memset(&p, 0, sizeof p);
    p.fill = ' ';
    if (i < len && ex_is_digit(s[i] & 0xFF)) {
        idx = ex_read_int(s, len, &i);
        kind = 1;
    } else if (i < len && ex_is_id_start(s[i] & 0xFF)) {
        save = i;
        while (i < len && ex_is_id_char(s[i] & 0xFF)) i++;
        nm = ex_name_from_bytes(s + save, i - save);
        kind = 2;
    }
    if (i < len && s[i] == ':') {
        i++;
        i = ex_fmt_spec(f, &p, s, len, i);
    }
    if (i >= len || s[i] != '}') ex_fmt_err_found(f, s, len, i, "`}`");
    i++;
    if (kind == 0) p.arg = ex_fmt_next(f);
    else if (kind == 1) p.arg = ex_fmt_index(f, idx);
    else p.arg = ex_fmt_name(f, nm);
    ex_fmt_add(f, &p);
    return i;
}

static void ex_fmt_parse(ExFmt *f, const char *s, long len) {
    StrBuf lit;
    long i = 0;
    sb_init(&lit);
    while (i < len) {
        int c = s[i] & 0xFF;
        if (c == '{') {
            if (i + 1 < len && s[i + 1] == '{') {
                sb_putc(&lit, '{');
                i += 2;
                continue;
            }
            ex_fmt_add_lit(f, &lit);
            i = ex_fmt_placeholder(f, s, len, i + 1);
        } else if (c == '}') {
            if (i + 1 < len && s[i + 1] == '}') {
                sb_putc(&lit, '}');
                i += 2;
                continue;
            }
            ex_fmt_err(f, "unmatched `}` found");
        } else {
            sb_putc(&lit, c);
            i++;
        }
    }
    ex_fmt_add_lit(f, &lit);
    free(lit.s);
}

static int ex_eval_lit(Node *call, const Token *t, long n, StrBuf *out, int strict);

/* the bytes of the format string expression t[0..n) */
static void ex_fmt_string(Node *call, const Token *t, long n, StrBuf *out) {
    if (n == 0 || !ex_eval_lit(call, t, n, out, 1)) ex_err(call, "format argument must be a string literal");
}

/*
 * Lowers the formatting of `ft` (the format string followed by the arguments)
 * to tokens: see design-expand.md.  prefix / suffix are added to the literal
 * text.  flags: 1 = if the result has no placeholders, emit nothing and
 * return the text in *lit (returns 1); 2 = a format string is required.
 */
static int ex_format(Node *call, const Token *ft, long nft, const char *prefix, const char *suffix,
                     ExToks *out, StrBuf *lit_out, int flags) {
    ExFmt f;
    ExSpan *sp = 0;
    int nsp = 0, i, k, pure, named_seen = 0, nunused;
    StrBuf fs, cat;
    ExPiece *np;
    int nnp, cnp;
    char buf[300];

    memset(&f, 0, sizeof f);
    f.call = call;
    sb_init(&fs);
    if (nft > 0) sp = ex_split(call, ft, nft, &nsp);
    else if (flags & 2) ex_err(call, "requires at least a format string argument");
    if (nsp > 0) {
        f.args = xalloc(sizeof(ExArg) * (size_t)(nsp + 8));
        f.cargs = nsp + 8;
        for (i = 1; i < nsp; i++) {
            ExArg *a = &f.args[f.nargs];
            a->toks = ft + sp[i].s;
            a->n = sp[i].e - sp[i].s;
            a->name = sp[i].name;
            if (a->name) {
                int j;
                for (j = 0; j < f.nargs; j++)
                    if (f.args[j].name == a->name) ex_err(call, "duplicate argument named `%s`", a->name->s);
                named_seen = 1;
            } else if (named_seen) {
                ex_err(call, "positional arguments cannot follow named arguments");
            }
            f.nargs++;
        }
        f.nexplicit = f.nargs;
        ex_fmt_string(call, ft + sp[0].s, sp[0].e - sp[0].s, &fs);
        ex_fmt_parse(&f, fs.s, fs.len);
        if (f.implicit_over) {
            char what[100];
            ex_arg_count_text(what, f.nexplicit);
            ex_err(call, "%d positional argument%s in format string, but %s", f.implicit_count,
                   f.implicit_count == 1 ? "" : "s", what);
        }
        nunused = 0;
        for (i = 0; i < f.nexplicit; i++)
            if (!f.args[i].used) nunused++;
        if (nunused > 1) ex_err(call, "multiple unused formatting arguments");
        for (i = 0; i < f.nexplicit; i++)
            if (!f.args[i].used) ex_err(call, f.args[i].name ? "named argument never used" : "argument never used");
    }
    free(fs.s);

    /* add the prefix and suffix, merge neighbouring literals */
    np = 0;
    nnp = 0;
    cnp = 0;
    sb_init(&cat);
    if (prefix) sb_puts(&cat, prefix);
    for (k = 0; k <= f.npc; k++) {
        if (k < f.npc && f.pc[k].is_lit) {
            long j;
            for (j = 0; j < f.pc[k].len; j++) sb_putc(&cat, f.pc[k].s[j] & 0xFF);
            continue;
        }
        if (k == f.npc && suffix) sb_puts(&cat, suffix);
        if (cat.len > 0) {
            ExPiece p;
            memset(&p, 0, sizeof p);
            p.is_lit = 1;
            p.len = cat.len;
            p.s = sb_take(&cat);
            if (nnp == cnp) {
                cnp = cnp ? cnp * 2 : 16;
                np = xrealloc(np, sizeof(ExPiece) * (size_t)cnp);
            }
            np[nnp++] = p;
        }
        if (k < f.npc) {
            if (nnp == cnp) {
                cnp = cnp ? cnp * 2 : 16;
                np = xrealloc(np, sizeof(ExPiece) * (size_t)cnp);
            }
            np[nnp++] = f.pc[k];
        }
    }
    free(cat.s);
    pure = 1;
    for (k = 0; k < nnp; k++)
        if (!np[k].is_lit) pure = 0;
    if (pure && (flags & 1)) {
        sb_init(lit_out);
        for (k = 0; k < nnp; k++) {
            long j;
            for (j = 0; j < np[k].len; j++) sb_putc(lit_out, np[k].s[j] & 0xFF);
        }
        return 1;
    }

    ex_snip(out, "{");
    for (i = 0; i < f.nargs; i++) {
        sprintf(buf, "let __a%d = &(", i);
        ex_snip(out, buf);
        ex_tappend(out, f.args[i].toks, f.args[i].n);
        ex_snip(out, ");");
    }
    for (k = 0; k < nnp; k++) {
        ExPiece *p = &np[k];
        if (p->is_lit) {
            ex_snip(out, "__f.write_str(");
            ex_tstr(out, p->s, p->len);
            ex_snip(out, ");");
            continue;
        }
        if (p->has_spec) {
            char w[60], pr[60];
            if (p->wk == 1) sprintf(w, "%ldusize, true", p->wv);
            else if (p->wk == 2) sprintf(w, "*__a%ld, true", p->wv);
            else sprintf(w, "0usize, false");
            if (p->pk == 1) sprintf(pr, "%ldusize, true", p->pv);
            else if (p->pk == 2) sprintf(pr, "*__a%ld, true", p->pv);
            else sprintf(pr, "0usize, false");
            snprintf(buf, sizeof buf, "__f.set_spec('\\u{%lx}', %du8, %du8, %s, %s, %s, %s);", p->fill, p->align,
                     p->sign, p->alt ? "true" : "false", p->zero ? "true" : "false", w, pr);
            ex_snip(out, buf);
        }
        snprintf(buf, sizeof buf, "let _ = ::core::fmt::%s::fmt(__a%d, &mut __f);", ex_trait_names[p->trait], p->arg);
        ex_snip(out, buf);
        if (p->has_spec) ex_snip(out, "__f.clear_spec();");
    }
    ex_snip(out, "}");
    return 0;
}

/* ================================================================ literals: concat!, stringify!, format string arguments */

static void ex_int_text(StrBuf *b, const Token *t) {
    char tmp[40];
    if (t->radix == 10 || t->radix == 0 || t->big) {
        ex_put_native(b, t->text->s);
    } else {
        sprintf(tmp, "%lu", (unsigned long)t->value);
        sb_puts(b, tmp);
    }
}

static void ex_loc_text(StrBuf *b, int which) {
    char tmp[40];
    if (which == 0) {
        sprintf(tmp, "%d", ex_line);
        sb_puts(b, tmp);
    } else if (which == 1) {
        sprintf(tmp, "%d", ex_col);
        sb_puts(b, tmp);
    } else ex_put_native(b, files[ex_file].disp);
}

/* the builtin macro name of a path (last segment) */
static const char *ex_call_name(Node *call) {
    Node *path = call->a;
    return path->list.v[path->list.n - 1]->name->s;
}

/*
 * Evaluates tokens that must denote a literal: a string / char / number /
 * bool literal, or a call of concat!, stringify!, line!, column!, file!.
 * strict: only a string value is accepted (format strings).  Returns 0 if
 * the tokens are not such a literal (strict) or reports an error.
 */
static int ex_eval_lit(Node *call, const Token *t, long n, StrBuf *out, int strict) {
    while (n >= 2 && ex_is_inv(&t[0]) && ex_is_inv(&t[n - 1])) {
        t++;
        n -= 2;
    }
    if (n == 1) {
        switch (t[0].kind) {
        case TK_STR:
            sb_init(out);
            { long i; for (i = 0; i < t[0].slen; i++) sb_putc(out, t[0].str[i] & 0xFF); }
            return 1;
        default:
            break;
        }
        if (strict) return 0;
        sb_init(out);
        switch (t[0].kind) {
        case TK_CHAR: ex_put_utf8(out, t[0].value); return 1;
        case TK_INT: ex_int_text(out, &t[0]); return 1;
        case TK_FLOAT: ex_put_native(out, t[0].text->s); return 1;
        case TK_KW_true: sb_puts(out, "true"); return 1;
        case TK_KW_false: sb_puts(out, "false"); return 1;
        default: break;
        }
        ex_err(call, "expected a literal");
    }
    if (!strict && n == 2 && t[0].kind == TK_MINUS && (t[1].kind == TK_INT || t[1].kind == TK_FLOAT)) {
        sb_init(out);
        sb_putc(out, '-');
        if (t[1].kind == TK_INT) ex_int_text(out, &t[1]);
        else ex_put_native(out, t[1].text->s);
        return 1;
    }
    /* name ! ( ... ) , possibly with a path prefix */
    if (n >= 3 && t[0].kind != TK_STR) {
        long i = 0, close;
        const char *name = 0;
        while (i + 1 < n && (t[i].kind == TK_PATHSEP || (t[i].kind == TK_IDENT && t[i + 1].kind == TK_PATHSEP))) i++;
        if (i < n && t[i].kind == TK_IDENT && i + 2 < n && t[i + 1].kind == TK_NOT && ex_is_open(t[i + 2].kind)) {
            name = t[i].text->s;
            close = ex_close_of(t, n, i + 2);
            if (close == n - 1) {
                const Token *in = t + i + 3;
                long nin = close - (i + 3);
                if (strcmp(name, "concat") == 0) {
                    int cnt, k;
                    ExSpan *sp = nin > 0 ? ex_split(call, in, nin, &cnt) : 0;
                    sb_init(out);
                    if (!sp) cnt = 0;
                    for (k = 0; k < cnt; k++) {
                        StrBuf one;
                        long j;
                        ex_eval_lit(call, in + sp[k].s, sp[k].e - sp[k].s, &one, 0);
                        for (j = 0; j < one.len; j++) sb_putc(out, one.s[j] & 0xFF);
                        free(one.s);
                    }
                    return 1;
                }
                if (strcmp(name, "stringify") == 0) {
                    sb_init(out);
                    ex_stringify(out, in, nin);
                    return 1;
                }
                if (strcmp(name, "line") == 0 && !strict) {
                    sb_init(out);
                    ex_loc_text(out, 0);
                    return 1;
                }
                if (strcmp(name, "column") == 0 && !strict) {
                    sb_init(out);
                    ex_loc_text(out, 1);
                    return 1;
                }
                if (strcmp(name, "file") == 0) {
                    sb_init(out);
                    ex_loc_text(out, 2);
                    return 1;
                }
            }
        }
    }
    if (strict) return 0;
    ex_err(call, "expected a literal");
    return 0;
}

/* ================================================================ built-in macros */

static const char *ex_builtin_names[] = {
    "print", "println", "eprint", "eprintln", "format", "write", "writeln", "panic", "assert", "assert_eq",
    "assert_ne", "debug_assert", "debug_assert_eq", "debug_assert_ne", "unreachable", "todo", "unimplemented",
    "vec", "matches", "line", "column", "file", "stringify", "concat", "dbg", "compile_error", 0};

static int ex_is_builtin(const char *s) {
    int i;
    for (i = 0; ex_builtin_names[i]; i++)
        if (strcmp(ex_builtin_names[i], s) == 0) return 1;
    return 0;
}

/* `::core::panicking::panic_str(FILE, LINE, COL, "msg")` */
static void ex_emit_panic_str(ExToks *out, const char *bytes, long len) {
    ex_snip(out, "::core::panicking::panic_str(");
    ex_emit_file(out);
    ex_snipf(out, ", %du32, %du32,", ex_line, ex_col);
    ex_tstr(out, bytes, len);
    ex_snip(out, ")");
}

static void ex_builtin(Node *call, const char *name, ExToks *out) {
    const Token *t = call->toks;
    long n = call->ntoks;
    ExSpan *sp = 0;
    int nsp = 0, nl = 0, i;
    ex_set_site(call);

    if (strcmp(name, "print") == 0 || strcmp(name, "println") == 0 || strcmp(name, "eprint") == 0 ||
        strcmp(name, "eprintln") == 0) {
        int err = name[0] == 'e';
        nl = name[strlen(name) - 1] == 'n';
        ex_snip(out, err ? "{ let mut __out = ::core::io::stderr_raw();" : "{ let mut __out = ::core::io::stdout_raw();");
        ex_snip(out, "let mut __f = ::core::fmt::Formatter::new(&mut __out);");
        ex_format(call, t, n, 0, nl ? "\n" : 0, out, 0, nl ? 0 : 2);
        ex_snip(out, "}");
    } else if (strcmp(name, "format") == 0) {
        ex_snip(out, "{ let mut __s = ::core::string::String::new(); { let mut __f = ::core::fmt::Formatter::new(&mut __s);");
        ex_format(call, t, n, 0, 0, out, 0, 2);
        ex_snip(out, "} __s }");
    } else if (strcmp(name, "write") == 0 || strcmp(name, "writeln") == 0) {
        nl = name[strlen(name) - 1] == 'n';
        if (n > 0) sp = ex_split(call, t, n, &nsp);
        if (nsp < 1) ex_err(call, "unexpected end of macro invocation");
        ex_snip(out, "{ let mut __f = ::core::fmt::Formatter::new(&mut *(");
        ex_tappend(out, t + sp[0].s, sp[0].e - sp[0].s);
        ex_snip(out, "));");
        if (nsp > 1) ex_format(call, t + sp[1].s, n - sp[1].s, 0, nl ? "\n" : 0, out, 0, nl ? 0 : 2);
        else ex_format(call, t, 0, 0, nl ? "\n" : 0, out, 0, nl ? 0 : 2);
        ex_snip(out, "::core::result::Result::Ok(()) }");
    } else if (strcmp(name, "panic") == 0 || strcmp(name, "unreachable") == 0 || strcmp(name, "todo") == 0 ||
               strcmp(name, "unimplemented") == 0) {
        const char *dflt = "explicit panic", *prefix = 0;
        StrBuf lit;
        if (name[0] == 'u' && name[2] == 'r') {
            dflt = "internal error: entered unreachable code";
            prefix = "internal error: entered unreachable code: ";
        } else if (name[0] == 't') {
            dflt = "not yet implemented";
            prefix = "not yet implemented: ";
        } else if (name[0] == 'u') {
            dflt = "not implemented";
            prefix = "not implemented: ";
        }
        if (n == 0) {
            ex_emit_panic_str(out, dflt, (long)strlen(dflt));
        } else {
            ExToks blk;
            memset(&blk, 0, sizeof blk);
            if (ex_format(call, t, n, prefix, 0, &blk, &lit, 1)) {
                ex_emit_panic_str(out, lit.s, lit.len);
                free(lit.s);
            } else {
                ex_snip(out, "{ let mut __out = ::core::panicking::begin(");
                ex_emit_file(out);
                ex_snipf(out, ", %du32, %du32);", ex_line, ex_col);
                ex_snip(out, "let mut __f = ::core::fmt::Formatter::new(&mut __out);");
                ex_tappend(out, blk.v, blk.n);
                ex_snip(out, "::core::panicking::end() }");
            }
        }
    } else if (strcmp(name, "assert") == 0 || strcmp(name, "debug_assert") == 0) {
        if (n > 0) sp = ex_split(call, t, n, &nsp);
        if (nsp < 1) ex_err(call, "macro requires a boolean expression as an argument");
        ex_snip(out, "if !(");
        ex_tappend(out, t + sp[0].s, sp[0].e - sp[0].s);
        ex_snip(out, ") {");
        if (nsp > 1) {
            ex_snip(out, "::core::panic!(");
            ex_tappend(out, t + sp[1].s, n - sp[1].s);
            ex_snip(out, ")");
        } else {
            StrBuf m;
            sb_init(&m);
            sb_puts(&m, "assertion failed: ");
            ex_pretty(&m, t + sp[0].s, sp[0].e - sp[0].s);
            ex_emit_panic_str(out, m.s, m.len);
            free(m.s);
        }
        ex_snip(out, "}");
    } else if (strcmp(name, "assert_eq") == 0 || strcmp(name, "assert_ne") == 0 ||
               strcmp(name, "debug_assert_eq") == 0 || strcmp(name, "debug_assert_ne") == 0) {
        int ne = name[strlen(name) - 1] == 'e' && name[strlen(name) - 2] == 'n';
        if (n > 0) sp = ex_split(call, t, n, &nsp);
        if (nsp < 2) ex_err(call, "unexpected end of macro invocation");
        ex_snip(out, "match (&(");
        ex_tappend(out, t + sp[0].s, sp[0].e - sp[0].s);
        ex_snip(out, "), &(");
        ex_tappend(out, t + sp[1].s, sp[1].e - sp[1].s);
        ex_snipf(out, ")) { (__l, __r) => if !(*__l %s *__r) {", ne ? "!=" : "==");
        ex_snipf(out, "::core::panicking::assert_failed%s(\"%s\", &*__l, &*__r,", nsp > 2 ? "_msg" : "",
                 ne ? "!=" : "==");
        ex_emit_file(out);
        ex_snipf(out, ", %du32, %du32", ex_line, ex_col);
        if (nsp > 2) {
            ex_snip(out, ", &::core::format!(");
            ex_tappend(out, t + sp[2].s, n - sp[2].s);
            ex_snip(out, ")");
        }
        ex_snip(out, ") } }");
    } else if (strcmp(name, "vec") == 0) {
        if (n == 0) {
            ex_snip(out, "::core::vec::Vec::new()");
        } else {
            long e = ex_expr_end(call, t, n, 0);
            if (e < n && t[e].kind == TK_SEMI) {
                long e2 = ex_expr_end(call, t, n, e + 1);
                if (e2 != n) ex_err(call, "unexpected token in vec! macro");
                ex_snip(out, "::core::vec::from_elem(");
                ex_tappend(out, t, e);
                ex_snip(out, ",");
                ex_tappend(out, t + e + 1, n - e - 1);
                ex_snip(out, ")");
            } else {
                ex_snip(out, "::core::vec::Vec::from_array([");
                ex_tappend(out, t, n);
                ex_snip(out, "])");
            }
        }
    } else if (strcmp(name, "matches") == 0) {
        long e = n > 0 ? ex_expr_end(call, t, n, 0) : 0, pe = n;
        if (n == 0 || e >= n || t[e].kind != TK_COMMA) ex_err(call, "unexpected end of macro invocation");
        if (pe > e + 1 && t[pe - 1].kind == TK_COMMA) pe--;
        if (pe <= e + 1) ex_err(call, "unexpected end of macro invocation");
        ex_snip(out, "match (");
        ex_tappend(out, t, e);
        ex_snip(out, ") {");
        ex_tappend(out, t + e + 1, pe - e - 1);
        ex_snip(out, "=> true, _ => false }");
    } else if (strcmp(name, "line") == 0) {
        ex_snipf(out, "%du32", ex_line);
    } else if (strcmp(name, "column") == 0) {
        ex_snipf(out, "%du32", ex_col);
    } else if (strcmp(name, "file") == 0) {
        ex_emit_file(out);
    } else if (strcmp(name, "stringify") == 0) {
        StrBuf b;
        sb_init(&b);
        ex_stringify(&b, t, n);
        ex_tstr(out, b.s, b.len);
        free(b.s);
    } else if (strcmp(name, "concat") == 0) {
        StrBuf b, one;
        long j;
        sb_init(&b);
        if (n > 0) sp = ex_split(call, t, n, &nsp);
        for (i = 0; i < nsp; i++) {
            ex_eval_lit(call, t + sp[i].s, sp[i].e - sp[i].s, &one, 0);
            for (j = 0; j < one.len; j++) sb_putc(&b, one.s[j] & 0xFF);
            free(one.s);
        }
        ex_tstr(out, b.s, b.len);
        free(b.s);
    } else if (strcmp(name, "dbg") == 0) {
        ex_err(call, "dbg! is not supported");
    } else if (strcmp(name, "compile_error") == 0) {
        StrBuf m;
        if (n == 1 && t[0].kind == TK_STR && ex_eval_lit(call, t, n, &m, 1)) ex_err(call, "%s", ex_name_from_bytes(m.s, m.len)->s);
        ex_err(call, "compile_error! takes a string literal");
    }
}

/* ================================================================ macro_rules! */

enum { EXM_TOK = 1, EXM_FRAG, EXM_REP };

enum { EF_EXPR = 1, EF_IDENT, EF_TY, EF_PAT, EF_PAT_PARAM, EF_PATH, EF_STMT, EF_BLOCK, EF_ITEM, EF_LITERAL,
       EF_TT, EF_LIFETIME, EF_VIS, EF_META };

typedef struct ExM ExM;
struct ExM {
    int kind;
    Token tok;               /* EXM_TOK: the token to match; EXM_REP: nothing */
    Name *name;              /* EXM_FRAG */
    int frag;
    int var;                 /* EXM_FRAG: variable number */
    ExM *sub;                /* EXM_REP: the repeated sequence */
    int nsub;
    int has_sep;
    Token sep;
    int op;                  /* TK_STAR, TK_PLUS, TK_QUESTION */
    int v0, v1;              /* EXM_REP: the variables [v0, v1) bound inside */
};

typedef struct {
    ExM *m;
    int nm;
    Name **vars;
    int nvars;
    Token *tr;
    long ntr;
} ExRule;

typedef struct {
    Name *name;
    Node *def;
    ExRule *rules;
    int nrules;
    int compiled;
} ExMac;

typedef struct {
    Name **v;
    int n, cap;
} ExVarTab;

static int ex_frag_kind(const char *s) {
    if (strcmp(s, "expr") == 0) return EF_EXPR;
    if (strcmp(s, "ident") == 0) return EF_IDENT;
    if (strcmp(s, "ty") == 0) return EF_TY;
    if (strcmp(s, "pat") == 0) return EF_PAT;
    if (strcmp(s, "pat_param") == 0) return EF_PAT_PARAM;
    if (strcmp(s, "path") == 0) return EF_PATH;
    if (strcmp(s, "stmt") == 0) return EF_STMT;
    if (strcmp(s, "block") == 0) return EF_BLOCK;
    if (strcmp(s, "item") == 0) return EF_ITEM;
    if (strcmp(s, "literal") == 0) return EF_LITERAL;
    if (strcmp(s, "tt") == 0) return EF_TT;
    if (strcmp(s, "lifetime") == 0) return EF_LIFETIME;
    if (strcmp(s, "vis") == 0) return EF_VIS;
    if (strcmp(s, "meta") == 0) return EF_META;
    return 0;
}

static void ex_def_err(Node *def, const char *fmt, const char *arg) {
    char buf[300];
    snprintf(buf, sizeof buf, fmt, arg);
    error_at(def->file, def->line, "%s", buf);
}

static int ex_is_op(int k) {
    return k == TK_STAR || k == TK_PLUS || k == TK_QUESTION;
}

/* compiles the matcher tokens t[0..n) */
static ExM *ex_compile(Node *def, const Token *t, long n, ExVarTab *vt, int *outn) {
    ExM *v = 0;
    int cnt = 0, cap = 0;
    long i = 0;
    while (i < n) {
        ExM m;
        memset(&m, 0, sizeof m);
        if (t[i].kind == TK_DOLLAR && i + 1 < n && t[i + 1].kind == TK_LPAREN) {
            long j = ex_close_of(t, n, i + 1), k;
            if (j < 0) ex_def_err(def, "unclosed delimiter in macro matcher%s", "");
            m.kind = EXM_REP;
            m.v0 = vt->n;
            m.sub = ex_compile(def, t + i + 2, j - (i + 2), vt, &m.nsub);
            m.v1 = vt->n;
            k = j + 1;
            if (k < n && ex_is_op(t[k].kind)) {
                m.op = t[k].kind;
                i = k + 1;
            } else if (k + 1 < n && ex_is_op(t[k + 1].kind)) {
                m.has_sep = 1;
                m.sep = t[k];
                m.op = t[k + 1].kind;
                i = k + 2;
            } else ex_def_err(def, "expected one of: `*`, `+`, or `?`%s", "");
        } else if (t[i].kind == TK_DOLLAR && i + 1 < n && t[i + 1].text &&
                   (t[i + 1].kind == TK_IDENT || tk_is_kw(t[i + 1].kind))) {
            int f, d;
            if (i + 3 >= n || t[i + 2].kind != TK_COLON || t[i + 3].kind != TK_IDENT)
                ex_def_err(def, "missing fragment specifier%s", "");
            f = ex_frag_kind(t[i + 3].text->s);
            if (!f) ex_def_err(def, "invalid fragment specifier `%s`", t[i + 3].text->s);
            for (d = 0; d < vt->n; d++)
                if (vt->v[d] == t[i + 1].text) ex_def_err(def, "duplicate matcher binding `%s`", t[i + 1].text->s);
            if (vt->n == vt->cap) {
                vt->cap = vt->cap ? vt->cap * 2 : 8;
                vt->v = xrealloc(vt->v, sizeof(Name *) * (size_t)vt->cap);
            }
            m.kind = EXM_FRAG;
            m.name = t[i + 1].text;
            m.frag = f;
            m.var = vt->n;
            vt->v[vt->n++] = m.name;
            i += 4;
        } else {
            m.kind = EXM_TOK;
            m.tok = t[i];
            i++;
        }
        if (cnt == cap) {
            cap = cap ? cap * 2 : 8;
            v = xrealloc(v, sizeof(ExM) * (size_t)cap);
        }
        v[cnt++] = m;
    }
    *outn = cnt;
    return v;
}

static void ex_mac_compile(ExMac *m) {
    long i;
    if (m->compiled) return;
    m->compiled = 1;
    m->nrules = (int)m->def->list.n;
    m->rules = xalloc(sizeof(ExRule) * (size_t)(m->nrules + 1));
    for (i = 0; i < m->nrules; i++) {
        Node *r = m->def->list.v[i];
        ExVarTab vt;
        ExRule *er = &m->rules[i];
        memset(&vt, 0, sizeof vt);
        er->m = ex_compile(r->a, r->a->toks, r->a->ntoks, &vt, &er->nm);
        er->vars = vt.v;
        er->nvars = vt.n;
        er->tr = r->b->toks;
        er->ntr = r->b->ntoks;
    }
}

/* ---------------------------------------------------------------- matching */

typedef struct ExB ExB;
struct ExB {
    int isrep;
    int frag;
    const Token *toks;
    long ntoks;
    ExB **reps;
    long nreps;
};

typedef struct {
    const Token *in;       /* the call's tokens */
    Token *work;           /* a copy plus TK_EOF, for the parser */
    long nin;
    int nvars;
    int file;
} ExCtx;

static long ex_best_fail;

static void ex_fail(long pos) {
    if (pos > ex_best_fail) ex_best_fail = pos;
}

static int ex_tok_eq(const Token *a, const Token *b) {
    if (a->kind != b->kind) return 0;
    switch (a->kind) {
    case TK_IDENT:
        return a->text == b->text;
    case TK_LIFETIME:
        return a->text == b->text;
    case TK_INT:
        return a->value == b->value && a->text == b->text && a->suffix == b->suffix;
    case TK_FLOAT:
        return a->text == b->text && a->suffix == b->suffix;
    case TK_CHAR:
    case TK_BYTE:
        return a->value == b->value;
    case TK_STR:
    case TK_BSTR:
        return a->slen == b->slen && memcmp(a->str, b->str, (size_t)a->slen * sizeof(char)) == 0;
    default:
        return 1;
    }
}

static int ex_can_start(int frag, const Token *t) {
    int k = t->kind;
    if (k == TK_EOF || ex_is_close(k)) return 0;
    switch (frag) {
    case EF_EXPR:
    case EF_STMT:
        switch (k) {
        case TK_IDENT: case TK_LIFETIME: case TK_INT: case TK_FLOAT: case TK_CHAR: case TK_BYTE: case TK_STR:
        case TK_BSTR: case TK_LPAREN: case TK_LBRACK: case TK_LBRACE: case TK_MINUS: case TK_NOT: case TK_STAR:
        case TK_AND: case TK_ANDAND: case TK_OR: case TK_OROR: case TK_DOTDOT: case TK_DOTDOTEQ: case TK_LT:
        case TK_PATHSEP: case TK_POUND: case TK_UNDERSCORE:
            return 1;
        case TK_KW_true: case TK_KW_false: case TK_KW_self: case TK_KW_Self: case TK_KW_super: case TK_KW_crate:
        case TK_KW_if: case TK_KW_match: case TK_KW_loop: case TK_KW_while: case TK_KW_for: case TK_KW_unsafe:
        case TK_KW_move: case TK_KW_async: case TK_KW_return: case TK_KW_break: case TK_KW_continue:
        case TK_KW_let: case TK_KW_const:
            return 1;
        case TK_KW_fn: case TK_KW_struct: case TK_KW_enum: case TK_KW_impl: case TK_KW_trait: case TK_KW_type:
        case TK_KW_mod: case TK_KW_use: case TK_KW_static: case TK_KW_extern: case TK_KW_pub:
            return frag == EF_STMT;
        default:
            return 0;
        }
    case EF_TY:
        switch (k) {
        case TK_IDENT: case TK_LPAREN: case TK_LBRACK: case TK_NOT: case TK_STAR: case TK_AND: case TK_ANDAND:
        case TK_LT: case TK_PATHSEP: case TK_UNDERSCORE: case TK_LIFETIME: case TK_QUESTION:
        case TK_KW_Self: case TK_KW_self: case TK_KW_super: case TK_KW_crate: case TK_KW_dyn: case TK_KW_impl:
        case TK_KW_fn: case TK_KW_unsafe: case TK_KW_extern: case TK_KW_for:
            return 1;
        default:
            return 0;
        }
    case EF_PAT:
    case EF_PAT_PARAM:
        switch (k) {
        case TK_IDENT: case TK_INT: case TK_FLOAT: case TK_CHAR: case TK_BYTE: case TK_STR: case TK_BSTR:
        case TK_LPAREN: case TK_LBRACK: case TK_AND: case TK_ANDAND: case TK_MINUS: case TK_UNDERSCORE:
        case TK_DOTDOT: case TK_DOTDOTEQ: case TK_PATHSEP: case TK_LT:
        case TK_KW_true: case TK_KW_false: case TK_KW_ref: case TK_KW_mut: case TK_KW_box: case TK_KW_self:
        case TK_KW_Self: case TK_KW_super: case TK_KW_crate:
            return 1;
        case TK_OR:
            return frag == EF_PAT;
        default:
            return 0;
        }
    case EF_PATH:
        return k == TK_IDENT || k == TK_PATHSEP || k == TK_LT || k == TK_KW_self || k == TK_KW_Self ||
               k == TK_KW_super || k == TK_KW_crate;
    case EF_META:
        return k == TK_IDENT || tk_is_kw(k) || k == TK_PATHSEP;
    default:
        return 1;
    }
}

/* the extent of one fragment starting at work[pos]; 0 if it does not match */
static int ex_frag_extent(ExCtx *c, int frag, long pos, long *end) {
    const Token *t = &c->work[pos];
    long j;
    PState sv;
    int ok = 1;
    switch (frag) {
    case EF_TT:
        if (pos >= c->nin || ex_is_close(t->kind)) return 0;
        if (ex_is_open(t->kind)) {
            j = ex_close_of(c->work, c->nin, pos);
            if (j < 0) return 0;
            *end = j + 1;
        } else *end = pos + 1;
        return 1;
    case EF_IDENT:
        if (pos >= c->nin) return 0;
        if (t->kind != TK_IDENT && !tk_is_kw(t->kind)) return 0;
        *end = pos + 1;
        return 1;
    case EF_LIFETIME:
        if (pos >= c->nin || t->kind != TK_LIFETIME) return 0;
        *end = pos + 1;
        return 1;
    case EF_LITERAL:
        j = pos;
        if (j < c->nin && c->work[j].kind == TK_MINUS) j++;
        if (j >= c->nin) return 0;
        switch (c->work[j].kind) {
        case TK_INT: case TK_FLOAT: case TK_CHAR: case TK_BYTE: case TK_STR: case TK_BSTR: case TK_KW_true:
        case TK_KW_false:
            break;
        default:
            return 0;
        }
        if (j > pos && c->work[j].kind != TK_INT && c->work[j].kind != TK_FLOAT) return 0;
        *end = j + 1;
        return 1;
    case EF_BLOCK:
        if (pos >= c->nin || t->kind != TK_LBRACE) return 0;
        j = ex_close_of(c->work, c->nin, pos);
        if (j < 0) return 0;
        *end = j + 1;
        return 1;
    case EF_VIS:
        *end = pos;
        if (pos < c->nin && t->kind == TK_KW_pub) {
            *end = pos + 1;
            if (pos + 1 < c->nin && c->work[pos + 1].kind == TK_LPAREN && pos + 2 < c->nin &&
                (c->work[pos + 2].kind == TK_KW_crate || c->work[pos + 2].kind == TK_KW_super ||
                 c->work[pos + 2].kind == TK_KW_self || c->work[pos + 2].kind == TK_KW_in)) {
                j = ex_close_of(c->work, c->nin, pos + 1);
                if (j > 0) *end = j + 1;
            }
        }
        return 1;
    default:
        break;
    }
    if (pos >= c->nin || !ex_can_start(frag, t)) {
        if (!(frag == EF_ITEM && pos < c->nin)) return 0;
    }
    sv = p_save();
    p_begin(c->work, (int)c->nin + 1, c->file);
    P.pos = (int)pos;
    switch (frag) {
    case EF_EXPR:
        parse_expr();
        break;
    case EF_TY:
        parse_type();
        break;
    case EF_PAT:
        parse_pattern();
        break;
    case EF_PAT_PARAM:
        parse_pattern_no_alt();
        break;
    case EF_PATH:
        parse_path(PATH_TYPE);
        break;
    case EF_ITEM:
        if (!at_item_start() && !at(TK_POUND)) ok = 0;
        else if (!parse_item()) ok = 0;
        break;
    case EF_STMT:
        if (at(TK_KW_let)) {
            next();
            parse_pattern();
            if (accept(TK_COLON)) parse_type();
            if (accept(TK_EQ)) {
                parse_expr();
                if (accept(TK_KW_else)) parse_block();
            }
        } else if (at_item_start()) {
            if (!parse_item()) ok = 0;
        } else parse_expr();
        break;
    case EF_META:
        parse_path(PATH_SIMPLE);
        if (ex_is_open(cur()->kind)) {
            j = ex_close_of(c->work, c->nin, P.pos);
            if (j > 0) P.pos = (int)j + 1;
        } else if (accept(TK_EQ)) parse_expr();
        break;
    default:
        ok = 0;
        break;
    }
    *end = P.pos;
    p_restore(sv);
    if (*end <= pos) ok = 0;
    return ok;
}

static int ex_match_seq(ExCtx *c, ExM *seq, int nseq, long *pos, ExB **frame) {
    int i;
    for (i = 0; i < nseq; i++) {
        ExM *m = &seq[i];
        if (m->kind == EXM_TOK) {
            if (*pos >= c->nin || !ex_tok_eq(&m->tok, &c->in[*pos])) {
                ex_fail(*pos);
                return 0;
            }
            (*pos)++;
        } else if (m->kind == EXM_FRAG) {
            long end = 0;
            ExB *b;
            if (!ex_frag_extent(c, m->frag, *pos, &end)) {
                ex_fail(*pos);
                return 0;
            }
            b = xalloc(sizeof(ExB));
            b->frag = m->frag;
            b->toks = c->in + *pos;
            b->ntoks = end - *pos;
            frame[m->var] = b;
            *pos = end;
        } else {
            ExB ***iters = 0;
            long count = 0, cap = 0, p = *pos, q;
            int v;
            for (;;) {
                ExB **sub;
                q = p;
                if (count > 0 && m->has_sep) {
                    if (q >= c->nin || !ex_tok_eq(&m->sep, &c->in[q])) break;
                    q++;
                }
                if (m->op == TK_QUESTION && count == 1) break;
                sub = xalloc(sizeof(ExB *) * (size_t)(c->nvars + 1));
                if (!ex_match_seq(c, m->sub, m->nsub, &q, sub)) break;
                if (q == p) break;
                if (count == cap) {
                    cap = cap ? cap * 2 : 8;
                    iters = xrealloc(iters, sizeof(ExB **) * (size_t)cap);
                }
                iters[count++] = sub;
                p = q;
            }
            if (m->op == TK_PLUS && count == 0) {
                ex_fail(p);
                return 0;
            }
            for (v = m->v0; v < m->v1; v++) {
                ExB *b = xalloc(sizeof(ExB));
                long k;
                b->isrep = 1;
                b->nreps = count;
                b->reps = xalloc(sizeof(ExB *) * (size_t)(count + 1));
                for (k = 0; k < count; k++) b->reps[k] = iters[k][v];
                frame[v] = b;
            }
            *pos = p;
        }
    }
    return 1;
}

/* ---------------------------------------------------------------- transcription */

typedef struct {
    ExRule *rule;
    Node *call;
    Name **hn;          /* hygiene: names bound by let / for / closure parameters in the transcriber */
    Name **hr;          /* ... and what they are renamed to in this expansion */
    int nh;
} ExTr;

static int ex_hyg_counter;

static void ex_hyg_add(ExTr *c, Name *nm, const char *suffix) {
    int i;
    StrBuf b;
    for (i = 0; i < c->nh; i++)
        if (c->hn[i] == nm) return;
    c->hn = xrealloc(c->hn, sizeof(Name *) * (size_t)(c->nh + 1));
    c->hr = xrealloc(c->hr, sizeof(Name *) * (size_t)(c->nh + 1));
    sb_init(&b);
    sb_puts(&b, nm->s);
    sb_puts(&b, suffix);
    c->hn[c->nh] = nm;
    c->hr[c->nh] = name_intern(b.s, b.len);
    c->nh++;
    free(b.s);
}

/* the binders in t[s..e): identifiers of a pattern (`let` / `for`) or of closure parameters */
static void ex_hyg_binders(ExTr *c, const Token *t, long s, long e, const char *suffix, int closure) {
    long k;
    int depth = 0, intype = 0;
    for (k = s; k < e; k++) {
        const Token *x = &t[k];
        if (ex_is_open(x->kind)) depth++;
        else if (ex_is_close(x->kind)) depth--;
        else if (depth == 0 && x->kind == TK_COMMA) intype = 0;
        else if (closure && depth == 0 && x->kind == TK_COLON) intype = 1;
        else if (!closure && depth == 0 && x->kind == TK_COLON) return;
        else if (x->kind == TK_IDENT && !intype && !x->raw) {
            const char *s0 = x->text->s;
            if (k > s && (t[k - 1].kind == TK_DOLLAR || t[k - 1].kind == TK_PATHSEP)) continue;
            if (k + 1 < e && (t[k + 1].kind == TK_PATHSEP || t[k + 1].kind == TK_LPAREN || t[k + 1].kind == TK_LBRACE ||
                              t[k + 1].kind == TK_NOT))
                continue;
            if (s0[0] >= 'A' && s0[0] <= 'Z') continue;
            ex_hyg_add(c, x->text, suffix);
        }
    }
}

static void ex_hyg_build(ExTr *c, const Token *t, long n) {
    long i, j;
    char suffix[32];
    int depth;
    sprintf(suffix, "__h%d", ++ex_hyg_counter);
    for (i = 0; i < n; i++) {
        if (t[i].kind == TK_KW_let || t[i].kind == TK_KW_for) {
            int is_for = t[i].kind == TK_KW_for;
            depth = 0;
            for (j = i + 1; j < n; j++) {
                if (is_for && depth == 0 && (t[j].kind == TK_LBRACE || t[j].kind == TK_KW_where)) break;
                if (ex_is_open(t[j].kind)) depth++;
                else if (ex_is_close(t[j].kind)) {
                    depth--;
                    if (depth < 0) break;
                } else if (depth == 0 && (t[j].kind == TK_EQ || t[j].kind == TK_SEMI || t[j].kind == TK_KW_in ||
                                         t[j].kind == TK_KW_else))
                    break;
            }
            if (is_for && !(j < n && t[j].kind == TK_KW_in)) continue;    /* `for<'a>` and so on */
            ex_hyg_binders(c, t, i + 1, j, suffix, 0);
        } else if (t[i].kind == TK_OR && (i == 0 || !ex_operand_end(&t[i - 1]))) {
            depth = 0;
            for (j = i + 1; j < n; j++) {
                if (ex_is_open(t[j].kind)) depth++;
                else if (ex_is_close(t[j].kind)) {
                    depth--;
                    if (depth < 0) break;
                } else if (depth == 0 && t[j].kind == TK_OR) break;
            }
            if (j < n && t[j].kind == TK_OR) ex_hyg_binders(c, t, i + 1, j, suffix, 1);
            i = j;
        }
    }
}

/* the renamed spelling of the transcriber token t[i] (a use of a name bound by the macro itself), or 0 */
static Name *ex_hyg_rename(ExTr *c, const Token *t, long n, long i) {
    int k;
    if (c->nh == 0 || t[i].kind != TK_IDENT || t[i].raw) return 0;
    if (i > 0 && (t[i - 1].kind == TK_DOT || t[i - 1].kind == TK_PATHSEP || t[i - 1].kind == TK_DOLLAR)) return 0;
    if (i + 1 < n && (t[i + 1].kind == TK_PATHSEP || t[i + 1].kind == TK_NOT)) return 0;
    if (i > 0 && i + 1 < n && t[i + 1].kind == TK_COLON && (t[i - 1].kind == TK_LBRACE || t[i - 1].kind == TK_COMMA))
        return 0;
    for (k = 0; k < c->nh; k++)
        if (c->hn[k] == t[i].text) return c->hr[k];
    return 0;
}

static int ex_var_index(ExRule *r, Name *nm) {
    int i;
    for (i = 0; i < r->nvars; i++)
        if (r->vars[i] == nm) return i;
    return -1;
}

static void ex_put_site_tok(ExToks *out, const Token *src) {
    Token t = *src;
    ex_stamp(&t);
    ex_tpush(out, &t);
}

static void ex_put_inv_paren(ExToks *out, int kind) {
    Token t;
    memset(&t, 0, sizeof t);
    t.kind = kind;
    t.text = name_intern_str(kind == TK_LPAREN ? "(" : ")");
    t.raw = 2;
    ex_stamp(&t);
    ex_tpush(out, &t);
}

static void ex_transcribe(ExTr *c, const Token *t, long n, ExB **env, ExToks *out) {
    long i = 0;
    while (i < n) {
        const Token *tk = &t[i];
        if (tk->kind == TK_DOLLAR && i + 1 < n) {
            const Token *nx = &t[i + 1];
            if (nx->kind == TK_LPAREN) {
                long j = ex_close_of(t, n, i + 1), k;
                const Token *sep = 0;
                int op, nv = 0, count = 0, r, cap = 0, a;
                int *rv = 0;
                long b;
                if (j < 0) ex_err(c->call, "unclosed delimiter in macro transcriber");
                k = j + 1;
                if (k < n && ex_is_op(t[k].kind)) {
                    op = t[k].kind;
                } else if (k + 1 < n && ex_is_op(t[k + 1].kind)) {
                    sep = &t[k];
                    op = t[k + 1].kind;
                    k++;
                } else ex_err(c->call, "expected one of: `*`, `+`, or `?`");
                /* the repeating variables used in the body */
                for (b = i + 2; b + 1 < j; b++) {
                    if (t[b].kind == TK_DOLLAR && t[b + 1].text) {
                        int v = ex_var_index(c->rule, t[b + 1].text);
                        if (v >= 0 && env[v] && env[v]->isrep) {
                            int dup = 0;
                            for (a = 0; a < nv; a++)
                                if (rv[a] == v) dup = 1;
                            if (!dup) {
                                if (nv == cap) {
                                    cap = cap ? cap * 2 : 8;
                                    rv = xrealloc(rv, sizeof(int) * (size_t)cap);
                                }
                                rv[nv++] = v;
                            }
                        }
                    }
                }
                if (nv == 0)
                    ex_err(c->call, "attempted to repeat an expression containing no syntax variables matched as repeating at this depth");
                count = (int)env[rv[0]]->nreps;
                for (a = 1; a < nv; a++)
                    if (env[rv[a]]->nreps != count)
                        ex_err(c->call, "meta-variable `%s` repeats %d times, but `%s` repeats %d times",
                               c->rule->vars[rv[0]]->s, count, c->rule->vars[rv[a]]->s, (int)env[rv[a]]->nreps);
                for (r = 0; r < count; r++) {
                    ExB **ne = xalloc(sizeof(ExB *) * (size_t)(c->rule->nvars + 1));
                    memcpy(ne, env, sizeof(ExB *) * (size_t)(c->rule->nvars));
                    for (a = 0; a < nv; a++) ne[rv[a]] = env[rv[a]]->reps[r];
                    if (r > 0 && sep) ex_put_site_tok(out, sep);
                    ex_transcribe(c, t + i + 2, j - (i + 2), ne, out);
                }
                (void)op;
                i = k + 1;
                continue;
            }
            if (nx->kind == TK_KW_crate) {
                ex_snip(out, "crate");
                i += 2;
                continue;
            }
            if (nx->text && (nx->kind == TK_IDENT || tk_is_kw(nx->kind))) {
                int v = ex_var_index(c->rule, nx->text);
                if (v >= 0 && env[v]) {
                    ExB *b = env[v];
                    if (b->isrep)
                        ex_err(c->call, "variable '%s' is still repeating at this depth", nx->text->s);
                    if (b->frag == EF_EXPR && b->ntoks > 1) {
                        ex_put_inv_paren(out, TK_LPAREN);
                        ex_tappend(out, b->toks, b->ntoks);
                        ex_put_inv_paren(out, TK_RPAREN);
                    } else ex_tappend(out, b->toks, b->ntoks);
                    i += 2;
                    continue;
                }
            }
        }
        {
            Name *rn = ex_hyg_rename(c, t, n, i);
            if (rn) {
                Token cp = *tk;
                cp.text = rn;
                ex_put_site_tok(out, &cp);
            } else ex_put_site_tok(out, tk);
        }
        i++;
    }
}

/* the call's tokens against each rule in turn; the transcription of the first match */
static void ex_user_expand(Node *call, ExMac *m, ExToks *out) {
    int r;
    ex_mac_compile(m);
    ex_best_fail = -1;
    for (r = 0; r < m->nrules; r++) {
        ExRule *er = &m->rules[r];
        ExCtx c;
        ExB **frame;
        long pos = 0;
        c.in = call->toks;
        c.nin = call->ntoks;
        c.work = ex_copy_eof(call, call->toks, call->ntoks);
        c.nvars = er->nvars;
        c.file = call->file;
        frame = xalloc(sizeof(ExB *) * (size_t)(er->nvars + 1));
        if (ex_match_seq(&c, er->m, er->nm, &pos, frame)) {
            if (pos == c.nin) {
                ExTr tr;
                tr.rule = er;
                tr.call = call;
                tr.hn = 0;
                tr.hr = 0;
                tr.nh = 0;
                ex_set_site(call);
                ex_hyg_build(&tr, er->tr, er->ntr);
                ex_transcribe(&tr, er->tr, er->ntr, frame, out);
                return;
            }
            ex_fail(pos);
        }
    }
    if (ex_best_fail < 0) ex_best_fail = 0;
    if (ex_best_fail >= call->ntoks) {
        ex_err(call, "unexpected end of macro invocation");
    } else {
        StrBuf b;
        Name *nm;
        sb_init(&b);
        ex_tok_src(&b, &call->toks[ex_best_fail]);
        nm = ex_name_from_bytes(b.s, b.len);
        ex_err(call, "no rules expected the token `%s`", nm->s);
    }
}

/* ================================================================ scopes of macro definitions */

static ExMac **ex_scope;
static int ex_nscope, ex_cscope;
static ExMac **ex_exp;
static int ex_nexp, ex_cexp;

static int ex_has_attr(Node *n, const char *name) {
    long i;
    for (i = 0; i < n->attrs.n; i++)
        if (n->attrs.v[i]->name && strcmp(n->attrs.v[i]->name->s, name) == 0) return 1;
    return 0;
}

static ExMac *ex_exp_find(Node *def) {
    int i;
    for (i = 0; i < ex_nexp; i++)
        if (ex_exp[i]->def == def) return ex_exp[i];
    return 0;
}

static void ex_push_mac(ExMac ***v, int *n, int *cap, ExMac *m) {
    if (*n == *cap) {
        *cap = *cap ? *cap * 2 : 16;
        *v = xrealloc(*v, sizeof(ExMac *) * (size_t)*cap);
    }
    (*v)[(*n)++] = m;
}

static void ex_register(Node *def) {
    ExMac *m = ex_exp_find(def);
    if (!m) {
        m = xalloc(sizeof(ExMac));
        m->name = def->name;
        m->def = def;
        if (ex_has_attr(def, "macro_export")) ex_push_mac(&ex_exp, &ex_nexp, &ex_cexp, m);
    }
    ex_push_mac(&ex_scope, &ex_nscope, &ex_cscope, m);
}

static void ex_collect_exports(NodeList *l) {
    long i;
    for (i = 0; i < l->n; i++) {
        Node *n = l->v[i];
        if (n->kind == N_ITEM_MACRO_RULES && ex_has_attr(n, "macro_export")) {
            ExMac *m = xalloc(sizeof(ExMac));
            m->name = n->name;
            m->def = n;
            ex_push_mac(&ex_exp, &ex_nexp, &ex_cexp, m);
        } else if (n->kind == N_ITEM_MOD) ex_collect_exports(&n->list);
    }
}

/* a user macro: textual scope (innermost first), then #[macro_export]ed ones */
static ExMac *ex_lookup(Name *nm, int textual) {
    int i;
    if (textual)
        for (i = ex_nscope - 1; i >= 0; i--)
            if (ex_scope[i]->name == nm) return ex_scope[i];
    for (i = ex_nexp - 1; i >= 0; i--)
        if (ex_exp[i]->name == nm) return ex_exp[i];
    return 0;
}

/* which macro does this call name: a user macro (returned), a built-in (*builtin = 1), or neither */
static ExMac *ex_resolve(Node *call, int *builtin) {
    Node *path = call->a;
    long nseg = path->list.n;
    Name *nm = path->list.v[nseg - 1]->name;
    ExMac *m = 0;
    *builtin = 0;
    if (nseg == 1 && !(path->flags & F_GLOBAL)) {
        m = ex_lookup(nm, 1);
        if (m) return m;
        if (ex_is_builtin(nm->s)) *builtin = 1;
        return 0;
    }
    {
        Name *first = path->list.v[0]->name;
        const char *f = first->s;
        if (strcmp(f, "std") == 0 || strcmp(f, "core") == 0 || strcmp(f, "alloc") == 0) {
            if (ex_is_builtin(nm->s)) *builtin = 1;
            return 0;
        }
        m = ex_lookup(nm, 0);
        if (m) return m;
        if (ex_is_builtin(nm->s)) *builtin = 1;
    }
    return 0;
}

/* ================================================================ #[derive] */

/*
 * #[derive(Trait, ...)] on a struct or enum: for each trait the text of an
 * impl is generated (absolute ::core:: paths, bindings named __aN / __bN), lexed
 * and re-parsed with reparse_items, and the generics of the type are patched
 * into the impl: a deep copy of the type's generics with the trait added as a
 * bound on every type parameter (defaults removed), and a copy of its where
 * clause.  The impls are placed right after the type.
 */

enum { DV_DEBUG, DV_CLONE, DV_COPY, DV_PEQ, DV_EQ, DV_PORD, DV_ORD, DV_DEFAULT };

static Node *ex_clone(Node *n) {
    Node *c;
    long i;
    if (!n) return 0;
    c = ast_alloc(sizeof(Node));
    *c = *n;
    c->ty = 0;
    c->sym = 0;
    c->adj = 0;
    c->a = ex_clone(n->a);
    c->b = ex_clone(n->b);
    c->c = ex_clone(n->c);
    c->d = ex_clone(n->d);
    c->vis = ex_clone(n->vis);
    memset(&c->list, 0, sizeof c->list);
    memset(&c->attrs, 0, sizeof c->attrs);
    for (i = 0; i < n->list.n; i++) list_push(&c->list, ex_clone(n->list.v[i]));
    for (i = 0; i < n->attrs.n; i++) list_push(&c->attrs, ex_clone(n->attrs.v[i]));
    return c;
}

static void dv_num(StrBuf *b, long n) {
    char t[24];
    int k = 0;
    if (n == 0) {
        sb_putc(b, '0');
        return;
    }
    while (n > 0) {
        t[k++] = (char)('0' + n % 10);
        n /= 10;
    }
    while (k > 0) sb_putc(b, t[--k]);
}

static void dv_name(StrBuf *b, Name *nm) {
    ex_put_native(b, nm->s);
}

static int dv_is_enum(Node *item) {
    return item->kind == N_ITEM_ENUM;
}

static long dv_nvar(Node *item) {
    return dv_is_enum(item) ? item->list.n : 1;
}

/* variant i of an enum, or the struct itself (its flags and fields have the same shape) */
static Node *dv_var(Node *item, long i) {
    return dv_is_enum(item) ? item->list.v[i] : item;
}

/* Name or Name::Variant */
static void dv_path(StrBuf *b, Node *item, long i) {
    dv_name(b, item->name);
    if (dv_is_enum(item)) {
        sb_puts(b, "::");
        dv_name(b, item->list.v[i]->name);
    }
}

/* the pattern of variant i: bindings `ref __<pfx>K`, or with wild every field ignored */
static void dv_pat(StrBuf *b, Node *item, long i, int pfx, int wild) {
    Node *v = dv_var(item, i);
    long k;
    dv_path(b, item, i);
    if (v->flags & F_NAMED) {
        sb_puts(b, " { ");
        if (wild) sb_puts(b, ".. ");
        else
            for (k = 0; k < v->list.n; k++) {
                dv_name(b, v->list.v[k]->name);
                sb_puts(b, ": ref __");
                sb_putc(b, pfx);
                dv_num(b, k);
                sb_puts(b, ", ");
            }
        sb_putc(b, '}');
    } else if (v->flags & F_TUPLE) {
        sb_putc(b, '(');
        if (wild) sb_puts(b, "..");
        else
            for (k = 0; k < v->list.n; k++) {
                sb_puts(b, "ref __");
                sb_putc(b, pfx);
                dv_num(b, k);
                sb_puts(b, ", ");
            }
        sb_putc(b, ')');
    }
}

/* the value of field k in a constructor: mode 0 = a clone of __aK, 1 = Default::default() */
static void dv_field_expr(StrBuf *b, int mode, long k) {
    if (mode == 0) {
        sb_puts(b, "::core::clone::Clone::clone(__a");
        dv_num(b, k);
        sb_putc(b, ')');
    } else sb_puts(b, "::core::default::Default::default()");
}

static void dv_cons(StrBuf *b, Node *item, long i, int mode) {
    Node *v = dv_var(item, i);
    long k;
    dv_path(b, item, i);
    if (v->flags & F_NAMED) {
        sb_puts(b, " { ");
        for (k = 0; k < v->list.n; k++) {
            dv_name(b, v->list.v[k]->name);
            sb_puts(b, ": ");
            dv_field_expr(b, mode, k);
            sb_puts(b, ", ");
        }
        sb_putc(b, '}');
    } else if (v->flags & F_TUPLE) {
        sb_putc(b, '(');
        for (k = 0; k < v->list.n; k++) {
            dv_field_expr(b, mode, k);
            sb_puts(b, ", ");
        }
        sb_putc(b, ')');
    }
}

/* `impl<'a, T: TRAIT, const N: usize> TRAIT for Name<'a, T, N> {` */
static void dv_impl_open(StrBuf *b, Node *item, const char *trait) {
    Node *g = item->a;
    long i;
    sb_puts(b, "impl");
    if (g && g->list.n > 0) {
        sb_putc(b, '<');
        for (i = 0; i < g->list.n; i++) {
            Node *p = g->list.v[i];
            if (i) sb_puts(b, ", ");
            if (p->kind == N_GEN_LIFETIME) dv_name(b, p->name);
            else if (p->kind == N_GEN_TYPE) {
                dv_name(b, p->name);
                sb_puts(b, ": ");
                sb_puts(b, trait);
            } else {
                sb_puts(b, "const ");
                dv_name(b, p->name);
                sb_puts(b, ": usize");
            }
        }
        sb_putc(b, '>');
    }
    sb_putc(b, ' ');
    sb_puts(b, trait);
    sb_puts(b, " for ");
    dv_name(b, item->name);
    if (g && g->list.n > 0) {
        sb_putc(b, '<');
        for (i = 0; i < g->list.n; i++) {
            if (i) sb_puts(b, ", ");
            dv_name(b, g->list.v[i]->name);
        }
        sb_putc(b, '>');
    }
    sb_puts(b, " {\n");
}

static void dv_debug(StrBuf *b, Node *item) {
    long i, k;
    dv_impl_open(b, item, "::core::fmt::Debug");
    sb_puts(b, "fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {\nmatch *self {\n");
    for (i = 0; i < dv_nvar(item); i++) {
        Node *v = dv_var(item, i);
        dv_pat(b, item, i, 'a', 0);
        sb_puts(b, " => ");
        if (v->list.n == 0) {
            sb_puts(b, "f.write_str(\"");
            dv_name(b, v->name);
            sb_puts(b, "\")");
        } else {
            sb_puts(b, (v->flags & F_NAMED) ? "f.debug_struct(\"" : "f.debug_tuple(\"");
            dv_name(b, v->name);
            sb_puts(b, "\")");
            for (k = 0; k < v->list.n; k++) {
                sb_puts(b, ".field(");
                if (v->flags & F_NAMED) {
                    sb_putc(b, '"');
                    dv_name(b, v->list.v[k]->name);
                    sb_puts(b, "\", ");
                }
                sb_puts(b, "__a");
                dv_num(b, k);
                sb_putc(b, ')');
            }
            sb_puts(b, ".finish()");
        }
        sb_puts(b, ",\n");
    }
    sb_puts(b, "}\n}\n}\n");
}

static void dv_clone(StrBuf *b, Node *item) {
    long i;
    dv_impl_open(b, item, "::core::clone::Clone");
    sb_puts(b, "fn clone(&self) -> Self {\nmatch *self {\n");
    for (i = 0; i < dv_nvar(item); i++) {
        dv_pat(b, item, i, 'a', 0);
        sb_puts(b, " => ");
        dv_cons(b, item, i, 0);
        sb_puts(b, ",\n");
    }
    sb_puts(b, "}\n}\n}\n");
}

static void dv_default(StrBuf *b, Node *item, Node *attr) {
    long i;
    dv_impl_open(b, item, "::core::default::Default");
    sb_puts(b, "fn default() -> Self {\n");
    if (dv_is_enum(item)) {
        Node *found = 0;
        for (i = 0; i < item->list.n; i++)
            if (ex_has_attr(item->list.v[i], "default")) {
                found = item->list.v[i];
                break;
            }
        if (!found) error_at(item->file, attr->line, "`#[derive(Default)]` on enum with no `#[default]`");
        if (found->list.n > 0 || (found->flags & (F_NAMED | F_TUPLE)))
            error_at(item->file, attr->line, "the `#[default]` attribute may only be used on unit enum variants");
        dv_path(b, item, i);
    } else dv_cons(b, item, 0, 1);
    sb_puts(b, "\n}\n}\n");
}

/* which: 0 PartialEq, 1 PartialOrd, 2 Ord.  The call comparing field k */
static void dv_call(StrBuf *b, int which, long k) {
    sb_puts(b, which == 0 ? "::core::cmp::PartialEq::eq(" : which == 1 ? "::core::cmp::PartialOrd::partial_cmp(" : "::core::cmp::Ord::cmp(");
    sb_puts(b, "__a");
    dv_num(b, k);
    sb_puts(b, ", __b");
    dv_num(b, k);
    sb_putc(b, ')');
}

static void dv_equal(StrBuf *b, int which) {
    sb_puts(b, which == 1 ? "::core::option::Option::Some(::core::cmp::Ordering::Equal)" : "::core::cmp::Ordering::Equal");
}

/* lexicographic comparison of fields k..nf */
static void dv_chain(StrBuf *b, int which, long nf, long k) {
    long j;
    if (which == 0) {
        if (nf == 0) sb_puts(b, "true");
        for (j = 0; j < nf; j++) {
            if (j) sb_puts(b, " && ");
            dv_call(b, 0, j);
        }
        return;
    }
    if (k == nf) {
        dv_equal(b, which);
    } else if (k == nf - 1) {
        dv_call(b, which, k);
    } else {
        sb_puts(b, "match ");
        dv_call(b, which, k);
        sb_puts(b, " {\n");
        dv_equal(b, which);
        sb_puts(b, " => ");
        dv_chain(b, which, nf, k + 1);
        sb_puts(b, ",\n__c => __c,\n}");
    }
}

static void dv_cmp(StrBuf *b, Node *item, int which) {
    long i, n = dv_nvar(item);
    int multi = dv_is_enum(item) && n > 1;
    if (which == 0) {
        dv_impl_open(b, item, "::core::cmp::PartialEq");
        sb_puts(b, "fn eq(&self, other: &Self) -> bool {\n");
    } else if (which == 1) {
        dv_impl_open(b, item, "::core::cmp::PartialOrd");
        sb_puts(b, "fn partial_cmp(&self, other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {\n");
    } else {
        dv_impl_open(b, item, "::core::cmp::Ord");
        sb_puts(b, "fn cmp(&self, other: &Self) -> ::core::cmp::Ordering {\n");
    }
    if (which != 0 && multi) {
        /* variants are ordered by declaration order */
        int w;
        for (w = 0; w < 2; w++) {
            sb_puts(b, w == 0 ? "let __si: isize = match *self {\n" : "let __oi: isize = match *other {\n");
            for (i = 0; i < n; i++) {
                dv_pat(b, item, i, 'a', 1);
                sb_puts(b, " => ");
                dv_num(b, i);
                sb_puts(b, "isize,\n");
            }
            sb_puts(b, "};\n");
        }
        sb_puts(b, "if __si != __oi {\n");
        sb_puts(b, which == 1 ? "::core::cmp::PartialOrd::partial_cmp(&__si, &__oi)" : "::core::cmp::Ord::cmp(&__si, &__oi)");
        sb_puts(b, "\n} else {\n");
    }
    sb_puts(b, "match *self {\n");
    for (i = 0; i < n; i++) {
        dv_pat(b, item, i, 'a', 0);
        sb_puts(b, " => match *other {\n");
        dv_pat(b, item, i, 'b', 0);
        sb_puts(b, " => ");
        dv_chain(b, which, dv_var(item, i)->list.n, 0);
        sb_puts(b, ",\n");
        if (multi) {
            sb_puts(b, "_ => ");
            if (which == 0) sb_puts(b, "false");
            else dv_equal(b, which);
            sb_puts(b, ",\n");
        }
        sb_puts(b, "},\n");
    }
    sb_puts(b, "}\n");
    if (which != 0 && multi) sb_puts(b, "}\n");
    sb_puts(b, "}\n}\n");
}

/* gives the freshly parsed impl the type's generics (with the derive bound) and where clause */
static void dv_patch(Node *impl, Node *item) {
    Node *og = item->a, *gen = impl->a, *ng;
    long i, j;
    if (og && gen && og->list.n == gen->list.n) {
        ng = ex_clone(og);
        for (i = 0; i < ng->list.n; i++) {
            Node *p = ng->list.v[i], *gp = gen->list.v[i];
            if (p->kind == N_GEN_TYPE) {
                p->a = 0;
                if (gp->b) {
                    if (!p->b) p->b = gp->b;
                    else
                        for (j = 0; j < gp->b->list.n; j++) list_push(&p->b->list, gp->b->list.v[j]);
                }
            } else if (p->kind == N_GEN_CONST) p->b = 0;
        }
        impl->a = ng;
    }
    impl->d = ex_clone(item->d);
}

static int dv_lookup(const char *s) {
    if (strcmp(s, "Debug") == 0) return DV_DEBUG;
    if (strcmp(s, "Clone") == 0) return DV_CLONE;
    if (strcmp(s, "Copy") == 0) return DV_COPY;
    if (strcmp(s, "PartialEq") == 0) return DV_PEQ;
    if (strcmp(s, "Eq") == 0) return DV_EQ;
    if (strcmp(s, "PartialOrd") == 0) return DV_PORD;
    if (strcmp(s, "Ord") == 0) return DV_ORD;
    if (strcmp(s, "Default") == 0) return DV_DEFAULT;
    return -1;
}

static void dv_one(Node *item, Node *attr, Name *nm, NodeList *out) {
    StrBuf b;
    ExToks t;
    NodeList sub;
    int which = dv_lookup(nm->s);
    long i;
    if (strcmp(nm->s, "Hash") == 0) error_at(item->file, attr->line, "derive(Hash) is not supported");
    if (which < 0) {
        StrBuf m;
        sb_init(&m);
        ex_put_native(&m, nm->s);
        error_at(item->file, attr->line, "cannot find derive macro `%s` in this scope", m.s);
    }
    sb_init(&b);
    switch (which) {
    case DV_DEBUG: dv_debug(&b, item); break;
    case DV_CLONE: dv_clone(&b, item); break;
    case DV_COPY: dv_impl_open(&b, item, "::core::marker::Copy"); sb_puts(&b, "}\n"); break;
    case DV_EQ: dv_impl_open(&b, item, "::core::cmp::Eq"); sb_puts(&b, "}\n"); break;
    case DV_PEQ: dv_cmp(&b, item, 0); break;
    case DV_PORD: dv_cmp(&b, item, 1); break;
    case DV_ORD: dv_cmp(&b, item, 2); break;
    default: dv_default(&b, item, attr); break;
    }
    memset(&t, 0, sizeof t);
    memset(&sub, 0, sizeof sub);
    ex_set_site(item);
    ex_snip(&t, b.s);
    ex_prep(item);
    reparse_items(t.v, t.n, &sub);
    for (i = 0; i < sub.n; i++) {
        if (sub.v[i]->kind == N_ITEM_IMPL) dv_patch(sub.v[i], item);
        list_push(out, sub.v[i]);
    }
}

/* appends the impls derived by the attributes of item to out */
static void ex_derive_item(Node *item, NodeList *out) {
    long i, j;
    int k = item->kind;
    if (item->attrs.n == 0) return;
    for (i = 0; i < item->attrs.n; i++) {
        Node *attr = item->attrs.v[i];
        Name *last = 0;
        if (!attr->name || strcmp(attr->name->s, "derive") != 0) continue;
        if (k == N_ITEM_UNION) error_at(item->file, attr->line, "this trait cannot be derived for unions");
        if (k != N_ITEM_STRUCT && k != N_ITEM_ENUM) {
            if (k == N_ITEM_FN || k == N_ITEM_TRAIT || k == N_ITEM_IMPL || k == N_ITEM_MOD || k == N_ITEM_CONST ||
                k == N_ITEM_STATIC || k == N_ITEM_TYPE)
                error_at(item->file, attr->line, "`derive` may only be applied to `struct`s, `enum`s and `union`s");
            continue;
        }
        for (j = 0; j <= attr->ntoks; j++) {
            if (j == attr->ntoks || attr->toks[j].kind == TK_COMMA) {
                if (last) dv_one(item, attr, last, out);
                last = 0;
            } else if (attr->toks[j].kind == TK_IDENT) last = attr->toks[j].text;
        }
    }
}

/* ================================================================ the walk */

enum { EX_EXPR, EX_TYPE, EX_PAT, EX_STMTS, EX_ITEMS };

static void ex_node(Node **slot, int role, int depth);
static void ex_walk(Node *n, int depth);
static void ex_stmts(NodeList *l, int depth);
static void ex_items(NodeList *l, int depth);

static void ex_depth_check(Node *call, int depth) {
    if (depth >= EX_DEPTH_LIMIT) ex_err(call, "recursion limit reached while expanding `%s!`", ex_call_name(call));
}

/* the tokens of the expansion of `call` (user or built-in); returns 1 if user macro */
static int ex_expansion_tokens(Node *call, int ctx, ExToks *t) {
    int builtin, user = 0;
    ExMac *m = ex_resolve(call, &builtin);
    if (m) {
        ex_user_expand(call, m, t);
        user = 1;
    } else if (builtin) {
        if (ctx != EX_EXPR && ctx != EX_STMTS)
            ex_err(call, "macro `%s!` cannot be used here", ex_call_name(call));
        ex_builtin(call, ex_call_name(call), t);
    } else ex_err(call, "cannot find macro `%s` in this scope", ex_call_name(call));
#ifdef EX_TRACE
    {
        StrBuf b;
        sb_init(&b);
        ex_pretty(&b, t->v, t->n);
        fprintf(stderr, "[expand %s! line %d] %s\n", ex_call_name(call), call->line, b.s);
        free(b.s);
    }
#endif
    return user;
}

static Node *ex_expand_expr(Node *call, int depth) {
    ExToks t;
    Node *e;
    int user;
    ex_depth_check(call, depth);
    memset(&t, 0, sizeof t);
    user = ex_expansion_tokens(call, EX_EXPR, &t);
    if (user && t.n > 0 && t.v[t.n - 1].kind == TK_SEMI) t.n--;
    ex_prep(call);
    e = reparse_expr(t.v, t.n);
    ex_node(&e, EX_EXPR, depth + 1);
    return e;
}

static Node *ex_expand_type(Node *call, int depth) {
    ExToks t;
    Node *e;
    ex_depth_check(call, depth);
    memset(&t, 0, sizeof t);
    ex_expansion_tokens(call, EX_TYPE, &t);
    ex_prep(call);
    e = reparse_type(t.v, t.n);
    ex_node(&e, EX_TYPE, depth + 1);
    return e;
}

static Node *ex_expand_pat(Node *call, int depth) {
    ExToks t;
    Node *e;
    ex_depth_check(call, depth);
    memset(&t, 0, sizeof t);
    ex_expansion_tokens(call, EX_PAT, &t);
    ex_prep(call);
    e = ex_reparse_pat(call, t.v, t.n);
    ex_node(&e, EX_PAT, depth + 1);
    return e;
}

/* a statement macro: appends the (fully expanded) statements to out */
static void ex_expand_stmts(Node *call, NodeList *out, int depth) {
    ExToks t;
    NodeList sub;
    int user;
    long i;
    ex_depth_check(call, depth);
    memset(&t, 0, sizeof t);
    memset(&sub, 0, sizeof sub);
    user = ex_expansion_tokens(call, EX_STMTS, &t);
    ex_prep(call);
    if (user) {
        reparse_stmts(t.v, t.n, &sub);
        if (sub.n > 0 && (call->flags & F_SEMI)) {
            Node *last = sub.v[sub.n - 1];
            if (last->kind == N_STMT_EXPR) last->flags |= F_SEMI;
            else if (last->kind == N_MACRO_CALL) last->flags |= F_SEMI;
        }
    } else {
        Node *e = reparse_expr(t.v, t.n);
        Node *s = mk(N_STMT_EXPR, call->line, call->col);
        s->a = e;
        if (call->flags & F_SEMI) s->flags |= F_SEMI;
        list_push(&sub, s);
    }
    ex_stmts(&sub, depth + 1);
    for (i = 0; i < sub.n; i++) list_push(out, sub.v[i]);
}

static void ex_expand_items(Node *call, NodeList *out, int depth) {
    ExToks t;
    NodeList sub;
    long i;
    ex_depth_check(call, depth);
    memset(&t, 0, sizeof t);
    memset(&sub, 0, sizeof sub);
    ex_expansion_tokens(call, EX_ITEMS, &t);
    ex_prep(call);
    reparse_items(t.v, t.n, &sub);
    ex_items(&sub, depth + 1);
    for (i = 0; i < sub.n; i++) list_push(out, sub.v[i]);
}

/* the role of the children of a node of kind k: slot 0..3 = a..d, 4 = list */
static int ex_role(int k, int slot) {
    switch (k) {
    case N_ITEM_FN:
        return slot == 1 ? EX_TYPE : EX_EXPR;
    case N_PARAM:
        return slot == 0 ? EX_PAT : (slot == 1 ? EX_TYPE : EX_EXPR);
    case N_SELF_PARAM:
        return slot == 1 ? EX_TYPE : EX_EXPR;
    case N_FIELD: case N_ITEM_CONST: case N_ITEM_STATIC: case N_GEN_TYPE: case N_GEN_CONST: case N_WHERE_TYPE:
    case N_ASSOC_BIND: case N_T_REF: case N_T_PTR: case N_T_SLICE: case N_T_ARRAY:
        return slot == 0 ? EX_TYPE : EX_EXPR;
    case N_ITEM_TYPE: case N_ITEM_IMPL: case N_E_CAST: case N_T_FN:
        return slot == 1 ? EX_TYPE : EX_EXPR;
    case N_T_TUPLE:
        return slot == 4 ? EX_TYPE : EX_EXPR;
    case N_PATH:
        return slot == 0 ? EX_TYPE : EX_EXPR;
    case N_PATH_SEG:
        return (slot == 0 || slot == 4) ? EX_TYPE : EX_EXPR;
    case N_E_CLOSURE:
        return slot == 0 ? EX_TYPE : EX_EXPR;
    case N_STMT_LET:
        return slot == 0 ? EX_PAT : (slot == 1 ? EX_TYPE : EX_EXPR);
    case N_E_LET: case N_ARM: case N_E_FOR: case N_P_IDENT: case N_P_REF: case N_P_FIELD:
        return slot == 0 ? EX_PAT : EX_EXPR;
    case N_P_TUPLE: case N_P_SLICE: case N_P_TUPLE_STRUCT: case N_P_OR:
        return slot == 4 ? EX_PAT : EX_EXPR;
    case N_P_RANGE:
        return slot <= 1 ? EX_PAT : EX_EXPR;
    default:
        return EX_EXPR;
    }
}

static void ex_node(Node **slot, int role, int depth) {
    Node *n = *slot;
    if (!n) return;
    if (n->kind == N_MACRO_CALL) {
        if (role == EX_TYPE) *slot = ex_expand_type(n, depth);
        else if (role == EX_PAT) *slot = ex_expand_pat(n, depth);
        else *slot = ex_expand_expr(n, depth);
        return;
    }
    ex_walk(n, depth);
}

static void ex_block(Node *n, int depth) {
    int save = ex_nscope;
    ex_stmts(&n->list, depth);
    if (n->a && n->a->kind == N_MACRO_CALL) {
        Node *call = n->a;
        int builtin;
        ExMac *m = ex_resolve(call, &builtin);
        if (m) {
            NodeList sub;
            long i;
            memset(&sub, 0, sizeof sub);
            n->a = 0;
            ex_expand_stmts(call, &sub, depth);
            if (sub.n > 0 && sub.v[sub.n - 1]->kind == N_STMT_EXPR && !(sub.v[sub.n - 1]->flags & F_SEMI)) {
                n->a = sub.v[sub.n - 1]->a;
                sub.n--;
            }
            for (i = 0; i < sub.n; i++) list_push(&n->list, sub.v[i]);
        } else ex_node(&n->a, EX_EXPR, depth);
    } else ex_node(&n->a, EX_EXPR, depth);
    ex_nscope = save;
}

static void ex_walk(Node *n, int depth) {
    int k = n->kind, save, i;
    if (k == N_ITEM_MACRO_RULES || k == N_ATTR) return;
    if (k == N_BLOCK) {
        ex_block(n, depth);
        return;
    }
    ex_node(&n->a, ex_role(k, 0), depth);
    ex_node(&n->b, ex_role(k, 1), depth);
    ex_node(&n->c, ex_role(k, 2), depth);
    ex_node(&n->d, ex_role(k, 3), depth);
    if (k == N_CRATE || k == N_ITEM_MOD || k == N_ITEM_TRAIT || k == N_ITEM_IMPL || k == N_ITEM_EXTERN_BLOCK) {
        save = ex_nscope;
        ex_items(&n->list, depth);
        if (k != N_CRATE && !(k == N_ITEM_MOD && ex_has_attr(n, "macro_use"))) ex_nscope = save;
        return;
    }
    for (i = 0; i < n->list.n; i++) ex_node(&n->list.v[i], ex_role(k, 4), depth);
}

static void ex_stmts(NodeList *l, int depth) {
    NodeList out;
    long i;
    memset(&out, 0, sizeof out);
    for (i = 0; i < l->n; i++) {
        Node *s = l->v[i];
        if (s->kind == N_MACRO_CALL) {
            ex_expand_stmts(s, &out, depth);
        } else if (s->kind == N_ITEM_MACRO_RULES) {
            ex_register(s);
            list_push(&out, s);
        } else {
            ex_node(&s, EX_EXPR, depth);
            list_push(&out, s);
            ex_derive_item(s, &out);
        }
    }
    *l = out;
}

static void ex_items(NodeList *l, int depth) {
    NodeList out;
    long i;
    memset(&out, 0, sizeof out);
    for (i = 0; i < l->n; i++) {
        Node *s = l->v[i];
        if (s->kind == N_MACRO_CALL) {
            ex_expand_items(s, &out, depth);
        } else if (s->kind == N_ITEM_MACRO_RULES) {
            ex_register(s);
            list_push(&out, s);
        } else {
            ex_walk(s, depth);
            list_push(&out, s);
            ex_derive_item(s, &out);
        }
    }
    *l = out;
}

void expand_crate(Node *crate) {
    ex_nscope = 0;
    ex_nexp = 0;
    ex_collect_exports(&crate->list);
    ex_walk(crate, 0);
}
