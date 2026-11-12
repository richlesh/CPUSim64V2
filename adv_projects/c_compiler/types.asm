///////////////////////////////////////////////////////////////////////////////
// types.asm
//
// C type objects.  Every scalar (char, int, long, double, pointer, ...)
// occupies exactly one 64-bit word, so sizes are measured in words.
//
//   types_init()               create the basic types
//   ty_pointer_to(base) / ty_array_of(base, len) / ty_function(ret) / ty_new(kind)
//   ty_is_integer / ty_is_arith / ty_is_scalar / ty_is_ptrlike ...
//   ty_compatible(a, b)        loose compatibility test
//   ty_print(sb, ty)           append a readable spelling of a type
///////////////////////////////////////////////////////////////////////////////

#global TY_VOID_T:    .dci 0
#global TY_BOOL_T:    .dci 0
#global TY_CHAR_T:    .dci 0
#global TY_UCHAR_T:   .dci 0
#global TY_INT_T:     .dci 0
#global TY_UINT_T:    .dci 0
#global TY_DOUBLE_T:  .dci 0
#global TY_PSTR_T:    .dci 0
#global TY_CHARP_T:   .dci 0

TYPES_ASM_START:
    jump    TYPES_ASM_END

///////////////////////////////////////////////////////////////////////////////
// ty_new(kind) -> new type of the given kind and size 1
///////////////////////////////////////////////////////////////////////////////
#def_func ty_new(kind)
    #var    t
    #call   zalloc(T_SIZEOF)
    move    t, r0
    load    r1, kind
    store   r1, t[T_KIND]
    move    r1, 1
    store   r1, t[T_SIZE]
    #return t
#end_func

// ty_copy(t) -> shallow copy of a type
#def_func ty_copy(tp)
    #var    t, c
    load    t, tp
    #call   zalloc(T_SIZEOF)
    move    c, r0
    move    r1, c
    move    r2, t
    move    r3, T_SIZEOF
    int     iMEMMOVE
    store   t, c[T_ORIGIN]
    #return c
#end_func

#def_func types_init()
    #var    t
    #call   ty_new(TY_VOID)
    store   r0, TY_VOID_T
    #call   ty_new(TY_BOOL)
    store   r0, TY_BOOL_T
    #call   ty_new(TY_BOOL)
    move    t, r0
    #call   ty_new(TY_CHAR)
    store   r0, TY_CHAR_T
    #call   ty_new(TY_CHAR)
    move    t, r0
    move    r1, TYF_UNSIGNED
    store   r1, t[T_FLAGS]
    store   t, TY_UCHAR_T
    #call   ty_new(TY_INT)
    store   r0, TY_INT_T
    #call   ty_new(TY_INT)
    move    t, r0
    move    r1, TYF_UNSIGNED
    store   r1, t[T_FLAGS]
    store   t, TY_UINT_T
    #call   ty_new(TY_DOUBLE)
    store   r0, TY_DOUBLE_T
    #call   ty_new(TY_PSTR)
    store   r0, TY_PSTR_T
    load    r1, TY_CHAR_T
    #call   ty_pointer_to(r1)
    store   r0, TY_CHARP_T
#end_func

// ty_pointer_to(base) -> pointer to base
#def_func ty_pointer_to(basep)
    #var    t
    #call   ty_new(TY_PTR)
    move    t, r0
    load    r1, basep
    store   r1, t[T_BASE]
    #return t
#end_func

// ty_array_of(base, len) -> array of len elements (len -1 = incomplete)
#def_func ty_array_of(basep, lenp)
    #var    t, b, l, sz
    load    b, basep
    load    l, lenp
    #call   ty_new(TY_ARRAY)
    move    t, r0
    store   b, t[T_BASE]
    store   l, t[T_LEN]
    load    sz, b[T_SIZE]
    #if_cond l < 0
        move    sz, 0
    #else_cond
        mult    sz, l
    #end_cond
    store   sz, t[T_SIZE]
    #return t
#end_func

// ty_function(ret) -> function type returning ret (params filled by caller)
#def_func ty_function(retp)
    #var    t
    #call   ty_new(TY_FUNC)
    move    t, r0
    load    r1, retp
    store   r1, t[T_BASE]
    #call   vec_new()
    store   r0, t[T_PARAMS]
    #call   vec_new()
    store   r0, t[T_PNAMES]
    #return t
#end_func

// kind accessor
#def_func ty_kind(tp)
    load    r0, tp
    #if_cond r0 == 0
        #return 0
    #end_cond
    load    r0, r0[T_KIND]
#end_func

///////////////////////////////////////////////////////////////////////////////
// predicates
///////////////////////////////////////////////////////////////////////////////
#def_func ty_is_integer(tp)
    #var    k
    load    r1, tp
    #call   ty_kind(r1)
    move    k, r0
    #if_cond k == TY_BOOL
        #return 1
    #end_cond
    #if_cond k == TY_CHAR
        #return 1
    #end_cond
    #if_cond k == TY_INT
        #return 1
    #end_cond
    #if_cond k == TY_ENUM
        #return 1
    #end_cond
    #return 0
