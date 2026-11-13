///////////////////////////////////////////////////////////////////////////////
// sema.asm
//
// Semantic analysis: type rules, implicit conversions and checked node
// constructors.  The parser calls these as it builds the AST, so every
// expression node is fully typed and every implicit conversion is an
// explicit ND_CAST node by the time code generation runs.
//
//   ty_decay(t)                array -> pointer, function -> pointer
//   sem_conv(node, ty)         convert (inserting ND_CAST if needed)
//   sem_assign_conv(node, ty, tok, msg)  checked assignment conversion
//   sem_binary(kind, a, b, tok)          + - * / % & | ^ << >> == ... && ||
//   sem_unary(kind, a, tok)              neg + ~ ! & *
//   sem_assign / sem_opassign / sem_incdec / sem_cast / sem_call /
//   sem_member / sem_cond / sem_comma
///////////////////////////////////////////////////////////////////////////////

SEMA_ASM_START:
    jump    SEMA_ASM_END

// ty_decay(t) -> pointer type for arrays and functions, else t
#def_func ty_decay(tp)
    #var    t, k
    load    t, tp
    #call   ty_kind(t)
    move    k, r0
    #if_cond k == TY_ARRAY
        load    r1, t[T_BASE]
        #call   ty_pointer_to(r1)
        #return r0
    #end_cond
    #if_cond k == TY_FUNC
        #call   ty_pointer_to(t)
        #return r0
    #end_cond
    #return t
#end_func

// rtype(node) -> decayed type of an expression
#def_func rtype(ndp)
    load    r0, ndp
    load    r1, r0[N_TYPE]
    #call   ty_decay(r1)
#end_func

// promote(t) -> type after integer promotion
#def_func promote(tp)
    #var    t, k
    load    t, tp
    #call   ty_kind(t)
    move    k, r0
    #if_cond k == TY_INT
        #return t
    #end_cond
    #if_cond k == TY_DOUBLE
        #return t
    #end_cond
    #call   ty_is_integer(t)
    #if_cond r0 != 0
        load    r0, TY_INT_T
        #return r0
    #end_cond
    #return t
#end_func

// usual_arith(ta, tb) -> common arithmetic type
#def_func usual_arith(tap, tbp)
    #var    ta, tb
    load    ta, tap
    load    tb, tbp
    #call   ty_is_double(ta)
    #if_cond r0 != 0
        load    r0, TY_DOUBLE_T
        #return r0
    #end_cond
    #call   ty_is_double(tb)
    #if_cond r0 != 0
        load    r0, TY_DOUBLE_T
        #return r0
    #end_cond
    #call   ty_is_unsigned(ta)
    #if_cond r0 != 0
        #call   ty_kind(ta)
        #if_cond r0 == TY_INT
            load    r0, TY_UINT_T
            #return r0
        #end_cond
    #end_cond
    #call   ty_is_unsigned(tb)
    #if_cond r0 != 0
        #call   ty_kind(tb)
        #if_cond r0 == TY_INT
            load    r0, TY_UINT_T
            #return r0
        #end_cond
    #end_cond
    load    r0, TY_INT_T
#end_func

///////////////////////////////////////////////////////////////////////////////
// sem_conv(node, ty) -> node converted to ty
///////////////////////////////////////////////////////////////////////////////
#def_func sem_conv(ndp, tp)
    #var    nd, t, c
    load    nd, ndp
    load    t, tp
    load    r1, nd[N_TYPE]
    #call   ty_same(r1, t)
    #if_cond r0 != 0
        #return nd
    #end_cond
    #call   nd_at(ND_CAST, nd)
    move    c, r0
    store   nd, c[N_A]
    store   t, c[N_TYPE]
    #return c
#end_func

// check_void(node, tok): error if the expression has void type
#def_func check_void(ndp, tk)
    #var    nd
    load    nd, ndp
    load    r1, nd[N_TYPE]
    #call   ty_kind(r1)
    #if_cond r0 == TY_VOID
        load    r1, tk
        #call   error_tok(r1, "void value not ignored as it ought to be")
    #end_cond
#end_func

// is_lvalue(node) -> 1 if node designates an object
#def_func is_lvalue(ndp)
    #var    nd, k, s
    load    nd, ndp
    load    k, nd[N_KIND]
    #if_cond k == ND_VAR
        load    s, nd[N_VAL]
        load    r0, s[S_KIND]
        #if_cond r0 == SK_LVAR
            #return 1
        #end_cond
        #if_cond r0 == SK_GVAR
            #return 1
        #end_cond
        #return 0
    #end_cond
    #if_cond k == ND_DEREF
        #return 1
    #end_cond
    #if_cond k == ND_MEMBER
        #return 1
    #end_cond
    #if_cond k == ND_STR
        #return 1
    #end_cond
    // compound literal: (init..., hidden variable)
    #if_cond k == ND_COMMA
        load    s, nd[N_B]
        load    r0, s[N_KIND]
        #if_cond r0 == ND_VAR
            load    r0, s[N_VAL]
            load    r0, r0[S_NAME]
            load    r1, ANON_TAG
            #if_cond r0 == r1
                #return 1
            #end_cond
        #end_cond
    #end_cond
    #return 0
#end_func

// check_modifiable(node, tok, msg)
#def_func check_modifiable(ndp, tk, msg)
    #var    nd, k
    load    nd, ndp
    #call   is_lvalue(nd)
    #if_cond r0 == 0
        load    r1, tk
        load    r2, msg
        #call   error_tok(r1, r2)
    #end_cond
    load    r1, nd[N_TYPE]
    #call   ty_kind(r1)
    move    k, r0
    #if_cond k == TY_ARRAY
        load    r1, tk
        #call   error_tok(r1, "assignment to expression with array type")
    #end_cond
    #if_cond k == TY_FUNC
        load    r1, tk
        load    r2, msg
        #call   error_tok(r1, r2)
    #end_cond
