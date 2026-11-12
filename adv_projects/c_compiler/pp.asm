///////////////////////////////////////////////////////////////////////////////
// pp.asm
//
// Token-level C preprocessor.
//
// All pending input lives on one work stack of tokens (PP_STACK, top = last
// element).  Including a file pushes a file-end marker and the file's tokens
// in reverse order; expanding a macro pushes a macro-end marker and the
// replacement tokens.  A macro is disabled (E_DIS) while its replacement is
// being read and re-enabled when its marker is popped; identifiers met while
// their macro is disabled are painted TF_NOEXP and never expanded again.
//
// Supported: #include "file" / <file>, object-like and function-like
// #define (including variadic macros, # and ##), #undef, #if, #ifdef,
// #ifndef, #elif, #else, #endif, #error; #pragma, #line and #warning are
// accepted and ignored.  <standard headers> are built in: when no such file
// exists the directive is ignored (the prelude declares the library).
//
//   pp_init()                  set up state
//   pp_run(tokens) -> vector   fully preprocessed tokens, ending in TK_EOF
///////////////////////////////////////////////////////////////////////////////

#global PP_STACK:   .dci 0          // work stack of tokens
#global PP_COND:    .dci 0          // conditional stack
#global PP_DEPTH:   .dci 0          // include depth
#global PP_RAW:     .dci 0          // >0 while expanding a token list only
#global PP_LASTTOK: .dci 0          // last token read from the stack

#define MK_MACRO    0               // marker AUX values
#define MK_FILE     1
#define MK_SENT     2

// conditional states (low 2 bits); bit 2 = parent active
#define CS_ACTIVE   0               // in the taken branch
#define CS_WAIT     1               // no branch taken yet
#define CS_DONE     2               // a branch was already taken
#define CS_PARENT   4

#define PP_MAX_DEPTH 32

PP_ASM_START:
    jump    PP_ASM_END

///////////////////////////////////////////////////////////////////////////////
// token helpers
///////////////////////////////////////////////////////////////////////////////
#def_func tok_copy(tk)
    #var    t, cpy
    load    t, tk
    #call   zalloc(TK_SIZE)
    move    cpy, r0
    move    r1, cpy
    move    r2, t
    move    r3, TK_SIZE
    int     iMEMMOVE
    #return cpy
#end_func

// tok_new(kind, val, text_wstr, like_tok) -> token positioned like like_tok
#def_func tok_new(kind, val, text, like)
    #var    t, l
    #call   zalloc(TK_SIZE)
    move    t, r0
    load    r1, kind
    store   r1, t[TK_KIND]
    load    r1, val
    store   r1, t[TK_VAL]
    load    r1, text
    store   r1, t[TK_TEXT]
    load    l, like
    #if_cond l != 0
        load    r1, l[TK_LINE]
        store   r1, t[TK_LINE]
        load    r1, l[TK_FILE]
        store   r1, t[TK_FILE]
        load    r1, l[TK_FLAGS]
        and     r1, TF_SPACE
        store   r1, t[TK_FLAGS]
    #end_cond
    #return t
#end_func

// tok_new_int(value, like_tok)
#def_func tok_new_int(val, like)
    #var    v, s
    load    v, val
    #call   vec_new()
    move    s, r0
    #call   sb_add_int(s, v)
    #call   sb_to_w(s)
    load    r1, like
    #call   tok_new(TK_INT, v, r0, r1)
#end_func

// tok_new_str(content_wstr, like_tok): TEXT is the quoted, escaped spelling
#def_func tok_new_str(content, like)
    #var    w, s, cnt, i, c
    load    w, content
    #call   vec_new()
    move    s, r0
    #call   vec_push(s, '"')
    load    cnt, w[0]
    #for    1, i <= cnt, 1
        load    c, w[i]
        #if_cond c == '"'
            #call   vec_push(s, '\\')
        #end_cond
        #if_cond c == '\\'
            #call   vec_push(s, '\\')
        #end_cond
        #if_cond c == '\n'
            #call   vec_push(s, '\\')
            move    c, 'n'
        #end_cond
        #call   vec_push(s, c)
    #end_for
    #call   vec_push(s, '"')
    #call   sb_to_w(s)
    load    r1, like
    #call   tok_new(TK_STR, w, r0, r1)
#end_func

