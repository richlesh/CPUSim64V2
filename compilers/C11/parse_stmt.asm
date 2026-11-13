///////////////////////////////////////////////////////////////////////////////
// parse_stmt.asm
//
// Statements, function definitions and the translation unit (CParser.g4
// 6.8 / 6.9):
//   statement : labeledStatement | compoundStatement | expressionStatement
//             | selectionStatement | iterationStatement | jumpStatement
//   compilationUnit : externalDeclaration* EOF
//   externalDeclaration : functionDefinition | declaration | ';'
///////////////////////////////////////////////////////////////////////////////

#global CUR_SWITCH: .dci 0          // innermost ND_SWITCH node
#global LOOP_DEPTH: .dci 0          // nesting of loops
#global BRK_DEPTH:  .dci 0          // nesting of loops and switches
#global FN_LABELS:  .dci 0          // vec of goto label records

// goto label record
#define LB_ENTRY    0
#define LB_DEFINED  1
#define LB_TOK      2
#define LB_LABEL    3
#define LB_SIZE     4

PARSE_STMT_ASM_START:
    jump    PARSE_STMT_ASM_END

///////////////////////////////////////////////////////////////////////////////
// locals
///////////////////////////////////////////////////////////////////////////////
// alloc_local(sym): reserve frame space and set the SF-relative offset
#def_func alloc_local(sp_)
    #var    s, sz, f
    load    s, sp_
    load    r1, s[S_TYPE]
    load    sz, r1[T_SIZE]
    #if_cond sz < 1
        move    sz, 1
    #end_cond
    load    f, CUR_FRAME
    add     r0, f, sz
    sub     r0, 1
    neg     r0
    store   r0, s[S_OFFSET]
    add     f, sz
    store   f, CUR_FRAME
#end_func

// new_temp_local(ty) -> unnamed local Sym (not bound in any scope)
#def_func new_temp_local(tp)
    #var    s
    #call   zalloc(S_SIZE)
    move    s, r0
    move    r1, SK_LVAR
    store   r1, s[S_KIND]
    load    r1, tp
    store   r1, s[S_TYPE]
    load    r1, ANON_TAG
    store   r1, s[S_NAME]
    #call   alloc_local(s)
    #return s
#end_func

// var_node(sym, tok) -> ND_VAR
#def_func var_node(sp_, tk)
    #var    s, nd
    load    s, sp_
    load    r1, tk
    #call   nd_new(ND_VAR, r1)
    move    nd, r0
    store   s, nd[N_VAL]
    load    r1, s[S_TYPE]
    store   r1, nd[N_TYPE]
    #return nd
#end_func

// lvalue_at(sym, off, ty, tok) -> lvalue of type ty at word offset off in sym
#def_func lvalue_at(sp_, offp, tp, tk)
    #var    s, off, ty, v, a, pt
    load    s, sp_
    load    off, offp
    load    ty, tp
    load    r4, tk
    #call   var_node(s, r4)
    move    v, r0
    #if_cond off == 0
        load    r1, s[S_TYPE]
        #call   ty_same(r1, ty)
        #if_cond r0 != 0
            #return v
        #end_cond
    #end_cond
    #call   ty_pointer_to(ty)
    move    pt, r0
    load    r1, s[S_TYPE]
    #call   ty_pointer_to(r1)
    load    r4, tk
    #call   mk_typed(ND_ADDR, v, 0, r0, r4)
    load    r4, tk
    #call   mk_typed(ND_CAST, r0, 0, pt, r4)
    move    a, r0
    #if_cond off != 0
        load    r1, TY_INT_T
        load    r2, tk
        #call   nd_number(off, r1, r2)
        load    r4, tk
        #call   mk_typed(ND_ADD, a, r0, pt, r4)
        move    a, r0
    #end_cond
    load    r4, tk
    #call   mk_typed(ND_DEREF, a, 0, ty, r4)
#end_func

// emit_local_init(stmts, sym, leaves, tok): statements initializing a local
#def_func emit_local_init(stp, sp_, lvp, tk)
    #var    st, s, lv, cnt, i, l, ty, lhs, nd
    load    st, stp
    load    s, sp_
    load    lv, lvp
    load    ty, s[S_TYPE]
    // aggregates (arrays, structs, unions) are zeroed before the leaves
    // are stored, so that elements without an initializer are 0
    #call   ty_is_struct(ty)
    move    nd, r0
    #call   ty_kind(ty)
    #if_cond r0 == TY_ARRAY
        move    nd, 1
    #end_cond
    #if_cond nd != 0
        load    r1, tk
        #call   nd_new(ND_MEMZERO, r1)
        move    nd, r0
        store   s, nd[N_VAL]
        load    r1, TY_VOID_T
        store   r1, nd[N_TYPE]
        load    r1, tk
        #call   nd_unary(ND_EXPR, nd, r1)
        #call   vec_push(st, r0)
    #end_cond
    #macro  VLEN(cnt, lv)
    #for    1, i <= cnt, 1
        #macro  VGET(l, lv, i)
        load    r1, l[L_OFF]
        load    r2, l[L_TYPE]
        load    r4, tk
        #call   lvalue_at(s, r1, r2, r4)
        move    lhs, r0
        load    r2, l[L_EXPR]
        load    r3, l[L_TYPE]
        load    r4, tk
        #call   mk_typed(ND_ASSIGN, lhs, r2, r3, r4)
        load    r1, tk
        #call   nd_unary(ND_EXPR, r0, r1)
        #call   vec_push(st, r0)
    #end_for