#end_func

///////////////////////////////////////////////////////////////////////////////
// sem_assign_conv(node, ty, tok, msg) -> node converted for assignment to ty
///////////////////////////////////////////////////////////////////////////////
#def_func sem_assign_conv(ndp, tp, tk, msg)
    #var    nd, t, tb, kt, kb
    load    nd, ndp
    load    t, tp
    load    r4, tk
    #call   check_void(nd, r4)
    #call   rtype(nd)
    move    tb, r0
    #call   ty_kind(t)
    move    kt, r0
    #call   ty_kind(tb)
    move    kb, r0
    // packed string parameters accept literals and char strings
    #if_cond kt == TY_PSTR
        load    r4, tk
        #call   to_pstr(nd, r4)
        #return r0
    #end_cond
    #call   ty_is_struct(t)
    #if_cond r0 != 0
        load    r1, nd[N_TYPE]
        #call   ty_same(r1, t)
        #if_cond r0 == 0
            load    r1, tk
            load    r2, msg
            #call   error_tok(r1, r2)
        #end_cond
        #return nd
    #end_cond
    #call   ty_is_struct(tb)
    #if_cond r0 != 0
        load    r1, tk
        load    r2, msg
        #call   error_tok(r1, r2)
    #end_cond
    #if_cond kt == TY_PTR
        #if_cond kb == TY_DOUBLE
            load    r1, tk
            load    r2, msg
            #call   error_tok(r1, r2)
        #end_cond
        #call   ty_is_ptrlike(tb)
        #if_cond r0 != 0
            #return nd
        #end_cond
        #call   sem_conv(nd, t)
        #return r0
    #end_cond
    #call   ty_is_ptrlike(tb)
    #if_cond r0 != 0
        #if_cond kt == TY_DOUBLE
            load    r1, tk
            load    r2, msg
            #call   error_tok(r1, r2)
        #end_cond
    #end_cond
    #call   sem_conv(nd, t)
#end_func

///////////////////////////////////////////////////////////////////////////////
// to_pstr(node, tok) -> node usable as a packed string
//      string literal -> ND_STR typed __pstr;  char*/char[] -> ND_C2P
///////////////////////////////////////////////////////////////////////////////
#def_func to_pstr(ndp, tk)
    #var    nd, c, k
    load    nd, ndp
    load    k, nd[N_KIND]
    // look through casts / decay of a literal
    #if_cond k == ND_STR
        #call   nd_at(ND_STR, nd)
        move    c, r0
        load    r1, nd[N_VAL]
        store   r1, c[N_VAL]
        load    r1, TY_PSTR_T
        store   r1, c[N_TYPE]
        #return c
    #end_cond
    load    r1, nd[N_TYPE]
    #call   ty_kind(r1)
    #if_cond r0 == TY_PSTR
        #return nd
    #end_cond
    load    r1, nd[N_TYPE]
    #call   ty_is_char_ptr(r1)
    #if_cond r0 != 0
        #call   nd_at(ND_C2P, nd)
        move    c, r0
        store   nd, c[N_A]
        load    r1, TY_PSTR_T
        store   r1, c[N_TYPE]
        #return c
    #end_cond
    load    r1, tk
    #call   error_tok(r1, "expected a string argument")
#end_func

///////////////////////////////////////////////////////////////////////////////
// scale(node, size) -> integer node multiplied by size (words)
///////////////////////////////////////////////////////////////////////////////
#def_func scale(ndp, szp, tk)
    #var    nd, sz, c
    load    nd, ndp
    load    sz, szp
    load    r1, TY_INT_T
    #call   sem_conv(nd, r1)
    move    nd, r0
    #if_cond sz == 1
        #return nd
    #end_cond
    load    r0, nd[N_KIND]
    #if_cond r0 == ND_NUM
        load    r1, nd[N_VAL]
        mult    r1, sz
        load    r2, TY_INT_T
        load    r3, tk
        #call   nd_number(r1, r2, r3)
        #return r0
    #end_cond
    load    r1, TY_INT_T
    load    r3, tk
    #call   nd_number(sz, r1, r3)
    move    c, r0
    load    r3, tk
    #call   nd_binary(ND_MUL, nd, c, r3)
    load    r1, TY_INT_T
    store   r1, r0[N_TYPE]
#end_func

// ptr_elem_size(ptr_type, tok) -> size of the pointee in words
#def_func ptr_elem_size(tp, tk)
    #var    t, b, sz
    load    t, tp
    load    b, t[T_BASE]
    #call   ty_kind(b)
    #if_cond r0 == TY_VOID
        #return 1
    #end_cond
    #if_cond r0 == TY_FUNC
        #return 1
    #end_cond
    load    sz, b[T_SIZE]
    #if_cond sz <= 0
        load    r1, tk
        #call   error_tok(r1, "arithmetic on a pointer to an incomplete type")
    #end_cond
    #return sz
#end_func

// mk_typed(kind, a, b, type, tok)
#def_func mk_typed(kind, ap, bp, tp, tk)
    #var    nd
    load    r1, kind
    load    r2, ap
    load    r3, bp
    load    r0, tk
    #call   nd_binary(r1, r2, r3, r0)
    move    nd, r0
    load    r1, tp
    store   r1, nd[N_TYPE]
    #return nd
#end_func

#def_func bad_operands(tk, kind)
    #var    sb
    #call   vec_new()
    move    sb, r0
    #call   sb_add_p(sb, "invalid operands to binary ")
    load    r1, kind
    #call   nd_name(r1)
    #call   sb_add_p(sb, r0)
    #call   sb_to_p(sb)
    load    r1, tk
    #call   error_tok(r1, r0)
#end_func