// is_marker(tok, aux) -> 1 if tok is a TK_MEND with the given AUX
#def_func is_marker(tk, aux)
    #var    t
    load    t, tk
    load    r0, t[TK_KIND]
    #if_cond r0 != TK_MEND
        #return 0
    #end_cond
    load    r0, t[TK_AUX]
    load    r1, aux
    #if_cond r0 == r1
        #return 1
    #end_cond
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_init()
///////////////////////////////////////////////////////////////////////////////
#def_func pp_init()
    #call   vec_new()
    store   r0, PP_STACK
    #call   vec_new()
    store   r0, PP_COND
    move    r0, 0
    store   r0, PP_DEPTH
    store   r0, PP_RAW
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_push_list(vec, from, to): push vec[from..to] so that vec[from] is read
//                              first
///////////////////////////////////////////////////////////////////////////////
#def_func pp_push_list(vp, fromp, top)
    #var    v, i, lo, stk, t
    load    v, vp
    load    lo, fromp
    load    i, top
    load    stk, PP_STACK
    #while  i >= lo
        #macro  VGET(t, v, i)
        #call   vec_push(stk, t)
        sub     i, 1
    #end_while
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_push_marker(aux, val, like)
///////////////////////////////////////////////////////////////////////////////
#def_func pp_push_marker(aux, val, like)
    #var    t
    load    r1, val
    load    r2, like
    #call   tok_new(TK_MEND, r1, 0, r2)
    move    t, r0
    load    r1, aux
    store   r1, t[TK_AUX]
    load    r1, PP_STACK
    #call   vec_push(r1, t)
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_include_tokens(toks): push a file's tokens with a file-end marker
///////////////////////////////////////////////////////////////////////////////
#def_func pp_include_tokens(toksp)
    #var    toks, cnt
    load    toks, toksp
    load    r0, PP_DEPTH
    add     r0, 1
    store   r0, PP_DEPTH
    #call   pp_push_marker(MK_FILE, 0, 0)
    #macro  VLEN(cnt, toks)
    #call   pp_push_list(toks, 1, cnt)
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_pop() -> raw next token (0 if the stack is empty).
//             Macro-end markers are consumed here (re-enabling the macro);
//             file-end and sentinel markers are returned to the caller.
///////////////////////////////////////////////////////////////////////////////
#def_func pp_pop()
    #var    t, e
    #while  TRUE
        load    r1, PP_STACK
        #call   vec_pop(r1)
        move    t, r0
        #if_cond t == 0
            #return 0
        #end_cond
        #call   is_marker(t, MK_MACRO)
        #if_cond r0 != 0
            load    e, t[TK_VAL]
            move    r1, 0
            store   r1, e[E_DIS]
            #continue
        #end_cond
        load    r0, t[TK_KIND]
        #if_cond r0 != TK_MEND
            store   t, PP_LASTTOK
        #end_cond
        #return t
    #end_while
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_peek() -> next token that is not a macro-end marker (not removed), or 0
///////////////////////////////////////////////////////////////////////////////
#def_func pp_peek()
    #var    stk, a, i, t
    load    stk, PP_STACK
    load    a, stk[VEC_ARR]
    load    i, a[0]
    #while  i > 0
        load    t, a[i]
        #call   is_marker(t, MK_MACRO)
        #if_cond r0 == 0
            #return t
        #end_cond
        sub     i, 1
    #end_while
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_line_rest() -> vec of the remaining tokens on the current directive line
///////////////////////////////////////////////////////////////////////////////
#def_func pp_line_rest()
    #var    v, t, stk
    #call   vec_new()
    move    v, r0
    load    stk, PP_STACK
    #while  TRUE
        #call   vec_last(stk)
        move    t, r0
        #if_cond t == 0
            #break
        #end_cond
        load    r0, t[TK_KIND]
        #if_cond r0 == TK_MEND
            #break
        #end_cond
        load    r0, t[TK_FLAGS]
        and     r0, TF_BOL
        #if_cond r0 != 0
            #break
        #end_cond
        #call   vec_pop(stk)
        #call   vec_push(v, t)
    #end_while
    #return v
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_active() -> 1 if tokens are currently being kept
///////////////////////////////////////////////////////////////////////////////
#def_func pp_active()
    #var    c
    load    r1, PP_COND
    #call   vec_last(r1)
    move    c, r0
    load    r1, PP_COND
    #call   vec_len(r1)
    #if_cond r0 == 0
        #return 1
    #end_cond
    and     c, 3
    #if_cond c == CS_ACTIVE
        #return 1
    #end_cond
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_find_param(macro, entry) -> 1-based parameter index or 0
///////////////////////////////////////////////////////////////////////////////
#def_func pp_find_param(mp, ep)
    #var    m, e, ps, cnt, i, x
    load    m, mp
    load    e, ep
    load    r0, m[M_FUNC]
    #if_cond r0 == 0
        #return 0
    #end_cond
    load    ps, m[M_PARAMS]
    #macro  VLEN(cnt, ps)
    #for    1, i <= cnt, 1
        #macro  VGET(x, ps, i)
        #if_cond x == e
            #return i
        #end_cond
    #end_for
    #return 0
#end_func

