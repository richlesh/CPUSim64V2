///////////////////////////////////////////////////////////////////////////////
// parse_expr.asm
//
// Token cursor and expression parser (recursive descent, precedence
// climbing for the binary operator levels of CParser.g4):
//
//   expression            : assignmentExpression (',' assignmentExpression)*
//   assignmentExpression  : conditionalExpression
//                         | unaryExpression assignmentOperator assignmentExpression
//   conditionalExpression : logicalOrExpression ('?' expression ':' conditionalExpression)?
//   logicalOr .. multiplicative : precedence climbing (binop_prec)
//   castExpression        : '(' typeName ')' castExpression | unaryExpression
//   unaryExpression       : postfixExpression | ('++'|'--') unaryExpression
//                         | unaryOperator castExpression | sizeof ...
//   postfixExpression     : primaryExpression ('[' expr ']' | '(' args ')'
//                           | '.' Identifier | '->' Identifier | '++' | '--')*
//   primaryExpression     : Identifier | constant | StringLiteral+ | '(' expression ')'
///////////////////////////////////////////////////////////////////////////////

#global P_TOKS:     .dci 0          // token vector
#global P_POS:      .dci 0          // index of the current token
#global P_CNT:      .dci 0          // number of tokens
#global STRLITS:    .dci 0          // vec of string literal wstrs (id = index)
#global CUR_FUNC:   .dci 0          // Sym of the function being parsed

PARSE_EXPR_ASM_START:
    jump    PARSE_EXPR_ASM_END

///////////////////////////////////////////////////////////////////////////////
// cursor
///////////////////////////////////////////////////////////////////////////////
#def_func parser_init(toksp)
    #var    toks
    load    toks, toksp
    store   toks, P_TOKS
    #call   vec_len(toks)
    store   r0, P_CNT
    move    r0, 1
    store   r0, P_POS
    #call   vec_new()
    store   r0, STRLITS
#end_func

// cur() -> current token
#def_func cur()
    #var    i, cnt
    load    i, P_POS
    load    cnt, P_CNT
    #if_cond i > cnt
        move    i, cnt
    #end_cond
    load    r1, P_TOKS
    #macro  VGET(r0, r1, i)
#end_func

// peek_tok(k) -> token k positions ahead
#def_func peek_tok(kk)
    #var    i, cnt
    load    i, P_POS
    load    r0, kk
    add     i, r0
    load    cnt, P_CNT
    #if_cond i > cnt
        move    i, cnt
    #end_cond
    load    r1, P_TOKS
    #macro  VGET(r0, r1, i)
#end_func

// ck() -> kind of the current token
#def_func ck()
    #call   cur()
    load    r0, r0[TK_KIND]
#end_func

// peek_kind(k) -> kind of the token k positions ahead
#def_func peek_kind(kk)
    load    r1, kk
    #call   peek_tok(r1)
    load    r0, r0[TK_KIND]
#end_func

#def_func advance()
    #var    i, cnt
    load    i, P_POS
    load    cnt, P_CNT
    #if_cond i < cnt
        add     i, 1
        store   i, P_POS
    #end_cond
#end_func

// consume(kind) -> 1 and advance if the current token has the kind
#def_func consume(kind)
    #call   ck()
    load    r1, kind
    #if_cond r0 == r1
        #call   advance()
        #return 1
    #end_cond
    #return 0
#end_func

// kind_spelling(kind) -> packed spelling of a punctuator or keyword kind
#def_func kind_spelling(kind)
    #var    k
    load    k, kind
    #if_cond k >= KW_BASE
        sub     k, KW_BASE
        add     k, 1
        load    r1, LX_KWNAMES
        #macro  VGET(r1, r1, k)
        #call   ent_p(r1)
        #return r0
    #end_cond
    #if_cond k >= TK_PUNCT_BASE
        sub     k, TK_PUNCT_BASE
        add     k, 1
        load    r1, LX_PUNCTS
        #macro  VGET(r1, r1, k)
        #call   p_from_w(r1)
        #return r0
    #end_cond
    #if_cond k == TK_IDENT
        #return "identifier"
    #end_cond
    #return "token"
