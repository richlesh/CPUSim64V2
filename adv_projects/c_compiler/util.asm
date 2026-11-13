///////////////////////////////////////////////////////////////////////////////
// util.asm
//
// Core utilities for the C compiler:
//   zalloc          zeroed heap allocation with out-of-memory check
//   vec_*           growable 1-based word vectors (also used as strings)
//   sb_*            string-builder helpers on codepoint vectors
//   w_* / p_*       wstr (word-per-codepoint) <-> packed string helpers
//   intern*         identifier interning hash table
//
// Requires: defs.def, errors.asm (cc_fatal)
///////////////////////////////////////////////////////////////////////////////

#global IT_TABLE:   .dci 0          // intern bucket array (IT_BUCKETS words)

UTIL_ASM_START:
    jump    UTIL_ASM_END

///////////////////////////////////////////////////////////////////////////////
// zalloc(cnt) -> address of cnt zeroed words.  Fatal error if out of memory.
///////////////////////////////////////////////////////////////////////////////
#def_func zalloc(cnt)
    #var    sz, ptr
    load    sz, cnt
    #if_cond sz < 1
        move    sz, 1
    #end_cond
    move    r1, sz
    int     iALLOC
    #if_cond r0 == 0
        #call   cc_fatal("out of memory")
    #end_cond
    move    ptr, r0
    move    r1, ptr
    move    r2, sz
    int     iMEMCLEAR
    #return ptr
#end_func

///////////////////////////////////////////////////////////////////////////////
// vec_new() -> new empty vector
///////////////////////////////////////////////////////////////////////////////
#def_func vec_new()
    #var    v
    #call   zalloc(2)
    move    v, r0
    #call   zalloc(9)
    store   r0, v[VEC_ARR]
    move    r1, 8
    store   r1, v[VEC_CAP]
    #return v
#end_func

///////////////////////////////////////////////////////////////////////////////
// vec_push(v, x): append x
///////////////////////////////////////////////////////////////////////////////
#def_func vec_push(vp, x)
    #var    v, a, len, cap
    load    v, vp
    load    a, v[VEC_ARR]
    load    len, a[0]
    load    cap, v[VEC_CAP]
    #if_cond len >= cap
        mult    cap, 2
        move    r1, a
        add     r2, cap, 1
        int     iREALLOC
        #if_cond r0 == 0
            #call   cc_fatal("out of memory")
        #end_cond
        move    a, r0
        store   a, v[VEC_ARR]
        store   cap, v[VEC_CAP]
    #end_cond
    add     len, 1
    store   len, a[0]
    load    r0, x
    store   r0, a[len]
#end_func

///////////////////////////////////////////////////////////////////////////////
// vec_pop(v) -> last element (removed).  Returns 0 if empty.
///////////////////////////////////////////////////////////////////////////////
#def_func vec_pop(vp)
    #var    a, len, x
    load    r0, vp
    load    a, r0[VEC_ARR]
    load    len, a[0]
    #if_cond len == 0
        #return 0
    #end_cond
    load    x, a[len]
    sub     len, 1
    store   len, a[0]
    #return x
#end_func

///////////////////////////////////////////////////////////////////////////////
// vec_last(v) -> last element or 0
///////////////////////////////////////////////////////////////////////////////
#def_func vec_last(vp)
    #var    a, len
    load    r0, vp
    load    a, r0[VEC_ARR]
    load    len, a[0]
    #if_cond len == 0
        #return 0
    #end_cond
    load    r0, a[len]
#end_func

///////////////////////////////////////////////////////////////////////////////
// vec_len(v) -> length
///////////////////////////////////////////////////////////////////////////////
#def_func vec_len(vp)
    load    r0, vp
    load    r0, r0[VEC_ARR]
    load    r0, r0[0]
#end_func

///////////////////////////////////////////////////////////////////////////////
// vec_get(v, i) -> element i (1-based)
///////////////////////////////////////////////////////////////////////////////
#def_func vec_get(vp, ip)
    #var    i
    load    i, ip
    load    r0, vp
    load    r0, r0[VEC_ARR]
    load    r0, r0[i]
#end_func