#end_func

#def_func ty_is_double(tp)
    load    r1, tp
    #call   ty_kind(r1)
    #if_cond r0 == TY_DOUBLE
        #return 1
    #end_cond
    #return 0
#end_func

#def_func ty_is_arith(tp)
    load    r1, tp
    #call   ty_is_integer(r1)
    #if_cond r0 != 0
        #return 1
    #end_cond
    load    r1, tp
    #call   ty_is_double(r1)
#end_func

// pointer-like: pointers, arrays (decay), functions (decay), pstr
#def_func ty_is_ptrlike(tp)
    #var    k
    load    r1, tp
    #call   ty_kind(r1)
    move    k, r0
    #if_cond k == TY_PTR
        #return 1
    #end_cond
    #if_cond k == TY_ARRAY
        #return 1
    #end_cond
    #if_cond k == TY_FUNC
        #return 1
    #end_cond
    #if_cond k == TY_PSTR
        #return 1
    #end_cond
    #return 0
#end_func

#def_func ty_is_scalar(tp)
    load    r1, tp
    #call   ty_is_arith(r1)
    #if_cond r0 != 0
        #return 1
    #end_cond
    load    r1, tp
    #call   ty_is_ptrlike(r1)
#end_func

#def_func ty_is_struct(tp)
    #var    k
    load    r1, tp
    #call   ty_kind(r1)
    move    k, r0
    #if_cond k == TY_STRUCT
        #return 1
    #end_cond
    #if_cond k == TY_UNION
        #return 1
    #end_cond
    #return 0
#end_func

#def_func ty_is_unsigned(tp)
    #var    t
    load    t, tp
    #call   ty_kind(t)
    #if_cond r0 == TY_PTR
        #return 1
    #end_cond
    #call   ty_is_integer(t)
    #if_cond r0 == 0
        #return 0
    #end_cond
    load    r0, t[T_FLAGS]
    and     r0, TYF_UNSIGNED
    #if_cond r0 != 0
        #return 1
    #end_cond
    #return 0
#end_func

// ty_is_void_ptr(t)
#def_func ty_is_void_ptr(tp)
    #var    t
    load    t, tp
    #call   ty_kind(t)
    #if_cond r0 != TY_PTR
        #return 0
    #end_cond
    load    r1, t[T_BASE]
    #call   ty_kind(r1)
    #if_cond r0 == TY_VOID
        #return 1
    #end_cond
    #return 0
#end_func

// ty_is_char_ptr(t): char* or char[] (word string)
#def_func ty_is_char_ptr(tp)
    #var    t, k
    load    t, tp
    #call   ty_kind(t)
    move    k, r0
    #if_cond k == TY_ARRAY
        move    k, TY_PTR
    #end_cond
    #if_cond k != TY_PTR
        #return 0
    #end_cond
    load    r1, t[T_BASE]
    #call   ty_kind(r1)
    #if_cond r0 == TY_CHAR
        #return 1
    #end_cond
    #return 0
#end_func

// ty_base(t) -> pointee / element type
#def_func ty_base(tp)
    load    r0, tp
    load    r0, r0[T_BASE]
#end_func