#end_func

// error_expected(what): "expected WHAT before 'tok'"
#def_func error_expected(what)
    #var    sb, t
    #call   cur()
    move    t, r0
    #call   vec_new()
    move    sb, r0
    #call   sb_add_p(sb, "expected ")
    load    r1, what
    #call   sb_add_p(sb, r1)
    #call   sb_add_p(sb, " before '")
    #call   tok_spelling(t)
    #call   sb_add_w(sb, r0)
    #call   sb_add_p(sb, "'")
    #call   sb_to_p(sb)
    #call   error_tok(t, r0)
#end_func

// expect(kind): consume a required punctuator / keyword
#def_func expect(kind)
    #var    k, sb
    load    k, kind
    #call   consume(k)
    #if_cond r0 != 0
        #return 0
    #end_cond
    #call   vec_new()
    move    sb, r0
    #call   vec_push(sb, '\'')
    #call   kind_spelling(k)
    #call   sb_add_p(sb, r0)
    #call   vec_push(sb, '\'')
    #call   sb_to_p(sb)
    #call   error_expected(r0)
#end_func

// expect_ident() -> identifier token (advanced past)
#def_func expect_ident()
    #var    t
    #call   cur()
    move    t, r0
    load    r0, t[TK_KIND]
    #if_cond r0 != TK_IDENT
        #call   error_expected("identifier")
    #end_cond
    #call   advance()
    #return t
#end_func

///////////////////////////////////////////////////////////////////////////////
// tok_to_binop(kind) -> AST node kind for a binary operator token
///////////////////////////////////////////////////////////////////////////////
#def_func tok_to_binop(kind)
    #var    k
    load    k, kind
    #if_cond k == TK_OROR
        #return ND_LOR
    #end_cond
    #if_cond k == TK_ANDAND
        #return ND_LAND
    #end_cond
    #if_cond k == TK_PIPE
        #return ND_BOR
    #end_cond
    #if_cond k == TK_CARET
        #return ND_BXOR
    #end_cond
    #if_cond k == TK_AMP
        #return ND_BAND
    #end_cond
    #if_cond k == TK_EQ
        #return ND_EQ
    #end_cond
    #if_cond k == TK_NE
        #return ND_NE
    #end_cond
    #if_cond k == TK_LT
        #return ND_LT
    #end_cond
    #if_cond k == TK_GT
        #return ND_GT
    #end_cond
    #if_cond k == TK_LE
        #return ND_LE
    #end_cond
    #if_cond k == TK_GE
        #return ND_GE
    #end_cond
    #if_cond k == TK_SHL
        #return ND_SHL
    #end_cond
    #if_cond k == TK_SHR
        #return ND_SHR
    #end_cond
    #if_cond k == TK_PLUS
        #return ND_ADD
    #end_cond
    #if_cond k == TK_MINUS
        #return ND_SUB
    #end_cond
    #if_cond k == TK_STAR
        #return ND_MUL
    #end_cond
    #if_cond k == TK_SLASH
        #return ND_DIV
    #end_cond
    #if_cond k == TK_PERCENT
        #return ND_MOD
    #end_cond
    // compound assignment operators
    #if_cond k == TK_ADDEQ
        #return ND_ADD
    #end_cond
    #if_cond k == TK_SUBEQ
        #return ND_SUB
    #end_cond
    #if_cond k == TK_MULEQ
        #return ND_MUL
    #end_cond
    #if_cond k == TK_DIVEQ
        #return ND_DIV
    #end_cond
    #if_cond k == TK_MODEQ
        #return ND_MOD
    #end_cond
    #if_cond k == TK_ANDEQ
        #return ND_BAND
    #end_cond
    #if_cond k == TK_OREQ
        #return ND_BOR
    #end_cond
    #if_cond k == TK_XOREQ
        #return ND_BXOR
    #end_cond
    #if_cond k == TK_SHLEQ
        #return ND_SHL
    #end_cond
    #if_cond k == TK_SHREQ
        #return ND_SHR
    #end_cond
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// expression
///////////////////////////////////////////////////////////////////////////////
#def_func parse_expr()
    #var    nd, t, b
    #call   parse_assign()
    move    nd, r0
    #while  TRUE
        #call   cur()
        move    t, r0
        #call   consume(TK_COMMA)
        #if_cond r0 == 0
            #break
        #end_cond
        #call   parse_assign()
        move    b, r0
        #call   sem_comma(nd, b, t)
        move    nd, r0
    #end_while
    #return nd
