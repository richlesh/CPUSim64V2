///////////////////////////////////////////////////////////////////////////////
// parse_decl.asm
//
// Declarations (CParser.g4 6.7):
//   declarationSpecifiers, declarator / directDeclarator, abstract
//   declarators, parameter lists, struct/union/enum specifiers, typedef
//   names and initializers.
//
// Initializers are lowered into "leaves": (word offset, type, expression)
// triples relative to the start of the object being initialized.
///////////////////////////////////////////////////////////////////////////////

#global DS_STORAGE: .dci 0          // storage class bits of the last declspec
#global DECL_NAME:  .dci 0          // intern entry of the last declarator (or 0)
#global DECL_TOK:   .dci 0          // token of the last declarator name
#global INIT_LEN:   .dci 0          // element count found by an array initializer
#global LABEL_ID:   .dci 0          // counter for generated labels
#global GLOBALS:    .dci 0          // vec of global variable Syms (in order)
#global FUNCS:      .dci 0          // vec of function Syms (in order)
#global CUR_FRAME:  .dci 0          // words of locals in the current function
#global ANON_TAG:   .dci 0          // entry used for anonymous struct tags

#define SC_TYPEDEF  1
#define SC_STATIC   2
#define SC_EXTERN   4
#define SC_INLINE   8

// type specifier counters (chibicc-style)
#define DS_VOID     1
#define DS_BOOL     4
#define DS_CHAR     16
#define DS_SHORT    64
#define DS_INT      256
#define DS_LONG     1024
#define DS_FLOAT    4096
#define DS_DOUBLE   16384
#define DS_OTHER    65536
#define DS_SIGNED   131072
#define DS_UNSIGNED 262144

// initializer leaf
#define L_OFF       0
#define L_TYPE      1
#define L_EXPR      2
#define L_SIZE      3

PARSE_DECL_ASM_START:
    jump    PARSE_DECL_ASM_END

#def_func decl_init()
    #call   vec_new()
    store   r0, GLOBALS
    #call   vec_new()
    store   r0, FUNCS
    #call   intern_p("<anonymous>")
    store   r0, ANON_TAG
#end_func

///////////////////////////////////////////////////////////////////////////////
// new_label(prefix, entry) -> packed "__<prefix><id>_<name>"
///////////////////////////////////////////////////////////////////////////////
#def_func new_label(prefix, ep)
    #var    sb, id, e
    load    e, ep
    load    id, LABEL_ID
    add     id, 1
    store   id, LABEL_ID
    #call   vec_new()
    move    sb, r0
    #call   sb_add_p(sb, "__")
    load    r1, prefix
    #call   sb_add_p(sb, r1)
    #call   sb_add_int(sb, id)
    #if_cond e != 0
        #call   vec_push(sb, '_')
        load    r1, e[E_W]
        #call   sb_add_w(sb, r1)
    #end_cond
    #call   sb_to_p(sb)
#end_func

///////////////////////////////////////////////////////////////////////////////
// link_label(prefix, entry, storage, tok) -> packed label of a file-scope
//      function or object.  In module mode (several files, or -lstd) a
//      symbol with external linkage gets the stable label "__c_<name>" that
//      other modules can import; library objects get it only with -lstd
//      (without -lstd every module has its own private copy of the library).
//      Otherwise, and for static symbols, a private generated label.
///////////////////////////////////////////////////////////////////////////////
#def_func link_label(prefix, ep, stp, tk)
    #var    e, st, sb
    load    e, ep
    load    st, stp
    load    r0, CC_MODULE
    #if_cond r0 == MOD_NONE
        jump    LINK_LABEL_PRIVATE
    #end_cond
    and     r0, st, SC_STATIC
    #if_cond r0 != 0
        jump    LINK_LABEL_PRIVATE
    #end_cond
    load    r1, tk
    load    r1, r1[TK_FILE]
    #call   is_lib_file(r1)
    #if_cond r0 != 0
        load    r0, CC_LSTD
        #if_cond r0 == 0
            jump    LINK_LABEL_PRIVATE
        #end_cond
    #end_cond
    #call   vec_new()
    move    sb, r0
    #call   sb_add_p(sb, "__c_")
    load    r1, e[E_W]
    #call   sb_add_w(sb, r1)
    #call   sb_to_p(sb)
    #return r0
LINK_LABEL_PRIVATE:
    load    r1, prefix
    #call   new_label(r1, e)
#end_func