#end_func

///////////////////////////////////////////////////////////////////////////////
// global registry helpers
///////////////////////////////////////////////////////////////////////////////
// bind_global(g, tok): make sure name of g resolves to g in this scope
#def_func bind_global(gp, tk)
    #var    g, e, a
    load    g, gp
    load    e, g[S_NAME]
    #call   sym_lookup(e)
    #if_cond r0 == g
        #return g
    #end_cond
    #if_cond r0 != 0
        load    r1, r0[S_ALIAS]
        #if_cond r1 == g
            load    r1, r0[S_DEPTH]
            load    r2, SCOPE_DEPTH
            #if_cond r1 == r2
                #return g
            #end_cond
        #end_cond
    #end_cond
    load    r1, g[S_KIND]
    load    r2, g[S_TYPE]
    load    r3, tk
    #call   sym_declare(e, r1, r2, r3)
    move    a, r0
    store   g, a[S_ALIAS]
    #return g
#end_func

// check_redecl(entry, tok): error if the name is already declared in this
// scope as something other than a registry alias
#def_func check_redecl(ep, tk)
    #var    e, s
    load    e, ep
    #call   sym_in_scope(e)
    move    s, r0
    #if_cond s == 0
        #return 0
    #end_cond
    load    r1, tk
    #call   error_tok2(r1, "redefinition of", e)
#end_func

///////////////////////////////////////////////////////////////////////////////
// drop_prelude(entry): forget a prelude declaration that a user declaration
// of a different kind / type replaces.  Returns 1 if dropped.
///////////////////////////////////////////////////////////////////////////////
#def_func drop_prelude(ep)
    #var    e, g
    load    e, ep
    load    g, e[E_GLOBAL]
    #if_cond g == 0
        #return 0
    #end_cond
    load    r0, g[S_FLAGS]
    and     r0, SF_PRELUDE
    #if_cond r0 == 0
        #return 0
    #end_cond
    load    r0, e[E_SYM]
    #if_cond r0 == g
        load    r1, g[S_NEXT]
        store   r1, e[E_SYM]
    #end_cond
    move    r1, 0
    store   r1, e[E_GLOBAL]
    #return 1
#end_func

///////////////////////////////////////////////////////////////////////////////
// declare_function(entry, type, storage, tok) -> registry Sym
///////////////////////////////////////////////////////////////////////////////
#def_func declare_function(ep, tp, stp, tk)
    #var    e, ty, g, s, lab
    load    e, ep
    load    ty, tp
    load    g, e[E_GLOBAL]
    #if_cond g != 0
        load    r0, g[S_KIND]
        #if_cond r0 != SK_FUNC
            #call   drop_prelude(e)
            #if_cond r0 != 0
                move    g, 0
            #else_cond
                load    r1, tk
                #call   error_tok2(r1, "redeclared as different kind of symbol", e)
            #end_cond
        #end_cond
    #end_cond
    #if_cond g != 0
        load    r1, g[S_TYPE]
        #call   ty_same(r1, ty)
        #if_cond r0 == 0
            // a user prototype of a library function keeps the library type
            load    r0, g[S_FLAGS]
            and     r0, SF_PRELUDE
            #if_cond r0 != 0
                load    r1, tk
                load    r1, r1[TK_FILE]
                #call   is_lib_file(r1)
                #if_cond r0 == 0
                    load    r1, g[S_TYPE]
                    load    r1, r1[T_PARAMS]
                    #call   vec_len(r1)
                    move    r2, r0
                    load    r1, ty[T_PARAMS]
                    #call   vec_len(r1)
                    #if_cond r0 == r2
                        load    r1, tk
                        #call   bind_global(g, r1)
                        #return g
                    #end_cond
                #end_cond
            #end_cond
            load    r1, tk
            #call   error_tok2(r1, "conflicting types for", e)
        #end_cond
        // a prototype replaces an old-style declaration
        load    r1, g[S_TYPE]
        load    r0, r1[T_FLAGS]
        and     r0, TYF_OLDSTYLE
        #if_cond r0 != 0
            store   ty, g[S_TYPE]
        #end_cond
        // a definition supplies the parameter names
        load    r0, ty[T_FLAGS]
        and     r0, TYF_OLDSTYLE
        #if_cond r0 == 0
            store   ty, g[S_TYPE]
        #end_cond
        load    r1, tk
        #call   bind_global(g, r1)
        #return g
    #end_cond
    #call   sym_in_scope(e)
    #if_cond r0 != 0
        load    r1, tk
        #call   error_tok2(r1, "redeclared as different kind of symbol", e)
    #end_cond
    load    r3, tk
    #call   sym_declare(e, SK_FUNC, ty, r3)
    move    g, r0
    store   g, e[E_GLOBAL]
    // functions declared by the prelude are provided by the runtime
    load    r1, tk
    load    r1, r1[TK_FILE]
    #call   is_lib_file(r1)
    #if_cond r0 != 0
        load    r0, g[S_FLAGS]
        or      r0, SF_BUILTIN
        store   r0, g[S_FLAGS]
        #call   vec_new()
        move    lab, r0
        #call   sb_add_p(lab, "__crt_")
        load    r1, e[E_W]
        #call   sb_add_w(lab, r1)
        #call   sb_to_p(lab)
        store   r0, g[S_LABEL]
    #else_cond
        #call   new_label("F", e)
        store   r0, g[S_LABEL]
    #end_cond
    load    r1, FUNCS
    #call   vec_push(r1, g)
    #return g
