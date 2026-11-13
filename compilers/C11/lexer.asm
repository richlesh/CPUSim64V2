///////////////////////////////////////////////////////////////////////////////
// lexer.asm
//
// Converts a source file (wstr of codepoints) into a vector of tokens.
// Follows the token structure of CLexer.g4 restricted to the supported
// subset: identifiers, keywords (recognized after preprocessing), integer,
// floating and character constants, string literals and punctuators.
//
//   lex_init()                  build punctuator / keyword tables
//   lex_file(src, file_entry)   -> vector of tokens (no EOF token)
//   tok_spelling(tok)           -> wstr spelling of a token
///////////////////////////////////////////////////////////////////////////////

#global LX_SRC:     .dci 0          // source wstr
#global LX_LEN:     .dci 0
#global LX_POS:     .dci 0          // current index (1-based)
#global LX_LINE:    .dci 0
#global LX_FILE:    .dci 0          // intern entry of the file name
#global LX_PUNCTS:   .dci 0          // vec of punctuator wstrs (kind = 9 + index)
#global LX_KWNAMES: .dci 0          // vec of keyword entries (kind = 99 + index)

LEXER_ASM_START:
    jump    LEXER_ASM_END

// Longest spellings first so that the first match is the maximal munch.
PUNCT_SPELLINGS:
    .dcs    "... <<= >>= -> ++ -- << >> <= >= == != && || *= /= %= += -= &= ^= |= ## [ ] ( ) { } . & * + - ~ ! / % < > ^ | ? : ; = , #"
KEYWORD_SPELLINGS:
    .dcs    "auto break case char const continue default do double else enum extern float for goto if inline int long register restrict return short signed sizeof static struct switch typedef union unsigned void volatile while _Bool _Noreturn bool true false _Complex _Atomic _Generic _Alignas _Thread_local _Imaginary _Static_assert nullptr __func__ __pstr _Alignof __asm__"

///////////////////////////////////////////////////////////////////////////////
// split_words(p) -> vec of wstrs split on spaces
///////////////////////////////////////////////////////////////////////////////
#def_func split_words(pstr)
    #var    cp, cnt, i, c, v, curv
    load    r1, pstr
    int     iGET_CODEPOINTS
    move    cp, r0
    load    cnt, cp[0]
    #call   vec_new()
    move    v, r0
    #call   vec_new()
    move    curv, r0
    #for    1, i <= cnt, 1
        load    c, cp[i]
        #if_cond c == ' '
            #call   sb_to_w(curv)
            #call   vec_push(v, r0)
            #call   vec_clear(curv)
        #else_cond
            #call   vec_push(curv, c)
        #end_cond
    #end_for
    #call   sb_to_w(curv)
    #call   vec_push(v, r0)
    #return v
#end_func

///////////////////////////////////////////////////////////////////////////////
// set_spec(name, code): mark an identifier as special for the preprocessor
///////////////////////////////////////////////////////////////////////////////
#def_func set_spec(name, code)
    #var    c
    load    c, code
    load    r1, name
    #call   intern_p(r1)
    store   c, r0[E_SPEC]
#end_func

///////////////////////////////////////////////////////////////////////////////
// lex_init(): build punctuator and keyword tables, special identifiers
///////////////////////////////////////////////////////////////////////////////
#def_func lex_init()
    #var    v, cnt, i, w, e, k
    #call   split_words(PUNCT_SPELLINGS)
    store   r0, LX_PUNCTS
    #call   split_words(KEYWORD_SPELLINGS)
    move    v, r0
    #call   vec_new()
    store   r0, LX_KWNAMES
    #macro  VLEN(cnt, v)
    #for    1, i <= cnt, 1
        #macro  VGET(w, v, i)
        #call   intern_w(w)
        move    e, r0
        add     k, i, KW_BASE
        sub     k, 1
        store   k, e[E_KW]
        load    r1, LX_KWNAMES
        #call   vec_push(r1, e)
    #end_for
    #call   set_spec("__LINE__", SPEC_LINE)
    #call   set_spec("__FILE__", SPEC_FILE)
    #call   set_spec("defined", SPEC_DEFINED)
    #call   set_spec("__VA_ARGS__", SPEC_VAARGS)
    #call   set_spec("define", SPEC_D_DEFINE)
    #call   set_spec("undef", SPEC_D_UNDEF)
    #call   set_spec("include", SPEC_D_INCLUDE)
    #call   set_spec("ifdef", SPEC_D_IFDEF)
    #call   set_spec("ifndef", SPEC_D_IFNDEF)
    #call   set_spec("elif", SPEC_D_ELIF)
    #call   set_spec("endif", SPEC_D_ENDIF)
    #call   set_spec("error", SPEC_D_ERROR)
    #call   set_spec("pragma", SPEC_D_PRAGMA)
    #call   set_spec("line", SPEC_D_LINE)
    #call   set_spec("warning", SPEC_D_WARNING)
    // "if" and "else" are keywords; the preprocessor checks E_KW for them