///////////////////////////////////////////////////////////////////////////////
// is_typename(tok) -> 1 if tok starts a declaration specifier / type name
///////////////////////////////////////////////////////////////////////////////
#def_func is_typename(tk)
    #var    t, k, s
    load    t, tk
    load    k, t[TK_KIND]
    #if_cond k == TK_IDENT
        load    r1, t[TK_VAL]
        #call   sym_lookup(r1)
        move    s, r0
        #if_cond s == 0
            #return 0
        #end_cond
        load    r0, s[S_KIND]
        #if_cond r0 == SK_TYPEDEF
            #return 1
        #end_cond
        #return 0
    #end_cond
    #if_cond k < KW_BASE
        #return 0
    #end_cond
    // keywords that are part of declaration specifiers
    #if_cond k == KW_AUTO
        #return 1
    #end_cond
    #if_cond k == KW_CHAR
        #return 1
    #end_cond
    #if_cond k == KW_CONST
        #return 1
    #end_cond
    #if_cond k == KW_DOUBLE
        #return 1
    #end_cond
    #if_cond k == KW_ENUM
        #return 1
    #end_cond
    #if_cond k == KW_EXTERN
        #return 1
    #end_cond
    #if_cond k == KW_FLOAT
        #return 1
    #end_cond
    #if_cond k == KW_INLINE
        #return 1
    #end_cond
    #if_cond k == KW_INT
        #return 1
    #end_cond
    #if_cond k == KW_LONG
        #return 1
    #end_cond
    #if_cond k == KW_REGISTER
        #return 1
    #end_cond
    #if_cond k == KW_RESTRICT
        #return 1
    #end_cond
    #if_cond k == KW_SHORT
        #return 1
    #end_cond
    #if_cond k == KW_SIGNED
        #return 1
    #end_cond
    #if_cond k == KW_STATIC
        #return 1
    #end_cond
    #if_cond k == KW_STRUCT
        #return 1
    #end_cond
    #if_cond k == KW_TYPEDEF
        #return 1
    #end_cond
    #if_cond k == KW_UNION
        #return 1
    #end_cond
    #if_cond k == KW_UNSIGNED
        #return 1
    #end_cond
    #if_cond k == KW_VOID
        #return 1
    #end_cond
    #if_cond k == KW_VOLATILE
        #return 1
    #end_cond
    #if_cond k == KW_BOOL_
        #return 1
    #end_cond
    #if_cond k == KW_BOOL
        #return 1
    #end_cond
    #if_cond k == KW_NORETURN
        #return 1
    #end_cond
    #if_cond k == KW_COMPLEX
        #return 1
    #end_cond
    #if_cond k == KW_IMAG
        #return 1
    #end_cond
    #if_cond k == KW_ATOMIC
        #return 1
    #end_cond
    #if_cond k == KW_ALIGNAS
        #return 1
    #end_cond
    #if_cond k == KW_THREADL
        #return 1
    #end_cond
    #if_cond k == KW_PSTR
        #return 1
    #end_cond
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// declspec() -> type;  DS_STORAGE = storage class bits
///////////////////////////////////////////////////////////////////////////////
#def_func declspec()
    #var    t, k, cnt, ty, st, s, depth, ints, start
    move    cnt, 0
    move    st, 0
    load    ty, TY_INT_T
    #call   cur()
    move    start, r0
    #while  TRUE
        #call   cur()
        move    t, r0
        load    k, t[TK_KIND]
        // ---- storage classes and function specifiers
        #if_cond k == KW_TYPEDEF
            or      st, SC_TYPEDEF
            #call   advance()
            #continue
        #end_cond
        #if_cond k == KW_STATIC
            or      st, SC_STATIC
            #call   advance()
            #continue
        #end_cond
        #if_cond k == KW_EXTERN
            or      st, SC_EXTERN
            #call   advance()
            #continue
        #end_cond
        #if_cond k == KW_INLINE
            or      st, SC_INLINE
            #call   advance()
            #continue
        #end_cond
        #if_cond k == KW_AUTO
            #call   advance()
            #continue
        #end_cond
        #if_cond k == KW_REGISTER
            #call   advance()
            #continue
        #end_cond
        #if_cond k == KW_NORETURN
            #call   advance()
            #continue
        #end_cond
        #if_cond k == KW_THREADL
            #call   advance()
            #continue
        #end_cond
        // ---- qualifiers (accepted and ignored)
        #if_cond k == KW_CONST
            #call   advance()
            #continue
        #end_cond
        #if_cond k == KW_VOLATILE
            #call   advance()
            #continue
        #end_cond
        #if_cond k == KW_RESTRICT
            #call   advance()
            #continue
        #end_cond
        #if_cond k == KW_ATOMIC
            #call   error_tok(t, "_Atomic is not supported")
        #end_cond
        #if_cond k == KW_COMPLEX
            #call   error_tok(t, "complex types are not supported")
        #end_cond
        #if_cond k == KW_IMAG
            #call   error_tok(t, "complex types are not supported")
        #end_cond
        #if_cond k == KW_ALIGNAS
            // _Alignas ( ... ) is accepted and ignored
            #call   advance()
            #call   expect(TK_LPAREN)
            move    depth, 1
            #while  depth > 0
                #call   ck()
                #if_cond r0 == TK_LPAREN
                    add     depth, 1
                #end_cond
                #if_cond r0 == TK_RPAREN
                    sub     depth, 1
                #end_cond
                #if_cond r0 == TK_EOF
                    #call   error_expected("')'")
                #end_cond
                #call   advance()
            #end_while
            #continue
        #end_cond
        // ---- user-defined types
        move    s, 0
        #if_cond k == TK_IDENT
            #if_cond cnt != 0
                #break
            #end_cond
            #call   is_typename(t)
            #if_cond r0 == 0
                #break
            #end_cond
            load    r1, t[TK_VAL]
            #call   sym_lookup(r1)
            load    ty, r0[S_TYPE]
            add     cnt, DS_OTHER
            #call   advance()
            #continue
        #end_cond
        #if_cond k == KW_STRUCT
            move    s, 1
        #end_cond
        #if_cond k == KW_UNION
            move    s, 1
        #end_cond
        #if_cond s != 0
            #if_cond cnt != 0
                #call   error_tok(t, "two or more data types in declaration specifiers")
            #end_cond
            #call   struct_union_decl()
            move    ty, r0
            add     cnt, DS_OTHER
            #continue
        #end_cond
        #if_cond k == KW_ENUM
            #if_cond cnt != 0
                #call   error_tok(t, "two or more data types in declaration specifiers")
            #end_cond
            #call   enum_decl()
            move    ty, r0
            add     cnt, DS_OTHER
            #continue
        #end_cond
        #if_cond k == KW_PSTR
            #if_cond cnt != 0
                #call   error_tok(t, "two or more data types in declaration specifiers")
            #end_cond
            load    ty, TY_PSTR_T
            add     cnt, DS_OTHER
            #call   advance()
            #continue
        #end_cond
        // ---- basic type specifiers
        move    s, 0
        #if_cond k == KW_VOID
            move    s, DS_VOID
        #end_cond
        #if_cond k == KW_BOOL_
            move    s, DS_BOOL
        #end_cond
        #if_cond k == KW_BOOL
            move    s, DS_BOOL
        #end_cond
        #if_cond k == KW_CHAR
            move    s, DS_CHAR
        #end_cond
        #if_cond k == KW_SHORT
            move    s, DS_SHORT
        #end_cond
        #if_cond k == KW_INT
            move    s, DS_INT
        #end_cond
        #if_cond k == KW_LONG
            move    s, DS_LONG
        #end_cond
        #if_cond k == KW_FLOAT
            move    s, DS_FLOAT
        #end_cond
        #if_cond k == KW_DOUBLE
            move    s, DS_DOUBLE
        #end_cond
        #if_cond k == KW_SIGNED
            move    s, DS_SIGNED
        #end_cond
        #if_cond k == KW_UNSIGNED
            move    s, DS_UNSIGNED
        #end_cond
        #if_cond s == 0
            #break
        #end_cond
        and     r0, cnt, DS_OTHER
        #if_cond r0 != 0
            #call   error_tok(t, "two or more data types in declaration specifiers")
        #end_cond
        add     cnt, s
        #call   advance()
    #end_while
    store   st, DS_STORAGE
    and     r0, cnt, DS_OTHER
    #if_cond r0 != 0
        #return ty
    #end_cond
    #if_cond cnt == 0
        #if_cond st == 0
            #call   error_tok(start, "expected declaration specifiers")
        #end_cond
        #call   error_tok(start, "type specifier missing")
    #end_cond
    // validate the combination
    and     r0, cnt, DS_SIGNED
    and     r1, cnt, DS_UNSIGNED
    #if_cond r0 != 0
        #if_cond r1 != 0
            #call   error_tok(start, "both 'signed' and 'unsigned' in declaration specifiers")
        #end_cond
    #end_cond
    and     ints, cnt, 131071           // without signed/unsigned
    #if_cond ints == DS_VOID
        #if_cond cnt != DS_VOID
            #call   error_tok(start, "invalid combination of type specifiers")
        #end_cond
        load    r0, TY_VOID_T
        #return r0
    #end_cond
    #if_cond ints == DS_BOOL
        #if_cond cnt != DS_BOOL
            #call   error_tok(start, "invalid combination of type specifiers")
        #end_cond
        load    r0, TY_BOOL_T
        #return r0
    #end_cond
    #if_cond ints == DS_CHAR
        and     r0, cnt, DS_UNSIGNED
        #if_cond r0 != 0
            load    r0, TY_UCHAR_T
            #return r0
        #end_cond
        load    r0, TY_CHAR_T
        #return r0
    #end_cond
    #if_cond ints == DS_FLOAT
        #call   error_tok(start, "float is not supported (use double)")
    #end_cond
    #if_cond ints == DS_DOUBLE
        #if_cond cnt != DS_DOUBLE
            #call   error_tok(start, "invalid combination of type specifiers")
        #end_cond
        load    r0, TY_DOUBLE_T
        #return r0
    #end_cond
    move    r0, DS_LONG
    add     r0, DS_DOUBLE
    #if_cond ints == r0
        #call   error_tok(start, "long double is not supported")
    #end_cond
    // integer types: short, int, long, long long (+ int)
    move    s, 0
    #if_cond ints == 0
        move    s, 1
    #end_cond
    #if_cond ints == DS_SHORT
        move    s, 1
    #end_cond
    move    r0, DS_SHORT
    add     r0, DS_INT
    #if_cond ints == r0
        move    s, 1
    #end_cond
    #if_cond ints == DS_INT
        move    s, 1
    #end_cond
    #if_cond ints == DS_LONG
        move    s, 1
    #end_cond
    move    r0, DS_LONG
    add     r0, DS_INT
    #if_cond ints == r0
        move    s, 1
    #end_cond
    move    r0, DS_LONG
    add     r0, DS_LONG
    #if_cond ints == r0
        move    s, 1
    #end_cond
    add     r0, r0, DS_INT
    #if_cond ints == r0
        move    s, 1
    #end_cond
    #if_cond s == 0
        #call   error_tok(start, "invalid combination of type specifiers")
    #end_cond
    and     r0, cnt, DS_UNSIGNED
    #if_cond r0 != 0
        load    r0, TY_UINT_T
        #return r0
    #end_cond
    load    r0, TY_INT_T
