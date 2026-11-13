///////////////////////////////////////////////////////////////////////////////
// ast.asm
//
// AST node construction and the --ast dumper.
//
//   nd_new(kind, tok)              -> node with position of tok
//   nd_unary(kind, a, tok)
//   nd_binary(kind, a, b, tok)
//   nd_number(value, type, tok)
//   ast_dump_node(node, indent)    print an s-expression
///////////////////////////////////////////////////////////////////////////////

#global ND_NAMES:   .dci 0          // vec of packed node names, index = kind

AST_ASM_START:
    jump    AST_ASM_END

// index = node kind (1-based list; "?" for unused kinds)
ND_NAME_LIST:
    .dcs    "num fnum str var + - * / % & | ^ << >> == != < <= && || ! ~ neg addr deref = ?: comma call member cast incdec opassign p2c c2p memzero ? > >= __func__ ? ? ? ? ? ? ? ? ? return if for do switch case block expr goto label break continue null"

#def_func ast_init()
    #var    v, cnt, i, w
    #call   split_words(ND_NAME_LIST)
    move    v, r0
    #call   vec_new()
    store   r0, ND_NAMES
    #macro  VLEN(cnt, v)
    #for    1, i <= cnt, 1
        #macro  VGET(w, v, i)
        #call   p_from_w(w)
        load    r1, ND_NAMES
        #call   vec_push(r1, r0)
    #end_for
#end_func

#def_func nd_name(kind)
    #var    k, cnt
    load    k, kind
    load    r1, ND_NAMES
    #macro  VLEN(cnt, r1)
    #if_cond k < 1
        #return "?"
    #end_cond
    #if_cond k > cnt
        #return "?"
    #end_cond
    load    r1, ND_NAMES
    #macro  VGET(r0, r1, k)
#end_func

///////////////////////////////////////////////////////////////////////////////
// constructors
///////////////////////////////////////////////////////////////////////////////
#def_func nd_new(kind, tk)
    #var    nd, t
    #call   zalloc(N_SIZE)
    move    nd, r0
    load    r1, kind
    store   r1, nd[N_KIND]
    load    t, tk
    #if_cond t != 0
        load    r1, t[TK_LINE]
        store   r1, nd[N_LINE]
        load    r1, t[TK_FILE]
        store   r1, nd[N_FILE]
    #end_cond
    #return nd
#end_func

// nd_at(kind, like_node): node positioned like another node
#def_func nd_at(kind, like)
    #var    nd, l
    #call   zalloc(N_SIZE)
    move    nd, r0
    load    r1, kind
    store   r1, nd[N_KIND]
    load    l, like
    #if_cond l != 0
        load    r1, l[N_LINE]
        store   r1, nd[N_LINE]
        load    r1, l[N_FILE]
        store   r1, nd[N_FILE]
    #end_cond
    #return nd
#end_func

#def_func nd_unary(kind, ap, tk)
    #var    nd
    load    r1, kind
    load    r2, tk
    #call   nd_new(r1, r2)
    move    nd, r0
    load    r1, ap
    store   r1, nd[N_A]
    #return nd
#end_func

#def_func nd_binary(kind, ap, bp, tk)
    #var    nd
    load    r1, kind
    load    r2, tk
    #call   nd_new(r1, r2)
    move    nd, r0
    load    r1, ap
    store   r1, nd[N_A]
    load    r1, bp
    store   r1, nd[N_B]
    #return nd
#end_func

#def_func nd_number(val, typ, tk)
    #var    nd
    load    r2, tk
    #call   nd_new(ND_NUM, r2)
    move    nd, r0
    load    r1, val
    store   r1, nd[N_VAL]
    load    r1, typ
    store   r1, nd[N_TYPE]
    #return nd
#end_func

///////////////////////////////////////////////////////////////////////////////
// dumper
///////////////////////////////////////////////////////////////////////////////
#def_func dump_indent(ind)
    #var    i, cnt
    load    cnt, ind
    #for    1, i <= cnt, 1
        #call   printf("  ")
    #end_for
#end_func

// dump_type(t): print a type
#def_func dump_type(tp)
    #var    sb
    #call   vec_new()
    move    sb, r0
    load    r1, tp
    #call   ty_print(sb, r1)
    #call   sb_to_p(sb)
    #call   puts(r0)
#end_func

// dump_vec(v, indent): dump each node of a vector on its own line
#def_func dump_vec(vp, ind)
    #var    v, cnt, i, d
    load    v, vp
    load    d, ind
    #if_cond v == 0
        #return 0
    #end_cond
    #macro  VLEN(cnt, v)
    #for    1, i <= cnt, 1
        #call   printf("\n")
        #macro  VGET(r1, v, i)
        #call   ast_dump_node(r1, d)
    #end_for