#end_func

///////////////////////////////////////////////////////////////////////////////
// declare_global(entry, type, storage, tok) -> registry Sym (file scope or
// block-scope extern)
///////////////////////////////////////////////////////////////////////////////
#def_func declare_global(ep, tp, stp, tk)
    #var    e, ty, st, g, gty
    load    e, ep
    load    ty, tp
    load    st, stp
    load    g, e[E_GLOBAL]
    #if_cond g != 0
        load    r0, g[S_KIND]
        #if_cond r0 != SK_GVAR
            #call   drop_prelude(e)
            #if_cond r0 == 0
                load    r1, tk
                #call   error_tok2(r1, "redeclared as different kind of symbol", e)
            #end_cond
            move    g, 0
        #end_cond
    #end_cond
    #if_cond g != 0
        load    gty, g[S_TYPE]
        #call   ty_same(gty, ty)
        #if_cond r0 == 0
            load    r1, tk
            #call   error_tok2(r1, "conflicting types for", e)
        #end_cond
        // complete an incomplete array
        #call   ty_kind(gty)
        #if_cond r0 == TY_ARRAY
            load    r0, gty[T_LEN]
            #if_cond r0 < 0
                store   ty, g[S_TYPE]
            #end_cond
        #end_cond
        and     r0, st, SC_EXTERN
        #if_cond r0 == 0
            load    r0, g[S_FLAGS]
            and     r0, -3                  // clear SF_EXTERN
            store   r0, g[S_FLAGS]
        #end_cond
        load    r1, tk
        #call   bind_global(g, r1)
        #return g
    #end_cond
    #call   sym_in_scope(e)
    #if_cond r0 != 0
        load    r1, tk
        #call   error_tok2(r1, "redeclared as different kind of symbol", e)
    #end_cond
    load    r3, tk
    #call   sym_declare(e, SK_GVAR, ty, r3)
    move    g, r0
    store   g, e[E_GLOBAL]
    #call   new_label("G", e)
    store   r0, g[S_LABEL]
    and     r0, st, SC_EXTERN
    #if_cond r0 != 0
        load    r0, g[S_FLAGS]
        or      r0, SF_EXTERN
        store   r0, g[S_FLAGS]
    #end_cond
    load    r1, GLOBALS
    #call   vec_push(r1, g)
    #return g
#end_func

// global_init(sym, tok): optional '= initializer' for a global object
#def_func global_init(sp_, tk)
    #var    s, lv, ty
    load    s, sp_
    #call   consume(TK_ASSIGN)
    #if_cond r0 == 0
        #return 0
    #end_cond
    load    r0, s[S_FLAGS]
    and     r0, SF_DEFINED
    #if_cond r0 != 0
        load    r1, tk
        load    r2, s[S_NAME]
        #call   error_tok2(r1, "redefinition of", r2)
    #end_cond
    load    r1, s[S_TYPE]
    #call   parse_initializer(r1)
    move    lv, r0
    load    r1, s[S_TYPE]
    load    r2, tk
    #call   complete_array(r1, r2)
    store   r0, s[S_TYPE]
    store   lv, s[S_INIT]
    load    r0, s[S_FLAGS]
    or      r0, SF_DEFINED
    and     r0, -3                          // clear SF_EXTERN
    store   r0, s[S_FLAGS]
    // keep the scope binding's type in sync
    load    r1, s[S_NAME]
    #call   sym_lookup(r1)
    load    r1, s[S_TYPE]
    store   r1, r0[S_TYPE]
#end_func

// typedef_decl(entry, type, tok)
#def_func typedef_decl(ep, tp, tk)
    #var    e, ty, s
    load    e, ep
    load    ty, tp
    #call   sym_in_scope(e)
    move    s, r0
    #if_cond s != 0
        load    r0, s[S_FLAGS]
        and     r0, SF_PRELUDE
        #if_cond r0 == 0
            load    r0, s[S_KIND]
            #if_cond r0 == SK_TYPEDEF
                load    r1, s[S_TYPE]
                #call   ty_same(r1, ty)
                #if_cond r0 != 0
                    #return 0
                #end_cond
                load    r1, tk
                #call   error_tok2(r1, "conflicting types for", e)
            #end_cond
            load    r1, tk
            #call   error_tok2(r1, "redeclared as different kind of symbol", e)
        #end_cond
        #call   drop_prelude(e)
    #end_cond
    load    r3, tk
    #call   sym_declare(e, SK_TYPEDEF, ty, r3)
#end_func