///////////////////////////////////////////////////////////////////////////////
// sem_binary(kind, a, b, tok) -> typed node
///////////////////////////////////////////////////////////////////////////////
#def_func sem_binary(kind, ap, bp, tk)
    #var    k, a, b, ta, tb, t, sz, nd, ia, ib, pa, pb
    load    k, kind
    load    a, ap
    load    b, bp
    load    r4, tk
    #call   check_void(a, r4)
    load    r4, tk
    #call   check_void(b, r4)
    #call   rtype(a)
    move    ta, r0
    #call   rtype(b)
    move    tb, r0
    #call   ty_is_arith(ta)
    move    ia, r0
    #call   ty_is_arith(tb)
    move    ib, r0
    #call   ty_is_ptrlike(ta)
    move    pa, r0
    #call   ty_is_ptrlike(tb)
    move    pb, r0
    // ---- logical
    #if_cond k == ND_LAND
        jump    SB_LOGIC
    #end_cond
    #if_cond k == ND_LOR
        jump    SB_LOGIC
    #end_cond
    // ---- relational: a > b  ==>  b < a
    #if_cond k == ND_GT
        move    k, ND_LT
        move    t, a
        move    a, b
        move    b, t
        move    t, ta
        move    ta, tb
        move    tb, t
        move    t, ia
        move    ia, ib
        move    ib, t
        move    t, pa
        move    pa, pb
        move    pb, t
    #end_cond
    #if_cond k == ND_GE
        move    k, ND_LE
        move    t, a
        move    a, b
        move    b, t
        move    t, ta
        move    ta, tb
        move    tb, t
        move    t, ia
        move    ia, ib
        move    ib, t
        move    t, pa
        move    pa, pb
        move    pb, t
    #end_cond
    #if_cond k == ND_EQ
        jump    SB_COMPARE
    #end_cond
    #if_cond k == ND_NE
        jump    SB_COMPARE
    #end_cond
    #if_cond k == ND_LT
        jump    SB_COMPARE
    #end_cond
    #if_cond k == ND_LE
        jump    SB_COMPARE
    #end_cond
    // ---- additive
    #if_cond k == ND_ADD
        jump    SB_ADD
    #end_cond
    #if_cond k == ND_SUB
        jump    SB_SUB
    #end_cond
    // ---- multiplicative / bitwise
    and     r0, ia, ib
    #if_cond r0 == 0
        load    r4, tk
        #call   bad_operands(r4, k)
    #end_cond
    #if_cond k == ND_MUL
        jump    SB_ARITH
    #end_cond
    #if_cond k == ND_DIV
        jump    SB_ARITH
    #end_cond
    // integer-only operators
    #call   ty_is_integer(ta)
    move    ia, r0
    #call   ty_is_integer(tb)
    and     r0, ia
    #if_cond r0 == 0
        load    r4, tk
        #call   bad_operands(r4, k)
    #end_cond
    #if_cond k == ND_SHL
        jump    SB_SHIFT
    #end_cond
    #if_cond k == ND_SHR
        jump    SB_SHIFT
    #end_cond
SB_ARITH:
    #call   usual_arith(ta, tb)
    move    t, r0
    #call   sem_conv(a, t)
    move    a, r0
    #call   sem_conv(b, t)
    move    b, r0
    load    r4, tk
    #call   mk_typed(k, a, b, t, r4)
    #return r0
SB_SHIFT:
    #call   promote(ta)
    move    t, r0
    #call   sem_conv(a, t)
    move    a, r0
    load    r1, TY_INT_T
    #call   sem_conv(b, r1)
    move    b, r0
    load    r4, tk
    #call   mk_typed(k, a, b, t, r4)
    #return r0
SB_LOGIC:
    #call   ty_is_scalar(ta)
    move    ia, r0
    #call   ty_is_scalar(tb)
    and     r0, ia
    #if_cond r0 == 0
        load    r4, tk
        #call   bad_operands(r4, k)
    #end_cond
    load    r1, TY_INT_T
    load    r4, tk
    #call   mk_typed(k, a, b, r1, r4)
    #return r0
SB_COMPARE:
    and     r0, ia, ib
    #if_cond r0 != 0
        #call   usual_arith(ta, tb)
        move    t, r0
        #call   sem_conv(a, t)
        move    a, r0
        #call   sem_conv(b, t)
        move    b, r0
    #else_cond
        // pointer comparisons (pointer vs pointer or integer)
        or      r0, pa, pb
        #if_cond r0 == 0
            load    r4, tk
            #call   bad_operands(r4, k)
        #end_cond
        #if_cond pa == 0
            #if_cond ia == 0
                load    r4, tk
                #call   bad_operands(r4, k)
            #end_cond
        #end_cond
        #if_cond pb == 0
            #if_cond ib == 0
                load    r4, tk
                #call   bad_operands(r4, k)
            #end_cond
        #end_cond
    #end_cond
    load    r1, TY_INT_T
    load    r4, tk
    #call   mk_typed(k, a, b, r1, r4)
    #return r0
SB_ADD:
    and     r0, ia, ib
    #if_cond r0 != 0
        jump    SB_ARITH
    #end_cond
    // integer + pointer  ==>  pointer + integer
    #if_cond pb != 0
        #if_cond pa == 0
            move    t, a
            move    a, b
            move    b, t
            move    t, ta
            move    ta, tb
            move    tb, t
            move    t, ia
            move    ia, ib
            move    ib, t
            move    t, pa
            move    pa, pb
            move    pb, t
        #end_cond
    #end_cond
    #if_cond pa == 0
        load    r4, tk
        #call   bad_operands(r4, k)
    #end_cond
    #call   ty_is_integer(tb)
    #if_cond r0 == 0
        load    r4, tk
        #call   bad_operands(r4, k)
    #end_cond
    load    r4, tk
    #call   ptr_elem_size(ta, r4)
    move    sz, r0
    load    r4, tk
    #call   scale(b, sz, r4)
    move    b, r0
    load    r4, tk
    #call   mk_typed(ND_ADD, a, b, ta, r4)
    #return r0