#end_func

#def_func parse_assign()
    #var    lhs, t, k, op, rhs
    #call   parse_conditional()
    move    lhs, r0
    #call   cur()
    move    t, r0
    load    k, t[TK_KIND]
    #if_cond k == TK_ASSIGN
        #call   advance()
        #call   parse_assign()
        #call   sem_assign(lhs, r0, t)
        #return r0
    #end_cond
    move    op, 0
    #if_cond k >= TK_MULEQ
        #if_cond k <= TK_OREQ
            #call   tok_to_binop(k)
            move    op, r0
        #end_cond
    #end_cond
    #if_cond k == TK_SHLEQ
        move    op, ND_SHL
    #end_cond
    #if_cond k == TK_SHREQ
        move    op, ND_SHR
    #end_cond
    #if_cond op != 0
        #call   advance()
        #call   parse_assign()
        move    rhs, r0
        #call   sem_opassign(op, lhs, rhs, t)
        #return r0
    #end_cond
    #return lhs
#end_func

#def_func parse_conditional()
    #var    c, a, b, t
    #call   parse_binary(1)
    move    c, r0
    #call   cur()
    move    t, r0
    #call   consume(TK_QUESTION)
    #if_cond r0 == 0
        #return c
    #end_cond
    #call   parse_expr()
    move    a, r0
    #call   expect(TK_COLON)
    #call   parse_conditional()
    move    b, r0
    #call   sem_cond(c, a, b, t)
#end_func

#def_func parse_binary(minp)
    #var    lhs, rhs, k, pr, mp, t
    load    mp, minp
    #call   parse_cast()
    move    lhs, r0
    #while  TRUE
        #call   cur()
        move    t, r0
        load    k, t[TK_KIND]
        #call   binop_prec(k)
        move    pr, r0
        #if_cond pr == 0
            #break
        #end_cond
        #if_cond pr < mp
            #break
        #end_cond
        #call   advance()
        add     r1, pr, 1
        #call   parse_binary(r1)
        move    rhs, r0
        #call   tok_to_binop(k)
        #call   sem_binary(r0, lhs, rhs, t)
        move    lhs, r0
    #end_while
    #return lhs
#end_func

///////////////////////////////////////////////////////////////////////////////
// cast / unary / postfix / primary
///////////////////////////////////////////////////////////////////////////////
#def_func parse_cast()
    #var    t, ty, nd
    #call   ck()
    #if_cond r0 == TK_LPAREN
        #call   peek_tok(1)
        #call   is_typename(r0)
        #if_cond r0 != 0
            #call   cur()
            move    t, r0
            #call   advance()
            #call   parse_typename()
            move    ty, r0
            #call   expect(TK_RPAREN)
            #call   ck()
            #if_cond r0 == TK_LBRACE
                #call   parse_compound_literal(ty, t)
                #call   parse_postfix_tail(r0)
                #return r0
            #end_cond
            #call   parse_cast()
            #call   sem_cast(r0, ty, t)
            #return r0
        #end_cond
    #end_cond
    #call   parse_unary()
#end_func

