/*
 * lexer.c - C++14 lexical analysis (ISO C++14 [lex], following
 * CPP14Lexer.g4) producing preprocessing tokens.
 *
 * lex_file() turns a source file into a vector of tokens.  Line splices
 * (backslash-newline) are removed first; comments become white space.  A
 * token records its file, its line, whether it is the first token on a
 * logical line (TF_BOL, which is how the preprocessor finds directives) and
 * whether white space precedes it (TF_SPACE).
 *
 * Identifiers are interned (Name); keywords carry their KW_xxx number and
 * the alternative tokens (and, or, not, ...) become punctuators.  Digraphs
 * become the punctuators they stand for.  String and character literals keep
 * their whole spelling (prefix, quotes, raw-string delimiter); the sema
 * decodes them.  A lone quote that does not start a literal (an apostrophe in
 * a skipped #if group, say) becomes a K_OTHER token, which is an error only
 * if it reaches the parser.
 *
 * Trigraphs are not supported.
 */

static const char *kw_spell[] = {
    "",
#define KW(x) #x,
#include "keywords.inc"
#undef KW
    ""
};

static const char *punct_spell[] = {
    "",
#define P(id, s) s,
#include "puncts.inc"
#undef P
    ""
};

const char *punct_spelling(int id) {
    return punct_spell[id];
}

const char *kw_spelling(int id) {
    return kw_spell[id];
}

/* the alternative tokens: and, and_eq, bitand, bitor, compl, not, not_eq,
   or, or_eq, xor, xor_eq */
static const char *alt_names[] = {
    "and", "and_eq", "bitand", "bitor", "compl", "not", "not_eq",
    "or", "or_eq", "xor", "xor_eq", 0
};
static const int alt_ids[] = {
    P_ANDAND, P_ANDEQ, P_AMP, P_PIPE, P_TILDE, P_NOT, P_NE,
    P_OROR, P_OREQ, P_CARET, P_XOREQ
};

/* digraphs: <: :> <% %> %: %:%: */
static const char *digraph_names[] = { "<:", ":>", "<%", "%>", "%:%:", "%:", 0 };
static const int digraph_ids[] = { P_LBRACK, P_RBRACK, P_LBRACE, P_RBRACE, P_HASHHASH, P_HASH };

/* punctuators by first character: indexes into the punctuator table, longest
   spelling first */
#define PUNCT_FIRST 128
static int *punct_by_char[PUNCT_FIRST];

/* character classes of the ASCII characters; codepoints >= 128 are letters */
static char cls_tab[128];
#define CL_ID 1
#define CL_DIG 2
#define is_digit(c) ((c) >= '0' && (c) <= '9')
#define is_idstart(c) ((c) >= 128 || (cls_tab[c] & CL_ID))
#define is_idchar(c) ((c) >= 128 || (cls_tab[c] & (CL_ID | CL_DIG)))

static int lexer_ready = 0;

void lex_init(void) {
    int i, c, k;
    if (lexer_ready) return;
    lexer_ready = 1;
    for (c = 'a'; c <= 'z'; c++) cls_tab[c] = cls_tab[c - 32] = CL_ID;
    cls_tab['_'] = cls_tab['$'] = CL_ID;
    for (c = '0'; c <= '9'; c++) cls_tab[c] = CL_DIG;
    for (i = 1; i < KW_LAST; i++) intern_str(kw_spell[i])->kw = i;
    for (i = 0; alt_names[i]; i++) intern_str(alt_names[i])->alt = alt_ids[i];
    for (c = 0; c < PUNCT_FIRST; c++) {
        int n = 0, len;
        int *list;
        for (i = 1; i < P_LAST; i++)
            if (punct_spell[i][0] == c) n++;
        if (n == 0) continue;
        list = xalloc(sizeof(int) * (n + 1));
        k = 0;
        for (len = 3; len >= 1; len--)
            for (i = 1; i < P_LAST; i++)
                if (punct_spell[i][0] == c && (int)strlen(punct_spell[i]) == len) list[k++] = i;
        list[k] = 0;
        punct_by_char[c] = list;
    }
}

const char *tok_kind_name(int kind) {
    switch (kind) {
    case K_EOF: return "eof";
    case K_IDENT: return "ident";
    case K_NUM: return "number";
    case K_CHAR: return "char";
    case K_STR: return "string";
    case K_PUNCT: return "punct";
    case K_OTHER: return "other";
    }
    return "placemarker";
}

/* ------------------------------------------------------------ the lexer state */

typedef struct {
    int *s;              /* codepoints, line splices removed */
    int n;
    int i;               /* index of the next character */
    int file;
    int line;
    int *splices;        /* indexes in s where a line splice was removed */
    int nsplices, nextsp;
    int bol, space;
} Lexer;