#end_func

// skip_qualifiers(): const volatile restrict after '*' or in '[...]'
#def_func skip_qualifiers()
    #while  TRUE
        #call   ck()
        #if_cond r0 == KW_CONST
            #call   advance()
            #continue
        #end_cond
        #if_cond r0 == KW_VOLATILE
            #call   advance()
            #continue
        #end_cond
        #if_cond r0 == KW_RESTRICT
            #call   advance()
            #continue
        #end_cond
        #if_cond r0 == KW_STATIC
            #call   advance()
            #continue
        #end_cond
        #break
    #end_while
#end_func

///////////////////////////////////////////////////////////////////////////////
// declarator(ty) -> type;  DECL_NAME / DECL_TOK = declared name (or 0)
///////////////////////////////////////////////////////////////////////////////
#def_func declarator(tp)
    #var    ty, start, depth, k, endp, name, ntok, nt
    load    ty, tp
    #while  TRUE
        #call   consume(TK_STAR)
        #if_cond r0 == 0
            #break
        #end_cond
        #call   ty_pointer_to(ty)
        move    ty, r0
        #call   skip_qualifiers()
    #end_while
    #call   ck()
    #if_cond r0 == TK_LPAREN
        // nested declarator unless this is a parameter list
        #call   peek_tok(1)
        move    nt, r0
        load    k, nt[TK_KIND]
        move    depth, 0
        #if_cond k == TK_STAR
            move    depth, 1
        #end_cond
        #if_cond k == TK_LPAREN
            move    depth, 1
        #end_cond
        #if_cond k == TK_LBRACK
            move    depth, 1
        #end_cond
        #if_cond k == TK_IDENT
            #call   is_typename(nt)
            #if_cond r0 == 0
                move    depth, 1
            #end_cond
        #end_cond
        #if_cond depth != 0
            load    start, P_POS
            #call   advance()
            // skip to the matching ')'
            #while  depth > 0
                #call   ck()
                move    k, r0
                #if_cond k == TK_EOF
                    #call   error_expected("')'")
                #end_cond
                #if_cond k == TK_LPAREN
                    add     depth, 1
                #end_cond
                #if_cond k == TK_RPAREN
                    sub     depth, 1
                #end_cond
                #call   advance()
            #end_while
            #call   type_suffix(ty)
            move    ty, r0
            load    endp, P_POS
            add     r0, start, 1
            store   r0, P_POS
            #call   declarator(ty)
            move    ty, r0
            #call   expect(TK_RPAREN)
            store   endp, P_POS
            #return ty
        #end_cond
    #end_cond
    move    name, 0
    #call   cur()
    move    ntok, r0
    load    r0, ntok[TK_KIND]
    #if_cond r0 == TK_IDENT
        load    name, ntok[TK_VAL]
        #call   advance()
    #end_cond
    #call   type_suffix(ty)
    move    ty, r0
    store   name, DECL_NAME
    store   ntok, DECL_TOK
    #return ty