#end_func

///////////////////////////////////////////////////////////////////////////////
// character classes
///////////////////////////////////////////////////////////////////////////////
#def_func is_digit(ch)
    #var    c
    load    c, ch
    #if_cond c < '0'
        #return 0
    #end_cond
    #if_cond c > '9'
        #return 0
    #end_cond
    #return 1
#end_func

#def_func is_ident_start(ch)
    #var    c
    load    c, ch
    #if_cond c == '_'
        #return 1
    #end_cond
    #if_cond c == '$'
        #return 1
    #end_cond
    #if_cond c >= 128
        #return 1
    #end_cond
    or      c, 0x20             // fold case
    #if_cond c < 'a'
        #return 0
    #end_cond
    #if_cond c > 'z'
        #return 0
    #end_cond
    #return 1
#end_func

#def_func is_ident_char(ch)
    #var    c
    load    c, ch
    #call   is_ident_start(c)
    #if_cond r0 != 0
        #return 1
    #end_cond
    #call   is_digit(c)
#end_func

#def_func hex_val(ch)               // -> 0..15 or -1
    #var    c
    load    c, ch
    #if_cond c >= '0'
        #if_cond c <= '9'
            sub     r0, c, '0'
            #return r0
        #end_cond
    #end_cond
    or      c, 0x20
    #if_cond c >= 'a'
        #if_cond c <= 'f'
            sub     r0, c, 'a'
            add     r0, 10
            #return r0
        #end_cond
    #end_cond
    #return -1
#end_func

///////////////////////////////////////////////////////////////////////////////
// lx_peekc(k) -> codepoint at LX_POS+k or 0 past the end
///////////////////////////////////////////////////////////////////////////////
#def_func lx_peekc(kk)
    #var    i, cnt
    load    i, LX_POS
    load    r0, kk
    add     i, r0
    load    cnt, LX_LEN
    #if_cond i > cnt
        #return 0
    #end_cond
    load    r0, LX_SRC
    load    r0, r0[i]
#end_func

///////////////////////////////////////////////////////////////////////////////
// lx_error(msg)
///////////////////////////////////////////////////////////////////////////////
#def_func lx_error(msg)
    load    r1, LX_FILE
    load    r2, LX_LINE
    load    r3, msg
    #call   error_at(r1, r2, r3)
#end_func

///////////////////////////////////////////////////////////////////////////////
// lx_escape() -> value of the escape sequence at LX_POS (just after '\')
//                LX_POS is advanced past it.
///////////////////////////////////////////////////////////////////////////////
#def_func lx_escape()
    #var    c, v, k, d, pos
    #call   lx_peekc(0)
    move    c, r0
    load    pos, LX_POS
    add     pos, 1
    store   pos, LX_POS
    #if_cond c == 'n'
        #return 10
    #end_cond
    #if_cond c == 't'
        #return 9
    #end_cond
    #if_cond c == 'r'
        #return 13
    #end_cond
    #if_cond c == '0'
        // fall through to octal handling below
    #end_cond
    #if_cond c == 'a'
        #return 7
    #end_cond
    #if_cond c == 'b'
        #return 8
    #end_cond
    #if_cond c == 'f'
        #return 12
    #end_cond
    #if_cond c == 'v'
        #return 11
    #end_cond
    #if_cond c == 'e'
        #return 27
    #end_cond
    #if_cond c == 'x'
        move    v, 0
        move    k, 0
        #while  TRUE
            #call   lx_peekc(0)
            #call   hex_val(r0)
            move    d, r0
            #if_cond d < 0
                #break
            #end_cond
            lshift  v, 4
            add     v, d
            add     k, 1
            load    pos, LX_POS
            add     pos, 1
            store   pos, LX_POS
        #end_while
        #if_cond k == 0
            #call   lx_error("invalid hexadecimal escape sequence")
        #end_cond
        #return v
    #end_cond
    #if_cond c == 'u'
        move    k, 4
        #call   lx_ucn(k)
        #return r0
    #end_cond
    #if_cond c == 'U'
        move    k, 8
        #call   lx_ucn(k)
        #return r0
    #end_cond
    #if_cond c >= '0'
        #if_cond c <= '7'
            sub     v, c, '0'
            move    k, 1
            #while  k < 3
                #call   lx_peekc(0)
                move    d, r0
                #if_cond d < '0'
                    #break
                #end_cond
                #if_cond d > '7'
                    #break
                #end_cond
                lshift  v, 3
                sub     d, '0'
                add     v, d
                add     k, 1
                load    pos, LX_POS
                add     pos, 1
                store   pos, LX_POS
            #end_while
            #return v
        #end_cond
    #end_cond
    // \\ \' \" \? and any other character stand for themselves
    #return c