///////////////////////////////////////////////////////////////////////////////
// parse_local_decl(stmts): a declaration inside a block
///////////////////////////////////////////////////////////////////////////////
#def_func parse_local_decl(stp)
    #var    st, bt, sc, ty, e, nt, s, lv, k
    load    st, stp
    #call   declspec()
    move    bt, r0
    load    sc, DS_STORAGE
    #call   consume(TK_SEMI)
    #if_cond r0 != 0
        #return 0
    #end_cond
    #while  TRUE
        #call   declarator(bt)
        move    ty, r0
        load    e, DECL_NAME
        load    nt, DECL_TOK
        #if_cond e == 0
            #call   error_tok(nt, "expected identifier or '('")
        #end_cond
        and     r0, sc, SC_TYPEDEF
        #if_cond r0 != 0
            #call   typedef_decl(e, ty, nt)
            jump    PLD_NEXT
        #end_cond
        #call   ty_kind(ty)
        #if_cond r0 == TY_FUNC
            #call   declare_function(e, ty, sc, nt)
            jump    PLD_NEXT
        #end_cond
        and     r0, sc, SC_EXTERN
        #if_cond r0 != 0
            #call   declare_global(e, ty, sc, nt)
            jump    PLD_NEXT
        #end_cond
        #call   ty_kind(ty)
        #if_cond r0 == TY_VOID
            #call   error_tok2(nt, "variable declared void", e)
        #end_cond
        #call   check_redecl(e, nt)
        and     r0, sc, SC_STATIC
        #if_cond r0 != 0
            // static local: a global object with a private label
            #call   sym_declare(e, SK_GVAR, ty, nt)
            move    s, r0
            #call   new_label("S", e)
            store   r0, s[S_LABEL]
            load    r1, GLOBALS
            #call   vec_push(r1, s)
            #call   global_init(s, nt)
            load    r1, s[S_TYPE]
            #call   check_complete_object(r1, nt, e)
            jump    PLD_NEXT
        #end_cond
        // automatic variable
        move    lv, 0
        #call   consume(TK_ASSIGN)
        #if_cond r0 != 0
            // the variable is in scope inside its own initializer
            #call   sym_declare(e, SK_LVAR, ty, nt)
            move    s, r0
            #call   parse_initializer(ty)
            move    lv, r0
            #call   complete_array(ty, nt)
            move    ty, r0
            store   ty, s[S_TYPE]
        #else_cond
            #call   sym_declare(e, SK_LVAR, ty, nt)
            move    s, r0
        #end_cond
        #call   check_complete_object(ty, nt, e)
        #call   alloc_local(s)
        #if_cond lv != 0
            #call   emit_local_init(st, s, lv, nt)
        #end_cond
PLD_NEXT:
        #call   consume(TK_COMMA)
        #if_cond r0 == 0
            #break
        #end_cond
    #end_while
    #call   expect(TK_SEMI)
#end_func

// check_complete_object(type, tok, entry): storage size must be known
#def_func check_complete_object(tp, tk, ep)
    #var    ty
    load    ty, tp
    #call   ty_kind(ty)
    #if_cond r0 == TY_ARRAY
        load    r0, ty[T_LEN]
        #if_cond r0 < 0
            load    r1, tk
            load    r2, ep
            #call   error_tok2(r1, "array size missing in", r2)
        #end_cond
    #end_cond
    #call   ty_is_struct(ty)
    #if_cond r0 != 0
        load    r0, ty[T_FLAGS]
        and     r0, TYF_COMPLETE
        #if_cond r0 == 0
            load    r1, tk
            load    r2, ep
            #call   error_tok2(r1, "storage size isn't known for", r2)
        #end_cond
    #end_cond
#end_func

// static_assert_decl(): _Static_assert ( constant-expression , string ) ;
#def_func static_assert_decl()
    #var    t, v, msg
    #call   cur()
    move    t, r0
    #call   advance()
    #call   expect(TK_LPAREN)
    #call   const_expr()
    move    v, r0
    move    msg, 0
    #call   consume(TK_COMMA)
    #if_cond r0 != 0
        #call   cur()
        load    msg, r0[TK_VAL]
        #call   expect(TK_STR)
    #end_cond
    #call   expect(TK_RPAREN)
    #call   expect(TK_SEMI)
    #if_cond v == 0
        #if_cond msg != 0
            #call   p_from_w(msg)
            #call   error_tok2(t, "static assertion failed:", r0)
        #end_cond
        #call   error_tok(t, "static assertion failed")
    #end_cond
#end_func

///////////////////////////////////////////////////////////////////////////////
// compound statement
///////////////////////////////////////////////////////////////////////////////
#def_func is_label_start()
    #call   ck()
    #if_cond r0 != TK_IDENT
        #return 0
    #end_cond
    #call   peek_kind(1)
    #if_cond r0 == TK_COLON
        #return 1
    #end_cond
    #return 0
#end_func

#def_func parse_compound()
    #var    t, nd, st
    #call   cur()
    move    t, r0
    #call   expect(TK_LBRACE)
    #call   nd_new(ND_BLOCK, t)
    move    nd, r0
    #call   vec_new()
    move    st, r0
    store   st, nd[N_VAL]
    #call   scope_push()
    #while  TRUE
        #call   consume(TK_RBRACE)
        #if_cond r0 != 0
            #break
        #end_cond
        #call   ck()
        #if_cond r0 == TK_EOF
            #call   error_expected("'}'")
        #end_cond
        #if_cond r0 == KW_SASSERT
            #call   static_assert_decl()
            #continue
        #end_cond
        #call   is_label_start()
        #if_cond r0 == 0
            #call   cur()
            #call   is_typename(r0)
            #if_cond r0 != 0
                #call   parse_local_decl(st)
                #continue
            #end_cond
        #end_cond
        #call   parse_stmt()
        #call   vec_push(st, r0)
    #end_while
    #call   scope_pop()
    #return nd
#end_func