#def_func parse_unary()
    #var    t, k, nd, ty, sz
    #call   cur()
    move    t, r0
    load    k, t[TK_KIND]
    #if_cond k == TK_INC
        #call   advance()
        #call   parse_unary()
        #call   sem_incdec(r0, 1, 0, t)
        #return r0
    #end_cond
    #if_cond k == TK_DEC
        #call   advance()
        #call   parse_unary()
        #call   sem_incdec(r0, -1, 0, t)
        #return r0
    #end_cond
    #if_cond k == TK_AMP
        #call   advance()
        #call   parse_cast()
        #call   sem_unary(ND_ADDR, r0, t)
        #return r0
    #end_cond
    #if_cond k == TK_STAR
        #call   advance()
        #call   parse_cast()
        #call   sem_unary(ND_DEREF, r0, t)
        #return r0
    #end_cond
    #if_cond k == TK_PLUS
        #call   advance()
        #call   parse_cast()
        #call   sem_unary(ND_ADD, r0, t)
        #return r0
    #end_cond
    #if_cond k == TK_MINUS
        #call   advance()
        #call   parse_cast()
        move    nd, r0
        // fold negative literals
        load    r0, nd[N_KIND]
        #if_cond r0 == ND_NUM
            load    r1, nd[N_VAL]
            neg     r1
            store   r1, nd[N_VAL]
            #return nd
        #end_cond
        #if_cond r0 == ND_FNUM
            load    f1, nd[N_VAL]
            neg     f1
            store   f1, nd[N_VAL]
            #return nd
        #end_cond
        #call   sem_unary(ND_NEG, nd, t)
        #return r0
    #end_cond
    #if_cond k == TK_TILDE
        #call   advance()
        #call   parse_cast()
        #call   sem_unary(ND_BNOT, r0, t)
        #return r0
    #end_cond
    #if_cond k == TK_NOT
        #call   advance()
        #call   parse_cast()
        #call   sem_unary(ND_NOT, r0, t)
        #return r0
    #end_cond
    #if_cond k == KW_SIZEOF
        #call   advance()
        #call   ck()
        #if_cond r0 == TK_LPAREN
            #call   peek_tok(1)
            #call   is_typename(r0)
            #if_cond r0 != 0
                #call   advance()
                #call   parse_typename()
                move    ty, r0
                #call   expect(TK_RPAREN)
                #call   sizeof_type(ty, t)
                #return r0
            #end_cond
        #end_cond
        #call   parse_unary()
        load    ty, r0[N_TYPE]
        #call   sizeof_type(ty, t)
        #return r0
    #end_cond
    #if_cond k == KW_ALIGNOF
        #call   advance()
        #call   expect(TK_LPAREN)
        #call   parse_typename()
        #call   expect(TK_RPAREN)
        load    r1, TY_UINT_T
        #call   nd_number(1, r1, t)
        #return r0
    #end_cond
    #call   parse_postfix()
#end_func

// sizeof_type(ty, tok) -> ND_NUM with the size of ty in words
#def_func sizeof_type(tp, tk)
    #var    ty, sz
    load    ty, tp
    #call   ty_kind(ty)
    #if_cond r0 == TY_FUNC
        load    r1, tk
        #call   error_tok(r1, "invalid application of 'sizeof' to a function type")
    #end_cond
    #if_cond r0 == TY_VOID
        move    sz, 1
    #else_cond
        load    sz, ty[T_SIZE]
        #call   ty_is_struct(ty)
        #if_cond r0 != 0
            load    r0, ty[T_FLAGS]
            and     r0, TYF_COMPLETE
            #if_cond r0 == 0
                load    r1, tk
                #call   error_tok(r1, "invalid application of 'sizeof' to incomplete type")
            #end_cond
        #end_cond
        #call   ty_kind(ty)
        #if_cond r0 == TY_ARRAY
            load    r0, ty[T_LEN]
            #if_cond r0 < 0
                load    r1, tk
                #call   error_tok(r1, "invalid application of 'sizeof' to incomplete type")
            #end_cond
        #end_cond
    #end_cond
    load    r1, TY_UINT_T
    load    r2, tk
    #call   nd_number(sz, r1, r2)