///////////////////////////////////////////////////////////////////////////////
// vec_clear(v): set length to 0
///////////////////////////////////////////////////////////////////////////////
#def_func vec_clear(vp)
    load    r0, vp
    load    r0, r0[VEC_ARR]
    move    r1, 0
    store   r1, r0[0]
#end_func

///////////////////////////////////////////////////////////////////////////////
// vec_free(v): free a vector and its storage
///////////////////////////////////////////////////////////////////////////////
#def_func vec_free(vp)
    #var    v
    load    v, vp
    load    r1, v[VEC_ARR]
    int     iFREE
    move    r1, v
    int     iFREE
#end_func

///////////////////////////////////////////////////////////////////////////////
// sb_add_p(v, ptr): append packed string ptr to codepoint vector v
///////////////////////////////////////////////////////////////////////////////
#def_func sb_add_p(vp, pstr)
    #var    v, cp, cnt, i, c
    load    v, vp
    load    r1, pstr
    int     iGET_CODEPOINTS
    move    cp, r0
    load    cnt, cp[0]
    #for    1, i <= cnt, 1
        load    c, cp[i]
        #call   vec_push(v, c)
    #end_for
    move    r1, cp
    int     iFREE
#end_func

///////////////////////////////////////////////////////////////////////////////
// sb_add_w(v, w): append wstr w
///////////////////////////////////////////////////////////////////////////////
#def_func sb_add_w(vp, wp)
    #var    v, w, cnt, i, c
    load    v, vp
    load    w, wp
    load    cnt, w[0]
    #for    1, i <= cnt, 1
        load    c, w[i]
        #call   vec_push(v, c)
    #end_for
#end_func

///////////////////////////////////////////////////////////////////////////////
// sb_add_int(v, cnt): append decimal representation of cnt
///////////////////////////////////////////////////////////////////////////////
#def_func sb_add_int(vp, nparm)
    #var    ptr
    load    r1, nparm
    int     iFMT_DEC
    move    ptr, r0
    load    r1, vp
    #call   sb_add_p(r1, ptr)
    move    r1, ptr
    int     iFREE
#end_func

///////////////////////////////////////////////////////////////////////////////
// sb_to_p(v) -> new heap packed string from codepoint vector v
///////////////////////////////////////////////////////////////////////////////
#def_func sb_to_p(vp)
    load    r0, vp
    load    r1, r0[VEC_ARR]
    int     iFROM_CODEPOINTS
#end_func

///////////////////////////////////////////////////////////////////////////////
// sb_to_w(v) -> new heap wstr copy of the codepoint vector v
///////////////////////////////////////////////////////////////////////////////
#def_func sb_to_w(vp)
    #var    a
    load    r0, vp
    load    a, r0[VEC_ARR]
    #call   w_copy(a)
#end_func

///////////////////////////////////////////////////////////////////////////////
// w_copy(w) -> new heap copy of wstr w
///////////////////////////////////////////////////////////////////////////////
#def_func w_copy(wp)
    #var    w, cnt, r
    load    w, wp
    load    cnt, w[0]
    add     r1, cnt, 1
    #call   zalloc(r1)
    move    r, r0
    move    r1, r
    move    r2, w
    add     r3, cnt, 1
    int     iMEMMOVE
    #return r
#end_func

///////////////////////////////////////////////////////////////////////////////
// w_sub(arr, start, len) -> new wstr from arr[start .. start+len-1]
///////////////////////////////////////////////////////////////////////////////
#def_func w_sub(ap, startp, lp)
    #var    a, s, cnt, r
    load    a, ap
    load    s, startp
    load    cnt, lp
    add     r1, cnt, 1
    #call   zalloc(r1)
    move    r, r0
    store   cnt, r[0]
    add     r1, r, 1
    add     r2, a, s
    move    r3, cnt
    int     iMEMMOVE
    #return r
#end_func

///////////////////////////////////////////////////////////////////////////////
// w_from_p(ptr) -> new wstr from packed string
///////////////////////////////////////////////////////////////////////////////
#def_func w_from_p(pstr)
    load    r1, pstr
    int     iGET_CODEPOINTS