///////////////////////////////////////////////////////////////////////////////
// ty_same(a, b) -> 1 if the types are the same (structurally)
///////////////////////////////////////////////////////////////////////////////
#def_func ty_same(ap, bp)
    #var    a, b, ka, kb, i, cnt, pa, pb
    load    a, ap
    load    b, bp
    #while  TRUE
        #if_cond a == b
            #return 1
        #end_cond
        load    r0, a[T_ORIGIN]
        #if_cond r0 != 0
            load    r1, a[T_KIND]
            load    r2, r0[T_KIND]
            #if_cond r1 == r2
                move    a, r0
                #continue
            #end_cond
        #end_cond
        load    r0, b[T_ORIGIN]
        #if_cond r0 != 0
            load    r1, b[T_KIND]
            load    r2, r0[T_KIND]
            #if_cond r1 == r2
                move    b, r0
                #continue
            #end_cond
        #end_cond
        load    ka, a[T_KIND]
        load    kb, b[T_KIND]
        // enums are compatible with int
        #if_cond ka == TY_ENUM
            move    ka, TY_INT
        #end_cond
        #if_cond kb == TY_ENUM
            move    kb, TY_INT
        #end_cond
        #if_cond ka != kb
            #return 0
        #end_cond
        #if_cond ka == TY_INT
            load    r0, a[T_FLAGS]
            load    r1, b[T_FLAGS]
            xor     r0, r1
            and     r0, TYF_UNSIGNED
            #if_cond r0 != 0
                #return 0
            #end_cond
        #end_cond
        #if_cond ka == TY_CHAR
            load    r0, a[T_FLAGS]
            load    r1, b[T_FLAGS]
            xor     r0, r1
            and     r0, TYF_UNSIGNED
            #if_cond r0 != 0
                #return 0
            #end_cond
        #end_cond
        #if_cond ka == TY_PTR
            load    a, a[T_BASE]
            load    b, b[T_BASE]
            #continue
        #end_cond
        #if_cond ka == TY_ARRAY
            load    a, a[T_BASE]
            load    b, b[T_BASE]
            #continue
        #end_cond
        #if_cond ka == TY_FUNC
            load    r1, a[T_BASE]
            load    r2, b[T_BASE]
            #call   ty_same(r1, r2)
            #if_cond r0 == 0
                #return 0
            #end_cond
            load    r0, a[T_FLAGS]
            load    r1, b[T_FLAGS]
            or      r0, r1
            and     r0, TYF_OLDSTYLE
            #if_cond r0 != 0
                #return 1
            #end_cond
            load    pa, a[T_PARAMS]
            load    pb, b[T_PARAMS]
            #macro  VLEN(cnt, pa)
            #macro  VLEN(r1, pb)
            #if_cond cnt != r1
                #return 0
            #end_cond
            #for    1, i <= cnt, 1
                #macro  VGET(r1, pa, i)
                #macro  VGET(r2, pb, i)
                #call   ty_same(r1, r2)
                #if_cond r0 == 0
                    #return 0
                #end_cond
            #end_for
            #return 1
        #end_cond
        #if_cond ka == TY_STRUCT
            #return 0
        #end_cond
        #if_cond ka == TY_UNION
            #return 0
        #end_cond
        #return 1
    #end_while
#end_func

///////////////////////////////////////////////////////////////////////////////
// ty_print(sb, t): append a C-like spelling of type t to string builder sb
///////////////////////////////////////////////////////////////////////////////
#def_func ty_print(sbp, tp)
    #var    sb, t, k, cnt, i, ps
    load    sb, sbp
    load    t, tp
    #if_cond t == 0
        #call   sb_add_p(sb, "?")
        #return 0
    #end_cond
    load    k, t[T_KIND]
    load    r0, t[T_FLAGS]
    and     r0, TYF_UNSIGNED
    #if_cond r0 != 0
        #call   sb_add_p(sb, "unsigned ")
    #end_cond
    #if_cond k == TY_VOID
        #call   sb_add_p(sb, "void")
    #end_cond
    #if_cond k == TY_BOOL
        #call   sb_add_p(sb, "_Bool")
    #end_cond
    #if_cond k == TY_CHAR
        #call   sb_add_p(sb, "char")
    #end_cond
    #if_cond k == TY_INT
        #call   sb_add_p(sb, "int")
    #end_cond
    #if_cond k == TY_DOUBLE
        #call   sb_add_p(sb, "double")
    #end_cond
    #if_cond k == TY_PSTR
        #call   sb_add_p(sb, "__pstr")
    #end_cond
    #if_cond k == TY_ENUM
        #call   sb_add_p(sb, "enum ")
        load    r1, t[T_TAG]
        #call   ent_p(r1)
        #call   sb_add_p(sb, r0)
    #end_cond
    #if_cond k == TY_STRUCT
        #call   sb_add_p(sb, "struct ")
        load    r1, t[T_TAG]
        #call   ent_p(r1)
        #call   sb_add_p(sb, r0)
    #end_cond
    #if_cond k == TY_UNION
        #call   sb_add_p(sb, "union ")
        load    r1, t[T_TAG]
        #call   ent_p(r1)
        #call   sb_add_p(sb, r0)
    #end_cond
    #if_cond k == TY_PTR
        #call   sb_add_p(sb, "*")
        load    r1, t[T_BASE]
        #call   ty_print(sb, r1)
    #end_cond
    #if_cond k == TY_ARRAY
        #call   sb_add_p(sb, "[")
        load    r1, t[T_LEN]
        #if_cond r1 >= 0
            #call   sb_add_int(sb, r1)
        #end_cond
        #call   sb_add_p(sb, "]")
        load    r1, t[T_BASE]
        #call   ty_print(sb, r1)
    #end_cond
    #if_cond k == TY_FUNC
        #call   sb_add_p(sb, "(")
        load    ps, t[T_PARAMS]
        #macro  VLEN(cnt, ps)
        #for    1, i <= cnt, 1
            #if_cond i > 1
                #call   sb_add_p(sb, ",")
            #end_cond
            #macro  VGET(r1, ps, i)
            #call   ty_print(sb, r1)
        #end_for
        load    r0, t[T_FLAGS]
        and     r0, TYF_VARIADIC
        #if_cond r0 != 0
            #call   sb_add_p(sb, ",...")
        #end_cond
        #call   sb_add_p(sb, ")")
        load    r1, t[T_BASE]
        #call   ty_print(sb, r1)
    #end_cond
#end_func

TYPES_ASM_END:
    nop