#end_func

#def_func parse_postfix()
    #call   parse_primary()
    #call   parse_postfix_tail(r0)
#end_func

// parse_postfix_tail(node): apply postfix operators
#def_func parse_postfix_tail(ndp)
    #var    nd, t, k, idx, margs, nt
    load    nd, ndp
    #while  TRUE
        #call   cur()
        move    t, r0
        load    k, t[TK_KIND]
        #if_cond k == TK_LBRACK
            #call   advance()
            #call   parse_expr()
            move    idx, r0
            #call   expect(TK_RBRACK)
            #call   sem_binary(ND_ADD, nd, idx, t)
            #call   sem_unary(ND_DEREF, r0, t)
            move    nd, r0
            #continue
        #end_cond
        #if_cond k == TK_LPAREN
            #call   advance()
            #call   vec_new()
            move    margs, r0
            #call   ck()
            #if_cond r0 != TK_RPAREN
                #while  TRUE
                    #call   parse_assign()
                    #call   vec_push(margs, r0)
                    #call   consume(TK_COMMA)
                    #if_cond r0 == 0
                        #break
                    #end_cond
                #end_while
            #end_cond
            #call   expect(TK_RPAREN)
            #call   sem_call(nd, margs, t)
            move    nd, r0
            #call   call_struct_temp(nd)
            #continue
        #end_cond
        #if_cond k == TK_DOT
            #call   advance()
            #call   expect_ident()
            #call   sem_member(nd, r0, 0)
            move    nd, r0
            #continue
        #end_cond
        #if_cond k == TK_ARROW
            #call   advance()
            #call   expect_ident()
            #call   sem_member(nd, r0, 1)
            move    nd, r0
            #continue
        #end_cond
        #if_cond k == TK_INC
            #call   advance()
            #call   sem_incdec(nd, 1, 1, t)
            move    nd, r0
            #continue
        #end_cond
        #if_cond k == TK_DEC
            #call   advance()
            #call   sem_incdec(nd, -1, 1, t)
            move    nd, r0
            #continue
        #end_cond
        #break
    #end_while
    #return nd
#end_func

// call_struct_temp(call): a call returning a struct gets a hidden local
#def_func call_struct_temp(ndp)
    #var    nd, ty
    load    nd, ndp
    load    ty, nd[N_TYPE]
    #call   ty_is_struct(ty)
    #if_cond r0 == 0
        #return 0
    #end_cond
    load    r0, CUR_FUNC
    #if_cond r0 == 0
        #call   error_node(nd, "initializer element is not constant")
    #end_cond
    #call   new_temp_local(ty)
    store   r0, nd[N_C]
#end_func

// add_strlit(wstr) -> literal id
#def_func add_strlit(wp)
    load    r1, STRLITS
    load    r2, wp
    #call   vec_push(r1, r2)
    load    r1, STRLITS
    #call   vec_len(r1)
#end_func

// str_node(wstr, tok) -> ND_STR typed char[len+1]
#def_func str_node(wp, tk)
    #var    w, nd, id, ty
    load    w, wp
    #call   add_strlit(w)
    move    id, r0
    load    r2, tk
    #call   nd_new(ND_STR, r2)
    move    nd, r0
    store   id, nd[N_VAL]
    load    r2, w[0]
    add     r2, 1
    load    r1, TY_CHAR_T
    #call   ty_array_of(r1, r2)
    store   r0, nd[N_TYPE]
    #return nd
#end_func