// label_record(entry, tok) -> goto label record for the current function
#def_func label_record(ep, tk)
    #var    e, v, cnt, i, l
    load    e, ep
    load    v, FN_LABELS
    #macro  VLEN(cnt, v)
    #for    1, i <= cnt, 1
        #macro  VGET(l, v, i)
        load    r0, l[LB_ENTRY]
        #if_cond r0 == e
            #return l
        #end_cond
    #end_for
    #call   zalloc(LB_SIZE)
    move    l, r0
    store   e, l[LB_ENTRY]
    load    r1, tk
    store   r1, l[LB_TOK]
    #call   new_label("U", e)
    store   r0, l[LB_LABEL]
    #call   vec_push(v, l)
    #return l
#end_func

// cond_expr(): '(' expression ')' that must be scalar
#def_func cond_expr()
    #var    t, nd
    #call   expect(TK_LPAREN)
    #call   cur()
    move    t, r0
    #call   parse_expr()
    move    nd, r0
    #call   expect(TK_RPAREN)
    #call   check_scalar(nd, t)
    #return nd
#end_func

#def_func check_scalar(ndp, tk)
    #var    nd
    load    nd, ndp
    load    r4, tk
    #call   check_void(nd, r4)
    #call   rtype(nd)
    #call   ty_is_scalar(r0)
    #if_cond r0 == 0
        load    r1, tk
        #call   error_tok(r1, "used struct type value where scalar is required")
    #end_cond
#end_func