SB_SUB:
    and     r0, ia, ib
    #if_cond r0 != 0
        jump    SB_ARITH
    #end_cond
    #if_cond pa == 0
        load    r4, tk
        #call   bad_operands(r4, k)
    #end_cond
    #if_cond pb != 0
        // pointer difference
        load    r4, tk
        #call   ptr_elem_size(ta, r4)
        move    sz, r0
        load    r1, TY_INT_T
        load    r4, tk
        #call   mk_typed(ND_SUB, a, b, r1, r4)
        move    nd, r0
        #if_cond sz == 1
            #return nd
        #end_cond
        load    r1, TY_INT_T
        load    r3, tk
        #call   nd_number(sz, r1, r3)
        load    r1, TY_INT_T
        load    r4, tk
        #call   mk_typed(ND_DIV, nd, r0, r1, r4)
        #return r0
    #end_cond
    #call   ty_is_integer(tb)
    #if_cond r0 == 0
        load    r4, tk
        #call   bad_operands(r4, k)
    #end_cond
    load    r4, tk
    #call   ptr_elem_size(ta, r4)
    move    sz, r0
    load    r4, tk
    #call   scale(b, sz, r4)
    move    b, r0
    load    r4, tk
    #call   mk_typed(ND_SUB, a, b, ta, r4)
#end_func

///////////////////////////////////////////////////////////////////////////////
// sem_unary(kind, a, tok) -> typed node
///////////////////////////////////////////////////////////////////////////////
#def_func sem_unary(kind, ap, tk)
    #var    k, a, ta, t, nd, b
    load    k, kind
    load    a, ap
    #if_cond k == ND_ADDR
        #call   is_lvalue(a)
        #if_cond r0 == 0
            load    r1, a[N_TYPE]
            #call   ty_kind(r1)
            #if_cond r0 != TY_FUNC
                load    r1, tk
                #call   error_tok(r1, "lvalue required as unary '&' operand")
            #end_cond
        #end_cond
        load    r1, a[N_TYPE]
        #call   ty_pointer_to(r1)
        move    t, r0
        load    r3, tk
        #call   nd_unary(ND_ADDR, a, r3)
        store   t, r0[N_TYPE]
        #return r0
    #end_cond
    load    r4, tk
    #call   check_void(a, r4)
    #call   rtype(a)
    move    ta, r0
    #if_cond k == ND_DEREF
        #call   ty_is_ptrlike(ta)
        #if_cond r0 == 0
            load    r1, tk
            #call   error_tok(r1, "invalid type argument of unary '*'")
        #end_cond
        load    b, ta[T_BASE]
        #call   ty_kind(b)
        #if_cond r0 == TY_VOID
            load    r1, tk
            #call   error_tok(r1, "dereferencing 'void *' pointer")
        #end_cond
        // *function is the function itself
        #if_cond r0 == TY_FUNC
            #return a
        #end_cond
        load    r3, tk
        #call   nd_unary(ND_DEREF, a, r3)
        store   b, r0[N_TYPE]
        #return r0
    #end_cond
    #if_cond k == ND_NOT
        #call   ty_is_scalar(ta)
        #if_cond r0 == 0
            load    r1, tk
            #call   error_tok(r1, "invalid operand to unary '!'")
        #end_cond
        load    r3, tk
        #call   nd_unary(ND_NOT, a, r3)
        load    r1, TY_INT_T
        store   r1, r0[N_TYPE]
        #return r0
    #end_cond
    #if_cond k == ND_BNOT
        #call   ty_is_integer(ta)
        #if_cond r0 == 0
            load    r1, tk
            #call   error_tok(r1, "invalid operand to unary '~'")
        #end_cond
    #else_cond
        #call   ty_is_arith(ta)
        #if_cond r0 == 0
            load    r1, tk
            #call   error_tok(r1, "invalid operand to unary operator")
        #end_cond
    #end_cond
    #call   promote(ta)
    move    t, r0
    #call   sem_conv(a, t)
    move    a, r0
    #if_cond k == ND_ADD
        #return a                       // unary +
    #end_cond
    load    r3, tk
    #call   nd_unary(k, a, r3)
    store   t, r0[N_TYPE]
#end_func

///////////////////////////////////////////////////////////////////////////////
// sem_assign(a, b, tok)
///////////////////////////////////////////////////////////////////////////////
#def_func sem_assign(ap, bp, tk)
    #var    a, b, ta
    load    a, ap
    load    b, bp
    load    r4, tk
    #call   check_modifiable(a, r4, "lvalue required as left operand of assignment")
    load    ta, a[N_TYPE]
    load    r4, tk
    #call   sem_assign_conv(b, ta, r4, "incompatible types in assignment")
    move    b, r0
    load    r4, tk
    #call   mk_typed(ND_ASSIGN, a, b, ta, r4)
#end_func

///////////////////////////////////////////////////////////////////////////////
// sem_opassign(op, a, b, tok): a op= b
///////////////////////////////////////////////////////////////////////////////
#def_func sem_opassign(opk, ap, bp, tk)
    #var    op, a, b, ta, tb, t, nd, sz
    load    op, opk
    load    a, ap
    load    b, bp
    load    r4, tk
    #call   check_modifiable(a, r4, "lvalue required as left operand of assignment")
    load    r4, tk
    #call   check_void(b, r4)
    load    ta, a[N_TYPE]
    #call   rtype(b)
    move    tb, r0
    #call   ty_kind(ta)
    #if_cond r0 == TY_PTR
        #if_cond op == ND_ADD
            jump    SO_PTR
        #end_cond
        #if_cond op == ND_SUB
            jump    SO_PTR
        #end_cond
        load    r4, tk
        #call   bad_operands(r4, op)
    #end_cond
    // compute the operator's type with an ordinary binary node
    load    r4, tk
    #call   sem_binary(op, a, b, r4)
    move    nd, r0
    load    t, nd[N_TYPE]
    load    b, nd[N_B]
    load    r4, tk
    #call   mk_typed(ND_OPASSIGN, a, b, ta, r4)
    move    nd, r0
    store   op, nd[N_VAL]
    store   t, nd[N_VAL2]
    #return nd