// tok_param(macro, tok) -> parameter index if tok is a parameter name
#def_func tok_param(mp, tk)
    #var    t
    load    t, tk
    #if_cond t == 0
        #return 0
    #end_cond
    load    r0, t[TK_KIND]
    #if_cond r0 != TK_IDENT
        #return 0
    #end_cond
    load    r1, mp
    load    r2, t[TK_VAL]
    #call   pp_find_param(r1, r2)
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_stringize(tokens, like) -> TK_STR token spelling the argument tokens
///////////////////////////////////////////////////////////////////////////////
#def_func pp_stringize(vp, like)
    #var    v, cnt, i, t, s, w, k
    load    v, vp
    #call   vec_new()
    move    s, r0
    #macro  VLEN(cnt, v)
    #for    1, i <= cnt, 1
        #macro  VGET(t, v, i)
        #if_cond i > 1
            load    r0, t[TK_FLAGS]
            and     r0, TF_SPACE
            #if_cond r0 != 0
                #call   vec_push(s, ' ')
            #end_cond
        #end_cond
        #call   tok_spelling(t)
        #call   sb_add_w(s, r0)
    #end_for
    #call   sb_to_w(s)
    move    w, r0
    load    r1, like
    #call   tok_new_str(w, r1)
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_paste(left, right) -> single token made of both spellings
///////////////////////////////////////////////////////////////////////////////
#def_func pp_paste(lp, rp)
    #var    l, r, s, w, toks, t, cnt
    load    l, lp
    load    r, rp
    #call   vec_new()
    move    s, r0
    #call   tok_spelling(l)
    #call   sb_add_w(s, r0)
    #call   tok_spelling(r)
    #call   sb_add_w(s, r0)
    #call   vec_push(s, '\n')
    #call   sb_to_w(s)
    move    w, r0
    load    r1, l[TK_FILE]
    #call   lex_file(w, r1)
    move    toks, r0
    #macro  VLEN(cnt, toks)
    #if_cond cnt != 1
        #call   error_tok(l, "pasting does not give a valid preprocessing token")
    #end_cond
    #macro  VGET(t, toks, 1)
    load    r1, l[TK_LINE]
    store   r1, t[TK_LINE]
    load    r1, l[TK_FLAGS]
    and     r1, TF_SPACE
    store   r1, t[TK_FLAGS]
    #return t
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_expand_list(tokens) -> new vec: the tokens fully macro-expanded
///////////////////////////////////////////////////////////////////////////////
#def_func pp_expand_list(vp)
    #var    v, cnt, outv, t
    load    v, vp
    #call   vec_new()
    move    outv, r0
    load    r0, PP_RAW
    add     r0, 1
    store   r0, PP_RAW
    #call   pp_push_marker(MK_SENT, 0, 0)
    #macro  VLEN(cnt, v)
    #call   pp_push_list(v, 1, cnt)
    #while  TRUE
        #call   pp_next()
        move    t, r0
        #call   is_marker(t, MK_SENT)
        #if_cond r0 != 0
            #break
        #end_cond
        #call   vec_push(outv, t)
    #end_while
    load    r0, PP_RAW
    sub     r0, 1
    store   r0, PP_RAW
    #return outv
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_collect_args(macro, name_tok) -> vec of raw argument vecs
//      The '(' has already been verified to be next.
///////////////////////////////////////////////////////////////////////////////
#def_func pp_collect_args(mp, ntk)
    #var    m, nt, margs, curv, depth, t, k, nparams, var, e
    load    m, mp
    load    nt, ntk
    load    r1, m[M_PARAMS]
    #call   vec_len(r1)
    move    nparams, r0
    load    var, m[M_VARIADIC]
    #call   pp_pop()                    // the '('
    #call   vec_new()
    move    margs, r0
    #call   vec_new()
    move    curv, r0
    move    depth, 0
    #while  TRUE
        #call   pp_pop()
        move    t, r0
        #if_cond t == 0
            #call   error_tok(nt, "unterminated argument list invoking macro")
        #end_cond
        load    k, t[TK_KIND]
        #if_cond k == TK_MEND
            #call   error_tok(nt, "unterminated argument list invoking macro")
        #end_cond
        #if_cond k == TK_LPAREN
            add     depth, 1
        #end_cond
        #if_cond k == TK_RPAREN
            #if_cond depth == 0
                #break
            #end_cond
            sub     depth, 1
        #end_cond
        #if_cond k == TK_COMMA
            #if_cond depth == 0
                // the variadic parameter swallows the remaining commas
                #call   vec_len(margs)
                add     r0, 1
                #if_cond var != 0
                    #if_cond r0 >= nparams
                        #call   vec_push(curv, t)
                        #continue
                    #end_cond
                #end_cond
                #call   vec_push(margs, curv)
                #call   vec_new()
                move    curv, r0
                #continue
            #end_cond
        #end_cond
        #call   vec_push(curv, t)
    #end_while
    #call   vec_push(margs, curv)
    // f() for a one-parameter macro passes one empty argument; for a macro
    // with no parameters it passes none.
    #call   vec_len(margs)
    #if_cond r0 == 1
        #if_cond nparams == 0
            #call   vec_len(curv)
            #if_cond r0 == 0
                #call   vec_pop(margs)
            #end_cond
        #end_cond
    #end_cond
    // a variadic macro may omit the variable arguments entirely
    #call   vec_len(margs)
    add     r0, 1
    #if_cond var != 0
        #if_cond r0 == nparams
            #call   vec_new()
            #call   vec_push(margs, r0)
        #end_cond
    #end_cond
    #call   vec_len(margs)
    #if_cond r0 != nparams
        load    e, nt[TK_VAL]
        #call   error_tok2(nt, "wrong number of arguments to macro", e)
    #end_cond
    #return margs
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_append_copies(out, tokens, like, first_flags_from_like)
///////////////////////////////////////////////////////////////////////////////
#def_func pp_append_copies(outp, vp, like)
    #var    outv, v, cnt, i, t, l
    load    outv, outp
    load    v, vp
    load    l, like
    #macro  VLEN(cnt, v)
    #for    1, i <= cnt, 1
        #macro  VGET(t, v, i)
        #call   tok_copy(t)
        move    t, r0
        load    r1, l[TK_LINE]
        store   r1, t[TK_LINE]
        load    r1, l[TK_FILE]
        store   r1, t[TK_FILE]
        load    r1, t[TK_FLAGS]
        and     r1, -2
        store   r1, t[TK_FLAGS]
        #call   vec_push(outv, t)
    #end_for
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_expand_macro(entry, name_tok) -> 1 if expanded (tokens pushed), else 0
///////////////////////////////////////////////////////////////////////////////
#def_func pp_expand_macro(ep, ntk)
    #var    e, nt, m, margs, body, outv, cnt, i, t, k, pidx, nxt, arg, pend, eargs, ppi
    load    e, ep
    load    nt, ntk
    load    m, e[E_MACRO]
    move    margs, 0
    load    r0, m[M_FUNC]
    #if_cond r0 != 0
        #call   pp_peek()
        #if_cond r0 == 0
            #return 0
        #end_cond
        load    r0, r0[TK_KIND]
        #if_cond r0 != TK_LPAREN
            #return 0
        #end_cond
        #call   pp_collect_args(m, nt)
        move    margs, r0
    #end_cond
    // expanded arguments are computed lazily
    #call   vec_new()
    move    eargs, r0
    #if_cond margs != 0
        #macro  VLEN(cnt, margs)
        #for    1, i <= cnt, 1
            #call   vec_push(eargs, 0)
        #end_for
    #end_cond
    load    body, m[M_BODY]
    #call   vec_new()
    move    outv, r0
    move    pend, 0                     // last appended item was an empty arg
    #macro  VLEN(cnt, body)
    move    i, 1
    #while  i <= cnt
        #macro  VGET(t, body, i)
        load    k, t[TK_KIND]
        move    nxt, 0
        #if_cond i < cnt
            add     r1, i, 1
            #macro  VGET(nxt, body, r1)
        #end_cond
        // ---- # param
        #if_cond k == TK_HASH
            load    r0, m[M_FUNC]
            #if_cond r0 != 0
                #call   tok_param(m, nxt)
                move    pidx, r0
                #if_cond pidx == 0
                    #call   error_tok(t, "'#' is not followed by a macro parameter")
                #end_cond
                #macro  VGET(arg, margs, pidx)
                #call   pp_stringize(arg, t)
                move    t, r0
                load    r1, nt[TK_LINE]
                store   r1, t[TK_LINE]
                load    r1, nt[TK_FILE]
                store   r1, t[TK_FILE]
                #call   vec_push(outv, t)
                move    pend, 0
                add     i, 2
                #continue
            #end_cond
        #end_cond
        // ---- ## rhs
        #if_cond k == TK_HASHHASH
            #if_cond nxt == 0
                #call   error_tok(t, "'##' cannot appear at either end of a macro expansion")
            #end_cond
            #call   tok_param(m, nxt)
            move    pidx, r0
            #call   vec_new()
            move    arg, r0
            #if_cond pidx != 0
                #macro  VGET(r1, margs, pidx)
                #call   pp_append_copies(arg, r1, nt)
            #else_cond
                #call   vec_new()
                move    ppi, r0
                #call   vec_push(ppi, nxt)
                #call   pp_append_copies(arg, ppi, nt)
            #end_cond
            #call   vec_len(arg)
            #if_cond r0 == 0
                // GNU: , ## __VA_ARGS__ with empty arguments drops the comma
                load    r1, nxt[TK_VAL]
                load    r1, r1[E_SPEC]
                #if_cond r1 == SPEC_VAARGS
                    #call   vec_last(outv)
                    #if_cond r0 != 0
                        load    r0, r0[TK_KIND]
                        #if_cond r0 == TK_COMMA
                            #call   vec_pop(outv)
                        #end_cond
                    #end_cond
                #end_cond
                add     i, 2
                #continue
            #end_cond
            #if_cond pend != 0
                #call   pp_append_copies(outv, arg, nt)
                move    pend, 0
                add     i, 2
                #continue
            #end_cond
            // GNU: , ## __VA_ARGS__ with arguments keeps the comma, no paste
            load    r1, nxt[TK_VAL]
            load    r1, r1[E_SPEC]
            #if_cond r1 == SPEC_VAARGS
                #call   vec_last(outv)
                load    r0, r0[TK_KIND]
                #if_cond r0 == TK_COMMA
                    #call   pp_append_copies(outv, arg, nt)
                    add     i, 2
                    #continue
                #end_cond
            #end_cond
            #call   vec_pop(outv)
            move    r1, r0
            #macro  VGET(r2, arg, 1)
            #call   pp_paste(r1, r2)
            #call   vec_push(outv, r0)
            #call   vec_len(arg)
            move    k, r0
            move    pidx, 2
            #while  pidx <= k
                #macro  VGET(r1, arg, pidx)
                #call   tok_copy(r1)
                #call   vec_push(outv, r0)
                add     pidx, 1
            #end_while
            move    pend, 0
            add     i, 2
            #continue
        #end_cond
        // ---- parameter
        #call   tok_param(m, t)
        move    pidx, r0
        #if_cond pidx != 0
            move    ppi, 0
            #if_cond nxt != 0
                load    r0, nxt[TK_KIND]
                #if_cond r0 == TK_HASHHASH
                    move    ppi, 1
                #end_cond
            #end_cond
            #if_cond ppi != 0
                // operand of ## : use the raw argument
                #macro  VGET(arg, margs, pidx)
            #else_cond
                #macro  VGET(arg, eargs, pidx)
                #if_cond arg == 0
                    #macro  VGET(r1, margs, pidx)
                    #call   pp_expand_list(r1)
                    move    arg, r0
                    #macro  VSET(eargs, pidx, arg)
                #end_cond
            #end_cond
            #call   vec_len(outv)
            move    k, r0
            #call   pp_append_copies(outv, arg, nt)
            // the first token takes the spacing of the parameter name
            #call   vec_len(arg)
            #if_cond r0 > 0
                add     k, 1
                #macro  VGET(r1, outv, k)
                load    r2, t[TK_FLAGS]
                and     r2, TF_SPACE
                load    r3, r1[TK_FLAGS]
                and     r3, -3
                or      r3, r2
                store   r3, r1[TK_FLAGS]
                move    pend, 0
            #else_cond
                move    pend, 1
            #end_cond
            add     i, 1
            #continue
        #end_cond
        // ---- ordinary token
        #call   vec_new()
        move    arg, r0
        #call   vec_push(arg, t)
        #call   pp_append_copies(outv, arg, nt)
        move    pend, 0
        add     i, 1
    #end_while
    // first token takes the spacing of the macro name
    #call   vec_len(outv)
    #if_cond r0 > 0
        #macro  VGET(t, outv, 1)
        load    r1, nt[TK_FLAGS]
        and     r1, TF_SPACE
        load    r2, t[TK_FLAGS]
        and     r2, -3
        or      r2, r1
        store   r2, t[TK_FLAGS]
    #end_cond
    move    r1, 1
    store   r1, e[E_DIS]
    #call   pp_push_marker(MK_MACRO, e, nt)
    #macro  VLEN(cnt, outv)
    #call   pp_push_list(outv, 1, cnt)
    #return 1
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_next() -> next fully preprocessed token; a TK_EOF token at the end.
//              In raw mode (PP_RAW > 0) the sentinel marker is returned.
///////////////////////////////////////////////////////////////////////////////
#def_func pp_next()
    #var    t, k, e, m, raw
    #while  TRUE
        #call   pp_pop()
        move    t, r0
        #if_cond t == 0
            load    r1, PP_COND
            #call   vec_len(r1)
            #if_cond r0 != 0
                load    r1, PP_LASTTOK
                #call   error_tok(r1, "unterminated conditional directive")
            #end_cond
            load    r1, PP_LASTTOK
            #call   tok_new(TK_EOF, 0, 0, r1)
            #return r0
        #end_cond
        load    k, t[TK_KIND]
        load    raw, PP_RAW
        #if_cond k == TK_MEND
            #call   is_marker(t, MK_SENT)
            #if_cond r0 != 0
                #return t
            #end_cond
            // end of an included file
            load    r0, PP_DEPTH
            sub     r0, 1
            store   r0, PP_DEPTH
            #continue
        #end_cond
        #if_cond raw == 0
            #if_cond k == TK_HASH
                load    r0, t[TK_FLAGS]
                and     r0, TF_BOL
                #if_cond r0 != 0
                    #call   pp_directive(t)
                    #continue
                #end_cond
            #end_cond
            #call   pp_active()
            #if_cond r0 == 0
                #continue
            #end_cond
        #end_cond
        #if_cond k == TK_IDENT
            load    e, t[TK_VAL]
            load    r0, e[E_SPEC]
            #if_cond r0 == SPEC_LINE
                load    r1, t[TK_LINE]
                #call   tok_new_int(r1, t)
                #return r0
            #end_cond
            #if_cond r0 == SPEC_FILE
                load    r1, t[TK_FILE]
                load    r1, r1[E_W]
                #call   tok_new_str(r1, t)
                #return r0
            #end_cond
            load    m, e[E_MACRO]
            #if_cond m != 0
                load    r0, t[TK_FLAGS]
                and     r0, TF_NOEXP
                #if_cond r0 != 0
                    #return t
                #end_cond
                load    r0, e[E_DIS]
                #if_cond r0 != 0
                    load    r0, t[TK_FLAGS]
                    or      r0, TF_NOEXP
                    store   r0, t[TK_FLAGS]
                    #return t
                #end_cond
                #call   pp_expand_macro(e, t)
                #if_cond r0 != 0
                    #continue
                #end_cond
            #end_cond
        #end_cond
        #return t
    #end_while
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_directive(hash_tok): process one directive line
///////////////////////////////////////////////////////////////////////////////
#def_func pp_directive(htk)
    #var    ht, line, cnt, nt, e, spec, kw, active, top, v
    load    ht, htk
    #call   pp_line_rest()
    move    line, r0
    #macro  VLEN(cnt, line)
    #if_cond cnt == 0
        #return 0                       // null directive
    #end_cond
    #macro  VGET(nt, line, 1)
    load    r0, nt[TK_KIND]
    #if_cond r0 != TK_IDENT
        #call   pp_active()
        #if_cond r0 != 0
            #call   error_tok(nt, "invalid preprocessing directive")
        #end_cond
        #return 0
    #end_cond
    load    e, nt[TK_VAL]
    load    spec, e[E_SPEC]
    load    kw, e[E_KW]
    #call   pp_active()
    move    active, r0
    // ---- conditionals are handled even in skipped regions
    #if_cond kw == KW_IF
        move    spec, SPEC_D_IF
    #end_cond
    #if_cond kw == KW_ELSE
        move    spec, SPEC_D_ELSE
    #end_cond
    #if_cond spec == SPEC_D_IF
        #call   pp_cond_open(line, spec, active)
        #return 0
    #end_cond
    #if_cond spec == SPEC_D_IFDEF
        #call   pp_cond_open(line, spec, active)
        #return 0
    #end_cond
    #if_cond spec == SPEC_D_IFNDEF
        #call   pp_cond_open(line, spec, active)
        #return 0
    #end_cond
    #if_cond spec == SPEC_D_ELIF
        #call   pp_cond_else(line, 1)
        #return 0
    #end_cond
    #if_cond spec == SPEC_D_ELSE
        #call   pp_cond_else(line, 0)
        #return 0
    #end_cond
    #if_cond spec == SPEC_D_ENDIF
        load    r1, PP_COND
        #call   vec_len(r1)
        #if_cond r0 == 0
            #call   error_tok(nt, "#endif without #if")
        #end_cond
        load    r1, PP_COND
        #call   vec_pop(r1)
        #return 0
    #end_cond
    #if_cond active == 0
        #return 0
    #end_cond
    // ---- active-only directives
    #if_cond spec == SPEC_D_DEFINE
        #call   pp_define(line)
        #return 0
    #end_cond
    #if_cond spec == SPEC_D_UNDEF
        #if_cond cnt < 2
            #call   error_tok(nt, "macro name missing")
        #end_cond
        #macro  VGET(r1, line, 2)
        load    r1, r1[TK_VAL]
        move    r2, 0
        store   r2, r1[E_MACRO]
        #return 0
    #end_cond
    #if_cond spec == SPEC_D_INCLUDE
        #call   pp_include(line)
        #return 0
    #end_cond
    #if_cond spec == SPEC_D_ERROR
        #call   vec_new()
        move    v, r0
        #call   sb_add_p(v, "#error")
        #macro  VLEN(cnt, line)
        move    top, 2
        #while  top <= cnt
            #call   vec_push(v, ' ')
            #macro  VGET(r1, line, top)
            #call   tok_spelling(r1)
            #call   sb_add_w(v, r0)
            add     top, 1
        #end_while
        #call   sb_to_p(v)
        #call   error_tok(nt, r0)
    #end_cond
    #if_cond spec == SPEC_D_PRAGMA
        #return 0
    #end_cond
    #if_cond spec == SPEC_D_LINE
        #return 0
    #end_cond
    #if_cond spec == SPEC_D_WARNING
        #return 0
    #end_cond
    #call   error_tok2(nt, "invalid preprocessing directive", e)