///////////////////////////////////////////////////////////////////////////////
// statement
///////////////////////////////////////////////////////////////////////////////
#def_func parse_stmt()
    #var    t, k, nd, a, b, c, d, sw, val, l, st, ft, rt
    #call   cur()
    move    t, r0
    load    k, t[TK_KIND]
    #if_cond k == TK_LBRACE
        #call   parse_compound()
        #return r0
    #end_cond
    #if_cond k == TK_SEMI
        #call   advance()
        #call   nd_new(ND_NULL, t)
        #return r0
    #end_cond
    #if_cond k == KW_IF
        #call   advance()
        #call   cond_expr()
        move    a, r0
        #call   parse_stmt()
        move    b, r0
        move    c, 0
        #call   consume(KW_ELSE)
        #if_cond r0 != 0
            #call   parse_stmt()
            move    c, r0
        #end_cond
        #call   nd_new(ND_IF, t)
        move    nd, r0
        store   a, nd[N_A]
        store   b, nd[N_B]
        store   c, nd[N_C]
        #return nd
    #end_cond
    #if_cond k == KW_WHILE
        #call   advance()
        #call   cond_expr()
        move    b, r0
        #call   loop_body()
        move    d, r0
        #call   nd_new(ND_FOR, t)
        move    nd, r0
        store   b, nd[N_B]
        store   d, nd[N_D]
        #return nd
    #end_cond
    #if_cond k == KW_DO
        #call   advance()
        #call   loop_body()
        move    a, r0
        #call   expect(KW_WHILE)
        #call   cond_expr()
        move    b, r0
        #call   expect(TK_SEMI)
        #call   nd_new(ND_DO, t)
        move    nd, r0
        store   a, nd[N_A]
        store   b, nd[N_B]
        #return nd
    #end_cond
    #if_cond k == KW_FOR
        #call   advance()
        #call   expect(TK_LPAREN)
        #call   scope_push()
        move    a, 0
        move    b, 0
        move    c, 0
        #call   cur()
        #call   is_typename(r0)
        #if_cond r0 != 0
            #call   nd_new(ND_BLOCK, t)
            move    a, r0
            #call   vec_new()
            store   r0, a[N_VAL]
            #call   parse_local_decl(r0)
        #else_cond
            #call   consume(TK_SEMI)
            #if_cond r0 == 0
                #call   cur()
                move    st, r0
                #call   parse_expr()
                #call   nd_unary(ND_EXPR, r0, st)
                move    a, r0
                #call   expect(TK_SEMI)
            #end_cond
        #end_cond
        #call   ck()
        #if_cond r0 != TK_SEMI
            #call   cur()
            move    st, r0
            #call   parse_expr()
            move    b, r0
            #call   check_scalar(b, st)
        #end_cond
        #call   expect(TK_SEMI)
        #call   ck()
        #if_cond r0 != TK_RPAREN
            #call   parse_expr()
            move    c, r0
        #end_cond
        #call   expect(TK_RPAREN)
        #call   loop_body()
        move    d, r0
        #call   scope_pop()
        #call   nd_new(ND_FOR, t)
        move    nd, r0
        store   a, nd[N_A]
        store   b, nd[N_B]
        store   c, nd[N_C]
        store   d, nd[N_D]
        #return nd
    #end_cond
    #if_cond k == KW_SWITCH
        #call   advance()
        #call   expect(TK_LPAREN)
        #call   cur()
        move    st, r0
        #call   parse_expr()
        move    a, r0
        #call   expect(TK_RPAREN)
        #call   rtype(a)
        #call   ty_is_integer(r0)
        #if_cond r0 == 0
            #call   error_tok(st, "switch quantity not an integer")
        #end_cond
        load    r1, a[N_TYPE]
        #call   promote(r1)
        #call   sem_conv(a, r0)
        move    a, r0
        #call   nd_new(ND_SWITCH, t)
        move    nd, r0
        store   a, nd[N_A]
        #call   vec_new()
        store   r0, nd[N_VAL]
        load    sw, CUR_SWITCH
        store   nd, CUR_SWITCH
        load    r0, BRK_DEPTH
        add     r0, 1
        store   r0, BRK_DEPTH
        #call   parse_stmt()
        store   r0, nd[N_B]
        load    r0, BRK_DEPTH
        sub     r0, 1
        store   r0, BRK_DEPTH
        store   sw, CUR_SWITCH
        #return nd
    #end_cond
    #if_cond k == KW_CASE
        #call   advance()
        load    sw, CUR_SWITCH
        #if_cond sw == 0
            #call   error_tok(t, "case label not within a switch statement")
        #end_cond
        #call   const_expr()
        move    val, r0
        #call   expect(TK_COLON)
        // duplicate check
        load    st, sw[N_VAL]
        #macro  VLEN(c, st)
        #for    1, d <= c, 1
            #macro  VGET(l, st, d)
            load    r0, l[N_VAL]
            #if_cond r0 == val
                #call   error_tok(t, "duplicate case value")
            #end_cond
        #end_for
        #call   nd_new(ND_CASE, t)
        move    nd, r0
        store   val, nd[N_VAL]
        #call   vec_push(st, nd)
        #call   case_body()
        store   r0, nd[N_A]
        #return nd
    #end_cond
    #if_cond k == KW_DEFAULT
        #call   advance()
        load    sw, CUR_SWITCH
        #if_cond sw == 0
            #call   error_tok(t, "'default' label not within a switch statement")
        #end_cond
        #call   expect(TK_COLON)
        load    r0, sw[N_VAL2]
        #if_cond r0 != 0
            #call   error_tok(t, "multiple default labels in one switch")
        #end_cond
        #call   nd_new(ND_CASE, t)
        move    nd, r0
        move    r1, -1
        store   r1, nd[N_VAL2]
        store   nd, sw[N_VAL2]
        #call   case_body()
        store   r0, nd[N_A]
        #return nd
    #end_cond
    #if_cond k == KW_BREAK
        #call   advance()
        load    r0, BRK_DEPTH
        #if_cond r0 == 0
            #call   error_tok(t, "break statement not within loop or switch")
        #end_cond
        #call   expect(TK_SEMI)
        #call   nd_new(ND_BREAK, t)
        #return r0
    #end_cond
    #if_cond k == KW_CONTINUE
        #call   advance()
        load    r0, LOOP_DEPTH
        #if_cond r0 == 0
            #call   error_tok(t, "continue statement not within a loop")
        #end_cond
        #call   expect(TK_SEMI)
        #call   nd_new(ND_CONTINUE, t)
        #return r0
    #end_cond
    #if_cond k == KW_RETURN
        #call   advance()
        load    r0, CUR_FUNC
        load    ft, r0[S_TYPE]
        load    rt, ft[T_BASE]
        move    a, 0
        #call   ck()
        #if_cond r0 != TK_SEMI
            #call   cur()
            move    st, r0
            #call   parse_expr()
            move    a, r0
            #call   ty_kind(rt)
            #if_cond r0 == TY_VOID
                load    r1, a[N_TYPE]
                #call   ty_kind(r1)
                #if_cond r0 != TY_VOID
                    #call   error_tok(st, "'return' with a value, in function returning void")
                #end_cond
            #else_cond
                #call   sem_assign_conv(a, rt, st, "incompatible types when returning")
                move    a, r0
            #end_cond
        #end_cond
        #call   expect(TK_SEMI)
        #call   nd_new(ND_RETURN, t)
        store   a, r0[N_A]
        #return r0
    #end_cond
    #if_cond k == KW_GOTO
        #call   advance()
        #call   expect_ident()
        move    st, r0
        load    r1, st[TK_VAL]
        #call   label_record(r1, st)
        move    l, r0
        #call   expect(TK_SEMI)
        #call   nd_new(ND_GOTO, t)
        move    nd, r0
        load    r1, st[TK_VAL]
        store   r1, nd[N_VAL]
        store   l, nd[N_VAL2]
        #return nd
    #end_cond
    #if_cond k == KW_SASSERT
        #call   static_assert_decl()
        #call   nd_new(ND_NULL, t)
        #return r0
    #end_cond
    #call   is_label_start()
    #if_cond r0 != 0
        #call   advance()
        #call   advance()
        load    r1, t[TK_VAL]
        #call   label_record(r1, t)
        move    l, r0
        load    r0, l[LB_DEFINED]
        #if_cond r0 != 0
            load    r1, t[TK_VAL]
            #call   error_tok2(t, "duplicate label", r1)
        #end_cond
        move    r0, 1
        store   r0, l[LB_DEFINED]
        #call   nd_new(ND_LABEL, t)
        move    nd, r0
        load    r1, t[TK_VAL]
        store   r1, nd[N_VAL]
        store   l, nd[N_VAL2]
        #call   case_body()
        store   r0, nd[N_A]
        #return nd
    #end_cond
    #call   cur()
    #call   is_typename(r0)
    #if_cond r0 != 0
        #call   error_tok(t, "a declaration is not a statement here; enclose it in braces")
    #end_cond
    #call   parse_expr()
    move    a, r0
    #call   expect(TK_SEMI)
    #call   nd_unary(ND_EXPR, a, t)
#end_func

// case_body(): statement after a label ('}' allowed: empty statement)
#def_func case_body()
    #call   ck()
    #if_cond r0 == TK_RBRACE
        #call   cur()
        #call   nd_new(ND_NULL, r0)
        #return r0
    #end_cond
    #call   parse_stmt()
