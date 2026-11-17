/*
 * lexer.c - tokens of a Rust source file (see token.h for the token format).
 *
 * Works on the codepoint array of the source file.  Handles identifiers
 * (Unicode, raw r#name), keywords (keywords.inc), lifetimes, number literals
 * with suffixes, char / byte / string / raw / byte string literals with all
 * escapes, doc comments (kept as TK_DOC), nested block comments, a leading
 * shebang line, and punctuation by longest match (puncts.inc).
 *
 * Written in the C subset of the CPUSim64 C11 compiler: no bit-fields, and
 * integer values are built from 32 bit limbs held in longs.
 */

/* ------------------------------------------------------------ tables */

static const char *tk_spell_tab[] = {
    "", "", "", "", "", "", "", "", "", "",      /* EOF .. DOC: no fixed spelling */
#define KW(x) #x,
#include "keywords.inc"
#undef KW
    "",                                          /* TK_KW_END */
#define P(id, s) s,
#include "puncts.inc"
#undef P
    ""
};

static const char *tk_id_tab[] = {
    "EOF", "IDENT", "LIFETIME", "INT", "FLOAT", "CHAR", "BYTE", "STR", "BSTR", "DOC",
#define KW(x) "KW",
#include "keywords.inc"
#undef KW
    "",
#define P(id, s) #id,
#include "puncts.inc"
#undef P
    ""
};

static Name *tk_name_tab[TK__COUNT + 1];     /* interned spelling of keywords and punctuation */
static int tk_len_tab[TK__COUNT + 1];
static Name *kw_slot_name[256];              /* open addressing on Name->hash */
static int kw_slot_kind[256];
static int lex_inited = 0;

const char *tk_spelling(int kind) {
    if (kind < 0 || kind >= TK__COUNT) return "";
    return tk_spell_tab[kind];
}

const char *tk_kind_name(int kind) {
    if (kind < 0 || kind >= TK__COUNT) return "";
    return tk_id_tab[kind];
}

static StrBuf lx_scratch;                    /* identifier and number spellings */
static StrBuf lx_body;                       /* string and comment bodies (bytes) */

static void lx_reset(StrBuf *b) {
    b->len = 0;
    b->s[0] = 0;
}

static void lex_init(void) {
    int k, h;
    if (lex_inited) return;
    lex_inited = 1;
    sb_init(&lx_scratch);
    sb_init(&lx_body);
    for (k = TK_DOC + 1; k < TK__COUNT; k++) {
        if (k == TK_KW_END) continue;
        tk_len_tab[k] = (int)strlen(tk_spell_tab[k]);
        tk_name_tab[k] = name_intern_str(tk_spell_tab[k]);
        if (tk_is_kw(k)) {
            h = (int)(tk_name_tab[k]->hash & 255);
            while (kw_slot_name[h]) h = (h + 1) & 255;
            kw_slot_name[h] = tk_name_tab[k];
            kw_slot_kind[h] = k;
        }
    }
}

/* the keyword kind of an interned name, or TK_IDENT */
static int kw_lookup(Name *nm) {
    int h = (int)(nm->hash & 255);
    while (kw_slot_name[h]) {
        if (kw_slot_name[h] == nm) return kw_slot_kind[h];
        h = (h + 1) & 255;
    }
    return TK_IDENT;
}

/* ------------------------------------------------------------ state */

static const int *lx_s;      /* the codepoints */
static int lx_n;
static int lx_i;             /* current index */
static int lx_line;          /* current line (1-based) */
static int lx_ls;            /* index of the first char of the current line */
static int lx_file;

static int pk(int k) {
    return lx_i + k < lx_n ? lx_s[lx_i + k] : 0;
}

static void lx_newline(int idx) {
    lx_line++;
    lx_ls = idx + 1;
}

/* renders a char for a message: itself if printable ASCII, else U+XXXX */
static void cp_text(char *buf, int c) {
    if (c >= 32 && c < 127) {
        buf[0] = (char)c;
        buf[1] = 0;
    } else {
        snprintf(buf, 16, "U+%04X", c);
    }
}