#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_cond_open(line, spec, active): #if / #ifdef / #ifndef
///////////////////////////////////////////////////////////////////////////////
#def_func pp_cond_open(linep, specp, activep)
    #var    line, spec, active, cnt, nt, st
    load    line, linep
    load    spec, specp
    load    active, activep
    #macro  VLEN(cnt, line)
    #macro  VGET(nt, line, 1)
    move    st, CS_DONE
    #if_cond active != 0
        #if_cond spec == SPEC_D_IF
            #call   pp_eval_line(line, 2)
        #else_cond
            #if_cond cnt < 2
                #call   error_tok(nt, "macro name missing")
            #end_cond
            #macro  VGET(r1, line, 2)
            load    r0, r1[TK_KIND]
            #if_cond r0 != TK_IDENT
                #call   error_tok(nt, "macro names must be identifiers")
            #end_cond
            load    r1, r1[TK_VAL]
            load    r0, r1[E_MACRO]
            #if_cond r0 != 0
                move    r0, 1
            #end_cond
            #if_cond spec == SPEC_D_IFNDEF
                xor     r0, 1
            #end_cond
        #end_cond
        move    st, CS_WAIT
        #if_cond r0 != 0
            move    st, CS_ACTIVE
        #end_cond
        or      st, CS_PARENT
    #end_cond
    load    r1, PP_COND
    #call   vec_push(r1, st)
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_cond_else(line, is_elif): #elif / #else
///////////////////////////////////////////////////////////////////////////////
#def_func pp_cond_else(linep, elifp)
    #var    line, iselif, nt, top, st
    load    line, linep
    load    iselif, elifp
    #macro  VGET(nt, line, 1)
    load    r1, PP_COND
    #call   vec_len(r1)
    #if_cond r0 == 0
        #if_cond iselif != 0
            #call   error_tok(nt, "#elif without #if")
        #end_cond
        #call   error_tok(nt, "#else without #if")
    #end_cond
    load    r1, PP_COND
    #call   vec_pop(r1)
    move    top, r0
    and     st, top, 3
    and     top, CS_PARENT
    #if_cond st == CS_WAIT
        #if_cond iselif != 0
            #call   pp_eval_line(line, 2)
            #if_cond r0 != 0
                move    st, CS_ACTIVE
            #end_cond
        #else_cond
            move    st, CS_ACTIVE
        #end_cond
    #else_cond
        move    st, CS_DONE
    #end_cond
    or      st, top
    load    r1, PP_COND
    #call   vec_push(r1, st)
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_define(line): line = [define, NAME, ...]
///////////////////////////////////////////////////////////////////////////////
#def_func pp_define(linep)
    #var    line, cnt, nt, e, m, i, t, k, params, body
    load    line, linep
    #macro  VLEN(cnt, line)
    #macro  VGET(t, line, 1)
    #if_cond cnt < 2
        #call   error_tok(t, "macro name missing")
    #end_cond
    #macro  VGET(nt, line, 2)
    load    r0, nt[TK_KIND]
    #if_cond r0 != TK_IDENT
        #call   error_tok(nt, "macro names must be identifiers")
    #end_cond
    load    e, nt[TK_VAL]
    load    r0, e[E_SPEC]
    #if_cond r0 == SPEC_DEFINED
        #call   error_tok(nt, "'defined' cannot be used as a macro name")
    #end_cond
    #call   zalloc(M_SIZE)
    move    m, r0
    #call   vec_new()
    move    params, r0
    store   params, m[M_PARAMS]
    move    i, 3
    #if_cond cnt >= 3
        #macro  VGET(t, line, 3)
        load    k, t[TK_KIND]
        load    r0, t[TK_FLAGS]
        and     r0, TF_SPACE
        #if_cond k == TK_LPAREN
            #if_cond r0 == 0
                // function-like macro
                move    r0, 1
                store   r0, m[M_FUNC]
                move    i, 4
                #macro  VGET(t, line, i)
                #if_cond i > cnt
                    #call   error_tok(nt, "missing ')' in macro parameter list")
                #end_cond
                load    k, t[TK_KIND]
                #if_cond k == TK_RPAREN
                    add     i, 1
                #else_cond
                    #while  TRUE
                        #if_cond i > cnt
                            #call   error_tok(nt, "missing ')' in macro parameter list")
                        #end_cond
                        #macro  VGET(t, line, i)
                        load    k, t[TK_KIND]
                        #if_cond k == TK_ELLIPSIS
                            move    r0, 1
                            store   r0, m[M_VARIADIC]
                            #call   intern_p("__VA_ARGS__")
                            #call   vec_push(params, r0)
                        #else_cond
                            #if_cond k != TK_IDENT
                                #call   error_tok(t, "invalid macro parameter")
                            #end_cond
                            load    r1, t[TK_VAL]
                            #call   vec_push(params, r1)
                        #end_cond
                        add     i, 1
                        #if_cond i > cnt
                            #call   error_tok(nt, "missing ')' in macro parameter list")
                        #end_cond
                        #macro  VGET(t, line, i)
                        load    k, t[TK_KIND]
                        add     i, 1
                        #if_cond k == TK_RPAREN
                            #break
                        #end_cond
                        #if_cond k != TK_COMMA
                            #call   error_tok(t, "expected ',' or ')' in macro parameter list")
                        #end_cond
                        load    r0, m[M_VARIADIC]
                        #if_cond r0 != 0
                            #call   error_tok(t, "'...' must be the last macro parameter")
                        #end_cond
                    #end_while
                #end_cond
            #end_cond
        #end_cond
    #end_cond
    #call   vec_new()
    move    body, r0
    store   body, m[M_BODY]
    #while  i <= cnt
        #macro  VGET(t, line, i)
        #call   vec_push(body, t)
        add     i, 1
    #end_while
    // the first body token never needs leading space
    #call   vec_len(body)
    #if_cond r0 > 0
        #macro  VGET(t, body, 1)
        load    r0, t[TK_FLAGS]
        and     r0, -3
        store   r0, t[TK_FLAGS]
    #end_cond
    store   m, e[E_MACRO]
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_include(line): line = [include, "file" | < tokens >]
///////////////////////////////////////////////////////////////////////////////
#def_func pp_include(linep)
    #var    line, cnt, it, t, k, name, sys, i, s, path, src, fe, dir
    load    line, linep
    #macro  VLEN(cnt, line)
    #macro  VGET(it, line, 1)
    #if_cond cnt < 2
        #call   error_tok(it, "#include expects \"FILENAME\" or <FILENAME>")
    #end_cond
    #macro  VGET(t, line, 2)
    load    k, t[TK_KIND]
    move    sys, 0
    #if_cond k == TK_STR
        load    r1, t[TK_VAL]
        #call   p_from_w(r1)
        move    name, r0
    #else_cond
        #if_cond k != TK_LT
            #call   error_tok(it, "#include expects \"FILENAME\" or <FILENAME>")
        #end_cond
        move    sys, 1
        #call   vec_new()
        move    s, r0
        move    i, 3
        #while  TRUE
            #if_cond i > cnt
                #call   error_tok(it, "missing terminating > character")
            #end_cond
            #macro  VGET(t, line, i)
            load    k, t[TK_KIND]
            #if_cond k == TK_GT
                #break
            #end_cond
            #call   tok_spelling(t)
            #call   sb_add_w(s, r0)
            add     i, 1
        #end_while
        #call   sb_to_p(s)
        move    name, r0
    #end_cond
    // search the including file's directory, then the main file's directory
    load    r1, it[TK_FILE]
    #call   ent_p(r1)
    #call   path_dir(r0)
    move    dir, r0
    #call   path_join(dir, name)
    move    path, r0
    #call   read_file(path)
    move    src, r0
    #if_cond src == 0
        load    r1, CC_INDIR
        #call   path_join(r1, name)
        move    path, r0
        #call   read_file(path)
        move    src, r0
    #end_cond
    #if_cond src == 0
        #if_cond sys != 0
            #return 0                   // built-in standard header
        #end_cond
        #call   intern_p(name)
        #call   error_tok2(it, "cannot open include file", r0)
    #end_cond
    load    r0, PP_DEPTH
    #if_cond r0 >= PP_MAX_DEPTH
        #call   error_tok(it, "#include nested too deeply")
    #end_cond
    #call   intern_p(path)
    move    fe, r0
    #call   lex_file(src, fe)
    #call   pp_include_tokens(r0)