#end_func

///////////////////////////////////////////////////////////////////////////////
// type_suffix(ty) -> ty with array / function suffixes applied
///////////////////////////////////////////////////////////////////////////////
#def_func type_suffix(tp)
    #var    ty, t, len, nd
    load    ty, tp
    #call   cur()
    move    t, r0
    #call   consume(TK_LBRACK)
    #if_cond r0 != 0
        #call   skip_qualifiers()
        move    len, -1
        #call   ck()
        #if_cond r0 != TK_RBRACK
            #call   parse_conditional()
            move    nd, r0
            #call   is_const(nd)
            #if_cond r0 == 0
                #call   error_tok(t, "variable length arrays are not supported")
            #end_cond
            #call   const_eval(nd)
            move    len, r0
            #if_cond len < 0
                #call   error_tok(t, "size of array is negative")
            #end_cond
        #end_cond
        #call   expect(TK_RBRACK)
        #call   type_suffix(ty)
        move    ty, r0
        #call   ty_kind(ty)
        #if_cond r0 == TY_ARRAY
            load    r0, ty[T_LEN]
            #if_cond r0 < 0
                #call   error_tok(t, "array type has incomplete element type")
            #end_cond
        #end_cond
        #call   ty_kind(ty)
        #if_cond r0 == TY_FUNC
            #call   error_tok(t, "declaration of array of functions")
        #end_cond
        #if_cond r0 == TY_VOID
            #call   error_tok(t, "declaration of array of voids")
        #end_cond
        #call   ty_array_of(ty, len)
        #return r0
    #end_cond
    #call   consume(TK_LPAREN)
    #if_cond r0 != 0
        #call   ty_kind(ty)
        #if_cond r0 == TY_FUNC
            #call   error_tok(t, "function returning a function")
        #end_cond
        #if_cond r0 == TY_ARRAY
            #call   error_tok(t, "function returning an array")
        #end_cond
        #call   func_params(ty)
        #return r0
    #end_cond
    #return ty
#end_func

///////////////////////////////////////////////////////////////////////////////
// func_params(ret) -> function type; '(' already consumed
///////////////////////////////////////////////////////////////////////////////
#def_func func_params(retp)
    #var    ft, pt, t, nm, cnt
    load    r1, retp
    #call   ty_function(r1)
    move    ft, r0
    #call   consume(TK_RPAREN)
    #if_cond r0 != 0
        move    r0, TYF_OLDSTYLE
        store   r0, ft[T_FLAGS]
        #return ft
    #end_cond
    #call   ck()
    #if_cond r0 == KW_VOID
        #call   peek_kind(1)
        #if_cond r0 == TK_RPAREN
            #call   advance()
            #call   advance()
            #return ft
        #end_cond
    #end_cond
    #while  TRUE
        #call   cur()
        move    t, r0
        #call   consume(TK_ELLIPSIS)
        #if_cond r0 != 0
            load    r0, ft[T_FLAGS]
            or      r0, TYF_VARIADIC
            store   r0, ft[T_FLAGS]
            #call   expect(TK_RPAREN)
            #break
        #end_cond
        #call   is_typename(t)
        #if_cond r0 == 0
            #call   error_tok(t, "expected declaration specifiers (old-style parameter lists are not supported)")
        #end_cond
        #call   declspec()
        #call   declarator(r0)
        move    pt, r0
        load    nm, DECL_TOK
        load    r0, DECL_NAME
        #if_cond r0 == 0
            move    nm, 0
        #end_cond
        #call   ty_kind(pt)
        #if_cond r0 == TY_ARRAY
            load    r1, pt[T_BASE]
            #call   ty_pointer_to(r1)
            move    pt, r0
        #end_cond
        #call   ty_kind(pt)
        #if_cond r0 == TY_FUNC
            #call   ty_pointer_to(pt)
            move    pt, r0
        #end_cond
        #call   ty_kind(pt)
        #if_cond r0 == TY_VOID
            #call   error_tok(t, "parameter has void type")
        #end_cond
        load    r1, ft[T_PARAMS]
        #call   vec_push(r1, pt)
        load    r1, ft[T_PNAMES]
        #call   vec_push(r1, nm)
        #call   consume(TK_COMMA)
        #if_cond r0 == 0
            #call   expect(TK_RPAREN)
            #break
        #end_cond
    #end_while
    #return ft
#end_func

// parse_typename() -> type of an abstract declarator
#def_func parse_typename()
    #var    ty
    #call   declspec()
    #call   declarator(r0)
    move    ty, r0
    load    r0, DECL_NAME
    #if_cond r0 != 0
        load    r1, DECL_TOK
        #call   error_tok(r1, "unexpected identifier in type name")
    #end_cond
    #return ty
#end_func