#end_func

// loop_body(): statement inside a loop
#def_func loop_body()
    #var    nd
    load    r0, LOOP_DEPTH
    add     r0, 1
    store   r0, LOOP_DEPTH
    load    r0, BRK_DEPTH
    add     r0, 1
    store   r0, BRK_DEPTH
    #call   parse_stmt()
    move    nd, r0
    load    r0, LOOP_DEPTH
    sub     r0, 1
    store   r0, LOOP_DEPTH
    load    r0, BRK_DEPTH
    sub     r0, 1
    store   r0, BRK_DEPTH
    #return nd
#end_func

///////////////////////////////////////////////////////////////////////////////
// function definition
///////////////////////////////////////////////////////////////////////////////
#def_func parse_funcdef(sp_, tp, tk)
    #var    s, ty, nt, pnames, ptypes, cnt, i, off, ps, prm, pt, pn, body, l, rt
    load    s, sp_
    load    ty, tp
    load    nt, tk
    load    r0, s[S_FLAGS]
    and     r0, SF_DEFINED
    #if_cond r0 != 0
        load    r1, s[S_NAME]
        #call   error_tok2(nt, "redefinition of", r1)
    #end_cond
    // a definition replaces a runtime-provided function (keeping its label)
    load    r0, s[S_FLAGS]
    and     r0, -137                    // clear SF_BUILTIN and SF_LAZY
    or      r0, SF_DEFINED
    store   r0, s[S_FLAGS]
    store   ty, s[S_TYPE]
    store   s, CUR_FUNC
    move    r0, 0
    store   r0, CUR_FRAME
    store   r0, CUR_SWITCH
    store   r0, LOOP_DEPTH
    store   r0, BRK_DEPTH
    #call   vec_new()
    store   r0, FN_LABELS
    #call   scope_push()
    load    pnames, ty[T_PNAMES]
    load    ptypes, ty[T_PARAMS]
    #call   vec_new()
    move    ps, r0
    store   ps, s[S_PARAMS]
    move    off, 3
    load    rt, ty[T_BASE]
    #call   ty_is_struct(rt)
    #if_cond r0 != 0
        add     off, 1                  // hidden struct return pointer
    #end_cond
    #macro  VLEN(cnt, ptypes)
    #for    1, i <= cnt, 1
        #macro  VGET(pt, ptypes, i)
        #macro  VGET(pn, pnames, i)
        #if_cond pn == 0
            #call   error_tok(nt, "parameter name omitted")
        #end_cond
        load    r1, pn[TK_VAL]
        #call   check_redecl(r1, pn)
        load    r1, pn[TK_VAL]
        #call   sym_declare(r1, SK_LVAR, pt, pn)
        move    prm, r0
        store   off, prm[S_OFFSET]
        load    r1, prm[S_FLAGS]
        or      r1, SF_PARAM
        store   r1, prm[S_FLAGS]
        #call   vec_push(ps, prm)
        load    r1, pt[T_SIZE]
        #if_cond r1 < 1
            move    r1, 1
        #end_cond
        add     off, r1
    #end_for
    #call   parse_compound()
    move    body, r0
    #call   scope_pop()
    store   body, s[S_INIT]
    load    r0, CUR_FRAME
    store   r0, s[S_FRAME]
    // every label used by goto must be defined
    load    l, FN_LABELS
    #macro  VLEN(cnt, l)
    #for    1, i <= cnt, 1
        #macro  VGET(prm, l, i)
        load    r0, prm[LB_DEFINED]
        #if_cond r0 == 0
            load    r1, prm[LB_TOK]
            load    r2, prm[LB_ENTRY]
            #call   error_tok2(r1, "label used but not defined", r2)
        #end_cond
    #end_for
    load    r0, CC_MODE
    #if_cond r0 == MODE_AST
        #call   dump_function(s)
    #else_cond
        #call   gen_function(s)
    #end_cond
    move    r0, 0
    store   r0, CUR_FUNC
#end_func

///////////////////////////////////////////////////////////////////////////////
// translation unit
///////////////////////////////////////////////////////////////////////////////
#def_func parse_tu()
    #var    t, bt, sc, ty, e, nt, s, first, k
    #while  TRUE
        #call   cur()
        move    t, r0
        load    k, t[TK_KIND]
        #if_cond k == TK_EOF
            #break
        #end_cond
        #call   consume(TK_SEMI)
        #if_cond r0 != 0
            #continue
        #end_cond
        #if_cond k == KW_SASSERT
            #call   static_assert_decl()
            #continue
        #end_cond
        #if_cond k == KW_ASM
            #call   error_tok(t, "asm declarations are not supported")
        #end_cond
        // implicit int: name(...) { ... }
        #call   is_typename(t)
        #if_cond r0 == 0
            #if_cond k == TK_IDENT
                #call   peek_kind(1)
                #if_cond r0 == TK_LPAREN
                    load    bt, TY_INT_T
                    move    sc, 0
                    jump    PTU_DECLS
                #end_cond
            #end_cond
            #call   error_expected("declaration specifiers")
        #end_cond
        #call   declspec()
        move    bt, r0
        load    sc, DS_STORAGE
        #call   consume(TK_SEMI)
        #if_cond r0 != 0
            #continue
        #end_cond