SO_PTR:
    #call   ty_is_integer(tb)
    #if_cond r0 == 0
        load    r4, tk
        #call   bad_operands(r4, op)
    #end_cond
    load    r4, tk
    #call   ptr_elem_size(ta, r4)
    move    sz, r0
    load    r4, tk
    #call   scale(b, sz, r4)
    move    b, r0
    load    r4, tk
    #call   mk_typed(ND_OPASSIGN, a, b, ta, r4)
    move    nd, r0
    store   op, nd[N_VAL]
    store   ta, nd[N_VAL2]
    #return nd
#end_func

///////////////////////////////////////////////////////////////////////////////
// sem_incdec(a, delta, post, tok)
///////////////////////////////////////////////////////////////////////////////
#def_func sem_incdec(ap, deltap, postp, tk)
    #var    a, d, ta, nd
    load    a, ap
    load    d, deltap
    load    r4, tk
    #call   check_modifiable(a, r4, "lvalue required as increment operand")
    load    ta, a[N_TYPE]
    #call   ty_is_scalar(ta)
    #if_cond r0 == 0
        load    r1, tk
        #call   error_tok(r1, "wrong type argument to increment")
    #end_cond
    #call   ty_kind(ta)
    #if_cond r0 == TY_PTR
        load    r4, tk
        #call   ptr_elem_size(ta, r4)
        mult    d, r0
    #end_cond
    load    r3, tk
    #call   nd_unary(ND_INCDEC, a, r3)
    move    nd, r0
    store   ta, nd[N_TYPE]
    store   d, nd[N_VAL]
    load    r1, postp
    store   r1, nd[N_VAL2]
    #return nd
#end_func

///////////////////////////////////////////////////////////////////////////////
// sem_cast(a, ty, tok)
///////////////////////////////////////////////////////////////////////////////
#def_func sem_cast(ap, tp, tk)
    #var    a, t, ta, c
    load    a, ap
    load    t, tp
    #call   ty_kind(t)
    #if_cond r0 == TY_VOID
        #call   nd_at(ND_CAST, a)
        move    c, r0
        store   a, c[N_A]
        store   t, c[N_TYPE]
        #return c
    #end_cond
    load    r4, tk
    #call   check_void(a, r4)
    #call   rtype(a)
    move    ta, r0
    #call   ty_is_scalar(t)
    #if_cond r0 == 0
        load    r1, tk
        #call   error_tok(r1, "conversion to non-scalar type requested")
    #end_cond
    #call   ty_is_scalar(ta)
    #if_cond r0 == 0
        load    r1, tk
        #call   error_tok(r1, "cannot convert a non-scalar value")
    #end_cond
    #call   ty_is_double(t)
    #if_cond r0 != 0
        #call   ty_is_ptrlike(ta)
        #if_cond r0 != 0
            load    r1, tk
            #call   error_tok(r1, "pointer value used where a floating-point was expected")
        #end_cond
    #end_cond
    #call   nd_at(ND_CAST, a)
    move    c, r0
    store   a, c[N_A]
    store   t, c[N_TYPE]
    #return c
#end_func

///////////////////////////////////////////////////////////////////////////////
// sem_call(fn, args, tok) -> ND_CALL
///////////////////////////////////////////////////////////////////////////////
#def_func sem_call(fnp, argsp, tk)
    #var    fn, margs, ft, ps, nparm, na, i, a, pt, nd, name, bi
    load    fn, fnp
    load    margs, argsp
    load    ft, fn[N_TYPE]
    #call   ty_kind(ft)
    #if_cond r0 == TY_PTR
        load    ft, ft[T_BASE]
    #end_cond
    #call   ty_kind(ft)
    #if_cond r0 != TY_FUNC
        load    r1, tk
        #call   error_tok(r1, "called object is not a function or function pointer")
    #end_cond
    move    name, 0
    load    r0, fn[N_KIND]
    #if_cond r0 == ND_VAR
        load    r0, fn[N_VAL]
        load    name, r0[S_NAME]
    #end_cond
    load    ps, ft[T_PARAMS]
    #macro  VLEN(nparm, ps)
    #macro  VLEN(na, margs)
    load    r0, ft[T_FLAGS]
    and     r0, TYF_OLDSTYLE
    #if_cond r0 == 0
        #if_cond na < nparm
            #if_cond name != 0
                load    r1, tk
                #call   error_tok2(r1, "too few arguments to function", name)
            #end_cond
            load    r1, tk
            #call   error_tok(r1, "too few arguments to function")
        #end_cond
        #if_cond na > nparm
            load    r0, ft[T_FLAGS]
            and     r0, TYF_VARIADIC
            #if_cond r0 == 0
                #if_cond name != 0
                    load    r1, tk
                    #call   error_tok2(r1, "too many arguments to function", name)
                #end_cond
                load    r1, tk
                #call   error_tok(r1, "too many arguments to function")
            #end_cond
        #end_cond
    #end_cond
    // calls to runtime library functions get packed-string conversions
    move    bi, 0
    #if_cond name != 0
        load    r0, fn[N_VAL]
        load    r0, r0[S_FLAGS]
        and     r0, SF_PRELUDE
        #if_cond r0 != 0
            move    bi, 1
        #end_cond
    #end_cond
    #for    1, i <= na, 1
        #macro  VGET(a, margs, i)
        #if_cond i <= nparm
            #macro  VGET(pt, ps, i)
            load    r4, tk
            #call   sem_assign_conv(a, pt, r4, "incompatible type for argument")
            move    a, r0
            // printf-style format literal: rewrite C conversions for CPUSim64
            #if_cond bi != 0
                #if_cond i == nparm
                    load    r0, ft[T_FLAGS]
                    and     r0, TYF_VARIADIC
                    #if_cond r0 != 0
                        #call   fix_format_node(a)
                        move    a, r0
                    #end_cond
                #end_cond
            #end_cond
        #else_cond
            #if_cond bi != 0
                #call   rtype(a)
                #call   ty_is_char_ptr(r0)
                #if_cond r0 != 0
                    load    r4, tk
                    #call   to_pstr(a, r4)
                    move    a, r0
                #end_cond
            #end_cond
            load    r4, tk
            #call   check_void(a, r4)
            load    r1, a[N_TYPE]
            #call   ty_is_integer(r1)
            #if_cond r0 != 0
                load    r1, a[N_TYPE]
                #call   promote(r1)
                #call   sem_conv(a, r0)
                move    a, r0
            #end_cond
        #end_cond
        #macro  VSET(margs, i, a)
    #end_for
    #if_cond bi != 0
        load    r0, ft[T_FLAGS]
        and     r0, TYF_VARIADIC
        #if_cond r0 != 0
            #if_cond nparm > 0
                load    r4, tk
                #call   check_format(margs, nparm, r4)
                #call   lower_unsigned_format(margs, nparm)
            #end_cond
        #end_cond
    #end_cond
    load    r3, tk
    #call   nd_unary(ND_CALL, fn, r3)
    move    nd, r0
    store   margs, nd[N_VAL]
    store   ft, nd[N_VAL2]
    load    r1, ft[T_BASE]
    store   r1, nd[N_TYPE]
    #return nd