/* The character functions are macros: calls are expensive in the code that
   the C11 compiler generates.  L->s has zero padding after L->n, so looking
   a few characters ahead is always safe. */
#define cur(L)      ((L)->s[(L)->i])
#define peekc(L, k) ((L)->s[(L)->i + (k)])

/* counts the line splices that lie behind the position L->i */
static void sync_splices(Lexer *L) {
    while (L->nextsp < L->nsplices && L->splices[L->nextsp] <= L->i) {
        L->line++;
        L->nextsp++;
    }
}

/* consumes one character, counting lines (newlines and removed splices) */
#define adv(L) do { \
        if ((L)->i < (L)->n) { \
            if ((L)->s[(L)->i] == '\n') (L)->line++; \
            (L)->i++; \
            if ((L)->nextsp < (L)->nsplices) sync_splices(L); \
        } \
    } while (0)


/* the native string for s[a .. b-1], in the arena */
static char *spell_range(Lexer *L, int a, int b) {
    char *r, *p;
    int k, n = 0;
#ifdef __CPUSIM64__
    r = anew(b - a + 1);
    for (k = a; k < b; k++) r[k - a] = (char)L->s[k];
    return r;
#else
    char tmp[8];
    for (k = a; k < b; k++) n += cp_encode(tmp, L->s[k]);
    r = anew(n + 1);
    p = r;
    for (k = a; k < b; k++) p += cp_encode(p, L->s[k]);
    return r;
#endif
}

/* the same, in a scratch buffer that is reused (for names that are interned) */
static char *scratch = 0;
static int scratch_cap = 0;

static char *spell_scratch(Lexer *L, int a, int b) {
    int need = (b - a) * 4 + 1, k;
    char *p;
    if (need > scratch_cap) {
        scratch_cap = need * 2;
        scratch = xrealloc(scratch, scratch_cap);
    }
    p = scratch;
#ifdef __CPUSIM64__
    for (k = a; k < b; k++) *p++ = (char)L->s[k];
#else
    for (k = a; k < b; k++) p += cp_encode(p, L->s[k]);
#endif
    *p = 0;
    return scratch;
}

static Token *new_tok(Lexer *L, int kind, int line, const char *text) {
    Token *t = tok_new(kind, text, L->file, line);
    t->flags = (L->bol ? TF_BOL : 0) | (L->space ? TF_SPACE : 0);
    L->bol = 0;
    L->space = 0;
    return t;
}

/* a literal whose spelling is s[a .. L->i-1] */
static Token *literal_tok(Lexer *L, int kind, int line, int a) {
    return new_tok(L, kind, line, spell_range(L, a, L->i));
}

/* converts a native string to codepoints */
int *native_to_cps(const char *s, int *n) {
    int len = (int)strlen(s);
    int *t = xalloc(sizeof(int) * (len + 1));
    int k = 0, i = 0;
#ifdef __CPUSIM64__
    for (i = 0; i < len; i++) t[k++] = (unsigned char)s[i] & 0x1FFFFF;
#else
    while (i < len) {
        int c = (unsigned char)s[i], need = 0, cp = 0, j;
        if (c < 0x80) { t[k++] = c; i++; continue; }
        if ((c & 0xE0) == 0xC0) { need = 1; cp = c & 0x1F; }
        else if ((c & 0xF0) == 0xE0) { need = 2; cp = c & 0x0F; }
        else if ((c & 0xF8) == 0xF0) { need = 3; cp = c & 0x07; }
        else { t[k++] = 0xFFFD; i++; continue; }
        for (j = 1; j <= need; j++) {
            if (i + j >= len || ((unsigned char)s[i + j] & 0xC0) != 0x80) break;
            cp = (cp << 6) | ((unsigned char)s[i + j] & 0x3F);
        }
        if (j <= need) { t[k++] = 0xFFFD; i++; continue; }
        t[k++] = cp;
        i += need + 1;
    }
#endif
    t[k] = 0;
    *n = k;
    return t;
}

/* ------------------------------------------------------------ literals */

/* lexes a quoted literal whose opening quote q is at L->i; prefix characters
   (already consumed) start at a.  Returns the token, or a K_OTHER token for
   the opening quote if the literal is not terminated on its line. */
static Token *lex_quoted(Lexer *L, int q, int a) {
    int line = L->line;
    int save_i = L->i, save_line = L->line, save_sp = L->nextsp;
    int kind = q == '"' ? K_STR : K_CHAR;
    adv(L);
    for (;;) {
        int c = cur(L);
        if (L->i >= L->n || c == '\n') break;
        if (c == '\\' && L->i + 1 < L->n && peekc(L, 1) != '\n') {
            adv(L);
            adv(L);
            continue;
        }
        adv(L);
        if (c == q) return literal_tok(L, kind, line, a);
    }
    /* unterminated: only the quote is a (stray) token */
    L->i = save_i;
    L->line = save_line;
    L->nextsp = save_sp;
    adv(L);
    return literal_tok(L, K_OTHER, line, save_i);
}

