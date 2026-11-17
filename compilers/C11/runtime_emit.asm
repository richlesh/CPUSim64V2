///////////////////////////////////////////////////////////////////////////////
// runtime_emit.asm
//
// The C runtime library that is copied into generated programs.  The text
// below is a sequence of .dcs lines; a line "@@name dep1 dep2 ..." starts the
// chunk that implements runtime function "name" (label __crt_name) and lists
// the other chunks it needs.  A chunk is emitted only if the program uses it.
//
//   crt_init()          index the chunks
//   crt_need(entry)     mark a chunk (and its dependencies) as needed
//   crt_emit()          write all needed chunks to the output
///////////////////////////////////////////////////////////////////////////////

#global CRT_RECS:   .dci 0          // vec of chunk records

#define CR_NAME     0
#define CR_START    1               // address of the first text line
#define CR_DEPS     2               // vec of intern entries
#define CR_NEEDED   3
#define CR_SIZE     4

RUNTIME_EMIT_ASM_START:
    jump    RUNTIME_EMIT_ASM_END

// dcs_next(line) -> address of the .dcs line following line
#def_func dcs_next(ln)
    load    r0, ln
    load    r1, r0[0]
    add     r1, 7
    rshift  r1, 3
    add     r0, r1
    add     r0, 1
#end_func

// is_chunk_header(line) -> 1 if the line starts with "@@"
#def_func is_chunk_header(ln)
    #var    a
    load    a, ln
    load    r0, a[0]
    #if_cond r0 < 2
        #return 0
    #end_cond
    load    r0, a[1]
    rshift  r0, 48
    #if_cond r0 == 0x4040
        #return 1
    #end_cond
    #return 0
#end_func

#def_func crt_init()
    #var    a, rec, words, cnt, i, w, e
    #call   vec_new()
    store   r0, CRT_RECS
    move    a, CRT_TEXT
    #while  TRUE
        load    r0, a[0]
        #if_cond r0 < 0
            #break
        #end_cond
        #call   is_chunk_header(a)
        #if_cond r0 != 0
            #call   split_words(a)
            move    words, r0
            #call   zalloc(CR_SIZE)
            move    rec, r0
            #macro  VGET(w, words, 1)
            load    cnt, w[0]
            sub     cnt, 2
            #call   intern(w, 3, cnt)
            move    e, r0
            store   e, rec[CR_NAME]
            store   rec, e[E_CRT]
            #call   vec_new()
            store   r0, rec[CR_DEPS]
            #macro  VLEN(cnt, words)
            #for    2, i <= cnt, 1
                #macro  VGET(w, words, i)
                #call   intern_w(w)
                load    r1, rec[CR_DEPS]
                #call   vec_push(r1, r0)
            #end_for
            #call   dcs_next(a)
            store   r0, rec[CR_START]
            load    r1, CRT_RECS
            #call   vec_push(r1, rec)
        #end_cond
        #call   dcs_next(a)
        move    a, r0
    #end_while
#end_func

// crt_need(entry): mark a runtime chunk (and its dependencies) as needed
#def_func crt_need(ep)
    #var    e, rec, deps, cnt, i
    load    e, ep
    load    rec, e[E_CRT]
    #if_cond rec == 0
        #return 0
    #end_cond
    load    r0, rec[CR_NEEDED]
    #if_cond r0 != 0
        #return 0
    #end_cond
    move    r0, 1
    store   r0, rec[CR_NEEDED]
    load    deps, rec[CR_DEPS]
    #macro  VLEN(cnt, deps)
    #for    1, i <= cnt, 1
        #macro  VGET(r1, deps, i)
        #call   crt_need(r1)
    #end_for
#end_func