#end_func

///////////////////////////////////////////////////////////////////////////////
// fix_format_node(node) -> node; a packed-string literal format is rewritten
//      so that C conversions work with the CPUSim64 (Java) formatter:
//      length modifiers (h l L q j z t) are dropped, %i -> %d,
//      %p -> %x, %F -> %f.
///////////////////////////////////////////////////////////////////////////////
#def_func is_fmt_flag(ch)
    #var    c
    load    c, ch
    #if_cond c == '-'
        #return 1
    #end_cond
    #if_cond c == '+'
        #return 1
    #end_cond
    #if_cond c == ' '
        #return 1
    #end_cond
    #if_cond c == '#'
        #return 1
    #end_cond
    #if_cond c == '.'
        #return 1
    #end_cond
    #call   is_digit(c)
#end_func

#def_func is_fmt_len(ch)
    #var    c
    load    c, ch
    #if_cond c == 'h'
        #return 1
    #end_cond
    #if_cond c == 'l'
        #return 1
    #end_cond
    #if_cond c == 'L'
        #return 1
    #end_cond
    #if_cond c == 'q'
        #return 1
    #end_cond
    #if_cond c == 'j'
        #return 1
    #end_cond
    #if_cond c == 'z'
        #return 1
    #end_cond
    #if_cond c == 't'
        #return 1
    #end_cond
    #return 0
#end_func

#def_func fix_format_node(ndp)
    #var    nd, w, cnt, i, c, outv, nw
    load    nd, ndp
    load    r0, nd[N_KIND]
    #if_cond r0 != ND_STR
        #return nd
    #end_cond
    load    r1, STRLITS
    load    r2, nd[N_VAL]
    #macro  VGET(w, r1, r2)
    load    cnt, w[0]
    #call   vec_new()
    move    outv, r0
    move    i, 1
    #while  i <= cnt
        load    c, w[i]
        add     i, 1
        #call   vec_push(outv, c)
        #if_cond c != '%'
            #continue
        #end_cond
        #if_cond i > cnt
            #break
        #end_cond
        load    c, w[i]
        #if_cond c == '%'
            #call   vec_push(outv, c)
            add     i, 1
            #continue
        #end_cond
        #while  i <= cnt
            load    c, w[i]
            #call   is_fmt_flag(c)
            #if_cond r0 == 0
                #break
            #end_cond
            #call   vec_push(outv, c)
            add     i, 1
        #end_while
        #while  i <= cnt
            load    c, w[i]
            #call   is_fmt_len(c)
            #if_cond r0 == 0
                #break
            #end_cond
            add     i, 1
        #end_while
        #if_cond i > cnt
            #break
        #end_cond
        load    c, w[i]
        add     i, 1
        #if_cond c == 'i'
            move    c, 'd'
        #end_cond
        #if_cond c == 'p'
            move    c, 'x'
        #end_cond
        #if_cond c == 'F'
            move    c, 'f'
        #end_cond
        #call   vec_push(outv, c)
    #end_while
    #call   sb_to_w(outv)
    move    nw, r0
    #call   add_strlit(nw)
    move    c, r0
    #call   nd_at(ND_STR, nd)
    move    outv, r0
    store   c, outv[N_VAL]
    load    r1, nd[N_TYPE]
    store   r1, outv[N_TYPE]
    #return outv
#end_func