#end_func

///////////////////////////////////////////////////////////////////////////////
// #if expression evaluation
///////////////////////////////////////////////////////////////////////////////
#global PE_TOKS:    .dci 0
#global PE_POS:     .dci 0
#global PE_LINE:    .dci 0          // directive token for error messages

// pp_eval_line(line, from) -> value of the #if expression in line[from..]
#def_func pp_eval_line(linep, fromp)
    #var    line, cnt, i, t, k, e, v, outv, nt, paren, val
    load    line, linep
    #macro  VLEN(cnt, line)
    #macro  VGET(t, line, 1)
    store   t, PE_LINE
    load    i, fromp
    #if_cond i > cnt
        #call   error_tok(t, "#if with no expression")
    #end_cond
    // replace defined X / defined(X) before macro expansion
    #call   vec_new()
    move    v, r0
    #while  i <= cnt
        #macro  VGET(t, line, i)
        load    k, t[TK_KIND]
        #if_cond k == TK_IDENT
            load    e, t[TK_VAL]
            load    r0, e[E_SPEC]
            #if_cond r0 == SPEC_DEFINED
                add     i, 1
                move    paren, 0
                #macro  VGET(nt, line, i)
                #if_cond i > cnt
                    #call   error_tok(t, "operator 'defined' requires an identifier")
                #end_cond
                load    r0, nt[TK_KIND]
                #if_cond r0 == TK_LPAREN
                    move    paren, 1
                    add     i, 1
                    #if_cond i > cnt
                        #call   error_tok(t, "operator 'defined' requires an identifier")
                    #end_cond
                    #macro  VGET(nt, line, i)
                #end_cond
                load    r0, nt[TK_KIND]
                #if_cond r0 != TK_IDENT
                    #call   error_tok(t, "operator 'defined' requires an identifier")
                #end_cond
                load    r0, nt[TK_VAL]
                load    val, r0[E_MACRO]
                #if_cond val != 0
                    move    val, 1
                #end_cond
                #if_cond paren != 0
                    add     i, 1
                    #macro  VGET(nt, line, i)
                    move    r0, 0
                    #if_cond i <= cnt
                        load    r0, nt[TK_KIND]
                    #end_cond
                    #if_cond r0 != TK_RPAREN
                        #call   error_tok(t, "missing ')' after 'defined'")
                    #end_cond
                #end_cond
                #call   tok_new_int(val, t)
                #call   vec_push(v, r0)
                add     i, 1
                #continue
            #end_cond
        #end_cond
        #call   vec_push(v, t)
        add     i, 1
    #end_while
    #call   pp_expand_list(v)
    move    v, r0
    // identifiers left over evaluate to 0 (true -> 1)
    #call   vec_new()
    move    outv, r0
    #macro  VLEN(cnt, v)
    #for    1, i <= cnt, 1
        #macro  VGET(t, v, i)
        load    k, t[TK_KIND]
        #if_cond k == TK_IDENT
            load    e, t[TK_VAL]
            load    r0, e[E_KW]
            move    val, 0
            #if_cond r0 == KW_TRUE
                move    val, 1
            #end_cond
            #call   tok_new_int(val, t)
            move    t, r0
        #end_cond
        #if_cond k == TK_CHAR
            load    r1, t[TK_VAL]
            #call   tok_new_int(r1, t)
            move    t, r0
        #end_cond
        #call   vec_push(outv, t)
    #end_for
    store   outv, PE_TOKS
    move    r0, 1
    store   r0, PE_POS
    #call   pe_cond()
    move    val, r0
    load    i, PE_POS
    #macro  VLEN(cnt, outv)
    #if_cond i <= cnt
        load    r1, PE_LINE
        #call   error_tok(r1, "missing binary operator in #if expression")
    #end_cond
    #return val