#def_func parse_primary()
    #var    t, k, e, s, nd, sb, sk, w
    #call   cur()
    move    t, r0
    load    k, t[TK_KIND]
    #if_cond k == TK_LPAREN
        #call   advance()
        #call   parse_expr()
        move    nd, r0
        #call   expect(TK_RPAREN)
        #return nd
    #end_cond
    #if_cond k == TK_INT
        #call   advance()
        load    r1, TY_INT_T
        load    r0, t[TK_AUX]
        and     r0, IS_UNSIGNED
        #if_cond r0 != 0
            load    r1, TY_UINT_T
        #end_cond
        load    r0, t[TK_VAL]
        #call   nd_number(r0, r1, t)
        #return r0
    #end_cond
    #if_cond k == TK_CHAR
        #call   advance()
        load    r0, t[TK_VAL]
        load    r1, TY_INT_T
        #call   nd_number(r0, r1, t)
        #return r0
    #end_cond
    #if_cond k == TK_FLOAT
        #call   advance()
        #call   nd_new(ND_FNUM, t)
        move    nd, r0
        load    r1, t[TK_VAL]
        store   r1, nd[N_VAL]
        load    r1, TY_DOUBLE_T
        store   r1, nd[N_TYPE]
        #return nd
    #end_cond
    #if_cond k == TK_STR
        // StringLiteral+ : adjacent literals are concatenated
        #call   vec_new()
        move    sb, r0
        #while  TRUE
            #call   ck()
            #if_cond r0 != TK_STR
                #break
            #end_cond
            #call   cur()
            load    r1, r0[TK_VAL]
            #call   sb_add_w(sb, r1)
            #call   advance()
        #end_while
        #call   sb_to_w(sb)
        #call   str_node(r0, t)
        #return r0
    #end_cond
    #if_cond k == KW_TRUE
        #call   advance()
        load    r1, TY_INT_T
        #call   nd_number(1, r1, t)
        #return r0
    #end_cond
    #if_cond k == KW_FALSE
        #call   advance()
        load    r1, TY_INT_T
        #call   nd_number(0, r1, t)
        #return r0
    #end_cond
    #if_cond k == KW_NULLPTR
        #call   advance()
        load    r1, TY_VOID_T
        #call   ty_pointer_to(r1)
        #call   nd_number(0, r0, t)
        #return r0
    #end_cond
    #if_cond k == KW_FUNC
        #call   advance()
        load    s, CUR_FUNC
        #if_cond s == 0
            #call   error_tok(t, "'__func__' is not defined outside of function scope")
        #end_cond
        load    r1, s[S_NAME]
        load    r1, r1[E_W]
        #call   str_node(r1, t)
        #return r0
    #end_cond
    #if_cond k == KW_GENERIC
        #call   error_tok(t, "_Generic is not supported")
    #end_cond
    #if_cond k == TK_IDENT
        #call   advance()
        load    e, t[TK_VAL]
        #call   sym_lookup(e)
        move    s, r0
        #if_cond s == 0
            #call   ck()
            #if_cond r0 == TK_LPAREN
                #call   error_tok2(t, "implicit declaration of function", e)
            #end_cond
            #call   error_tok2(t, "undeclared identifier", e)
        #end_cond
        load    r0, s[S_ALIAS]
        #if_cond r0 != 0
            move    s, r0
        #end_cond
        load    sk, s[S_KIND]
        #if_cond sk == SK_ENUMC
            load    r0, s[S_OFFSET]
            load    r1, TY_INT_T
            #call   nd_number(r0, r1, t)
            #return r0
        #end_cond
        #if_cond sk == SK_TYPEDEF
            #call   error_tok2(t, "unexpected type name", e)
        #end_cond
        load    r0, s[S_FLAGS]
        or      r0, SF_USED
        store   r0, s[S_FLAGS]
        #call   nd_new(ND_VAR, t)
        move    nd, r0
        store   s, nd[N_VAL]
        load    r1, s[S_TYPE]
        store   r1, nd[N_TYPE]
        #return nd
    #end_cond
    #call   error_expected("expression")
#end_func

///////////////////////////////////////////////////////////////////////////////
// constant expressions
///////////////////////////////////////////////////////////////////////////////
// const_expr() -> value of a conditional expression that must be constant
#def_func const_expr()
    #var    nd
    #call   parse_conditional()
    move    nd, r0
    #call   const_eval(nd)