/* lexes a raw string literal R"delim( ... )delim"; L->i is at the opening quote */
static Token *lex_raw_string(Lexer *L, int a) {
    int line = L->line;
    int save_i = L->i, save_line = L->line, save_sp = L->nextsp;
    int dstart, dlen, k, ok;
    adv(L);                                  /* " */
    dstart = L->i;
    while (L->i < L->n && cur(L) != '(' && cur(L) != '"' && cur(L) != '\n' && cur(L) != ' '
           && cur(L) != ')' && cur(L) != '\\' && L->i - dstart <= 16)
        adv(L);
    if (cur(L) != '(') {
        L->i = save_i; L->line = save_line; L->nextsp = save_sp;
        adv(L);
        return literal_tok(L, K_OTHER, line, save_i);
    }
    dlen = L->i - dstart;
    adv(L);                                  /* ( */
    while (L->i < L->n) {
        if (cur(L) == ')') {
            ok = 1;
            for (k = 0; k < dlen; k++)
                if (peekc(L, 1 + k) != L->s[dstart + k]) { ok = 0; break; }
            if (ok && peekc(L, 1 + dlen) == '"') {
                for (k = 0; k < dlen + 2; k++) adv(L);
                return literal_tok(L, K_STR, line, a);
            }
        }
        adv(L);
    }
    L->i = save_i; L->line = save_line; L->nextsp = save_sp;
    adv(L);
    return literal_tok(L, K_OTHER, line, save_i);
}

/* ------------------------------------------------------------ the main loop */