#end_func

///////////////////////////////////////////////////////////////////////////////
// p_from_w(w) -> new packed string from wstr
///////////////////////////////////////////////////////////////////////////////
#def_func p_from_w(wp)
    load    r1, wp
    int     iFROM_CODEPOINTS
#end_func

///////////////////////////////////////////////////////////////////////////////
// w_eq(a, b) -> 1 if equal wstrs else 0
///////////////////////////////////////////////////////////////////////////////
#def_func w_eq(ap, bp)
    #var    a, b, cnt, i, c, d
    load    a, ap
    load    b, bp
    load    cnt, a[0]
    load    c, b[0]
    #if_cond cnt != c
        #return 0
    #end_cond
    #for    1, i <= cnt, 1
        load    c, a[i]
        load    d, b[i]
        #if_cond c != d
            #return 0
        #end_cond
    #end_for
    #return 1
#end_func

///////////////////////////////////////////////////////////////////////////////
// w_eq_p(w, ptr) -> 1 if wstr w equals packed string ptr
///////////////////////////////////////////////////////////////////////////////
#def_func w_eq_p(wp, pstr)
    #var    t, r
    load    r1, pstr
    int     iGET_CODEPOINTS
    move    t, r0
    load    r1, wp
    #call   w_eq(r1, t)
    move    r, r0
    move    r1, t
    int     iFREE
    #return r
#end_func

///////////////////////////////////////////////////////////////////////////////
// intern_init(): allocate the bucket table
///////////////////////////////////////////////////////////////////////////////
#def_func intern_init()
    #call   zalloc(IT_BUCKETS)
    store   r0, IT_TABLE
#end_func

///////////////////////////////////////////////////////////////////////////////
// intern(arr, start, len) -> unique entry for arr[start .. start+len-1]
///////////////////////////////////////////////////////////////////////////////
#def_func intern(ap, startp, lp)
    #var    a, s, cnt, h, i, c, e, w, j, d, ok, tbl
    load    a, ap
    load    s, startp
    load    cnt, lp
    // hash = h*31 + c
    move    h, 0
    add     j, s, cnt             // j = end (exclusive)
    move    i, s
    #while  i < j
        load    c, a[i]
        mult    h, 31
        add     h, c
        add     i, 1
    #end_while
    move    i, h
    and     i, IT_MASK
    load    tbl, IT_TABLE
    load    e, tbl[i]
    #while  e != 0
        load    d, e[E_HASH]
        #if_cond d == h
            load    w, e[E_W]
            load    d, w[0]
            #if_cond d == cnt
                move    ok, 1
                move    j, 1
                #while  j <= cnt
                    add     r1, s, j
                    sub     r1, 1
                    load    c, a[r1]
                    load    d, w[j]
                    #if_cond c != d
                        move    ok, 0
                        #break
                    #end_cond
                    add     j, 1
                #end_while
                #if_cond ok != 0
                    #return e
                #end_cond
            #end_cond
        #end_cond
        load    e, e[E_NEXT]
    #end_while
    // create a new entry
    #call   w_sub(a, s, cnt)
    move    w, r0
    #call   zalloc(E_SIZE)
    move    e, r0
    store   w, e[E_W]
    store   h, e[E_HASH]
    load    r1, tbl[i]
    store   r1, e[E_NEXT]
    store   e, tbl[i]
    #return e
#end_func

///////////////////////////////////////////////////////////////////////////////
// intern_w(w) -> entry for the whole wstr
///////////////////////////////////////////////////////////////////////////////
#def_func intern_w(wp)
    #var    w, cnt
    load    w, wp
    load    cnt, w[0]
    #call   intern(w, 1, cnt)
#end_func

///////////////////////////////////////////////////////////////////////////////
// intern_p(ptr) -> entry for packed string
///////////////////////////////////////////////////////////////////////////////
#def_func intern_p(pstr)
    #var    t, e
    load    r1, pstr
    int     iGET_CODEPOINTS
    move    t, r0
    #call   intern_w(t)
    move    e, r0
    move    r1, t
    int     iFREE
    #return e
#end_func