#end_func

// pe_peek() -> kind of the next expression token (TK_EOF at end)
#def_func pe_peek()
    #var    i, cnt, toks
    load    toks, PE_TOKS
    load    i, PE_POS
    #macro  VLEN(cnt, toks)
    #if_cond i > cnt
        #return TK_EOF
    #end_cond
    #macro  VGET(r1, toks, i)
    load    r0, r1[TK_KIND]
#end_func

#def_func pe_advance()
    load    r0, PE_POS
    add     r0, 1
    store   r0, PE_POS
#end_func

// binop_prec(kind) -> binary precedence (higher binds tighter) or 0
#def_func binop_prec(kind)
    #var    k
    load    k, kind
    #if_cond k == TK_OROR
        #return 1
    #end_cond
    #if_cond k == TK_ANDAND
        #return 2
    #end_cond
    #if_cond k == TK_PIPE
        #return 3
    #end_cond
    #if_cond k == TK_CARET
        #return 4
    #end_cond
    #if_cond k == TK_AMP
        #return 5
    #end_cond
    #if_cond k == TK_EQ
        #return 6
    #end_cond
    #if_cond k == TK_NE
        #return 6
    #end_cond
    #if_cond k == TK_LT
        #return 7
    #end_cond
    #if_cond k == TK_GT
        #return 7
    #end_cond
    #if_cond k == TK_LE
        #return 7
    #end_cond
    #if_cond k == TK_GE
        #return 7
    #end_cond
    #if_cond k == TK_SHL
        #return 8
    #end_cond
    #if_cond k == TK_SHR
        #return 8
    #end_cond
    #if_cond k == TK_PLUS
        #return 9
    #end_cond
    #if_cond k == TK_MINUS
        #return 9
    #end_cond
    #if_cond k == TK_STAR
        #return 10
    #end_cond
    #if_cond k == TK_SLASH
        #return 10
    #end_cond
    #if_cond k == TK_PERCENT
        #return 10
    #end_cond
    #return 0