// crt_emit(): copy every needed chunk that the program did not define itself
#def_func crt_emit()
    #var    recs, cnt, i, rec, a, g, tc, tv
    // thread_create of a program with _Thread_local objects (always, in a
    // module: another module may define them) is the variant that gives the
    // new thread its initial values
    move    tv, 0
    #call   intern_p("thread_create")
    load    tc, r0[E_CRT]
    #if_cond tc != 0
        load    r0, tc[CR_NEEDED]
        #if_cond r0 != 0
            load    r0, CC_MODULE
            #if_cond r0 != MOD_NONE
                move    tv, 1
            #end_cond
            load    r0, CG_TLS_DEFS
            #if_cond r0 != 0
                move    tv, 1
            #end_cond
        #end_cond
    #end_cond
    #if_cond tv != 0
        #call   crt_use_p("__tls_copy")
        move    r0, 1
        store   r0, CG_NEEDHEAD
        #call   intern_p("__thread_create_tls")
        load    tv, r0[E_CRT]
    #end_cond
    load    recs, CRT_RECS
    #macro  VLEN(cnt, recs)
    #for    1, i <= cnt, 1
        #macro  VGET(rec, recs, i)
        load    r0, rec[CR_NEEDED]
        #if_cond r0 == 0
            #continue
        #end_cond
        load    r0, rec[CR_NAME]
        load    g, r0[E_GLOBAL]
        #if_cond g != 0
            load    r0, g[S_FLAGS]
            and     r0, SF_DEFINED
            #if_cond r0 != 0
                #continue
            #end_cond
        #end_cond
        #call   emit("\n")
        load    a, rec[CR_START]
        #if_cond rec == tc
            #if_cond tv != 0
                load    a, tv[CR_START]
            #end_cond
        #end_cond
        #while  TRUE
            load    r0, a[0]
            #if_cond r0 < 0
                #break
            #end_cond
            #call   is_chunk_header(a)
            #if_cond r0 != 0
                #break
            #end_cond
            #call   emit_p(a)
            load    r1, OUT_PORT
            int     iPUT_NL
            #call   dcs_next(a)
            move    a, r0
        #end_while
    #end_for
#end_func