///////////////////////////////////////////////////////////////////////////////
// struct_union_decl() -> struct or union type (current token is the keyword)
///////////////////////////////////////////////////////////////////////////////
#def_func struct_union_decl()
    #var    t, kind, tag, g, ty
    #call   cur()
    move    t, r0
    move    kind, TY_STRUCT
    load    r0, t[TK_KIND]
    #if_cond r0 == KW_UNION
        move    kind, TY_UNION
    #end_cond
    #call   advance()
    move    tag, 0
    #call   ck()
    #if_cond r0 == TK_IDENT
        #call   cur()
        load    tag, r0[TK_VAL]
        #call   advance()
    #end_cond
    #call   ck()
    #if_cond r0 != TK_LBRACE
        #if_cond tag == 0
            #call   error_expected("'{'")
        #end_cond
        #call   tag_lookup(tag)
        move    g, r0
        #if_cond g != 0
            load    ty, g[G_TYPE]
            load    r0, ty[T_KIND]
            #if_cond r0 != kind
                #call   error_tok2(t, "defined as wrong kind of tag", tag)
            #end_cond
            #return ty
        #end_cond
        #call   ty_new(kind)
        move    ty, r0
        move    r0, 0
        store   r0, ty[T_SIZE]
        store   tag, ty[T_TAG]
        #call   tag_declare(tag, ty)
        #return ty
    #end_cond
    move    ty, 0
    #if_cond tag != 0
        #call   tag_in_scope(tag)
        move    g, r0
        #if_cond g != 0
            load    ty, g[G_TYPE]
            load    r0, ty[T_KIND]
            #if_cond r0 != kind
                #call   error_tok2(t, "defined as wrong kind of tag", tag)
            #end_cond
            load    r0, ty[T_FLAGS]
            and     r0, TYF_COMPLETE
            #if_cond r0 != 0
                #call   error_tok2(t, "redefinition of", tag)
            #end_cond
        #end_cond
    #end_cond
    #if_cond ty == 0
        #call   ty_new(kind)
        move    ty, r0
        #if_cond tag == 0
            load    tag, ANON_TAG
            store   tag, ty[T_TAG]
        #else_cond
            store   tag, ty[T_TAG]
            #call   tag_declare(tag, ty)
        #end_cond
    #end_cond
    #call   struct_members(ty)
    #return ty
#end_func

// add_member(members, entry, type, offset, tok): checks duplicates
#def_func add_member(msp, ep, tp, offp, tk)
    #var    ms, e, m, cnt, i
    load    ms, msp
    load    e, ep
    #if_cond e != 0
        #macro  VLEN(cnt, ms)
        #for    1, i <= cnt, 1
            #macro  VGET(m, ms, i)
            load    r0, m[MB_NAME]
            #if_cond r0 == e
                load    r1, tk
                #call   error_tok2(r1, "duplicate member", e)
            #end_cond
        #end_for
    #end_cond
    #call   zalloc(MB_SIZE)
    move    m, r0
    store   e, m[MB_NAME]
    load    r1, tp
    store   r1, m[MB_TYPE]
    load    r1, offp
    store   r1, m[MB_OFFSET]
    #call   vec_push(ms, m)
#end_func

// struct_members(ty): parse '{' member declarations '}'
#def_func struct_members(tp)
    #var    ty, isu, ms, off, lim, bt, mt, t, sz, subm, cnt, i, m, flex
    load    ty, tp
    move    isu, 0
    load    r0, ty[T_KIND]
    #if_cond r0 == TY_UNION
        move    isu, 1
    #end_cond
    #call   expect(TK_LBRACE)
    #call   vec_new()
    move    ms, r0
    move    off, 0
    move    lim, 0
    move    flex, 0
    #while  TRUE
        #call   consume(TK_RBRACE)
        #if_cond r0 != 0
            #break
        #end_cond
        #call   cur()
        move    t, r0
        #if_cond flex != 0
            #call   error_tok(t, "flexible array member not at end of struct")
        #end_cond
        #call   is_typename(t)
        #if_cond r0 == 0
            #call   error_expected("specifier-qualifier-list")
        #end_cond
        #call   declspec()
        move    bt, r0
        // anonymous struct / union member
        #call   ck()
        #if_cond r0 == TK_SEMI
            #call   ty_is_struct(bt)
            #if_cond r0 != 0
                #call   advance()
                load    subm, bt[T_MEMBERS]
                #macro  VLEN(cnt, subm)
                #for    1, i <= cnt, 1
                    #macro  VGET(m, subm, i)
                    load    r1, m[MB_NAME]
                    load    r2, m[MB_TYPE]
                    load    r3, m[MB_OFFSET]
                    add     r3, off
                    #call   add_member(ms, r1, r2, r3, t)
                #end_for
                load    sz, bt[T_SIZE]
                #if_cond isu == 0
                    add     off, sz
                #else_cond
                    #if_cond sz > lim
                        move    lim, sz
                    #end_cond
                #end_cond
                #continue
            #end_cond
        #end_cond
        #while  TRUE
            #call   declarator(bt)
            move    mt, r0
            #call   ck()
            #if_cond r0 == TK_COLON
                #call   cur()
                #call   error_tok(r0, "bit-fields are not supported")
            #end_cond
            load    r0, DECL_NAME
            #if_cond r0 == 0
                load    r1, DECL_TOK
                #call   error_tok(r1, "expected member name")
            #end_cond
            #call   ty_kind(mt)
            #if_cond r0 == TY_FUNC
                load    r1, DECL_TOK
                #call   error_tok(r1, "field declared as a function")
            #end_cond
            #if_cond r0 == TY_VOID
                load    r1, DECL_TOK
                load    r2, DECL_NAME
                #call   error_tok2(r1, "field has incomplete type", r2)
            #end_cond
            #if_cond r0 == TY_ARRAY
                load    r0, mt[T_LEN]
                #if_cond r0 < 0
                    move    flex, 1
                #end_cond
            #end_cond
            #call   ty_is_struct(mt)
            #if_cond r0 != 0
                load    r0, mt[T_FLAGS]
                and     r0, TYF_COMPLETE
                #if_cond r0 == 0
                    load    r1, DECL_TOK
                    load    r2, DECL_NAME
                    #call   error_tok2(r1, "field has incomplete type", r2)
                #end_cond
            #end_cond
            load    r1, DECL_NAME
            #if_cond isu != 0
                load    r4, DECL_TOK
                #call   add_member(ms, r1, mt, 0, r4)
                load    sz, mt[T_SIZE]
                #if_cond sz > lim
                    move    lim, sz
                #end_cond
            #else_cond
                load    r4, DECL_TOK
                #call   add_member(ms, r1, mt, off, r4)
                load    sz, mt[T_SIZE]
                add     off, sz
            #end_cond
            #call   consume(TK_COMMA)
            #if_cond r0 == 0
                #break
            #end_cond
            #if_cond flex != 0
                #call   cur()
                #call   error_tok(r0, "flexible array member not at end of struct")
            #end_cond
        #end_while
        #call   expect(TK_SEMI)
    #end_while
    store   ms, ty[T_MEMBERS]
    #if_cond isu != 0
        store   lim, ty[T_SIZE]
    #else_cond
        store   off, ty[T_SIZE]
    #end_cond
    load    r0, ty[T_FLAGS]
    or      r0, TYF_COMPLETE
    store   r0, ty[T_FLAGS]