///////////////////////////////////////////////////////////////////////////////
// ent_p(e) -> packed string of an entry's name (cached)
///////////////////////////////////////////////////////////////////////////////
#def_func ent_p(ep)
    #var    e
    load    e, ep
    #if_cond e == 0
        #return "?"
    #end_cond
    load    r0, e[E_P]
    #if_cond r0 == 0
        load    r1, e[E_W]
        int     iFROM_CODEPOINTS
        store   r0, e[E_P]
    #end_cond
#end_func

///////////////////////////////////////////////////////////////////////////////
// is_lib_file(file_entry) -> 1 if the file is a library source (the prelude
//                            or a file from the lib/ directory)
///////////////////////////////////////////////////////////////////////////////
#def_func is_lib_file(ep)
    load    r0, ep
    #if_cond r0 == 0
        #return 0
    #end_cond
    load    r0, r0[E_SPEC]
    #if_cond r0 == SPEC_LIBFILE
        #return 1
    #end_cond
    #return 0
#end_func

// mark_lib_file(file_entry)
#def_func mark_lib_file(ep)
    load    r0, ep
    move    r1, SPEC_LIBFILE
    store   r1, r0[E_SPEC]
#end_func

///////////////////////////////////////////////////////////////////////////////
// u_divmod(a, b) -> unsigned a / b; the unsigned remainder is left in
//      U_REM.  CPUSim64 only divides signed, so (a >> 1) / b is computed and
//      corrected (for b >= 2^63 the quotient is 0 or 1).  b must not be 0.
///////////////////////////////////////////////////////////////////////////////
#global U_REM: .dci 0

#def_func u_divmod(ap, bp)
    #var    a, b, q, r
    load    a, ap
    load    b, bp
    #if_cond b < 0
        move    q, 0
        move    r, a
        #if_cond a < 0
            #if_cond a >= b
                move    q, 1
                sub     r, a, b
            #end_cond
        #end_cond
        store   r, U_REM
        #return q
    #end_cond
    move    q, a
    rshift  q, 1
    div     q, b
    lshift  q, 1
    mult    r, q, b
    sub     r, a, r
    // r < 2b; fix up once if r >= b (r may exceed 2^63 - 1)
    #if_cond r < 0
        add     q, 1
        sub     r, b
    #else_cond
        #if_cond r >= b
            add     q, 1
            sub     r, b
        #end_cond
    #end_cond
    store   r, U_REM
    #return q
#end_func

///////////////////////////////////////////////////////////////////////////////
// cmp_lt(a, b, unsigned) -> 1 if a < b, else 0 (no subtraction overflow)
///////////////////////////////////////////////////////////////////////////////
#def_func cmp_lt(ap, bp, up)
    #var    a, b, u
    load    a, ap
    load    b, bp
    load    u, up
    #if_cond u != 0
        // flipping the sign bits maps unsigned order onto signed order
        move    r1, 1
        lshift  r1, 63
        xor     a, r1
        xor     b, r1
    #end_cond
    xor     r0, a, b
    #if_cond r0 < 0
        // different signs: a < b exactly when a is negative
        #if_cond a < 0
            #return 1
        #end_cond
        #return 0
    #end_cond
    #if_cond a < b
        #return 1
    #end_cond
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// u2d(v) -> f0 = (double)v for an unsigned 64-bit v
// d2u()  -> r0 = (unsigned 64-bit)f0
///////////////////////////////////////////////////////////////////////////////
#def_func u2d(vp)
    #var    v, b
    load    v, vp
    #if_cond v >= 0
        move    f0, v
        #return 0
    #end_cond
    and     b, v, 1
    rshift  v, 1
    or      v, b
    move    f0, v
    add     f0, f0
#end_func

#def_func d2u()
    #var    r
    move    r1, 0x43E
    lshift  r1, 52
    store   r1, U_REM
    load    f1, U_REM
    cmp     f0, f1
    #if_cond_sr lt
        move    r0, f0
        #return r0
    #end_cond
    sub     f0, f1
    move    r, f0
    move    r1, 1
    lshift  r1, 63
    xor     r, r1
    #return r
#end_func

UTIL_ASM_END:
    nop