///////////////////////////////////////////////////////////////////////////////
// check_format(args, fmt_index, tok): the CPUSim64 formatter cannot survive
//      mismatched printf arguments, so a literal format is checked here.
///////////////////////////////////////////////////////////////////////////////
#def_func check_format(ap, fip, tk)
    #var    margs, fi, nd, w, cnt, i, c, ai, na, a, t, k
    load    margs, ap
    load    fi, fip
    #macro  VGET(nd, margs, fi)
    load    r0, nd[N_KIND]
    #if_cond r0 != ND_STR
        #return 0
    #end_cond
    load    r1, STRLITS
    load    r2, nd[N_VAL]
    #macro  VGET(w, r1, r2)
    load    cnt, w[0]
    #macro  VLEN(na, margs)
    add     ai, fi, 1
    move    i, 1
    #while  i <= cnt
        load    c, w[i]
        add     i, 1
        #if_cond c != '%'
            #continue
        #end_cond
        #if_cond i > cnt
            #break
        #end_cond
        load    c, w[i]
        #if_cond c == '%'
            add     i, 1
            #continue
        #end_cond
        #while  i <= cnt
            load    c, w[i]
            #call   is_fmt_flag(c)
            #if_cond r0 == 0
                #break
            #end_cond
            add     i, 1
        #end_while
        #if_cond i > cnt
            #break
        #end_cond
        load    c, w[i]
        add     i, 1
        #if_cond ai > na
            load    r1, tk
            #call   error_tok(r1, "too few arguments for format")
        #end_cond
        #macro  VGET(a, margs, ai)
        add     ai, 1
        #call   rtype(a)
        move    t, r0
        #call   ty_kind(t)
        move    k, r0
        #if_cond c == 's'
            #if_cond k != TY_PSTR
                load    r1, tk
                #call   error_tok(r1, "format '%s' expects a string argument")
            #end_cond
            #continue
        #end_cond
        #if_cond c == 'f'
            jump    CF_DOUBLE
        #end_cond
        #if_cond c == 'e'
            jump    CF_DOUBLE
        #end_cond
        #if_cond c == 'g'
            jump    CF_DOUBLE
        #end_cond
        #if_cond c == 'E'
            jump    CF_DOUBLE
        #end_cond
        #if_cond c == 'G'
            jump    CF_DOUBLE
        #end_cond
        #if_cond k == TY_DOUBLE
            load    r1, tk
            #call   error_tok(r1, "format expects an integer argument but the argument has type 'double'")
        #end_cond
        #call   ty_is_struct(t)
        #if_cond r0 != 0
            load    r1, tk
            #call   error_tok(r1, "format expects an integer argument")
        #end_cond
        #continue
CF_DOUBLE:
        #if_cond k != TY_DOUBLE
            load    r1, tk
            #call   error_tok(r1, "format expects a 'double' argument")
        #end_cond
    #end_while
#end_func

///////////////////////////////////////////////////////////////////////////////
// lower_unsigned_format(args, fmt_index): the CPUSim64 formatter has no
//      unsigned conversion, so each %u in a literal format becomes %s and its
//      argument is converted at run time to a decimal packed string
//      (ND_U2P, freed after the call).  The '0' flag and a precision become
//      zero padding done by the converter (Java rejects '0' with %s).
///////////////////////////////////////////////////////////////////////////////
#def_func lower_unsigned_format(ap, fip)
    #var    margs, fi, nd, w, cnt, i, c, ai, outv, st, zero, minus, width, prec, pad, j, a, u, nw
    load    margs, ap
    load    fi, fip
    #macro  VGET(nd, margs, fi)
    load    r0, nd[N_KIND]
    #if_cond r0 != ND_STR
        #return 0
    #end_cond
    load    r1, STRLITS
    load    r2, nd[N_VAL]
    #macro  VGET(w, r1, r2)
    load    cnt, w[0]
    // quick exit when there is no %u
    move    u, 0
    move    i, 1
    #while  i <= cnt
        load    c, w[i]
        add     i, 1
        #if_cond c == '%'
            #while  i <= cnt
                load    c, w[i]
                #call   is_fmt_flag(c)
                #if_cond r0 == 0
                    #break
                #end_cond
                add     i, 1
            #end_while
            #if_cond i <= cnt
                load    c, w[i]
                add     i, 1
                #if_cond c == 'u'
                    move    u, 1
                #end_cond
            #end_cond
        #end_cond
    #end_while
    #if_cond u == 0
        #return 0
    #end_cond
    #call   vec_new()
    move    outv, r0
    add     ai, fi, 1
    move    i, 1
    #while  i <= cnt
        load    c, w[i]
        add     i, 1
        #call   vec_push(outv, c)
        #if_cond c != '%'
            #continue
        #end_cond
        #if_cond i > cnt
            #break
        #end_cond
        load    c, w[i]
        #if_cond c == '%'
            #call   vec_push(outv, c)
            add     i, 1
            #continue
        #end_cond
        // flags, width and precision of this conversion
        move    st, i
        move    zero, 0
        move    minus, 0
        move    width, 0
        move    prec, -1
        #while  i <= cnt
            load    c, w[i]
            #if_cond c == '0'
                #if_cond width == 0
                    #if_cond prec < 0
                        move    zero, 1
                    #end_cond
                #end_cond
            #end_cond
            #if_cond c == '-'
                move    minus, 1
            #end_cond
            #if_cond c == '.'
                move    prec, 0
                add     i, 1
                #continue
            #end_cond
            #call   is_digit(c)
            #if_cond r0 != 0
                sub     r1, c, '0'
                #if_cond prec >= 0
                    mult    prec, 10
                    add     prec, r1
                #else_cond
                    #if_cond c != '0'
                        mult    width, 10
                        add     width, r1
                    #else_cond
                        #if_cond width != 0
                            mult    width, 10
                        #end_cond
                    #end_cond
                #end_cond
                add     i, 1
                #continue
            #end_cond
            #call   is_fmt_flag(c)
            #if_cond r0 == 0
                #break
            #end_cond
            add     i, 1
        #end_while
        #if_cond i > cnt
            #break
        #end_cond
        load    c, w[i]
        add     i, 1
        #if_cond c != 'u'
            // copy the conversion unchanged
            move    j, st
            #while  j < i
                load    r1, w[j]
                #call   vec_push(outv, r1)
                add     j, 1
            #end_while
            add     ai, 1
            #continue
        #end_cond
        // %u: zero padding moves into the converter
        move    pad, 0
        #if_cond zero != 0
            #if_cond minus == 0
                move    pad, width
                move    width, 0
            #end_cond
        #end_cond
        #if_cond prec > pad
            move    pad, prec
        #end_cond
        #if_cond minus != 0
            #call   vec_push(outv, '-')
        #end_cond
        #if_cond width > 0
            #call   sb_add_int(outv, width)
        #end_cond
        #call   vec_push(outv, 's')
        #macro  VGET(a, margs, ai)
        #call   nd_at(ND_U2P, a)
        move    nw, r0
        store   a, nw[N_A]
        store   pad, nw[N_VAL]
        load    r1, TY_PSTR_T
        store   r1, nw[N_TYPE]
        #macro  VSET(margs, ai, nw)
        add     ai, 1
    #end_while
    #call   sb_to_w(outv)
    #call   add_strlit(r0)
    store   r0, nd[N_VAL]