#end_func

///////////////////////////////////////////////////////////////////////////////
// enum_decl() -> enum type (current token is 'enum')
///////////////////////////////////////////////////////////////////////////////
#def_func enum_decl()
    #var    t, tag, g, ty, val, nt, s, e
    #call   cur()
    move    t, r0
    #call   advance()
    move    tag, 0
    #call   ck()
    #if_cond r0 == TK_IDENT
        #call   cur()
        load    tag, r0[TK_VAL]
        #call   advance()
    #end_cond
    #call   ck()
    #if_cond r0 != TK_LBRACE
        #if_cond tag == 0
            #call   error_expected("'{'")
        #end_cond
        #call   tag_lookup(tag)
        move    g, r0
        #if_cond g != 0
            load    ty, g[G_TYPE]
            load    r0, ty[T_KIND]
            #if_cond r0 != TY_ENUM
                #call   error_tok2(t, "defined as wrong kind of tag", tag)
            #end_cond
            #return ty
        #end_cond
        #call   ty_new(TY_ENUM)
        move    ty, r0
        store   tag, ty[T_TAG]
        #call   tag_declare(tag, ty)
        #return ty
    #end_cond
    #call   advance()
    #call   ty_new(TY_ENUM)
    move    ty, r0
    #if_cond tag != 0
        #call   tag_in_scope(tag)
        #if_cond r0 != 0
            #call   error_tok2(t, "redefinition of", tag)
        #end_cond
        store   tag, ty[T_TAG]
        #call   tag_declare(tag, ty)
    #else_cond
        load    r0, ANON_TAG
        store   r0, ty[T_TAG]
    #end_cond
    move    val, 0
    #while  TRUE
        #call   consume(TK_RBRACE)
        #if_cond r0 != 0
            #break
        #end_cond
        #call   expect_ident()
        move    nt, r0
        load    e, nt[TK_VAL]
        #call   sym_in_scope(e)
        #if_cond r0 != 0
            #call   error_tok2(nt, "redeclaration of enumerator", e)
        #end_cond
        #call   consume(TK_ASSIGN)
        #if_cond r0 != 0
            #call   const_expr()
            move    val, r0
        #end_cond
        load    r2, TY_INT_T
        #call   sym_declare(e, SK_ENUMC, r2, nt)
        store   val, r0[S_OFFSET]
        add     val, 1
        #call   consume(TK_COMMA)
        #if_cond r0 == 0
            #call   expect(TK_RBRACE)
            #break
        #end_cond
    #end_while
    load    r0, ty[T_FLAGS]
    or      r0, TYF_COMPLETE
    store   r0, ty[T_FLAGS]
    #return ty
#end_func

///////////////////////////////////////////////////////////////////////////////
// initializers
///////////////////////////////////////////////////////////////////////////////
// add_leaf(leaves, off, type, expr)
#def_func add_leaf(lvp, offp, tp, ep)
    #var    l
    #call   zalloc(L_SIZE)
    move    l, r0
    load    r1, offp
    store   r1, l[L_OFF]
    load    r1, tp
    store   r1, l[L_TYPE]
    load    r1, ep
    store   r1, l[L_EXPR]
    load    r1, lvp
    #call   vec_push(r1, l)
#end_func

// is_string_init(ty) -> 1 if ty is a char array and a string literal follows
#def_func is_string_init(tp)
    #var    ty
    load    ty, tp
    #call   ty_kind(ty)
    #if_cond r0 != TY_ARRAY
        #return 0
    #end_cond
    load    r1, ty[T_BASE]
    #call   ty_kind(r1)
    #if_cond r0 != TY_CHAR
        #return 0
    #end_cond
    #call   ck()
    #if_cond r0 == TK_STR
        #return 1
    #end_cond
    #if_cond r0 == TK_LBRACE
        #call   peek_kind(1)
        #if_cond r0 == TK_STR
            #return 1
        #end_cond
    #end_cond
    #return 0
#end_func

// init_string(ty, off, leaves): char array from a string literal
#def_func init_string(tp, offp, lvp)
    #var    ty, off, lv, brace, nd, w, cnt, len, i, c, t
    load    ty, tp
    load    off, offp
    load    lv, lvp
    #call   consume(TK_LBRACE)
    move    brace, r0
    #call   parse_primary()
    move    nd, r0
    #if_cond brace != 0
        #call   consume(TK_COMMA)
        #call   expect(TK_RBRACE)
    #end_cond
    load    r1, STRLITS
    load    r2, nd[N_VAL]
    #macro  VGET(w, r1, r2)
    load    cnt, w[0]
    load    len, ty[T_LEN]
    #if_cond len < 0
        add     len, cnt, 1
    #end_cond
    store   len, INIT_LEN
    load    t, ty[T_BASE]
    move    i, 0
    #while  i < len
        move    c, 0
        #if_cond i < cnt
            add     r1, i, 1
            load    c, w[r1]
        #end_cond
        load    r1, TY_CHAR_T
        #call   nd_number(c, r1, 0)
        add     r1, off, i
        #call   add_leaf(lv, r1, t, r0)
        add     i, 1
    #end_while
#end_func