#end_func

#def_func ast_dump_node(ndp, ind)
    #var    nd, d, k, d1, t, e
    load    nd, ndp
    load    d, ind
    add     d1, d, 1
    #call   dump_indent(d)
    #if_cond nd == 0
        #call   printf("()")
        #return 0
    #end_cond
    load    k, nd[N_KIND]
    #call   nd_name(k)
    #call   printf("(%s", r0)
    // expression nodes carry their type
    #if_cond k < ND_RETURN
        #call   printf(" ")
        load    r1, nd[N_TYPE]
        #call   dump_type(r1)
    #end_cond
    #if_cond k == ND_NUM
        load    r1, nd[N_VAL]
        #call   printf(" %d)", r1)
        #return 0
    #end_cond
    #if_cond k == ND_FNUM
        load    f1, nd[N_VAL]
        #call   printf(" %g)", f1)
        #return 0
    #end_cond
    #if_cond k == ND_STR
        load    r1, STRLITS
        load    r2, nd[N_VAL]
        #macro  VGET(r1, r1, r2)
        #call   p_from_w(r1)
        #call   printf(" \"%s\")", r0)
        #return 0
    #end_cond
    #if_cond k == ND_VAR
        load    t, nd[N_VAL]
        load    r1, t[S_NAME]
        #call   ent_p(r1)
        #call   printf(" %s)", r0)
        #return 0
    #end_cond
    #if_cond k == ND_FUNCNAME
        #call   printf(")")
        #return 0
    #end_cond
    #if_cond k == ND_MEMBER
        load    t, nd[N_VAL]
        load    r1, t[MB_NAME]
        #call   ent_p(r1)
        load    r2, t[MB_OFFSET]
        #call   printf(" .%s@%d\n", r0, r2)
        load    r1, nd[N_A]
        #call   ast_dump_node(r1, d1)
        #call   printf(")")
        #return 0
    #end_cond
    #if_cond k == ND_INCDEC
        load    r1, nd[N_VAL]
        load    r2, nd[N_VAL2]
        #call   printf(" %d post=%d\n", r1, r2)
        load    r1, nd[N_A]
        #call   ast_dump_node(r1, d1)
        #call   printf(")")
        #return 0
    #end_cond
    #if_cond k == ND_OPASSIGN
        load    r1, nd[N_VAL]
        #call   nd_name(r1)
        #call   printf(" %s=", r0)
    #end_cond
    #if_cond k == ND_CALL
        #call   printf("\n")
        load    r1, nd[N_A]
        #call   ast_dump_node(r1, d1)
        load    r1, nd[N_VAL]
        #call   dump_vec(r1, d1)
        #call   printf(")")
        #return 0
    #end_cond
    #if_cond k == ND_MEMZERO
        load    t, nd[N_VAL]
        load    r1, t[S_NAME]
        #call   ent_p(r1)
        #call   printf(" %s)", r0)
        #return 0
    #end_cond
    #if_cond k == ND_BLOCK
        load    r1, nd[N_VAL]
        #call   dump_vec(r1, d1)
        #call   printf(")")
        #return 0
    #end_cond
    #if_cond k == ND_CASE
        load    r1, nd[N_VAL]
        load    r2, nd[N_VAL2]
        #if_cond r2 == -1
            #call   printf(" default")
        #else_cond
            #call   printf(" %d", r1)
        #end_cond
    #end_cond
    #if_cond k == ND_GOTO
        load    r1, nd[N_VAL]
        #call   ent_p(r1)
        #call   printf(" %s)", r0)
        #return 0
    #end_cond
    #if_cond k == ND_LABEL
        load    r1, nd[N_VAL]
        #call   ent_p(r1)
        #call   printf(" %s", r0)
    #end_cond
    // generic children
    load    r1, nd[N_A]
    #if_cond r1 != 0
        #call   printf("\n")
        load    r1, nd[N_A]
        #call   ast_dump_node(r1, d1)
    #end_cond
    load    r1, nd[N_B]
    #if_cond r1 != 0
        #call   printf("\n")
        load    r1, nd[N_B]
        #call   ast_dump_node(r1, d1)
    #end_cond
    load    r1, nd[N_C]
    #if_cond r1 != 0
        #call   printf("\n")
        load    r1, nd[N_C]
        #call   ast_dump_node(r1, d1)
    #end_cond
    load    r1, nd[N_D]
    #if_cond r1 != 0
        #call   printf("\n")
        load    r1, nd[N_D]
        #call   ast_dump_node(r1, d1)
    #end_cond
    #call   printf(")")
#end_func

AST_ASM_END:
    nop