#end_func

// lx_ucn(k): k hex digits of a universal character name
#def_func lx_ucn(kk)
    #var    k, v, d, pos
    load    k, kk
    move    v, 0
    #while  k > 0
        #call   lx_peekc(0)
        #call   hex_val(r0)
        move    d, r0
        #if_cond d < 0
            #call   lx_error("invalid universal character name")
        #end_cond
        lshift  v, 4
        add     v, d
        load    pos, LX_POS
        add     pos, 1
        store   pos, LX_POS
        sub     k, 1
    #end_while
    #return v
#end_func

///////////////////////////////////////////////////////////////////////////////
// lx_string() -> decoded wstr of the string literal whose opening quote is
//                at LX_POS.  LX_POS ends just past the closing quote.
///////////////////////////////////////////////////////////////////////////////
#def_func lx_string()
    #var    v, c, pos
    #call   vec_new()
    move    v, r0
    load    pos, LX_POS
    add     pos, 1
    store   pos, LX_POS
    #while  TRUE
        #call   lx_peekc(0)
        move    c, r0
        #if_cond c == '"'
            #break
        #end_cond
        #if_cond c == '\n'
            #call   lx_error("unterminated string literal")
        #end_cond
        #if_cond c == 0
            #call   lx_error("unterminated string literal")
        #end_cond
        load    pos, LX_POS
        add     pos, 1
        store   pos, LX_POS
        #if_cond c == '\\'
            #call   lx_escape()
            move    c, r0
        #end_cond
        #call   vec_push(v, c)
    #end_while
    load    pos, LX_POS
    add     pos, 1
    store   pos, LX_POS
    load    r0, v[VEC_ARR]
    move    r1, v
    push    r0
    int     iFREE
    pop     r0
#end_func

///////////////////////////////////////////////////////////////////////////////
// lx_char() -> value of the character constant whose opening quote is at
//              LX_POS.  LX_POS ends just past the closing quote.
///////////////////////////////////////////////////////////////////////////////
#def_func lx_char()
    #var    c, v, pos
    load    pos, LX_POS
    add     pos, 1
    store   pos, LX_POS
    #call   lx_peekc(0)
    move    c, r0
    #if_cond c == '\''
        #call   lx_error("empty character constant")
    #end_cond
    #if_cond c == '\n'
        #call   lx_error("unterminated character constant")
    #end_cond
    add     pos, 1
    store   pos, LX_POS
    move    v, c
    #if_cond c == '\\'
        #call   lx_escape()
        move    v, r0
    #end_cond
    #call   lx_peekc(0)
    #if_cond r0 != '\''
        #call   lx_error("unterminated or multi-character character constant")
    #end_cond
    load    pos, LX_POS
    add     pos, 1
    store   pos, LX_POS
    #return v
#end_func