#end_func

// pe_cond() -> value of a conditional expression
#def_func pe_cond()
    #var    c, a, b
    #call   pe_binary(1)
    move    c, r0
    #call   pe_peek()
    #if_cond r0 != TK_QUESTION
        #return c
    #end_cond
    #call   pe_advance()
    #call   pe_cond()
    move    a, r0
    #call   pe_peek()
    #if_cond r0 != TK_COLON
        load    r1, PE_LINE
        #call   error_tok(r1, "expected ':' in #if expression")
    #end_cond
    #call   pe_advance()
    #call   pe_cond()
    move    b, r0
    #if_cond c != 0
        #return a
    #end_cond
    #return b
#end_func

// pe_binary(min_prec) -> value (precedence climbing)
#def_func pe_binary(minp)
    #var    lhs, rhs, k, pr, mp, q
    load    mp, minp
    #call   pe_unary()
    move    lhs, r0
    #while  TRUE
        #call   pe_peek()
        move    k, r0
        #call   binop_prec(k)
        move    pr, r0
        #if_cond pr == 0
            #break
        #end_cond
        #if_cond pr < mp
            #break
        #end_cond
        #call   pe_advance()
        add     r1, pr, 1
        #call   pe_binary(r1)
        move    rhs, r0
        #if_cond k == TK_OROR
            move    q, 0
            #if_cond lhs != 0
                move    q, 1
            #end_cond
            #if_cond rhs != 0
                move    q, 1
            #end_cond
            move    lhs, q
        #end_cond
        #if_cond k == TK_ANDAND
            move    q, 0
            #if_cond lhs != 0
                #if_cond rhs != 0
                    move    q, 1
                #end_cond
            #end_cond
            move    lhs, q
        #end_cond
        #if_cond k == TK_PIPE
            or      lhs, rhs
        #end_cond
        #if_cond k == TK_CARET
            xor     lhs, rhs
        #end_cond
        #if_cond k == TK_AMP
            and     lhs, rhs
        #end_cond
        #if_cond k == TK_EQ
            cmp     lhs, rhs
            move    eq, lhs, 1, 0
        #end_cond
        #if_cond k == TK_NE
            cmp     lhs, rhs
            move    ne, lhs, 1, 0
        #end_cond
        #if_cond k == TK_LT
            cmp     lhs, rhs
            move    lt, lhs, 1, 0
        #end_cond
        #if_cond k == TK_GT
            cmp     lhs, rhs
            move    gt, lhs, 1, 0
        #end_cond
        #if_cond k == TK_LE
            cmp     lhs, rhs
            move    le, lhs, 1, 0
        #end_cond
        #if_cond k == TK_GE
            cmp     lhs, rhs
            move    ge, lhs, 1, 0
        #end_cond
        #if_cond k == TK_SHL
            lshift  lhs, rhs
        #end_cond
        #if_cond k == TK_SHR
            arshift lhs, rhs
        #end_cond
        #if_cond k == TK_PLUS
            add     lhs, rhs
        #end_cond
        #if_cond k == TK_MINUS
            sub     lhs, rhs
        #end_cond
        #if_cond k == TK_STAR
            mult    lhs, rhs
        #end_cond
        #if_cond k == TK_SLASH
            #if_cond rhs == 0
                move    lhs, 0
            #else_cond
                div     lhs, rhs
            #end_cond
        #end_cond
        #if_cond k == TK_PERCENT
            #if_cond rhs == 0
                move    lhs, 0
            #else_cond
                div     q, lhs, lhs, rhs
            #end_cond
        #end_cond
    #end_while
    #return lhs