PTU_DECLS:
        move    first, 1
        #while  TRUE
            #call   declarator(bt)
            move    ty, r0
            load    e, DECL_NAME
            load    nt, DECL_TOK
            #if_cond e == 0
                #call   error_tok(nt, "expected identifier or '('")
            #end_cond
            and     r0, sc, SC_TYPEDEF
            #if_cond r0 != 0
                #call   typedef_decl(e, ty, nt)
            #else_cond
                #call   ty_kind(ty)
                #if_cond r0 == TY_FUNC
                    #call   declare_function(e, ty, sc, nt)
                    move    s, r0
                    #if_cond first != 0
                        #call   ck()
                        #if_cond r0 == TK_LBRACE
                            load    r1, nt[TK_FILE]
                            #call   is_lib_file(r1)
                            #if_cond r0 != 0
                                #call   skip_lazy_body(s, ty)
                            #else_cond
                                #call   parse_funcdef(s, ty, nt)
                            #end_cond
                            #break
                        #end_cond
                    #end_cond
                #else_cond
                    #call   ty_kind(ty)
                    #if_cond r0 == TY_VOID
                        #call   error_tok2(nt, "variable declared void", e)
                    #end_cond
                    #call   declare_global(e, ty, sc, nt)
                    move    s, r0
                    #call   global_init(s, nt)
                    load    r0, CC_MODE
                    #if_cond r0 == MODE_AST
                        #call   dump_global(s)
                    #end_cond
                #end_cond
            #end_cond
            move    first, 0
            #call   consume(TK_COMMA)
            #if_cond r0 == 0
                #call   expect(TK_SEMI)
                #break
            #end_cond
        #end_while
    #end_while
#end_func

///////////////////////////////////////////////////////////////////////////////
// skip_lazy_body(sym, type): record a prelude function body for on-demand
// compilation and skip over it
///////////////////////////////////////////////////////////////////////////////
#def_func skip_lazy_body(sp_, tp)
    #var    s, depth, k
    load    s, sp_
    // a function the program already defined keeps its definition; the
    // library body is skipped without being recorded
    load    r0, s[S_FLAGS]
    and     r0, SF_DEFINED
    #if_cond r0 == 0
        load    r1, P_POS
        store   r1, s[S_BODYPOS]
        load    r1, tp
        store   r1, s[S_TYPE]
        load    r0, s[S_FLAGS]
        or      r0, SF_LAZY
        store   r0, s[S_FLAGS]
    #end_cond
    move    depth, 0
    #while  TRUE
        #call   ck()
        move    k, r0
        #if_cond k == TK_EOF
            #call   error_expected("'}'")
        #end_cond
        #call   advance()
        #if_cond k == TK_LBRACE
            add     depth, 1
        #end_cond
        #if_cond k == TK_RBRACE
            sub     depth, 1
            #if_cond depth == 0
                #break
            #end_cond
        #end_cond
    #end_while
#end_func

///////////////////////////////////////////////////////////////////////////////
// compile_lazy(): compile every used prelude function body (repeating until
// no new functions become used)
///////////////////////////////////////////////////////////////////////////////
#def_func compile_lazy()
    #var    fs, cnt, i, s, changed, fl, svpos, t
    load    svpos, P_POS
    move    changed, 1
    #while  changed != 0
        move    changed, 0
        load    fs, FUNCS
        #macro  VLEN(cnt, fs)
        #for    1, i <= cnt, 1
            #macro  VGET(s, fs, i)
            load    fl, s[S_FLAGS]
            and     r0, fl, SF_LAZY
            #if_cond r0 != 0
                and     r0, fl, SF_USED
                #if_cond r0 != 0
                    load    r0, s[S_BODYPOS]
                    store   r0, P_POS
                    #call   cur()
                    move    t, r0
                    load    r2, s[S_TYPE]
                    #call   parse_funcdef(s, r2, t)
                    move    changed, 1
                #end_cond
            #end_cond
        #end_for
    #end_while
    store   svpos, P_POS
#end_func

///////////////////////////////////////////////////////////////////////////////
// AST dumps
///////////////////////////////////////////////////////////////////////////////
#def_func dump_function(sp_)
    #var    s
    load    s, sp_
    load    r1, s[S_NAME]
    #call   ent_p(r1)
    #call   printf("(func %s ", r0)
    load    r1, s[S_TYPE]
    #call   dump_type(r1)
    load    r1, s[S_FRAME]
    #call   printf(" frame=%d\n", r1)
    load    r1, s[S_INIT]
    #call   ast_dump_node(r1, 1)
    #call   printf(")\n")
#end_func

#def_func dump_global(sp_)
    #var    s, lv, cnt, i, l
    load    s, sp_
    load    r1, s[S_NAME]
    #call   ent_p(r1)
    #call   printf("(global %s ", r0)
    load    r1, s[S_TYPE]
    #call   dump_type(r1)
    load    r1, s[S_TYPE]
    load    r1, r1[T_SIZE]
    #call   printf(" size=%d", r1)
    load    lv, s[S_INIT]
    #if_cond lv != 0
        #macro  VLEN(cnt, lv)
        #for    1, i <= cnt, 1
            #macro  VGET(l, lv, i)
            load    r1, l[L_OFF]
            #call   printf("\n  [%d] ", r1)
            load    r1, l[L_EXPR]
            #call   ast_dump_node(r1, 0)
        #end_for
    #end_cond
    #call   printf(")\n")
#end_func

PARSE_STMT_ASM_END:
    nop