// init_value(ty, off, leaves): initializer for an object of type ty
#def_func init_value(tp, offp, lvp)
    #var    ty, off, lv, t, nd
    load    ty, tp
    load    off, offp
    load    lv, lvp
    #call   cur()
    move    t, r0
    #call   is_string_init(ty)
    #if_cond r0 != 0
        #call   init_string(ty, off, lv)
        #return 0
    #end_cond
    #call   ty_kind(ty)
    #if_cond r0 == TY_ARRAY
        #call   ck()
        #if_cond r0 != TK_LBRACE
            #call   error_tok(t, "array must be initialized with a brace-enclosed initializer")
        #end_cond
        #call   init_array(ty, off, lv)
        #return 0
    #end_cond
    #call   ty_is_struct(ty)
    #if_cond r0 != 0
        #call   ck()
        #if_cond r0 == TK_LBRACE
            #call   init_struct(ty, off, lv)
            #return 0
        #end_cond
        #call   parse_assign()
        #call   sem_assign_conv(r0, ty, t, "incompatible types when initializing")
        #call   add_leaf(lv, off, ty, r0)
        #return 0
    #end_cond
    // scalar, optionally in braces
    #call   consume(TK_LBRACE)
    #if_cond r0 != 0
        #call   init_value(ty, off, lv)
        #call   consume(TK_COMMA)
        #call   expect(TK_RBRACE)
        #return 0
    #end_cond
    #call   parse_assign()
    #call   sem_assign_conv(r0, ty, t, "incompatible types when initializing")
    #call   add_leaf(lv, off, ty, r0)
#end_func

// init_elem(ty, off, leaves): element of a braced list (brace elision)
#def_func init_elem(tp, offp, lvp)
    #var    ty, agg
    load    ty, tp
    move    agg, 0
    #call   ty_kind(ty)
    #if_cond r0 == TY_ARRAY
        move    agg, 1
    #end_cond
    #call   ty_is_struct(ty)
    #if_cond r0 != 0
        move    agg, 1
    #end_cond
    #if_cond agg != 0
        #call   ck()
        #if_cond r0 != TK_LBRACE
            #call   is_string_init(ty)
            #if_cond r0 == 0
                // a struct may be initialized from a struct expression
                #call   ty_is_struct(ty)
                #if_cond r0 != 0
                    load    r4, offp
                    load    r3, lvp
                    #call   init_struct_expr_or_elided(ty, r4, r3)
                    #return 0
                #end_cond
                load    r2, offp
                load    r3, lvp
                #call   init_elided(ty, r2, r3)
                #return 0
            #end_cond
        #end_cond
    #end_cond
    load    r2, offp
    load    r3, lvp
    #call   init_value(ty, r2, r3)
#end_func

// init_struct_expr_or_elided: struct element without braces
#def_func init_struct_expr_or_elided(tp, offp, lvp)
    #var    ty, svpos, nd, t
    load    ty, tp
    #call   cur()
    move    t, r0
    // try an assignment expression of the same struct type
    load    svpos, P_POS
    #call   ck()
    #if_cond r0 == TK_IDENT
        #call   cur()
        load    r1, r0[TK_VAL]
        #call   sym_lookup(r1)
        #if_cond r0 != 0
            load    r1, r0[S_TYPE]
            #call   ty_same(r1, ty)
            #if_cond r0 != 0
                #call   parse_assign()
                #call   sem_assign_conv(r0, ty, t, "incompatible types when initializing")
                load    r1, lvp
                load    r2, offp
                #call   add_leaf(r1, r2, ty, r0)
                #return 0
            #end_cond
        #end_cond
    #end_cond
    load    r2, offp
    load    r3, lvp
    #call   init_elided(ty, r2, r3)
#end_func

// more_elided() -> 1 if another elided element follows (',' then not '}' / designator)
#def_func more_elided()
    #call   ck()
    #if_cond r0 != TK_COMMA
        #return 0
    #end_cond
    #call   peek_kind(1)
    #if_cond r0 == TK_RBRACE
        #return 0
    #end_cond
    #if_cond r0 == TK_DOT
        #return 0
    #end_cond
    #if_cond r0 == TK_LBRACK
        #return 0
    #end_cond
    #return 1
#end_func

// init_elided(ty, off, leaves): aggregate initialized without its braces
#def_func init_elided(tp, offp, lvp)
    #var    ty, off, lv, len, i, bt, bsz, ms, m
    load    ty, tp
    load    off, offp
    load    lv, lvp
    #call   ty_kind(ty)
    #if_cond r0 == TY_ARRAY
        load    len, ty[T_LEN]
        load    bt, ty[T_BASE]
        load    bsz, bt[T_SIZE]
        move    i, 0
        #while  i < len
            mult    r2, i, bsz
            add     r2, off
            #call   init_elem(bt, r2, lv)
            add     i, 1
            #if_cond i >= len
                #break
            #end_cond
            #call   more_elided()
            #if_cond r0 == 0
                #break
            #end_cond
            #call   advance()
        #end_while
        #return 0
    #end_cond
    load    ms, ty[T_MEMBERS]
    #macro  VLEN(len, ms)
    load    r0, ty[T_KIND]
    #if_cond r0 == TY_UNION
        #if_cond len > 1
            move    len, 1
        #end_cond
    #end_cond
    move    i, 1
    #while  i <= len
        #macro  VGET(m, ms, i)
        load    r1, m[MB_TYPE]
        load    r2, m[MB_OFFSET]
        add     r2, off
        #call   init_elem(r1, r2, lv)
        add     i, 1
        #if_cond i > len
            #break
        #end_cond
        #call   more_elided()
        #if_cond r0 == 0
            #break
        #end_cond
        #call   advance()
    #end_while
#end_func

// init_array(ty, off, leaves): '{' ... '}'  (INIT_LEN = element count)
#def_func init_array(tp, offp, lvp)
    #var    ty, off, lv, len, bt, bsz, idx, lim, t
    load    ty, tp
    load    off, offp
    load    lv, lvp
    load    len, ty[T_LEN]
    load    bt, ty[T_BASE]
    load    bsz, bt[T_SIZE]
    #call   expect(TK_LBRACE)
    move    idx, 0
    move    lim, 0
    #while  TRUE
        #call   consume(TK_RBRACE)
        #if_cond r0 != 0
            #break
        #end_cond
        #call   cur()
        move    t, r0
        #call   consume(TK_LBRACK)
        #if_cond r0 != 0
            #call   const_expr()
            move    idx, r0
            #call   expect(TK_RBRACK)
            #call   expect(TK_ASSIGN)
            #if_cond idx < 0
                #call   error_tok(t, "array index in initializer is negative")
            #end_cond
        #end_cond
        #if_cond len >= 0
            #if_cond idx >= len
                #call   error_tok(t, "excess elements in array initializer")
            #end_cond
        #end_cond
        mult    r2, idx, bsz
        add     r2, off
        #call   init_elem(bt, r2, lv)
        add     idx, 1
        #if_cond idx > lim
            move    lim, idx
        #end_cond
        #call   consume(TK_COMMA)
        #if_cond r0 == 0
            #call   expect(TK_RBRACE)
            #break
        #end_cond
    #end_while
    store   lim, INIT_LEN