///////////////////////////////////////////////////////////////////////////////
// convert_num(tok): convert a pp-number token into TK_INT or TK_FLOAT
///////////////////////////////////////////////////////////////////////////////
#def_func convert_num(tk)
    #var    t, w, cnt, i, c, isf, ishex, base, v, d, suf, tmp
    load    t, tk
    load    w, t[TK_TEXT]
    load    cnt, w[0]
    move    ishex, 0
    move    isf, 0
    #if_cond cnt >= 2
        load    c, w[1]
        load    d, w[2]
        or      d, 0x20
        #if_cond c == '0'
            #if_cond d == 'x'
                move    ishex, 1
            #end_cond
        #end_cond
    #end_cond
    #for    1, i <= cnt, 1
        load    c, w[i]
        #if_cond c == '.'
            move    isf, 1
        #end_cond
        or      c, 0x20
        #if_cond ishex == 0
            #if_cond c == 'e'
                move    isf, 1
            #end_cond
        #else_cond
            #if_cond c == 'p'
                move    isf, 1
            #end_cond
        #end_cond
    #end_for
    #if_cond isf != 0
        // strip a float suffix (f, F, l, L)
        load    c, w[cnt]
        or      c, 0x20
        #if_cond c == 'f'
            #if_cond ishex == 0
                sub     cnt, 1
            #end_cond
        #end_cond
        #if_cond c == 'l'
            sub     cnt, 1
        #end_cond
        add     r1, cnt, 1
        #call   zalloc(r1)
        move    tmp, r0
        store   cnt, tmp[0]
        #for    1, i <= cnt, 1
            load    c, w[i]
            store   c, tmp[i]
        #end_for
        move    r1, tmp
        int     iFROM_CODEPOINTS
        move    v, r0
        move    r1, tmp
        int     iFREE
        move    r1, v
        int     iPARSE_FLOAT
        store   f0, t[TK_VAL]
        move    r1, v
        int     iFREE
        move    r0, TK_FLOAT
        store   r0, t[TK_KIND]
        #return 0
    #end_cond
    // integer constant
    move    base, 10
    move    i, 1
    #if_cond ishex != 0
        move    base, 16
        move    i, 3
    #else_cond
        load    c, w[1]
        #if_cond c == '0'
            #if_cond cnt >= 2
                load    d, w[2]
                or      d, 0x20
                #if_cond d == 'b'
                    move    base, 2
                    move    i, 3
                #else_cond
                    move    base, 8
                    move    i, 2
                #end_cond
            #end_cond
        #end_cond
    #end_cond
    move    v, 0
    move    d, 0
    #while  i <= cnt
        load    c, w[i]
        #call   hex_val(c)
        move    d, r0
        #if_cond d < 0
            #break
        #end_cond
        #if_cond d >= base
            #if_cond base != 10
                #call   error_tok(t, "invalid digit in integer constant")
            #end_cond
            #break
        #end_cond
        mult    v, base
        add     v, d
        add     i, 1
    #end_while
    // suffix
    move    suf, 0
    #while  i <= cnt
        load    c, w[i]
        or      c, 0x20
        #if_cond c == 'u'
            or      suf, IS_UNSIGNED
        #else_cond
            #if_cond c == 'l'
                or      suf, IS_LONG
            #else_cond
                #call   error_tok(t, "invalid suffix on integer constant")
            #end_cond
        #end_cond
        add     i, 1
    #end_while
    store   v, t[TK_VAL]
    store   suf, t[TK_AUX]
    move    r0, TK_INT
    store   r0, t[TK_KIND]
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// lx_new_tok(kind, flags, start) -> token; TEXT = src[start .. LX_POS-1]
//                                   (TEXT left 0 for punctuators)
///////////////////////////////////////////////////////////////////////////////
#def_func lx_new_tok(kind, flags, start)
    #var    t, k, s
    #call   zalloc(TK_SIZE)
    move    t, r0
    load    k, kind
    store   k, t[TK_KIND]
    load    r1, flags
    store   r1, t[TK_FLAGS]
    load    r1, LX_LINE
    store   r1, t[TK_LINE]
    load    r1, LX_FILE
    store   r1, t[TK_FILE]
    #if_cond k < TK_PUNCT_BASE
        load    s, start
        load    r2, LX_POS
        sub     r2, s
        load    r1, LX_SRC
        #call   w_sub(r1, s, r2)
        store   r0, t[TK_TEXT]
    #end_cond
    #return t
#end_func

