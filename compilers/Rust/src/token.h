/*
 * token.h - the Rust token and the lexer interface.
 *
 * lex_all() turns a source file (the codepoint array that load_file() made)
 * into an array of Token, ending with TK_EOF.  Included after ast.h (it uses
 * Name); lexer.c is included by rustc.c after ast.c.
 *
 * Kinds
 *   TK_EOF        end of input (always the last token)
 *   TK_IDENT      identifier; `r#name` is an identifier with raw = 1 (text is
 *                 name without the r#).  `_` alone is TK_UNDERSCORE.
 *   TK_LIFETIME   'a  'static  '_  (text includes the quote)
 *   TK_INT        integer literal
 *   TK_FLOAT      float literal
 *   TK_CHAR       'x'   value = the codepoint
 *   TK_BYTE       b'x'  value = the byte
 *   TK_STR        "..." and r"..." (raw = 1); str/slen = decoded UTF-8 bytes
 *   TK_BSTR       b"..." and br"..." (raw = 1); str/slen = the bytes
 *   TK_DOC        doc comment; str/slen = the text without the comment
 *                 markers (UTF-8 bytes); inner = 1 for //! and the block form
 *   TK_KW_xxx     a keyword (keywords.inc), e.g. TK_KW_fn, TK_KW_Self
 *   TK_xxx        punctuation and delimiters (puncts.inc), e.g. TK_PLUS
 *
 * Values
 *   text    interned Name.  Identifiers, keywords and punctuation: the
 *           spelling.  Lifetimes: the spelling with the quote.  INT and
 *           FLOAT: the digits without underscores and without the suffix
 *           (prefix 0x 0o 0b kept).  Other literals: 0.
 *   value   INT: the low 64 bits of the value (as a long, so a u64 above
 *           2^63 is negative); CHAR, BYTE: the codepoint / byte.
 *   big     INT: 1 if the value does not fit 64 bits (it is below 2^128,
 *           larger is a lexical error); value then holds the low 64 bits.
 *   radix   INT: 2, 8, 10 or 16.
 *   suffix  INT, FLOAT: the suffix as a Name (i32, u8, usize, f64, ...) or 0.
 *   sfx     the suffix as SFX_xxx (SFX_NONE if there is none).
 *   fval    FLOAT: the value (a double also for f32).
 *   str     TK_STR, TK_BSTR, TK_DOC: slen bytes, one byte (0..255) per char
 *           element whatever the host's char; mask with & 0xFF when reading
 *           a char, as node_set_str() does.  Also NUL terminated.
 *   space_before  whitespace or a (non doc) comment comes before the token.
 *   joint   the token is punctuation and the very next source char is one
 *           of + - * / % ^ ! & | < > = @ . , ; : # $ ? ~ (no space between).
 *           With longest match `>` in `>>` is not joint; `&` in `&-x` and
 *           `<` in `<-` are.  After lex_split_punct the first part is joint.
 *   line, col  1-based; col counts codepoints.  file indexes files[].
 *
 * Numbers after a `.` are always lexed as plain decimal INT without suffix,
 * so x.0.1 is x . 0 . 1.  A float is never lexed right after a dot.
 */
#ifndef TOKEN_H
#define TOKEN_H

enum {
    TK_EOF = 0,
    TK_IDENT,
    TK_LIFETIME,
    TK_INT,
    TK_FLOAT,
    TK_CHAR,
    TK_BYTE,
    TK_STR,
    TK_BSTR,
    TK_DOC,
#define KW(x) TK_KW_##x,
#include "keywords.inc"
#undef KW
    TK_KW_END,
#define P(id, s) TK_##id,
#include "puncts.inc"
#undef P
    TK__COUNT
};

#define tk_is_kw(k)    ((k) > TK_DOC && (k) < TK_KW_END)
#define tk_is_punct(k) ((k) >= TK_KW_END && (k) < TK__COUNT)

/* literal suffixes */
enum {
    SFX_NONE = 0,
    SFX_I8, SFX_I16, SFX_I32, SFX_I64, SFX_I128, SFX_ISIZE,
    SFX_U8, SFX_U16, SFX_U32, SFX_U64, SFX_U128, SFX_USIZE,
    SFX_F32, SFX_F64
};

typedef struct Token {
    int kind;                /* TK_xxx */
    int line, col, file;
    int space_before;
    int joint;
    Name *text;
    long value;
    int big;
    int radix;
    int raw;                 /* raw identifier, raw string */
    int inner;               /* inner doc comment */
    Name *suffix;
    int sfx;
    double fval;
    char *str;
    long slen;
} Token;

/* Lexes the whole file.  The result is allocated with xalloc; *ntok is the
 * number of tokens including the final TK_EOF.  A lexical error is reported
 * with error_at() (file:line: error: message) and ends the program. */
Token *lex_all(int file, int *ntok);

/* Splits the punctuation token t after its first n chars: t keeps the first
 * part (and gets joint = 1) and *rest the remainder, which starts at col + n
 * and has space_before = 0.  Both parts must be tokens in puncts.inc.
 * Returns 1, or 0 if t is not punctuation or the parts are no tokens
 * (nothing is changed then).  Examples: `>>` n=1 gives `>` `>`; `>>=`
 * n=1 gives `>` `>=`, n=2 gives `>>` `=`; `&&` `||` `<<` `>=` `..=` n=2
 * (`..` `=`). */
int lex_split_punct(Token *t, int n, Token *rest);

/* Splits a float token written `1.2` (or `1.`), which is only lexed when
 * it does not follow a dot, into INT `.` INT.  Fills out[0..2] and returns 3
 * (2 for the form `1.`), or returns 0 if t is not such a float. */
int lex_split_tuple_index(const Token *t, Token *out);

/* names and printing */
const char *tk_spelling(int kind);          /* "fn", "+=", ... ; "" for the literal kinds */
const char *tk_kind_name(int kind);         /* "IDENT", "KW", "PLUS", ... */
#define TOKP_FLAGS 1                        /* tok_print_line: add " space" / " joint" */
void tok_print_line(FILE *f, const Token *t, int pflags);   /* "line:col KIND text", newline */
void tok_put_source(FILE *f, const Token *t);               /* the token as source text, no newline */
void tok_text_list(FILE *f, const struct Token *toks, long ntoks);   /* for ast_tok_text */

#endif
