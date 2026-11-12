///////////////////////////////////////////////////////////////////////////////
// symtab.asm
//
// Scoped symbol table.  Each intern entry points (E_SYM / E_TAG) to its
// innermost binding; bindings chain to the outer binding of the same name.
// SCOPES is a stack of vectors recording what each scope declared so the
// bindings can be undone when the scope closes.
//
//   scope_push() / scope_pop()
//   sym_lookup(entry) -> Sym or 0
//   sym_declare(entry, kind, type, tok) -> new Sym in the current scope
//   tag_lookup(entry) / tag_declare(entry, type)
///////////////////////////////////////////////////////////////////////////////

#global SCOPES:     .dci 0          // vec of scope records
#global SCOPE_DEPTH: .dci 0         // 0 = file scope

#define SC_SYMS     0
#define SC_TAGS     1
#define SC_SIZE     2

SYMTAB_ASM_START:
    jump    SYMTAB_ASM_END

#def_func symtab_init()
    #call   vec_new()
    store   r0, SCOPES
    move    r0, -1
    store   r0, SCOPE_DEPTH
    #call   scope_push()
#end_func

#def_func scope_push()
    #var    sc
    #call   zalloc(SC_SIZE)
    move    sc, r0
    #call   vec_new()
    store   r0, sc[SC_SYMS]
    #call   vec_new()
    store   r0, sc[SC_TAGS]
    load    r1, SCOPES
    #call   vec_push(r1, sc)
    load    r0, SCOPE_DEPTH
    add     r0, 1
    store   r0, SCOPE_DEPTH
#end_func

#def_func scope_pop()
    #var    sc, v, cnt, i, s, e
    load    r1, SCOPES
    #call   vec_pop(r1)
    move    sc, r0
    load    v, sc[SC_SYMS]
    #macro  VLEN(cnt, v)
    move    i, cnt
    #while  i >= 1
        #macro  VGET(s, v, i)
        load    e, s[S_NAME]
        load    r1, s[S_NEXT]
        store   r1, e[E_SYM]
        sub     i, 1
    #end_while
    load    v, sc[SC_TAGS]
    #macro  VLEN(cnt, v)
    move    i, cnt
    #while  i >= 1
        #macro  VGET(s, v, i)
        load    e, s[G_NAME]
        load    r1, s[G_NEXT]
        store   r1, e[E_TAG]
        sub     i, 1
    #end_while
    load    r0, SCOPE_DEPTH
    sub     r0, 1
    store   r0, SCOPE_DEPTH
#end_func

#def_func sym_lookup(ep)
    load    r0, ep
    load    r0, r0[E_SYM]
#end_func

// sym_in_scope(entry) -> binding declared in the current scope or 0
#def_func sym_in_scope(ep)
    #var    s
    load    r0, ep
    load    s, r0[E_SYM]
    #if_cond s == 0
        #return 0
    #end_cond
    load    r0, s[S_DEPTH]
    load    r1, SCOPE_DEPTH
    #if_cond r0 == r1
        #return s
    #end_cond
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// sym_declare(entry, kind, type, tok) -> new Sym bound in the current scope
///////////////////////////////////////////////////////////////////////////////
#def_func sym_declare(ep, kindp, typ, tk)
    #var    e, s, sc
    load    e, ep
    #call   zalloc(S_SIZE)
    move    s, r0
    store   e, s[S_NAME]
    load    r1, kindp
    store   r1, s[S_KIND]
    load    r1, typ
    store   r1, s[S_TYPE]
    load    r1, SCOPE_DEPTH
    store   r1, s[S_DEPTH]
    load    r1, tk
    #if_cond r1 != 0
        load    r2, r1[TK_FILE]
        load    r3, CC_PRELUDE
        #if_cond r2 == r3
            move    r2, SF_PRELUDE
            store   r2, s[S_FLAGS]
        #end_cond
        load    r1, r1[TK_LINE]
    #end_cond
    store   r1, s[S_LINE]
    load    r1, e[E_SYM]
    store   r1, s[S_NEXT]
    store   s, e[E_SYM]
    load    r1, SCOPES
    #call   vec_last(r1)
    load    r1, r0[SC_SYMS]
    #call   vec_push(r1, s)
    #return s
#end_func

#def_func tag_lookup(ep)
    #var    g
    load    r0, ep
    load    g, r0[E_TAG]
    #if_cond g == 0
        #return 0
    #end_cond
    #return g
#end_func

// tag_in_scope(entry) -> Tag in the current scope or 0
#def_func tag_in_scope(ep)
    #var    g
    load    r0, ep
    load    g, r0[E_TAG]
    #if_cond g == 0
        #return 0
    #end_cond
    load    r0, g[G_DEPTH]
    load    r1, SCOPE_DEPTH
    #if_cond r0 == r1
        #return g
    #end_cond
    #return 0
#end_func

#def_func tag_declare(ep, typ)
    #var    e, g
    load    e, ep
    #call   zalloc(G_SIZE)
    move    g, r0
    store   e, g[G_NAME]
    load    r1, typ
    store   r1, g[G_TYPE]
    load    r1, SCOPE_DEPTH
    store   r1, g[G_DEPTH]
    load    r1, e[E_TAG]
    store   r1, g[G_NEXT]
    store   g, e[E_TAG]
    load    r1, SCOPES
    #call   vec_last(r1)
    load    r1, r0[SC_TAGS]
    #call   vec_push(r1, g)
    #return g
#end_func

SYMTAB_ASM_END:
    nop