///////////////////////////////////////////////////////////////////////////////
// lx_punct() -> punctuator kind at LX_POS (LX_POS advanced) or 0
///////////////////////////////////////////////////////////////////////////////
#def_func lx_punct()
    #var    tbl, cnt, i, w, wl, j, c, d, pos, src, ok, lim
    load    tbl, LX_PUNCTS
    load    src, LX_SRC
    load    pos, LX_POS
    load    lim, LX_LEN
    load    c, src[pos]
    #macro  VLEN(cnt, tbl)
    #for    1, i <= cnt, 1
        #macro  VGET(w, tbl, i)
        load    d, w[1]
        #if_cond d == c
            load    wl, w[0]
            move    ok, 1
            move    j, 2
            #while  j <= wl
                add     r1, pos, j
                sub     r1, 1
                #if_cond r1 > lim
                    move    ok, 0
                    #break
                #end_cond
                load    d, src[r1]
                load    r2, w[j]
                #if_cond d != r2
                    move    ok, 0
                    #break
                #end_cond
                add     j, 1
            #end_while
            #if_cond ok != 0
                add     pos, wl
                store   pos, LX_POS
                add     r0, i, TK_PUNCT_BASE
                sub     r0, 1
                #return r0
            #end_cond
        #end_cond
    #end_for
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// lex_file(src, file_entry) -> vector of tokens
///////////////////////////////////////////////////////////////////////////////
#def_func lex_file(srcp, filep)
    #var    src, cnt, toks, c, d, bol, space, start, flags, kind, t, e, pos
    load    src, srcp
    store   src, LX_SRC
    load    cnt, src[0]
    store   cnt, LX_LEN
    load    r0, filep
    store   r0, LX_FILE
    move    r0, 1
    store   r0, LX_LINE
    store   r0, LX_POS
    #call   vec_new()
    move    toks, r0
    move    bol, 1
    move    space, 0
    #while  TRUE
        load    pos, LX_POS
        #if_cond pos > cnt
            #break
        #end_cond
        load    c, src[pos]
        // ---- whitespace and newlines
        #if_cond c == '\n'
            load    r0, LX_LINE
            add     r0, 1
            store   r0, LX_LINE
            add     pos, 1
            store   pos, LX_POS
            move    bol, 1
            move    space, 0
            #continue
        #end_cond
        #if_cond c <= ' '
            add     pos, 1
            store   pos, LX_POS
            move    space, 1
            #continue
        #end_cond
        #call   lx_peekc(1)
        move    d, r0
        // ---- line splice between tokens
        #if_cond c == '\\'
            #if_cond d == '\n'
                load    r0, LX_LINE
                add     r0, 1
                store   r0, LX_LINE
                add     pos, 2
                store   pos, LX_POS
                move    space, 1
                #continue
            #end_cond
        #end_cond
        // ---- comments
        #if_cond c == '/'
            #if_cond d == '/'
                #while  pos <= cnt
                    load    c, src[pos]
                    #if_cond c == '\n'
                        #break
                    #end_cond
                    add     pos, 1
                #end_while
                store   pos, LX_POS
                move    space, 1
                #continue
            #end_cond
            #if_cond d == '*'
                add     pos, 2
                #while  TRUE
                    #if_cond pos >= cnt
                        #call   lx_error("unterminated comment")
                    #end_cond
                    load    c, src[pos]
                    #if_cond c == '\n'
                        load    r0, LX_LINE
                        add     r0, 1
                        store   r0, LX_LINE
                    #end_cond
                    #if_cond c == '*'
                        add     r1, pos, 1
                        load    d, src[r1]
                        #if_cond d == '/'
                            add     pos, 2
                            #break
                        #end_cond
                    #end_cond
                    add     pos, 1
                #end_while
                store   pos, LX_POS
                move    space, 1
                #continue
            #end_cond
        #end_cond
        // ---- start of a token
        move    flags, 0
        #if_cond bol != 0
            or      flags, TF_BOL
        #end_cond
        #if_cond space != 0
            or      flags, TF_SPACE
        #end_cond
        move    bol, 0
        move    space, 0
        move    start, pos
        // ---- pp-number
        move    kind, 0
        #call   is_digit(c)
        #if_cond r0 != 0
            move    kind, TK_PPNUM
        #else_cond
            #if_cond c == '.'
                #call   is_digit(d)
                #if_cond r0 != 0
                    move    kind, TK_PPNUM
                #end_cond
            #end_cond
        #end_cond
        #if_cond kind == TK_PPNUM
            add     pos, 1
            #while  pos <= cnt
                load    c, src[pos]
                or      d, c, 0x20
                #if_cond d == 'e'
                    add     r1, pos, 1
                    load    r1, src[r1]
                    #if_cond r1 == '+'
                        add     pos, 2
                        #continue
                    #end_cond
                    #if_cond r1 == '-'
                        add     pos, 2
                        #continue
                    #end_cond
                #end_cond
                #if_cond d == 'p'
                    add     r1, pos, 1
                    load    r1, src[r1]
                    #if_cond r1 == '+'
                        add     pos, 2
                        #continue
                    #end_cond
                    #if_cond r1 == '-'
                        add     pos, 2
                        #continue
                    #end_cond
                #end_cond
                #if_cond c == '.'
                    add     pos, 1
                    #continue
                #end_cond
                #call   is_ident_char(c)
                #if_cond r0 == 0
                    #break
                #end_cond
                add     pos, 1
            #end_while
            store   pos, LX_POS
            #call   lx_new_tok(TK_PPNUM, flags, start)
            move    t, r0
            #call   convert_num(t)
            #call   vec_push(toks, t)
            #continue
        #end_cond
        // ---- identifier (or string/char prefix)
        #call   is_ident_start(c)
        #if_cond r0 != 0
            add     pos, 1
            #while  pos <= cnt
                load    c, src[pos]
                #call   is_ident_char(c)
                #if_cond r0 == 0
                    #break
                #end_cond
                add     pos, 1
            #end_while
            store   pos, LX_POS
            // encoding prefixes L"..", u"..", U"..", u8".." and L'x'
            #if_cond c == '"'
                sub     r1, pos, start
                #if_cond r1 <= 2
                    load    d, src[start]
                    #if_cond d == 'L'
                        move    kind, TK_STR
                    #end_cond
                    #if_cond d == 'u'
                        move    kind, TK_STR
                    #end_cond
                    #if_cond d == 'U'
                        move    kind, TK_STR
                    #end_cond
                #end_cond
            #end_cond
            #if_cond c == '\''
                sub     r1, pos, start
                #if_cond r1 == 1
                    load    d, src[start]
                    #if_cond d == 'L'
                        move    kind, TK_CHAR
                    #end_cond
                    #if_cond d == 'u'
                        move    kind, TK_CHAR
                    #end_cond
                    #if_cond d == 'U'
                        move    kind, TK_CHAR
                    #end_cond
                #end_cond
            #end_cond
            #if_cond kind == 0
            #call   lx_new_tok(TK_IDENT, flags, start)
            move    t, r0
            sub     r2, pos, start
            #call   intern(src, start, r2)
            store   r0, t[TK_VAL]
            #call   vec_push(toks, t)
            #continue
            #end_cond
        #end_cond
        // ---- string literal
        #if_cond c == '"'
            move    kind, TK_STR
        #end_cond
        #if_cond c == '\''
            move    kind, TK_CHAR
        #end_cond
        #if_cond kind == TK_STR
            #call   lx_string()
            move    e, r0
            #call   lx_new_tok(TK_STR, flags, start)
            move    t, r0
            store   e, t[TK_VAL]
            #call   vec_push(toks, t)
            #continue
        #end_cond
        // ---- character constant
        #if_cond kind == TK_CHAR
            #call   lx_char()
            move    e, r0
            #call   lx_new_tok(TK_CHAR, flags, start)
            move    t, r0
            store   e, t[TK_VAL]
            #call   vec_push(toks, t)
            #continue
        #end_cond
        // ---- punctuator
        #call   lx_punct()
        move    kind, r0
        #if_cond kind == 0
            #call   lx_error("invalid character in source")
        #end_cond
        #call   lx_new_tok(kind, flags, start)
        #call   vec_push(toks, r0)
    #end_while
    #return toks
#end_func

///////////////////////////////////////////////////////////////////////////////
// tok_spelling(tok) -> wstr spelling of a token
///////////////////////////////////////////////////////////////////////////////
#def_func tok_spelling(tk)
    #var    t, k, e
    load    t, tk
    load    k, t[TK_KIND]
    #if_cond k >= KW_BASE
        load    e, t[TK_VAL]
        load    r0, e[E_W]
        #return r0
    #end_cond
    #if_cond k >= TK_PUNCT_BASE
        sub     k, TK_PUNCT_BASE
        add     k, 1
        load    r1, LX_PUNCTS
        #macro  VGET(r0, r1, k)
        #return r0
    #end_cond
    #if_cond k == TK_EOF
        #call   w_from_p("<end of file>")
        #return r0
    #end_cond
    load    r0, t[TK_TEXT]
#end_func

///////////////////////////////////////////////////////////////////////////////
// tok_p(tok) -> packed spelling of a token (heap)
///////////////////////////////////////////////////////////////////////////////
#def_func tok_p(tk)
    load    r1, tk
    #call   tok_spelling(r1)
    move    r1, r0
    int     iFROM_CODEPOINTS
#end_func

LEXER_ASM_END:
    nop