#end_func

// pe_unary() -> value
#def_func pe_unary()
    #var    k, v, t
    #call   pe_peek()
    move    k, r0
    #if_cond k == TK_PLUS
        #call   pe_advance()
        #call   pe_unary()
        #return r0
    #end_cond
    #if_cond k == TK_MINUS
        #call   pe_advance()
        #call   pe_unary()
        neg     r0
        #return r0
    #end_cond
    #if_cond k == TK_TILDE
        #call   pe_advance()
        #call   pe_unary()
        compl   r0
        #return r0
    #end_cond
    #if_cond k == TK_NOT
        #call   pe_advance()
        #call   pe_unary()
        test    r0
        move    z, r0, 1, 0
        #return r0
    #end_cond
    #if_cond k == TK_LPAREN
        #call   pe_advance()
        #call   pe_cond()
        move    v, r0
        #call   pe_peek()
        #if_cond r0 != TK_RPAREN
            load    r1, PE_LINE
            #call   error_tok(r1, "missing ')' in #if expression")
        #end_cond
        #call   pe_advance()
        #return v
    #end_cond
    #if_cond k == TK_INT
        load    r1, PE_TOKS
        load    r2, PE_POS
        #macro  VGET(t, r1, r2)
        load    v, t[TK_VAL]
        #call   pe_advance()
        #return v
    #end_cond
    load    r1, PE_LINE
    #call   error_tok(r1, "invalid token in #if expression")
#end_func

///////////////////////////////////////////////////////////////////////////////
// pp_run(prelude, tokens) -> preprocessed token vector ending with TK_EOF.
//                   Identifiers that are keywords become keyword tokens.
///////////////////////////////////////////////////////////////////////////////
#def_func pp_run(pretoks, toksp)
    #var    outv, t, k, e
    load    r1, toksp
    #call   pp_include_tokens(r1)
    load    r1, pretoks
    #call   pp_include_tokens(r1)
    #call   vec_new()
    move    outv, r0
    #while  TRUE
        #call   pp_next()
        move    t, r0
        load    k, t[TK_KIND]
        #if_cond k == TK_IDENT
            load    e, t[TK_VAL]
            load    r0, e[E_KW]
            #if_cond r0 != 0
                store   r0, t[TK_KIND]
            #end_cond
        #end_cond
        #call   vec_push(outv, t)
        #if_cond k == TK_EOF
            #break
        #end_cond
    #end_while
    #return outv
#end_func

///////////////////////////////////////////////////////////////////////////////
// dump_tokens(toks, mode): print tokens.
//    mode 1: reconstructed source (--pp);  mode 2: one token per line
///////////////////////////////////////////////////////////////////////////////
#def_func dump_tokens(toksp, modep)
    #var    toks, mode, cnt, i, t, k, line, file, w, first, pr
    load    toks, toksp
    load    mode, modep
    #macro  VLEN(cnt, toks)
    move    line, -1
    move    file, 0
    move    first, 1
    #for    1, i <= cnt, 1
        #macro  VGET(t, toks, i)
        load    k, t[TK_KIND]
        #if_cond k == TK_EOF
            #break
        #end_cond
        #call   tok_p(t)
        move    w, r0
        #if_cond mode == 2
            load    r1, t[TK_LINE]
            #call   tok_kind_name(k)
            move    r2, r0
            load    r3, t[TK_LINE]
            #call   printf("%d %s %s\n", r3, r2, w)
        #else_cond
            load    r0, t[TK_LINE]
            load    r1, t[TK_FILE]
            move    pr, 0
            #if_cond r0 != line
                move    pr, 1
            #end_cond
            #if_cond r1 != file
                move    pr, 1
            #end_cond
            #if_cond pr != 0
                #if_cond first == 0
                    #call   printf("\n")
                #end_cond
                load    line, t[TK_LINE]
                load    file, t[TK_FILE]
            #else_cond
                load    r0, t[TK_FLAGS]
                and     r0, TF_SPACE
                #if_cond r0 != 0
                    #call   printf(" ")
                #end_cond
            #end_cond
            #call   puts(w)
            move    first, 0
        #end_cond
        move    r1, w
        int     iFREE
    #end_for
    #if_cond mode == 1
        #if_cond first == 0
            #call   printf("\n")
        #end_cond
    #end_cond
#end_func

// tok_kind_name(kind) -> static packed name of a token class
#def_func tok_kind_name(kind)
    #var    k
    load    k, kind
    #if_cond k == TK_IDENT
        #return "ident"
    #end_cond
    #if_cond k == TK_INT
        #return "int"
    #end_cond
    #if_cond k == TK_FLOAT
        #return "float"
    #end_cond
    #if_cond k == TK_CHAR
        #return "char"
    #end_cond
    #if_cond k == TK_STR
        #return "string"
    #end_cond
    #if_cond k >= KW_BASE
        #return "keyword"
    #end_cond
    #return "punct"
#end_func

PP_ASM_END:
    nop