#end_func

///////////////////////////////////////////////////////////////////////////////
// find_member(struct_type, entry) -> Member or 0
///////////////////////////////////////////////////////////////////////////////
#def_func find_member(tp, ep)
    #var    t, e, ms, cnt, i, m
    load    t, tp
    load    e, ep
    load    ms, t[T_MEMBERS]
    #if_cond ms == 0
        #return 0
    #end_cond
    #macro  VLEN(cnt, ms)
    #for    1, i <= cnt, 1
        #macro  VGET(m, ms, i)
        load    r0, m[MB_NAME]
        #if_cond r0 == e
            #return m
        #end_cond
    #end_for
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// sem_member(a, name_tok, arrow) -> ND_MEMBER
///////////////////////////////////////////////////////////////////////////////
#def_func sem_member(ap, ntk, arrowp)
    #var    a, nt, t, m, nd
    load    a, ap
    load    nt, ntk
    load    r0, arrowp
    #if_cond r0 != 0
        #call   rtype(a)
        move    t, r0
        #call   ty_kind(t)
        #if_cond r0 != TY_PTR
            #call   error_tok(nt, "invalid type argument of '->'")
        #end_cond
        #call   sem_unary(ND_DEREF, a, nt)
        move    a, r0
    #end_cond
    load    t, a[N_TYPE]
    #call   ty_is_struct(t)
    #if_cond r0 == 0
        #call   error_tok(nt, "request for a member in something that is not a structure or union")
    #end_cond
    load    r0, t[T_FLAGS]
    and     r0, TYF_COMPLETE
    #if_cond r0 == 0
        #call   error_tok(nt, "dereferencing pointer to incomplete type")
    #end_cond
    load    r1, nt[TK_VAL]
    #call   find_member(t, r1)
    move    m, r0
    #if_cond m == 0
        load    r1, nt[TK_VAL]
        #call   error_tok2(nt, "no member named", r1)
    #end_cond
    #call   nd_unary(ND_MEMBER, a, nt)
    move    nd, r0
    store   m, nd[N_VAL]
    load    r1, m[MB_TYPE]
    store   r1, nd[N_TYPE]
    #return nd
#end_func

///////////////////////////////////////////////////////////////////////////////
// sem_cond(c, a, b, tok) -> ND_COND
///////////////////////////////////////////////////////////////////////////////
#def_func sem_cond(cp, ap, bp, tk)
    #var    c, a, b, ta, tb, t, nd, ka, kb
    load    c, cp
    load    a, ap
    load    b, bp
    #call   rtype(c)
    #call   ty_is_scalar(r0)
    #if_cond r0 == 0
        load    r1, tk
        #call   error_tok(r1, "used struct type value where scalar is required")
    #end_cond
    #call   rtype(a)
    move    ta, r0
    #call   rtype(b)
    move    tb, r0
    #call   ty_kind(ta)
    move    ka, r0
    #call   ty_kind(tb)
    move    kb, r0
    move    t, 0
    #call   ty_is_arith(ta)
    move    nd, r0
    #call   ty_is_arith(tb)
    and     r0, nd
    #if_cond r0 != 0
        #call   usual_arith(ta, tb)
        move    t, r0
        #call   sem_conv(a, t)
        move    a, r0
        #call   sem_conv(b, t)
        move    b, r0
    #else_cond
        #if_cond ka == TY_VOID
            #if_cond kb == TY_VOID
                move    t, ta
            #end_cond
        #end_cond
        #if_cond ka == TY_PTR
            move    t, ta
            #if_cond kb == TY_PTR
                #call   ty_is_void_ptr(ta)
                #if_cond r0 != 0
                    move    t, tb
                #end_cond
            #end_cond
        #end_cond
        #if_cond t == 0
            #if_cond kb == TY_PTR
                move    t, tb
            #end_cond
        #end_cond
        #if_cond t == 0
            #call   ty_is_struct(ta)
            #if_cond r0 != 0
                #call   ty_same(ta, tb)
                #if_cond r0 != 0
                    move    t, ta
                #end_cond
            #end_cond
        #end_cond
        #if_cond t == 0
            load    r1, tk
            #call   error_tok(r1, "type mismatch in conditional expression")
        #end_cond
    #end_cond
    load    r3, tk
    #call   nd_new(ND_COND, r3)
    move    nd, r0
    store   c, nd[N_A]
    store   a, nd[N_B]
    store   b, nd[N_C]
    store   t, nd[N_TYPE]
    #return nd
#end_func

#def_func sem_comma(ap, bp, tk)
    #var    b
    load    b, bp
    load    r1, b[N_TYPE]
    load    r2, ap
    load    r0, tk
    #call   mk_typed(ND_COMMA, r2, b, r1, r0)
#end_func

SEMA_ASM_END:
    nop