CRT_TEXT:
    .dcs    "@@__io_open c2p"
    .dcs    "// __io_open(name): open the file named by C string name as a random-access"
    .dcs    "// port (created if missing, never truncated).  Returns the port or -1."
    .dcs    "#def_func __crt___io_open(name)"
    .dcs    "    #var    ps, port"
    .dcs    "    load    r1, name"
    .dcs    "    #call   __crt_c2p(r1)"
    .dcs    "    move    ps, r0"
    .dcs    "    #call   openRandomAccessFile(ps)"
    .dcs    "    move    port, r0"
    .dcs    "    move    r1, ps"
    .dcs    "    int     iFREE"
    .dcs    "    #return port"
    .dcs    "#end_func"
    .dcs    "@@__io_seek"
    .dcs    "// __io_seek(port, pos): iSEEK; returns the new position or -1"
    .dcs    "__crt___io_seek:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    load    r2, sf[4]"
    .dcs    "    int     iSEEK"
    .dcs    "    return"
    .dcs    "@@__io_tell"
    .dcs    "// __io_tell(port): iTELL; returns the position or -1"
    .dcs    "__crt___io_tell:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    int     iTELL"
    .dcs    "    return"
    .dcs    "@@__io_truncate"
    .dcs    "// __io_truncate(name): empty a file by reopening it for writing"
    .dcs    "#def_func __crt___io_truncate(name)"
    .dcs    "    #var    ps, port"
    .dcs    "    load    r1, name"
    .dcs    "    #call   __crt_c2p(r1)"
    .dcs    "    move    ps, r0"
    .dcs    "    #call   openRawFile(ps, WRITE_MODE)"
    .dcs    "    move    port, r0"
    .dcs    "    #if_cond port != -1"
    .dcs    "        #call   closeFile(port)"
    .dcs    "    #end_cond"
    .dcs    "    move    r1, ps"
    .dcs    "    int     iFREE"
    .dcs    "    #if_cond port == -1"
    .dcs    "        #return -1"
    .dcs    "    #end_cond"
    .dcs    "    #return 0"
    .dcs    "#end_func"
    .dcs    "@@__io_close"
    .dcs    "__crt___io_close:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    #call   closeFile(r1)"
    .dcs    "    move    r0, 0"
    .dcs    "    return"
    .dcs    "@@__io_getc"
    .dcs    "// __io_getc(port): next codepoint, or -1 at end of file"
    .dcs    "__crt___io_getc:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    in      r0, CHAR, r1"
    .dcs    "    return"
    .dcs    "@@__io_getb"
    .dcs    "// __io_getb(port): next byte (raw files), or -1 at end of file"
    .dcs    "__crt___io_getb:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    in      r0, BYTE, r1"
    .dcs    "    return"
    .dcs    "@@__io_putc"
    .dcs    "// __io_putc(port, c): write one codepoint"
    .dcs    "__crt___io_putc:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    load    r2, sf[4]"
    .dcs    "    out     r2, CHAR, r1"
    .dcs    "    return"
    .dcs    "@@__io_putb"
    .dcs    "// __io_putb(port, b): write one byte (raw files)"
    .dcs    "__crt___io_putb:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    load    r2, sf[4]"
    .dcs    "    out     r2, BYTE, r1"
    .dcs    "    return"
    .dcs    "@@__io_write"
    .dcs    "// __io_write(port, buf, n): write n codepoints from a word buffer with one"
    .dcs    "// packed-string write"
    .dcs    "#def_func __crt___io_write(port, buf, cnt)"
    .dcs    "    #var    arr, nn, ps"
    .dcs    "    load    nn, cnt"
    .dcs    "    #if_cond nn <= 0"
    .dcs    "        #return 0"
    .dcs    "    #end_cond"
    .dcs    "    add     r1, nn, 1"
    .dcs    "    int     iALLOC"
    .dcs    "    move    arr, r0"
    .dcs    "    store   nn, arr[0]"
    .dcs    "    // copy with a loop: iMEMMOVE cannot read the stack"
    .dcs    "    load    r2, buf"
    .dcs    "    move    r3, 0"
    .dcs    "__crt___io_write_copy:"
    .dcs    "    load    r1, r2[r3]"
    .dcs    "    add     r3, 1"
    .dcs    "    store   r1, arr[r3]"
    .dcs    "    cmp     r3, nn"
    .dcs    "    jump    lt, __crt___io_write_copy"
    .dcs    "    move    r1, arr"
    .dcs    "    int     iFROM_CODEPOINTS"
    .dcs    "    move    ps, r0"
    .dcs    "    move    r1, arr"
    .dcs    "    int     iFREE"
    .dcs    "    load    r1, port"
    .dcs    "    move    r2, ps"
    .dcs    "    int     iPUTS"
    .dcs    "    move    r1, ps"
    .dcs    "    int     iFREE"
    .dcs    "    #return nn"
    .dcs    "#end_func"
    .dcs    "@@__io_flush"
    .dcs    "__crt___io_flush:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    int     iFLUSH"
    .dcs    "    move    r0, 0"
    .dcs    "    return"
    .dcs    "@@__io_remove c2p"
    .dcs    "// __io_remove(name): delete a file; 0 on success, -1 on failure"
    .dcs    "#def_func __crt___io_remove(name)"
    .dcs    "    #var    ps, ok"
    .dcs    "    load    r1, name"
    .dcs    "    #call   __crt_c2p(r1)"
    .dcs    "    move    ps, r0"
    .dcs    "    move    r1, ps"
    .dcs    "    int     iDELETE_FILE"
    .dcs    "    move    ok, r0"
    .dcs    "    move    r1, ps"
    .dcs    "    int     iFREE"
    .dcs    "    #if_cond ok == 0"
    .dcs    "        #return -1"
    .dcs    "    #end_cond"
    .dcs    "    #return 0"
    .dcs    "#end_func"
    .dcs    "@@__io_exists c2p"
    .dcs    "// __io_exists(name): 1 if the path exists as a regular file"
    .dcs    "#def_func __crt___io_exists(name)"
    .dcs    "    #var    ps, ok"
    .dcs    "    load    r1, name"
    .dcs    "    #call   __crt_c2p(r1)"
    .dcs    "    move    ps, r0"
    .dcs    "    move    r1, ps"
    .dcs    "    int     iIS_FILE"
    .dcs    "    move    ok, r0"
    .dcs    "    move    r1, ps"
    .dcs    "    int     iFREE"
    .dcs    "    #if_cond ok != 0"
    .dcs    "        #return 1"
    .dcs    "    #end_cond"
    .dcs    "    #return 0"
    .dcs    "#end_func"
    .dcs    "@@__io_tmpname p2buf"
    .dcs    "// __io_tmpname(buf): store a fresh temporary file path as a C string"
    .dcs    "#def_func __crt___io_tmpname(buf)"
    .dcs    "    #var    ps"
    .dcs    "    move    r1, \"cc_tmp\""
    .dcs    "    move    r2, \".tmp\""
    .dcs    "    int     iTEMP_FILE"
    .dcs    "    move    ps, r0"
    .dcs    "    #if_cond ps == 0"
    .dcs    "        #return 0"
    .dcs    "    #end_cond"
    .dcs    "    load    r1, buf"
    .dcs    "    #call   __crt_p2buf(r1, ps)"
    .dcs    "    #return 1"
    .dcs    "#end_func"
    .dcs    "@@p2buf"
    .dcs    "#def_func __crt_p2buf(buf, ps)"
    .dcs    "    #var    dst, cp, cnt, i, c"
    .dcs    "    load    dst, buf"
    .dcs    "    load    r1, ps"
    .dcs    "    int     iGET_CODEPOINTS"
    .dcs    "    move    cp, r0"
    .dcs    "    load    cnt, cp[0]"
    .dcs    "    #for    1, i <= cnt, 1"
    .dcs    "        load    c, cp[i]"
    .dcs    "        store   c, dst[0]"
    .dcs    "        add     dst, 1"
    .dcs    "    #end_for"
    .dcs    "    store   0, dst[0]"
    .dcs    "    move    r1, cp"
    .dcs    "    int     iFREE"
    .dcs    "    load    r1, ps"
    .dcs    "    int     iFREE"
    .dcs    "    #return cnt"
    .dcs    "#end_func"
    .dcs    "@@malloc"
    .dcs    "__crt_malloc:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    cmp     r1, 1"
    .dcs    "    move    lt, r1, 1, r1"
    .dcs    "    int     iALLOC"
    .dcs    "    return"
    .dcs    "@@calloc"
    .dcs    "#def_func __crt_calloc(cnt, sz)"
    .dcs    "    #var    total, ptr"
    .dcs    "    load    total, cnt"
    .dcs    "    load    r1, sz"
    .dcs    "    mult    total, r1"
    .dcs    "    #if_cond total < 1"
    .dcs    "        move    total, 1"
    .dcs    "    #end_cond"
    .dcs    "    move    r1, total"
    .dcs    "    int     iALLOC"
    .dcs    "    move    ptr, r0"
    .dcs    "    #if_cond ptr != 0"
    .dcs    "        move    r1, ptr"
    .dcs    "        move    r2, total"
    .dcs    "        int     iMEMCLEAR"
    .dcs    "    #end_cond"
    .dcs    "    #return ptr"
    .dcs    "#end_func"
    .dcs    "@@realloc"
    .dcs    "#def_func __crt_realloc(ptr, size)"
    .dcs    "    load    r1, ptr"
    .dcs    "    load    r2, size"
    .dcs    "    #if_cond r2 < 1"
    .dcs    "        move    r2, 1"
    .dcs    "    #end_cond"
    .dcs    "    #if_cond r1 == 0"
    .dcs    "        move    r1, r2"
    .dcs    "        int     iALLOC"
    .dcs    "    #else_cond"
    .dcs    "        int     iREALLOC"
    .dcs    "    #end_cond"
    .dcs    "#end_func"
    .dcs    "@@free"
    .dcs    "__crt_free:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    test    r1"
    .dcs    "    jump    z, __crt_free_done"
    .dcs    "    int     iFREE"
    .dcs    "__crt_free_done:"
    .dcs    "    return"
    .dcs    "@@_Exit"
    .dcs    "__crt__Exit:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    int     iEXIT"
    .dcs    "@@abort"
    .dcs    "__crt_abort:"
    .dcs    "    move    r1, 134"
    .dcs    "    int     iEXIT"
    .dcs    "@@system"
    .dcs    "__crt_system:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    int     iSYSTEM"
    .dcs    "    return"
    .dcs    "@@clock"
    .dcs    "__crt_clock:"
    .dcs    "    int     iCLOCK"
    .dcs    "    div     r0, 1000"
    .dcs    "    return"
    .dcs    "@@__time_now"
    .dcs    "__crt___time_now:"
    .dcs    "    int     iTIME"
    .dcs    "    div     r0, 1000000000"
    .dcs    "    return"
    .dcs    "@@__time_ns"
    .dcs    "__crt___time_ns:"
    .dcs    "    int     iTIME"
    .dcs    "    return"
    .dcs    "@@__udivmod"
    .dcs    "// unsigned 64-bit divide: r2 = r2 / r3, r3 = r2 % r3 (uses r4; r0, r1 kept)"
    .dcs    "__crt___udivmod:"
    .dcs    "    test    r3"
    .dcs    "    jump    n, __crt___udivmod_big"
    .dcs    "    move    r4, r2"
    .dcs    "    rshift  r4, 1"
    .dcs    "    div     r4, r3"
    .dcs    "    lshift  r4, 1"
    .dcs    "    push    r4"
    .dcs    "    mult    r4, r3"
    .dcs    "    sub     r2, r4"
    .dcs    "    pop     r4"
    .dcs    "    test    r2"
    .dcs    "    jump    n, __crt___udivmod_fix"
    .dcs    "    cmp     r2, r3"
    .dcs    "    jump    lt, __crt___udivmod_done"
    .dcs    "__crt___udivmod_fix:"
    .dcs    "    add     r4, 1"
    .dcs    "    sub     r2, r3"
    .dcs    "__crt___udivmod_done:"
    .dcs    "    move    r3, r2"
    .dcs    "    move    r2, r4"
    .dcs    "    return"
    .dcs    "__crt___udivmod_big:"
    .dcs    "    move    r4, 0"
    .dcs    "    test    r2"
    .dcs    "    jump    nn, __crt___udivmod_done"
    .dcs    "    cmp     r2, r3"
    .dcs    "    jump    lt, __crt___udivmod_done"
    .dcs    "    move    r4, 1"
    .dcs    "    sub     r2, r3"
    .dcs    "    jump    __crt___udivmod_done"
    .dcs    "@@setjmp"
    .dcs    "__crt_setjmp:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    load    r2, sf[1]"
    .dcs    "    store   r2, r1[0]"
    .dcs    "    load    r2, sf[2]"
    .dcs    "    store   r2, r1[1]"
    .dcs    "    add     r2, sf, 2"
    .dcs    "    store   r2, r1[2]"
    .dcs    "    move    r0, 0"
    .dcs    "    return"
    .dcs    "@@longjmp"
    .dcs    "__crt_longjmp:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    load    r0, sf[4]"
    .dcs    "    test    r0"
    .dcs    "    move    z, r0, 1, r0"
    .dcs    "    load    r2, r1[1]"
    .dcs    "    load    sp, r1[2]"
    .dcs    "    load    sf, r1[0]"
    .dcs    "    jump    r2"
    .dcs    "@@sqrt"
    .dcs    "__crt_sqrt:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iSQRT"
    .dcs    "    return"
    .dcs    "@@sin"
    .dcs    "__crt_sin:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iSIN"
    .dcs    "    return"
    .dcs    "@@cos"
    .dcs    "__crt_cos:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iCOS"
    .dcs    "    return"
    .dcs    "@@tan"
    .dcs    "__crt_tan:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iTAN"
    .dcs    "    return"
    .dcs    "@@asin"
    .dcs    "__crt_asin:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iASIN"
    .dcs    "    return"
    .dcs    "@@acos"
    .dcs    "__crt_acos:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iACOS"
    .dcs    "    return"
    .dcs    "@@atan"
    .dcs    "__crt_atan:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iATAN"
    .dcs    "    return"
    .dcs    "@@atan2"
    .dcs    "__crt_atan2:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    load    f2, sf[4]"
    .dcs    "    int     iATAN2"
    .dcs    "    return"
    .dcs    "@@exp"
    .dcs    "__crt_exp:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iEXP"
    .dcs    "    return"
    .dcs    "@@log"
    .dcs    "__crt_log:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iLOG"
    .dcs    "    return"
    .dcs    "@@pow"
    .dcs    "__crt_pow:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    load    f2, sf[4]"
    .dcs    "    int     iPOW"
    .dcs    "    return"
    .dcs    "@@fabs"
    .dcs    "__crt_fabs:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iABS_FP"
    .dcs    "    return"
    .dcs    "@@floor"
    .dcs    "__crt_floor:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iFLOOR"
    .dcs    "    return"
    .dcs    "@@ceil"
    .dcs    "__crt_ceil:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iCEIL"
    .dcs    "    return"
    .dcs    "@@round"
    .dcs    "__crt_round:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iROUND"
    .dcs    "    return"
    .dcs    "@@sinh"
    .dcs    "__crt_sinh:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iSINH"
    .dcs    "    return"
    .dcs    "@@cosh"
    .dcs    "__crt_cosh:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iCOSH"
    .dcs    "    return"
    .dcs    "@@tanh"
    .dcs    "__crt_tanh:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iTANH"
    .dcs    "    return"
    .dcs    "@@asinh"
    .dcs    "__crt_asinh:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iASINH"
    .dcs    "    return"
    .dcs    "@@acosh"
    .dcs    "__crt_acosh:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iACOSH"
    .dcs    "    return"
    .dcs    "@@atanh"
    .dcs    "__crt_atanh:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    int     iATANH"
    .dcs    "    return"
    .dcs    "@@remainder"
    .dcs    "__crt_remainder:"
    .dcs    "    load    f1, sf[3]"
    .dcs    "    load    f2, sf[4]"
    .dcs    "    int     iREMAINDER"
    .dcs    "    return"
    .dcs    "@@c2p"
    .dcs    "#def_func __crt_c2p(s)"
    .dcs    "    #var    src, cnt, arr, i, c"
    .dcs    "    load    src, s"
    .dcs    "    move    cnt, 0"
    .dcs    "    #if_cond src != 0"
    .dcs    "        load    c, src[0]"
    .dcs    "        #while  c != 0"
    .dcs    "            add     cnt, 1"
    .dcs    "            load    c, src[cnt]"
    .dcs    "        #end_while"
    .dcs    "    #end_cond"
    .dcs    "    add     r1, cnt, 1"
    .dcs    "    int     iALLOC"
    .dcs    "    move    arr, r0"
    .dcs    "    store   cnt, arr[0]"
    .dcs    "    #for    1, i <= cnt, 1"
    .dcs    "        sub     r1, i, 1"
    .dcs    "        load    c, src[r1]"
    .dcs    "        store   c, arr[i]"
    .dcs    "    #end_for"
    .dcs    "    move    r1, arr"
    .dcs    "    int     iFROM_CODEPOINTS"
    .dcs    "    move    c, r0"
    .dcs    "    move    r1, arr"
    .dcs    "    int     iFREE"
    .dcs    "    #return c"
    .dcs    "#end_func"
    .dcs    "@@p2c"
    .dcs    "#def_func __crt_p2c(ps)"
    .dcs    "    #var    cp, cnt, i, c, arr"
    .dcs    "    load    r1, ps"
    .dcs    "    int     iGET_CODEPOINTS"
    .dcs    "    move    cp, r0"
    .dcs    "    load    cnt, cp[0]"
    .dcs    "    add     r1, cnt, 1"
    .dcs    "    int     iALLOC"
    .dcs    "    move    arr, r0"
    .dcs    "    #for    1, i <= cnt, 1"
    .dcs    "        load    c, cp[i]"
    .dcs    "        sub     r1, i, 1"
    .dcs    "        store   c, arr[r1]"
    .dcs    "    #end_for"
    .dcs    "    store   0, arr[cnt]"
    .dcs    "    move    r1, cp"
    .dcs    "    int     iFREE"
    .dcs    "    #return arr"
    .dcs    "#end_func"
    .dcs    "@@args p2c"
    .dcs    "#def_func __crt_args()"
    .dcs    "    #var    argc, argv, i"
    .dcs    "    int     iARGC"
    .dcs    "    move    argc, r0"
    .dcs    "    add     r1, argc, 1"
    .dcs    "    int     iALLOC"
    .dcs    "    move    argv, r0"
    .dcs    "    #for    0, i < argc, 1"
    .dcs    "        move    r1, i"
    .dcs    "        int     iARGS"
    .dcs    "        #call   __crt_p2c(r0)"
    .dcs    "        store   r0, argv[i]"
    .dcs    "    #end_for"
    .dcs    "    store   0, argv[argc]"
    .dcs    "    #return argv"
    .dcs    "#end_func"
    .dcs    "@@__atof_exact c2p"
    .dcs    "#def_func __crt___atof_exact(s)"
    .dcs    "    #var    ps"
    .dcs    "    load    r1, s"
    .dcs    "    #call   __crt_c2p(r1)"
    .dcs    "    move    ps, r0"
    .dcs    "    move    r1, ps"
    .dcs    "    int     iPARSE_FLOAT"
    .dcs    "    push    f0"
    .dcs    "    move    r1, ps"
    .dcs    "    int     iFREE"
    .dcs    "    pop     f0"
    .dcs    "#end_func"
    .dcs    "@@__getenv_raw c2p"
    .dcs    "#def_func __crt___getenv_raw(s)"
    .dcs    "    #var    ps"
    .dcs    "    load    r1, s"
    .dcs    "    #call   __crt_c2p(r1)"
    .dcs    "    move    ps, r0"
    .dcs    "    move    r1, ps"
    .dcs    "    int     iENV"
    .dcs    "    push    r0"
    .dcs    "    move    r1, ps"
    .dcs    "    int     iFREE"
    .dcs    "    pop     r0"
    .dcs    "#end_func"
    .dcs    "@@__getenv_p2c p2c"
    .dcs    "#def_func __crt___getenv_p2c(ps)"
    .dcs    "    load    r1, ps"
    .dcs    "    #call   __crt_p2c(r1)"
    .dcs    "#end_func"
    .dcs    "@@__assert_fail"
    .dcs    "#def_func __crt___assert_fail(e, fl, ln)"
    .dcs    "    load    r1, e"
    .dcs    "    load    r2, fl"
    .dcs    "    load    r3, ln"
    .dcs    "    #call   fprintf(STDERR, \"%s:%d: Assertion `%s' failed.\\n\", r2, r3, r1)"
    .dcs    "    #call   exit(134)"
    .dcs    "#end_func"
    .dcs    "// ---- threads and processes (lib/_Thread.h)"
    .dcs    "@@thread_create"
    .dcs    "__crt_thread_create:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    load    r2, sf[4]"
    .dcs    "    int     iTHREAD"
    .dcs    "    return"
    .dcs    "@@__thread_create_tls __tls_copy"
    .dcs    "// thread_create(fn, arg) of a program with _Thread_local objects (selected by"
    .dcs    "// crt_emit instead of the one above): the new thread runs __crt_thread_start,"
    .dcs    "// which gives it its copy of the initial values, then calls fn(arg)"
    .dcs    "__crt_thread_create:"
    .dcs    "    move    r1, 2"
    .dcs    "    int     iALLOC"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    store   r1, r0[0]"
    .dcs    "    load    r1, sf[4]"
    .dcs    "    store   r1, r0[1]"
    .dcs    "    move    r2, r0"
    .dcs    "    move    r1, __crt_thread_start"
    .dcs    "    int     iTHREAD"
    .dcs    "    return"
    .dcs    "__crt_thread_start:"
    .dcs    "    add     sp, -1"
    .dcs    "    load    r1, __crt_tls_head+1"
    .dcs    "    move    r2, 0"
    .dcs    "    call    __crt_tls_copy"
    .dcs    "    load    r3, sf[3]"
    .dcs    "    load    r1, r3[0]"
    .dcs    "    store   r1, sf[0]"
    .dcs    "    load    r1, r3[1]"
    .dcs    "    push    r1"
    .dcs    "    move    r1, r3"
    .dcs    "    int     iFREE"
    .dcs    "    load    r0, sf[0]"
    .dcs    "    call    r0"
    .dcs    "    return"
    .dcs    "@@__tls_copy"
    .dcs    "// __crt_tls_copy: r1 = first record of the list of thread-local images, r2 = the"
    .dcs    "// record where to stop (not copied); copies each image to its object"
    .dcs    "__crt_tls_copy:"
    .dcs    "    cmp     r1, r2"
    .dcs    "    jump    eq, __crt_tls_copy_done"
    .dcs    "    load    r3, r1[1]"
    .dcs    "    load    r4, r1[2]"
    .dcs    "    load    r5, r1[3]"
    .dcs    "    move    r6, 0"
    .dcs    "__crt_tls_copy_word:"
    .dcs    "    load    r0, r4[r6]"
    .dcs    "    store   r0, r3[r6]"
    .dcs    "    add     r6, 1"
    .dcs    "    cmp     r6, r5"
    .dcs    "    jump    lt, __crt_tls_copy_word"
    .dcs    "    load    r1, r1[0]"
    .dcs    "    jump    __crt_tls_copy"
    .dcs    "__crt_tls_copy_done:"
    .dcs    "    return"
    .dcs    "@@thread_join"
    .dcs    "__crt_thread_join:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    int     iJOIN_THREAD"
    .dcs    "    return"
    .dcs    "@@thread_wake"
    .dcs    "__crt_thread_wake:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    int     iWAKE_THREAD"
    .dcs    "    return"
    .dcs    "@@fork"
    .dcs    "__crt_fork:"
    .dcs    "    int     iFORK"
    .dcs    "    return"
    .dcs    "@@wait_pid"
    .dcs    "__crt_wait_pid:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    int     iWAIT_PID"
    .dcs    "    return"
    .dcs    "@@wait_all"
    .dcs    "__crt_wait_all:"
    .dcs    "    int     iWAIT"
    .dcs    "    return"
    .dcs    "@@sleep_ms"
    .dcs    "__crt_sleep_ms:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    int     iSLEEP"
    .dcs    "    return"
    .dcs    "@@get_pid"
    .dcs    "__crt_get_pid:"
    .dcs    "    int     iGET_PID"
    .dcs    "    return"
    .dcs    "@@get_num_cores"
    .dcs    "__crt_get_num_cores:"
    .dcs    "    int     iGET_NUM_CORES"
    .dcs    "    return"
    .dcs    "@@alloc_shared"
    .dcs    "__crt_alloc_shared:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    cmp     r1, 1"
    .dcs    "    move    lt, r1, 1, r1"
    .dcs    "    int     iALLOC_SHARED"
    .dcs    "    return"
    .dcs    "@@atomic_cas"
    .dcs    "// atomic_cas(addr, expected, desired): 1 if *addr was expected and is now desired"
    .dcs    "__crt_atomic_cas:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    load    r2, sf[4]"
    .dcs    "    load    r3, sf[5]"
    .dcs    "    move    r0, 0"
    .dcs    "    cas     r2, r3, r1"
    .dcs    "    jump    no, __crt_atomic_cas_done"
    .dcs    "    move    r0, 1"
    .dcs    "__crt_atomic_cas_done:"
    .dcs    "    return"
    .dcs    "@@__ldb"
    .dcs    "// __ldb(byte_addr): the byte at a byte address (word*8 + index, index 0 = most significant)"
    .dcs    "__crt___ldb:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    ldb     r0, r1"
    .dcs    "    return"
    .dcs    "@@__stb"
    .dcs    "// __stb(byte_addr, v): stores the low 8 bits of v at a byte address"
    .dcs    "__crt___stb:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    load    r2, sf[4]"
    .dcs    "    stb     r2, r1"
    .dcs    "    return"
    .dci    -1

RUNTIME_EMIT_ASM_END:
    nop