/* appends the UTF-8 bytes of a codepoint (one byte per char element) */
static void put_utf8(StrBuf *b, int cp) {
    if (cp < 0x80) {
        sb_putc(b, cp);
    } else if (cp < 0x800) {
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
}

/* moves the bytes of b into the token (arena memory) */
static void tk_set_bytes(Token *tk, StrBuf *b) {
    long k;
    char *p = ast_alloc((size_t)(b->len + 1) * sizeof(char));
    for (k = 0; k < b->len; k++) p[k] = (char)(b->s[k] & 0xFF);
    p[b->len] = 0;
    tk->str = p;
    tk->slen = b->len;
}

/* ------------------------------------------------------------ characters */

static int hexval(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Pattern_White_Space beyond ASCII */
static int is_pattern_ws(int c) {
    return c == 0x85 || c == 0x200E || c == 0x200F || c == 0x2028 || c == 0x2029;
}

/* An approximation of XID_Start / XID_Continue for c >= 128 without tables:
   letters, marks and digits of the scripts are accepted, and the blocks of
   symbols, punctuation, spaces, emoji and private use are rejected. */
static int uni_ident_ok(int c) {
    if (c == 0xAA || c == 0xB5 || c == 0xBA || c == 0xB7) return 1;
    if (c < 0xC0) return 0;
    if (c == 0xD7 || c == 0xF7) return 0;
    if (c >= 0x2000 && c <= 0x206F) return 0;
    if (c >= 0x20A0 && c <= 0x20CF) return 0;
    if (c >= 0x2190 && c <= 0x2BFF) return 0;
    if (c >= 0x2E00 && c <= 0x2E7F) return 0;
    if (c >= 0x3000 && c <= 0x3004) return 0;
    if (c >= 0x3008 && c <= 0x3020) return 0;
    if (c == 0x3030) return 0;
    if (c >= 0xD800 && c <= 0xF8FF) return 0;
    if (c >= 0xFE10 && c <= 0xFE1F) return 0;
    if (c >= 0xFE30 && c <= 0xFE6F) return 0;
    if (c >= 0xFF00 && c <= 0xFF0F) return 0;
    if (c >= 0xFF1A && c <= 0xFF20) return 0;
    if (c >= 0xFF3B && c <= 0xFF40) return 0;
    if (c >= 0xFF5B && c <= 0xFF65) return 0;
    if (c >= 0xFFF0 && c <= 0xFFFF) return 0;
    if (c >= 0x1D000 && c <= 0x1D3FF) return 0;
    if (c >= 0x1F000 && c <= 0x1FBFF) return 0;
    if (c > 0x10FFFF) return 0;
    return 1;
}

static int is_id_start(int c) {
    if (c < 128) return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    return uni_ident_ok(c);
}

static int is_id_cont(int c) {
    if (c < 128) return is_id_start(c) || (c >= '0' && c <= '9');
    return uni_ident_ok(c);
}

static int is_joint_char(int c) {
    return c > 0 && c < 128 && strchr("+-*/%^!&|<>=@.,;:#$?~", c) != 0;
}

/* ------------------------------------------------------------ comments */

/* s[from] s[from+1] is the opening of a block comment; returns the index after the closing delimiter
   (nested comments are counted).  Counts the lines passed. */
static int scan_block(int from) {
    int depth = 1, j = from + 2, startline = lx_line, c;
    while (depth > 0) {
        if (j >= lx_n) error_at(lx_file, startline, "unterminated block comment");
        c = lx_s[j];
        if (c == '\n') {
            lx_newline(j);
            j++;
        } else if (c == '/' && j + 1 < lx_n && lx_s[j + 1] == '*') {
            depth++;
            j += 2;
        } else if (c == '*' && j + 1 < lx_n && lx_s[j + 1] == '/') {
            depth--;
            j += 2;
        } else j++;
    }
    return j;
}

/* a doc comment starts at lx_i: a line comment with three slashes or slash slash bang,
   or a block comment with two stars or a bang */
static void lex_doc(Token *tk) {
    int line_style = lx_s[lx_i + 1] == '/';
    int j, end;
    tk->kind = TK_DOC;
    tk->inner = lx_s[lx_i + 2] == '!';
    lx_reset(&lx_body);
    if (line_style) {
        j = lx_i + 3;
        end = j;
        while (end < lx_n && lx_s[end] != '\n') end++;
        if (end > j && lx_s[end - 1] == '\r') end--;
        for (; j < end; j++) put_utf8(&lx_body, lx_s[j]);
        while (lx_i < lx_n && lx_s[lx_i] != '\n') lx_i++;
    } else {
        int from = lx_i + 3;
        end = scan_block(lx_i) - 2;
        for (j = from; j < end; j++) {
            if (lx_s[j] == '\r' && j + 1 < end && lx_s[j + 1] == '\n') continue;
            put_utf8(&lx_body, lx_s[j]);
        }
        lx_i = end + 2;
    }
    tk_set_bytes(tk, &lx_body);
}

/* ------------------------------------------------------------ escapes */

/* s[lx_i] is the backslash of an escape.  bytemode: byte literals (\xNN up to
   FF, no \u); in_string: a backslash and a newline is a line continuation
   (the result is then -1).  Returns the value. */
static long lex_escape(int bytemode, int in_string, int startline) {
    int e, v = 0, k, d, ndig;
    long r;
    lx_i++;
    if (lx_i >= lx_n) error_at(lx_file, startline, "unterminated escape sequence");
    e = lx_s[lx_i++];
    if (e == 'n') return '\n';
    if (e == 'r') return '\r';
    if (e == 't') return '\t';
    if (e == '\\') return '\\';
    if (e == '0') return 0;
    if (e == '\'') return '\'';
    if (e == '"') return '"';
    if (e == 'x') {
        for (k = 0; k < 2; k++) {
            d = hexval(pk(0));
            if (d < 0) error_at(lx_file, lx_line, "numeric character escape is too short");
            v = v * 16 + d;
            lx_i++;
        }
        if (!bytemode && v > 0x7F) error_at(lx_file, lx_line, "out of range hex escape");
        return v;
    }
    if (e == 'u') {
        if (bytemode) error_at(lx_file, lx_line, "unicode escape in byte literal");
        if (pk(0) != '{') error_at(lx_file, lx_line, "incorrect unicode escape sequence");
        lx_i++;
        r = 0;
        ndig = 0;
        for (;;) {
            int c = pk(0);
            if (c == '_') { lx_i++; continue; }
            if (c == '}') break;
            d = hexval(c);
            if (d < 0) error_at(lx_file, lx_line, "invalid character in unicode escape");
            if (++ndig > 6) error_at(lx_file, lx_line, "overlong unicode escape (must have at most 6 hex digits)");
            r = r * 16 + d;
            lx_i++;
        }
        lx_i++;
        if (ndig == 0) error_at(lx_file, lx_line, "empty unicode escape (must have at least 1 hex digit)");
        if (r > 0x10FFFF || (r >= 0xD800 && r <= 0xDFFF))
            error_at(lx_file, lx_line, "invalid unicode character escape");
        return r;
    }
    if (in_string && (e == '\n' || (e == '\r' && pk(0) == '\n'))) {
        if (e == '\r') lx_i++;
        lx_newline(lx_i - 1);
        while (lx_i < lx_n) {
            int c = lx_s[lx_i];
            if (c == '\n') lx_newline(lx_i);
            else if (c != ' ' && c != '\t' && c != '\r') break;
            lx_i++;
        }
        return -1;
    }
    {
        char cb[16];
        cp_text(cb, e);
        error_at(lx_file, lx_line, "unknown character escape: `%s`", cb);
    }
    return 0;
}

/* ------------------------------------------------------------ literals */

/* s[lx_i] is the opening quote of a (byte) string */
static void lex_string(Token *tk, int bytemode) {
    int startline = lx_line, c;
    long v;
    char *what = bytemode ? "byte string" : "double quote string";
    lx_reset(&lx_body);
    lx_i++;
    for (;;) {
        if (lx_i >= lx_n) error_at(lx_file, startline, "unterminated %s", what);
        c = lx_s[lx_i];
        if (c == '"') {
            lx_i++;
            break;
        }
        if (c == '\\') {
            v = lex_escape(bytemode, 1, startline);
            if (v < 0) continue;
            if (bytemode) sb_putc(&lx_body, (int)v);
            else put_utf8(&lx_body, (int)v);
            continue;
        }
        if (c == '\r') {
            if (pk(1) != '\n') error_at(lx_file, lx_line, "bare CR not allowed in string, use `\\r` instead");
            lx_i++;
            continue;
        }
        if (c == '\n') lx_newline(lx_i);
        if (bytemode && c > 127) error_at(lx_file, lx_line, "non-ASCII character in byte string literal");
        if (bytemode) sb_putc(&lx_body, c);
        else put_utf8(&lx_body, c);
        lx_i++;
    }
    tk->kind = bytemode ? TK_BSTR : TK_STR;
    tk_set_bytes(tk, &lx_body);
}

/* s[lx_i] is the r of r"..." or r#"..."# */
static void lex_raw_string(Token *tk, int bytemode) {
    int startline = lx_line, hashes = 0, c, k, close;
    lx_i++;
    while (pk(0) == '#') {
        hashes++;
        lx_i++;
    }
    if (pk(0) != '"') error_at(lx_file, lx_line, "found invalid character; only `#` is allowed in raw string delimitation");
    lx_i++;
    lx_reset(&lx_body);
    for (;;) {
        if (lx_i >= lx_n) error_at(lx_file, startline, "unterminated raw string");
        c = lx_s[lx_i];
        if (c == '"') {
            close = 1;
            for (k = 0; k < hashes; k++) {
                if (pk(1 + k) != '#') {
                    close = 0;
                    break;
                }
            }
            if (close) {
                lx_i += 1 + hashes;
                break;
            }
        }
        if (c == '\r') {
            if (pk(1) != '\n') error_at(lx_file, lx_line, "bare CR not allowed in raw string");
            lx_i++;
            continue;
        }
        if (c == '\n') lx_newline(lx_i);
        if (bytemode && c > 127) error_at(lx_file, lx_line, "non-ASCII character in raw byte string literal");
        if (bytemode) sb_putc(&lx_body, c);
        else put_utf8(&lx_body, c);
        lx_i++;
    }
    tk->kind = bytemode ? TK_BSTR : TK_STR;
    tk->raw = 1;
    tk_set_bytes(tk, &lx_body);
}

/* s[lx_i] is the quote of 'x' (a char literal) */
static void lex_char(Token *tk) {
    int c;
    long v;
    lx_i++;
    c = pk(0);
    if (c == '\\') {
        v = lex_escape(0, 0, lx_line);
    } else {
        if (c == '\n' || c == '\r' || c == '\t') error_at(lx_file, lx_line, "character constant must be escaped");
        v = c;
        lx_i++;
    }
    if (pk(0) != '\'') error_at(lx_file, lx_line, "character literal may only contain one codepoint");
    lx_i++;
    tk->kind = TK_CHAR;
    tk->value = v;
}

/* s[lx_i] is the b of b'x' */
static void lex_byte(Token *tk) {
    int c;
    long v;
    lx_i += 2;
    c = pk(0);
    if (c == '\\') {
        v = lex_escape(1, 0, lx_line);
    } else {
        if (c == '\'') error_at(lx_file, lx_line, "empty byte literal");
        if (c == '\n' || c == '\r' || c == '\t') error_at(lx_file, lx_line, "byte constant must be escaped");
        if (c > 127 || c == 0) error_at(lx_file, lx_line, "non-ASCII character in byte literal");
        v = c;
        lx_i++;
    }
    if (pk(0) != '\'') error_at(lx_file, lx_line, "unterminated byte constant");
    lx_i++;
    tk->kind = TK_BYTE;
    tk->value = v;
}

/* a quote starts a char literal or a lifetime */
static void lex_quote(Token *tk) {
    int c1 = pk(1), c2 = pk(2), from;
    if (c1 == '\\' || (c2 == '\'' && c1 != '\'' && c1 != 0)) {
        lex_char(tk);
        return;
    }
    if (c1 == '\'') error_at(lx_file, lx_line, "empty character literal");
    if (is_id_start(c1)) {
        from = lx_i;
        lx_i++;
        while (lx_i < lx_n && is_id_cont(lx_s[lx_i])) lx_i++;
        if (pk(0) == '\'') error_at(lx_file, lx_line, "character literal may only contain one codepoint");
        lx_reset(&lx_scratch);
        for (; from < lx_i; from++) sb_put_cp(&lx_scratch, lx_s[from]);
        tk->kind = TK_LIFETIME;
        tk->text = name_intern(lx_scratch.s, lx_scratch.len);
        return;
    }
    error_at(lx_file, lx_line, "unterminated character literal");
}

/* ------------------------------------------------------------ numbers */

/* l (four 32 bit limbs, least significant first) = l * radix + d */
static void limbs_muladd(long *l, int radix, int d) {
    long carry = d, t;
    int k;
    for (k = 0; k < 4; k++) {
        t = l[k] * radix + carry;
        l[k] = t & 0xFFFFFFFFL;
        carry = t >> 32;
    }
    if (carry) error_at(lx_file, lx_line, "integer literal is too large");
}

static int suffix_code(const char *s) {
    static const char *names[] = {
        "i8", "i16", "i32", "i64", "i128", "isize",
        "u8", "u16", "u32", "u64", "u128", "usize", "f32", "f64"
    };
    int k;
    for (k = 0; k < 14; k++)
        if (strcmp(s, names[k]) == 0) return SFX_I8 + k;
    return SFX_NONE;
}

/* a number starts at lx_i.  After a dot only decimal digits are taken, so
   that t.0.1 is three INT tokens and no float. */
static void lex_number(Token *tk, int after_dot) {
    long l[4];
    int radix = 10, ndig = 0, c, c1, d, isfloat = 0, from, j, sx;
    l[0] = l[1] = l[2] = l[3] = 0;
    lx_reset(&lx_scratch);
    c1 = pk(1);
    if (!after_dot && lx_s[lx_i] == '0' && (c1 == 'x' || c1 == 'o' || c1 == 'b')) {
        radix = c1 == 'x' ? 16 : (c1 == 'o' ? 8 : 2);
        sb_putc(&lx_scratch, '0');
        sb_putc(&lx_scratch, c1);
        lx_i += 2;
        for (;;) {
            c = pk(0);
            if (c == '_') {
                lx_i++;
                continue;
            }
            if (radix == 16) {
                d = hexval(c);
                if (d < 0) break;
            } else {
                if (c < '0' || c > '9') break;
                d = c - '0';
                if (d >= radix) error_at(lx_file, lx_line, "invalid digit for a base %d literal", radix);
            }
            limbs_muladd(l, radix, d);
            sb_putc(&lx_scratch, c);
            ndig++;
            lx_i++;
        }
        if (ndig == 0) error_at(lx_file, lx_line, "no valid digits found for number");
    } else {
        for (;;) {
            c = pk(0);
            if (c == '_' && !after_dot) {
                lx_i++;
                continue;
            }
            if (c < '0' || c > '9') break;
            limbs_muladd(l, 10, c - '0');
            sb_putc(&lx_scratch, c);
            ndig++;
            lx_i++;
        }
        if (!after_dot) {
            c = pk(0);
            c1 = pk(1);
            if (c == '.') {
                if (c1 >= '0' && c1 <= '9') {
                    isfloat = 1;
                    sb_putc(&lx_scratch, '.');
                    lx_i++;
                    for (;;) {
                        c = pk(0);
                        if (c == '_') {
                            lx_i++;
                            continue;
                        }
                        if (c < '0' || c > '9') break;
                        sb_putc(&lx_scratch, c);
                        lx_i++;
                    }
                } else if (c1 != '.' && !is_id_start(c1)) {
                    isfloat = 1;
                    sb_putc(&lx_scratch, '.');
                    lx_i++;
                }
            }
            c = pk(0);
            if (c == 'e' || c == 'E') {
                int sign = 0;
                j = 1;
                if (pk(j) == '+' || pk(j) == '-') {
                    sign = pk(j);
                    j++;
                }
                while (pk(j) == '_') j++;
                if (pk(j) >= '0' && pk(j) <= '9') {
                    isfloat = 1;
                    sb_putc(&lx_scratch, 'e');
                    if (sign) sb_putc(&lx_scratch, sign);
                    lx_i += j;
                    for (;;) {
                        c = pk(0);
                        if (c == '_') {
                            lx_i++;
                            continue;
                        }
                        if (c < '0' || c > '9') break;
                        sb_putc(&lx_scratch, c);
                        lx_i++;
                    }
                } else if (sign) {
                    error_at(lx_file, lx_line, "expected at least one digit in exponent");
                }
            }
        }
    }
    tk->radix = radix;
    tk->text = name_intern(lx_scratch.s, lx_scratch.len);
    /* a suffix */
    if (!after_dot && lx_i < lx_n && is_id_start(lx_s[lx_i])) {
        from = lx_i;
        while (lx_i < lx_n && is_id_cont(lx_s[lx_i])) lx_i++;
        lx_reset(&lx_body);
        for (; from < lx_i; from++) sb_put_cp(&lx_body, lx_s[from]);
        sx = suffix_code(lx_body.s);
        if (sx == SFX_NONE || ((sx == SFX_F32 || sx == SFX_F64) && radix != 10) ||
            (isfloat && sx != SFX_F32 && sx != SFX_F64))
            error_at(lx_file, lx_line, "invalid suffix `%s` for number literal", lx_body.s);
        if (sx == SFX_F32 || sx == SFX_F64) isfloat = 1;
        tk->suffix = name_intern(lx_body.s, lx_body.len);
        tk->sfx = sx;
    }
    if (isfloat) {
        tk->kind = TK_FLOAT;
        tk->fval = strtod(lx_scratch.s, 0);
    } else {
        tk->kind = TK_INT;
        tk->value = (long)(((unsigned long)l[1] << 32) | (unsigned long)l[0]);
        tk->big = (l[2] != 0 || l[3] != 0);
    }
}

/* ------------------------------------------------------------ identifiers */

static void lex_ident(Token *tk, int raw) {
    int from = lx_i;
    Name *nm;
    while (lx_i < lx_n && is_id_cont(lx_s[lx_i])) lx_i++;
    lx_reset(&lx_scratch);
    for (; from < lx_i; from++) sb_put_cp(&lx_scratch, lx_s[from]);
    nm = name_intern(lx_scratch.s, lx_scratch.len);
    tk->text = nm;
    if (raw) {
        if (strcmp(nm->s, "crate") == 0 || strcmp(nm->s, "self") == 0 || strcmp(nm->s, "super") == 0 ||
            strcmp(nm->s, "Self") == 0)
            error_at(lx_file, lx_line, "`%s` cannot be a raw identifier", nm->s);
        tk->kind = TK_IDENT;
        tk->raw = 1;
    } else if (nm->len == 1 && nm->s[0] == '_') {
        tk->kind = TK_UNDERSCORE;
    } else {
        tk->kind = kw_lookup(nm);
    }
}

/* r"..", r#".."#, r#name, b'x', b"..", br".." : returns 1 if one was lexed */
static int lex_prefixed(Token *tk) {
    int c = lx_s[lx_i], c1 = pk(1), h;
    if (c == 'b') {
        if (c1 == '\'') {
            lex_byte(tk);
            return 1;
        }
        if (c1 == '"') {
            lx_i++;
            lex_string(tk, 1);
            return 1;
        }
        if (c1 == 'r') {
            h = 2;
            while (pk(h) == '#') h++;
            if (pk(h) == '"') {
                lx_i++;
                lex_raw_string(tk, 1);
                return 1;
            }
        }
        return 0;
    }
    if (c1 == '"') {
        lex_raw_string(tk, 0);
        return 1;
    }
    if (c1 == '#') {
        h = 1;
        while (pk(h) == '#') h++;
        if (pk(h) == '"') {
            lex_raw_string(tk, 0);
            return 1;
        }
        if (h == 2 && is_id_start(pk(2))) {
            lx_i += 2;
            lex_ident(tk, 1);
            return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------ punctuation */

/* the longest punctuation token at lx_i, or 0 */
static int match_punct(void) {
    int k, m, best = 0, bestlen = 0, c = lx_s[lx_i];
    const char *sp;
    for (k = TK_KW_END + 1; k < TK__COUNT; k++) {
        sp = tk_spell_tab[k];
        if (tk_len_tab[k] <= bestlen || sp[0] != c || sp[0] == '_') continue;
        for (m = 1; m < tk_len_tab[k]; m++)
            if (sp[m] != pk(m)) break;
        if (m == tk_len_tab[k]) {
            best = k;
            bestlen = tk_len_tab[k];
        }
    }
    return best;
}

/* the punctuation kind spelled exactly by the first len chars of s, or 0 */
static int punct_lookup(const char *s, int len) {
    int k;
    for (k = TK_KW_END + 1; k < TK__COUNT; k++)
        if (tk_len_tab[k] == len && strncmp(tk_spell_tab[k], s, (size_t)len) == 0 && s[0] != '_') return k;
    return 0;
}

int lex_split_punct(Token *t, int n, Token *rest) {
    const char *sp;
    int len, k1, k2;
    lex_init();
    if (!tk_is_punct(t->kind)) return 0;
    sp = tk_spell_tab[t->kind];
    len = tk_len_tab[t->kind];
    if (n < 1 || n >= len) return 0;
    k1 = punct_lookup(sp, n);
    k2 = punct_lookup(sp + n, len - n);
    if (!k1 || !k2) return 0;
    *rest = *t;
    rest->kind = k2;
    rest->text = tk_name_tab[k2];
    rest->col = t->col + n;
    rest->space_before = 0;
    t->kind = k1;
    t->text = tk_name_tab[k1];
    t->joint = 1;
    return 1;
}

int lex_split_tuple_index(const Token *t, Token *out) {
    const char *s;
    long k = 0, b, len, v = 0;
    int ndig;
    if (t->kind != TK_FLOAT || t->sfx != SFX_NONE || !t->text) return 0;
    s = t->text->s;
    len = t->text->len;
    while (k < len && s[k] >= '0' && s[k] <= '9') {
        v = v * 10 + (s[k] - '0');
        k++;
    }
    if (k == 0 || k >= len || s[k] != '.') return 0;
    ndig = (int)k;
    out[0] = *t;
    out[0].kind = TK_INT;
    out[0].radix = 10;
    out[0].value = v;
    out[0].text = name_intern(s, k);
    out[0].fval = 0;
    out[1] = *t;
    out[1].kind = TK_DOT;
    out[1].text = tk_name_tab[TK_DOT];
    out[1].col = t->col + ndig;
    out[1].space_before = 0;
    out[1].value = 0;
    out[1].fval = 0;
    out[0].joint = 0;
    if (k + 1 == len) return 2;
    k++;
    v = 0;
    for (b = k; b < len; b++) {
        if (s[b] < '0' || s[b] > '9') return 0;
        v = v * 10 + (s[b] - '0');
    }
    out[2] = *t;
    out[2].kind = TK_INT;
    out[2].radix = 10;
    out[2].value = v;
    out[2].text = name_intern(s + k, len - k);
    out[2].col = t->col + ndig + 1;
    out[2].space_before = 0;
    out[2].fval = 0;
    return 3;
}

/* ------------------------------------------------------------ main loop */

static void push_tok(Token **v, int *n, int *cap, const Token *t) {
    if (*n == *cap) {
        *cap = *cap ? *cap * 2 : 256;
        *v = xrealloc(*v, sizeof(Token) * (size_t)*cap);
    }
    (*v)[(*n)++] = *t;
}

Token *lex_all(int file, int *ntok) {
    Token *v = 0;
    int n = 0, cap = 0, ws = 0, prev = TK_EOF, c, k, isdoc;
    lex_init();
    lx_s = files[file].text;
    lx_n = files[file].len;
    lx_i = 0;
    lx_line = 1;
    lx_ls = 0;
    lx_file = file;
    if (lx_n > 0 && lx_s[0] == 0xFEFF) {
        lx_i = 1;
        lx_ls = 1;
    }
    /* a shebang line, but not the start of an inner attribute */
    if (pk(0) == '#' && pk(1) == '!') {
        k = 2;
        while (pk(k) == ' ' || pk(k) == '\t') k++;
        if (pk(k) != '[') {
            while (lx_i < lx_n && lx_s[lx_i] != '\n') lx_i++;
        }
    }
    for (;;) {
        Token tk;
        memset(&tk, 0, sizeof tk);
        tk.file = file;
        isdoc = 0;
        while (lx_i < lx_n) {
            c = lx_s[lx_i];
            if (c == '\n') {
                lx_newline(lx_i);
                lx_i++;
                ws = 1;
            } else if (c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v' || is_pattern_ws(c)) {
                lx_i++;
                ws = 1;
            } else if (c == '/' && pk(1) == '/') {
                if ((pk(2) == '/' && pk(3) != '/') || pk(2) == '!') {
                    isdoc = 1;
                    break;
                }
                while (lx_i < lx_n && lx_s[lx_i] != '\n') lx_i++;
                ws = 1;
            } else if (c == '/' && pk(1) == '*') {
                if ((pk(2) == '*' && pk(3) != '*' && pk(3) != '/') || pk(2) == '!') {
                    isdoc = 1;
                    break;
                }
                lx_i = scan_block(lx_i);
                ws = 1;
            } else break;
        }
        tk.line = lx_line;
        tk.col = lx_i - lx_ls + 1;
        tk.space_before = ws;
        ws = 0;
        if (isdoc) {
            lex_doc(&tk);
        } else if (lx_i >= lx_n) {
            tk.kind = TK_EOF;
            push_tok(&v, &n, &cap, &tk);
            *ntok = n;
            return v;
        } else {
            c = lx_s[lx_i];
            if ((c == 'r' || c == 'b') && lex_prefixed(&tk)) {
                /* done */
            } else if (is_id_start(c)) {
                lex_ident(&tk, 0);
            } else if (c >= '0' && c <= '9') {
                lex_number(&tk, prev == TK_DOT);
            } else if (c == '"') {
                lex_string(&tk, 0);
            } else if (c == '\'') {
                lex_quote(&tk);
            } else {
                k = match_punct();
                if (!k) {
                    char cb[16];
                    cp_text(cb, c);
                    error_at(file, lx_line, "unknown start of token: %s", cb);
                }
                tk.kind = k;
                tk.text = tk_name_tab[k];
                lx_i += tk_len_tab[k];
                tk.joint = lx_i < lx_n && is_joint_char(lx_s[lx_i]);
            }
        }
        prev = tk.kind;
        push_tok(&v, &n, &cap, &tk);
    }
}

/* ------------------------------------------------------------ printing */

static void put_native(FILE *f, const char *s) {
#ifdef __CPUSIM64__
    while (*s) fputc(*s++, f);       /* one codepoint per char */
#else
    fputs(s, f);
#endif
}

/* bytes in double quotes: \\ \" \n \r \t, other bytes outside 0x20..0x7E as \xHH */
static void put_escaped(FILE *f, const char *s, long len) {
    long k;
    fputc('"', f);
    for (k = 0; k < len; k++) {
        int c = s[k] & 0xFF;
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

void tok_put_source(FILE *f, const Token *t) {
    switch (t->kind) {
    case TK_EOF:
        break;
    case TK_IDENT:
        if (t->raw) fputs("r#", f);
        put_native(f, t->text->s);
        break;
    case TK_LIFETIME:
        put_native(f, t->text->s);
        break;
    case TK_INT:
    case TK_FLOAT:
        put_native(f, t->text->s);
        if (t->suffix) put_native(f, t->suffix->s);
        break;
    case TK_CHAR:
        if (t->value >= 0x20 && t->value < 0x7F && t->value != '\'' && t->value != '\\') fprintf(f, "'%c'", (int)t->value);
        else fprintf(f, "'\\u{%lX}'", (unsigned long)t->value);
        break;
    case TK_BYTE:
        fprintf(f, "b'\\x%02lX'", (unsigned long)t->value);
        break;
    case TK_STR:
        put_escaped(f, t->str, t->slen);
        break;
    case TK_BSTR:
        fputc('b', f);
        put_escaped(f, t->str, t->slen);
        break;
    case TK_DOC:
        fputs(t->inner ? "//!" : "///", f);
        put_escaped(f, t->str, t->slen);
        break;
    default:
        fputs(tk_spell_tab[t->kind], f);
        break;
    }
}

void tok_text_list(FILE *f, const struct Token *toks, long ntoks) {
    long k;
    for (k = 0; k < ntoks; k++) {
        if (k > 0) fputc(' ', f);
        tok_put_source(f, &toks[k]);
    }
}

void tok_print_line(FILE *f, const Token *t, int pflags) {
    fprintf(f, "%d:%d %s", t->line, t->col, tk_kind_name(t->kind));
    switch (t->kind) {
    case TK_EOF:
        break;
    case TK_IDENT:
    case TK_LIFETIME:
        fputc(' ', f);
        tok_put_source(f, t);
        break;
    case TK_INT:
        fputc(' ', f);
        put_native(f, t->text->s);
        fprintf(f, " =%lu", (unsigned long)t->value);
        if (t->big) fputs(" big", f);
        if (t->suffix) {
            fputc(' ', f);
            put_native(f, t->suffix->s);
        }
        break;
    case TK_FLOAT:
        fputc(' ', f);
        put_native(f, t->text->s);
        fprintf(f, " =%g", t->fval);
        if (t->suffix) {
            fputc(' ', f);
            put_native(f, t->suffix->s);
        }
        break;
    case TK_CHAR:
        fprintf(f, " U+%04lX", (unsigned long)t->value);
        break;
    case TK_BYTE:
        fprintf(f, " 0x%02lX", (unsigned long)t->value);
        break;
    case TK_STR:
    case TK_BSTR:
        fputc(' ', f);
        put_escaped(f, t->str, t->slen);
        if (t->raw) fputs(" raw", f);
        break;
    case TK_DOC:
        fputs(t->inner ? " inner " : " outer ", f);
        put_escaped(f, t->str, t->slen);
        break;
    default:
        fputc(' ', f);
        fputs(tk_spell_tab[t->kind], f);
        break;
    }
    if (pflags & TOKP_FLAGS) {
        if (t->space_before) fputs(" space", f);
        if (t->joint) fputs(" joint", f);
    }
    fputc('\n', f);
}