#end_func

// is_const(node) -> 1 if const_eval would succeed
#def_func is_const(ndp)
    #var    nd, k
    load    nd, ndp
    load    k, nd[N_KIND]
    #if_cond k == ND_NUM
        #return 1
    #end_cond
    #if_cond k == ND_FNUM
        #return 1
    #end_cond
    #if_cond k == ND_CAST
        load    r1, nd[N_A]
        #call   is_const(r1)
        #return r0
    #end_cond
    #if_cond k == ND_COND
        load    r1, nd[N_A]
        #call   is_const(r1)
        #if_cond r0 == 0
            #return 0
        #end_cond
        load    r1, nd[N_B]
        #call   is_const(r1)
        #if_cond r0 == 0
            #return 0
        #end_cond
        load    r1, nd[N_C]
        #call   is_const(r1)
        #return r0
    #end_cond
    #if_cond k <= ND_NEG
        #if_cond k >= ND_ADD
            load    r1, nd[N_A]
            #call   is_const(r1)
            #if_cond r0 == 0
                #return 0
            #end_cond
            load    r1, nd[N_B]
            #if_cond r1 == 0
                #return 1
            #end_cond
            #call   is_const(r1)
            #return r0
        #end_cond
    #end_cond
    #return 0
#end_func

// const_eval(node) -> integer value; error if not constant
#def_func const_eval(ndp)
    #var    nd, k, a, b, q, ty
    load    nd, ndp
    load    k, nd[N_KIND]
    #if_cond k == ND_NUM
        load    r0, nd[N_VAL]
        #return r0
    #end_cond
    #if_cond k == ND_FNUM
        load    f0, nd[N_VAL]
        move    r0, f0
        #return r0
    #end_cond
    #if_cond k == ND_CAST
        load    a, nd[N_A]
        load    ty, nd[N_TYPE]
        load    r1, a[N_TYPE]
        #call   ty_is_double(r1)
        #if_cond r0 != 0
            #call   const_eval_d(a)
            move    r0, f0
            #return r0
        #end_cond
        #call   const_eval(a)
        move    q, r0
        #call   ty_kind(ty)
        #if_cond r0 == TY_BOOL
            #if_cond q != 0
                move    q, 1
            #end_cond
        #end_cond
        #return q
    #end_cond
    #if_cond k == ND_COND
        load    r1, nd[N_A]
        #call   const_eval(r1)
        #if_cond r0 != 0
            load    r1, nd[N_B]
        #else_cond
            load    r1, nd[N_C]
        #end_cond
        #call   const_eval(r1)
        #return r0
    #end_cond
    #if_cond k == ND_NEG
        load    r1, nd[N_A]
        #call   const_eval(r1)
        neg     r0
        #return r0
    #end_cond
    #if_cond k == ND_BNOT
        load    r1, nd[N_A]
        #call   const_eval(r1)
        compl   r0
        #return r0
    #end_cond
    #if_cond k == ND_NOT
        load    r1, nd[N_A]
        #call   const_eval(r1)
        test    r0
        move    z, r0, 1, 0
        #return r0
    #end_cond
    #if_cond k < ND_ADD
        #call   error_node(nd, "expression is not constant")
    #end_cond
    #if_cond k > ND_LOR
        #call   error_node(nd, "expression is not constant")
    #end_cond
    load    r1, nd[N_A]
    #call   const_eval(r1)
    move    a, r0
    load    r1, nd[N_B]
    #call   const_eval(r1)
    move    b, r0
    #if_cond k == ND_ADD
        add     r0, a, b
        #return r0
    #end_cond
    #if_cond k == ND_SUB
        sub     r0, a, b
        #return r0
    #end_cond
    #if_cond k == ND_MUL
        mult    r0, a, b
        #return r0
    #end_cond
    #if_cond k == ND_DIV
        #if_cond b == 0
            #call   error_node(nd, "division by zero in constant expression")
        #end_cond
        div     r0, a, b
        #return r0
    #end_cond
    #if_cond k == ND_MOD
        #if_cond b == 0
            #call   error_node(nd, "division by zero in constant expression")
        #end_cond
        div     q, r0, a, b
        #return r0
    #end_cond
    #if_cond k == ND_BAND
        and     r0, a, b
        #return r0
    #end_cond
    #if_cond k == ND_BOR
        or      r0, a, b
        #return r0
    #end_cond
    #if_cond k == ND_BXOR
        xor     r0, a, b
        #return r0
    #end_cond
    #if_cond k == ND_SHL
        lshift  r0, a, b
        #return r0
    #end_cond
    #if_cond k == ND_SHR
        load    r1, nd[N_TYPE]
        #call   ty_is_unsigned(r1)
        #if_cond r0 != 0
            rshift  r0, a, b
        #else_cond
            arshift r0, a, b
        #end_cond
        #return r0
    #end_cond
    cmp     a, b
    #if_cond k == ND_EQ
        move    eq, r0, 1, 0
        #return r0
    #end_cond
    #if_cond k == ND_NE
        move    ne, r0, 1, 0
        #return r0
    #end_cond
    #if_cond k == ND_LT
        move    lt, r0, 1, 0
        #return r0
    #end_cond
    #if_cond k == ND_LE
        move    le, r0, 1, 0
        #return r0
    #end_cond
    #if_cond k == ND_LAND
        move    r0, 0
        #if_cond a != 0
            #if_cond b != 0
                move    r0, 1
            #end_cond
        #end_cond
        #return r0
    #end_cond
    #if_cond k == ND_LOR
        move    r0, 0
        #if_cond a != 0
            move    r0, 1
        #end_cond
        #if_cond b != 0
            move    r0, 1
        #end_cond
        #return r0
    #end_cond
    #call   error_node(nd, "expression is not constant")