#end_func

// init_struct(ty, off, leaves): '{' ... '}'
#def_func init_struct(tp, offp, lvp)
    #var    ty, off, lv, ms, cnt, i, m, t, nt, j, isu
    load    ty, tp
    load    off, offp
    load    lv, lvp
    load    r0, ty[T_FLAGS]
    and     r0, TYF_COMPLETE
    #if_cond r0 == 0
        #call   cur()
        #call   error_tok(r0, "initialization of incomplete type")
    #end_cond
    move    isu, 0
    load    r0, ty[T_KIND]
    #if_cond r0 == TY_UNION
        move    isu, 1
    #end_cond
    load    ms, ty[T_MEMBERS]
    #macro  VLEN(cnt, ms)
    #call   expect(TK_LBRACE)
    move    i, 1
    #while  TRUE
        #call   consume(TK_RBRACE)
        #if_cond r0 != 0
            #break
        #end_cond
        #call   cur()
        move    t, r0
        #call   consume(TK_DOT)
        #if_cond r0 != 0
            #call   expect_ident()
            move    nt, r0
            move    j, 1
            #while  j <= cnt
                #macro  VGET(m, ms, j)
                load    r0, m[MB_NAME]
                load    r1, nt[TK_VAL]
                #if_cond r0 == r1
                    #break
                #end_cond
                add     j, 1
            #end_while
            #if_cond j > cnt
                load    r1, nt[TK_VAL]
                #call   error_tok2(nt, "unknown field specified in initializer", r1)
            #end_cond
            move    i, j
            #call   expect(TK_ASSIGN)
        #end_cond
        #if_cond i > cnt
            #call   error_tok(t, "excess elements in struct initializer")
        #end_cond
        #if_cond isu != 0
            #if_cond i > 1
                #call   ck()
                // only one union member may be initialized (unless designated)
            #end_cond
        #end_cond
        #macro  VGET(m, ms, i)
        load    r1, m[MB_TYPE]
        load    r2, m[MB_OFFSET]
        add     r2, off
        #call   init_elem(r1, r2, lv)
        add     i, 1
        #if_cond isu != 0
            move    i, cnt
            add     i, 1
        #end_cond
        #call   consume(TK_COMMA)
        #if_cond r0 == 0
            #call   expect(TK_RBRACE)
            #break
        #end_cond
    #end_while
#end_func

// parse_initializer(ty) -> vec of leaves.  If ty is an incomplete array the
// returned INIT_LEN holds the element count.
#def_func parse_initializer(tp)
    #var    lv
    #call   vec_new()
    move    lv, r0
    move    r0, -1
    store   r0, INIT_LEN
    load    r1, tp
    #call   init_value(r1, 0, lv)
    #return lv
#end_func

// complete_array(ty) -> ty, or a completed copy if ty is an incomplete array
#def_func complete_array(tp, tk)
    #var    ty, len
    load    ty, tp
    #call   ty_kind(ty)
    #if_cond r0 != TY_ARRAY
        #return ty
    #end_cond
    load    r0, ty[T_LEN]
    #if_cond r0 >= 0
        #return ty
    #end_cond
    load    len, INIT_LEN
    #if_cond len < 0
        load    r1, tk
        #call   error_tok(r1, "array size missing")
    #end_cond
    load    r1, ty[T_BASE]
    #call   ty_array_of(r1, len)
#end_func

///////////////////////////////////////////////////////////////////////////////
// parse_compound_literal(ty, tok) -> expression for ( type-name ) { ... }
//      At block scope the literal is a hidden local initialized by a comma
//      chain (memzero, assignments..., variable); at file scope it is an
//      anonymous static object.
///////////////////////////////////////////////////////////////////////////////
#def_func parse_compound_literal(tp, tk)
    #var    ty, lv, s, nd, cnt, i, l, lhs, a, t
    load    ty, tp
    load    t, tk
    #call   parse_initializer(ty)
    move    lv, r0
    #call   complete_array(ty, t)
    move    ty, r0
    load    r0, CUR_FUNC
    #if_cond r0 == 0
        #call   zalloc(S_SIZE)
        move    s, r0
        move    r1, SK_GVAR
        store   r1, s[S_KIND]
        store   ty, s[S_TYPE]
        load    r1, ANON_TAG
        store   r1, s[S_NAME]
        store   lv, s[S_INIT]
        move    r1, SF_DEFINED
        store   r1, s[S_FLAGS]
        #call   new_label("C", 0)
        store   r0, s[S_LABEL]
        load    r1, GLOBALS
        #call   vec_push(r1, s)
        #call   var_node(s, t)
        #return r0
    #end_cond
    #call   new_temp_local(ty)
    move    s, r0
    #call   nd_new(ND_MEMZERO, t)
    move    nd, r0
    store   s, nd[N_VAL]
    load    r1, TY_VOID_T
    store   r1, nd[N_TYPE]
    #macro  VLEN(cnt, lv)
    #for    1, i <= cnt, 1
        #macro  VGET(l, lv, i)
        load    r1, l[L_OFF]
        load    r2, l[L_TYPE]
        #call   lvalue_at(s, r1, r2, t)
        move    lhs, r0
        load    r2, l[L_EXPR]
        load    r3, l[L_TYPE]
        #call   mk_typed(ND_ASSIGN, lhs, r2, r3, t)
        #call   mk_typed(ND_COMMA, nd, r0, r3, t)
        move    nd, r0
    #end_for
    #call   var_node(s, t)
    move    a, r0
    #call   mk_typed(ND_COMMA, nd, a, ty, t)
#end_func

PARSE_DECL_ASM_END:
    nop