/* lexes n codepoints (the splices are removed here) and appends the tokens */
void lex_text(int *text, int n, int file, int line, TokVec *out) {
    Lexer L;
    int i, k, c, d;
    lex_init();
    L.s = xalloc(sizeof(int) * (n + 8));
    L.splices = xalloc(sizeof(int) * (n / 2 + 2));
    L.nsplices = 0;
    k = 0;
    for (i = 0; i < n; i++) {
        c = text[i];
        if (c == '\\') {
            /* backslash, optional blanks?  Only an immediate newline splices
               (a CR LF pair counts as one newline). */
            if (i + 1 < n && text[i + 1] == '\n') {
                L.splices[L.nsplices++] = k;
                i++;
                continue;
            }
            if (i + 2 < n && text[i + 1] == '\r' && text[i + 2] == '\n') {
                L.splices[L.nsplices++] = k;
                i += 2;
                continue;
            }
        }
        if (c == '\r') {
            /* CR LF and a lone CR are newlines */
            if (i + 1 < n && text[i + 1] == '\n') continue;
            c = '\n';
        }
        L.s[k++] = c;
    }
    L.s[k] = 0;
    L.n = k;
    L.i = 0;
    L.file = file;
    L.line = line;
    L.nextsp = 0;
    L.bol = 1;
    L.space = 0;
    while (L.nextsp < L.nsplices && L.splices[L.nextsp] <= 0) {
        L.line++;
        L.nextsp++;
    }
    for (;;) {
        int start, tline;
        if (L.i >= L.n) break;
        c = cur(&L);
        if (c == '\n') {
            adv(&L);
            L.bol = 1;
            L.space = 1;
            continue;
        }
        if (c == ' ' || c == '\t' || c == 0x0B || c == 0x0C) {
            do L.i++; while (L.s[L.i] == ' ' || L.s[L.i] == '\t');
            if (L.i > L.n) L.i = L.n;
            if (L.nextsp < L.nsplices) sync_splices(&L);
            L.space = 1;
            continue;
        }
        d = peekc(&L, 1);
        if (c == '/' && d == '/') {
            while (L.i < L.n && cur(&L) != '\n') adv(&L);
            L.space = 1;
            continue;
        }
        if (c == '/' && d == '*') {
            int cline = L.line;
            adv(&L);
            adv(&L);
            for (;;) {
                if (L.i >= L.n) error_at(file, cline, "unterminated comment");
                if (cur(&L) == '*' && peekc(&L, 1) == '/') {
                    adv(&L);
                    adv(&L);
                    break;
                }
                adv(&L);
            }
            L.space = 1;
            continue;
        }
        start = L.i;
        tline = L.line;
        /* pp-number: digit or .digit, then digits, letters, '.', e+ e- p+ p- and digit separators */
        if (is_digit(c) || (c == '.' && is_digit(d))) {
            adv(&L);
            for (;;) {
                int x = cur(&L), y = peekc(&L, 1);
                if ((x == 'e' || x == 'E' || x == 'p' || x == 'P') && (y == '+' || y == '-')) {
                    adv(&L);
                    adv(&L);
                } else if (x == '\'' && (is_digit(y) || is_idstart(y))) {
                    adv(&L);
                    adv(&L);
                } else if (is_idchar(x) || x == '.') {
                    adv(&L);
                } else break;
            }
            tv_push(out, literal_tok(&L, K_NUM, tline, start));
            continue;
        }
        /* identifier, keyword, or the prefix of a string or character literal */
        if (is_idstart(c)) {
            int len, q;
            Name *nm;
            Token *t;
            {
                int *sp = L.s, ii = L.i;
                while (is_idchar(sp[ii])) ii++;
                L.i = ii;
                if (L.nextsp < L.nsplices) sync_splices(&L);
            }
            len = L.i - start;
            q = cur(&L);
            if (q == '"' || q == '\'') {
                /* encoding prefixes: L u U u8 (strings and characters), R forms (strings) */
                int p0 = L.s[start], p1 = len > 1 ? L.s[start + 1] : 0, p2 = len > 2 ? L.s[start + 2] : 0;
                int enc = 0, raw = 0;
                if (len == 1 && (p0 == 'L' || p0 == 'u' || p0 == 'U')) enc = 1;
                else if (len == 2 && p0 == 'u' && p1 == '8') enc = 1;
                else if (len == 1 && p0 == 'R') raw = 1;
                else if (len == 2 && (p0 == 'L' || p0 == 'u' || p0 == 'U') && p1 == 'R') raw = 1;
                else if (len == 3 && p0 == 'u' && p1 == '8' && p2 == 'R') raw = 1;
                if (raw && q == '"') {
                    tv_push(out, lex_raw_string(&L, start));
                    continue;
                }
                if (enc) {
                    tv_push(out, lex_quoted(&L, q, start));
                    continue;
                }
            }
            {
                char *sp = spell_scratch(&L, start, L.i);
                nm = intern(sp, (int)strlen(sp));
            }
            t = new_tok(&L, K_IDENT, tline, nm->s);
            if (nm->alt) {
                t->kind = K_PUNCT;
                t->id = nm->alt;
                t->text = punct_spell[nm->alt];
            } else {
                t->name = nm;
                t->id = nm->kw;
            }
            tv_push(out, t);
            continue;
        }
        if (c == '"' || c == '\'') {
            tv_push(out, lex_quoted(&L, c, start));
            continue;
        }
        /* digraphs */
        for (i = 0; digraph_names[i]; i++) {
            int len = (int)strlen(digraph_names[i]), j, ok = 1;
            for (j = 0; j < len; j++)
                if (peekc(&L, j) != digraph_names[i][j]) { ok = 0; break; }
            if (ok) {
                Token *t;
                for (j = 0; j < len; j++) adv(&L);
                t = new_tok(&L, K_PUNCT, tline, punct_spell[digraph_ids[i]]);
                t->id = digraph_ids[i];
                tv_push(out, t);
                break;
            }
        }
        if (digraph_names[i]) continue;
        /* punctuators: the longest match */
        if (c < PUNCT_FIRST && punct_by_char[c]) {
            int *list = punct_by_char[c];
            int found = 0;
            for (i = 0; list[i]; i++) {
                const char *sp = punct_spell[list[i]];
                int len = (int)strlen(sp), j, ok = 1;
                for (j = 1; j < len; j++)
                    if (peekc(&L, j) != sp[j]) { ok = 0; break; }
                if (ok) {
                    Token *t;
                    for (j = 0; j < len; j++) adv(&L);
                    t = new_tok(&L, K_PUNCT, tline, sp);
                    t->id = list[i];
                    tv_push(out, t);
                    found = 1;
                    break;
                }
            }
            if (found) continue;
        }
        adv(&L);
        tv_push(out, literal_tok(&L, K_OTHER, tline, start));
    }
    free(L.s);
    free(L.splices);
}

TokVec *lex_file(int file) {
    TokVec *tv = xalloc(sizeof(TokVec));
    lex_text(files[file].text, files[file].len, file, 1, tv);
    return tv;
}

/* ------------------------------------------------------------ dump */

/* --tokens: "line kind spelling", one token per line */
void dump_tokens(FILE *f, TokVec *tv) {
    int i;
    for (i = 0; i < tv->n; i++) {
        Token *t = tv->v[i];
        const char *kind = tok_kind_name(t->kind);
        if (t->kind == K_IDENT && t->id) kind = "keyword";
        fprintf(f, "%d %s ", t->line, kind);
        {
            const char *p;
            for (p = t->text; *p; p++) {
                if (*p == '\n') fputs("\\n", f);
                else fputc(*p, f);
            }
        }
        fputc('\n', f);
    }
}