#end_func

// const_eval_d(node) -> f0 = double value of a constant expression
#def_func const_eval_d(ndp)
    #var    nd, k
    load    nd, ndp
    load    k, nd[N_KIND]
    #if_cond k == ND_FNUM
        load    f0, nd[N_VAL]
        #return 0
    #end_cond
    #if_cond k == ND_NUM
        load    r0, nd[N_VAL]
        move    f0, r0
        #return 0
    #end_cond
    #if_cond k == ND_CAST
        load    r1, nd[N_A]
        load    r1, r1[N_TYPE]
        #call   ty_is_double(r1)
        #if_cond r0 != 0
            load    r1, nd[N_A]
            #call   const_eval_d(r1)
            #return 0
        #end_cond
        load    r1, nd[N_A]
        #call   const_eval(r1)
        move    f0, r0
        #return 0
    #end_cond
    #if_cond k == ND_NEG
        load    r1, nd[N_A]
        #call   const_eval_d(r1)
        neg     f0
        #return 0
    #end_cond
    #if_cond k >= ND_ADD
        #if_cond k <= ND_DIV
            load    r1, nd[N_A]
            #call   const_eval_d(r1)
            push    f0
            load    r1, nd[N_B]
            #call   const_eval_d(r1)
            move    f1, f0
            pop     f0
            #if_cond k == ND_ADD
                add     f0, f1
            #end_cond
            #if_cond k == ND_SUB
                sub     f0, f1
            #end_cond
            #if_cond k == ND_MUL
                mult    f0, f1
            #end_cond
            #if_cond k == ND_DIV
                div     f0, f1
            #end_cond
            #return 0
        #end_cond
    #end_cond
    #if_cond k == ND_COND
        load    r1, nd[N_A]
        #call   const_eval(r1)
        #if_cond r0 != 0
            load    r1, nd[N_B]
        #else_cond
            load    r1, nd[N_C]
        #end_cond
        #call   const_eval_d(r1)
        #return 0
    #end_cond
    #call   error_node(nd, "initializer element is not constant")
#end_func

PARSE_EXPR_ASM_END:
    nop
