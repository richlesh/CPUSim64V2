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
    #var    recs, cnt, i, rec, a, g
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
    .dcs    "@@printf"
    .dcs    "__crt_printf:"
    .dcs    "    jump    printf"
    .dcs    "@@fprintf"
    .dcs    "__crt_fprintf:"
    .dcs    "    load    r0, sf[3]"
    .dcs    "    sub     r0, 1"
    .dcs    "    store   r0, sf[3]"
    .dcs    "    jump    fprintf"
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
    .dcs    "@@sprintf p2buf"
    .dcs    "#def_func __crt_sprintf(buf, fmt, values...)"
    .dcs    "    add     sf, 1"
    .dcs    "    int     iSPRINTF"
    .dcs    "    sub     sf, 1"
    .dcs    "    load    r1, buf"
    .dcs    "    #call   __crt_p2buf(r1, r0)"
    .dcs    "#end_func"
    .dcs    "@@snprintf"
    .dcs    "#def_func __crt_snprintf(buf, lim, fmt, values...)"
    .dcs    "    #var    ps, cp, cnt, i, c, dst, mx"
    .dcs    "    add     sf, 2"
    .dcs    "    int     iSPRINTF"
    .dcs    "    sub     sf, 2"
    .dcs    "    move    ps, r0"
    .dcs    "    move    r1, ps"
    .dcs    "    int     iGET_CODEPOINTS"
    .dcs    "    move    cp, r0"
    .dcs    "    load    cnt, cp[0]"
    .dcs    "    load    dst, buf"
    .dcs    "    load    mx, lim"
    .dcs    "    #if_cond mx > 0"
    .dcs    "        sub     mx, 1"
    .dcs    "        #if_cond mx > cnt"
    .dcs    "            move    mx, cnt"
    .dcs    "        #end_cond"
    .dcs    "        #for    1, i <= mx, 1"
    .dcs    "            load    c, cp[i]"
    .dcs    "            store   c, dst[0]"
    .dcs    "            add     dst, 1"
    .dcs    "        #end_for"
    .dcs    "        store   0, dst[0]"
    .dcs    "    #end_cond"
    .dcs    "    move    r1, cp"
    .dcs    "    int     iFREE"
    .dcs    "    move    r1, ps"
    .dcs    "    int     iFREE"
    .dcs    "    #return cnt"
    .dcs    "#end_func"
    .dcs    "@@puts"
    .dcs    "#def_func __crt_puts(s)"
    .dcs    "    load    r1, s"
    .dcs    "    #call   putline(r1)"
    .dcs    "    move    r0, 0"
    .dcs    "#end_func"
    .dcs    "@@fputs"
    .dcs    "#def_func __crt_fputs(s, fp)"
    .dcs    "    load    r1, s"
    .dcs    "    load    r2, fp"
    .dcs    "    sub     r2, 1"
    .dcs    "    #call   fputs(r2, r1)"
    .dcs    "    move    r0, 0"
    .dcs    "#end_func"
    .dcs    "@@putchar"
    .dcs    "#def_func __crt_putchar(c)"
    .dcs    "    load    r1, c"
    .dcs    "    #call   putc(r1)"
    .dcs    "    load    r0, c"
    .dcs    "#end_func"
    .dcs    "@@fputc"
    .dcs    "#def_func __crt_fputc(c, fp)"
    .dcs    "    load    r1, fp"
    .dcs    "    sub     r1, 1"
    .dcs    "    load    r2, c"
    .dcs    "    #call   fputc(r1, r2)"
    .dcs    "    load    r0, c"
    .dcs    "#end_func"
    .dcs    "@@putc fputc"
    .dcs    "__crt_putc:"
    .dcs    "    jump    __crt_fputc"
    .dcs    "@@getchar"
    .dcs    "__crt_getchar:"
    .dcs    "    in      r0, CHAR, STDIN"
    .dcs    "    return"
    .dcs    "@@fgetc"
    .dcs    "__crt_fgetc:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    sub     r1, 1"
    .dcs    "    in      r0, CHAR, r1"
    .dcs    "    return"
    .dcs    "@@getc fgetc"
    .dcs    "__crt_getc:"
    .dcs    "    jump    __crt_fgetc"
    .dcs    "@@fopen"
    .dcs    "#def_func __crt_fopen(name, mode)"
    .dcs    "    #var    m, md, nm"
    .dcs    "    load    nm, name"
    .dcs    "    load    m, mode"
    .dcs    "    load    md, m[1]"
    .dcs    "    rshift  md, 56"
    .dcs    "    and     md, 255"
    .dcs    "    #if_cond md == 'w'"
    .dcs    "        move    md, WRITE_MODE"
    .dcs    "    #else_cond"
    .dcs    "        #if_cond md == 'a'"
    .dcs    "            move    md, APPEND_MODE"
    .dcs    "        #else_cond"
    .dcs    "            #call   fileExists(nm)"
    .dcs    "            #if_cond r0 == 0"
    .dcs    "                #return 0"
    .dcs    "            #end_cond"
    .dcs    "            move    md, READ_MODE"
    .dcs    "        #end_cond"
    .dcs    "    #end_cond"
    .dcs    "    #call   openTextFile(nm, md)"
    .dcs    "    #if_cond r0 == -1"
    .dcs    "        #return 0"
    .dcs    "    #end_cond"
    .dcs    "    add     r0, 1"
    .dcs    "#end_func"
    .dcs    "@@fclose"
    .dcs    "#def_func __crt_fclose(fp)"
    .dcs    "    load    r1, fp"
    .dcs    "    sub     r1, 1"
    .dcs    "    #call   closeFile(r1)"
    .dcs    "    move    r0, 0"
    .dcs    "#end_func"
    .dcs    "@@fflush"
    .dcs    "#def_func __crt_fflush(fp)"
    .dcs    "    load    r1, fp"
    .dcs    "    sub     r1, 1"
    .dcs    "    #if_cond r1 >= 0"
    .dcs    "        #call   flush(r1)"
    .dcs    "    #end_cond"
    .dcs    "    move    r0, 0"
    .dcs    "#end_func"
    .dcs    "@@feof"
    .dcs    "__crt_feof:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    sub     r1, 1"
    .dcs    "    int     iFEOF"
    .dcs    "    return"
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
    .dcs    "@@exit"
    .dcs    "__crt_exit:"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    int     iEXIT"
    .dcs    "@@abort"
    .dcs    "__crt_abort:"
    .dcs    "    move    r1, 134"
    .dcs    "    int     iEXIT"
    .dcs    "@@rand"
    .dcs    "__crt_rand:"
    .dcs    "    move    r1, 0"
    .dcs    "    move    r2, 2147483647"
    .dcs    "    int     iRAND"
    .dcs    "    return"
    .dcs    "@@srand"
    .dcs    "__crt_srand:"
    .dcs    "    return"
    .dcs    "@@time"
    .dcs    "__crt_time:"
    .dcs    "    int     iCLOCK"
    .dcs    "    div     r0, 1000000000"
    .dcs    "    load    r1, sf[3]"
    .dcs    "    test    r1"
    .dcs    "    jump    z, __crt_time_done"
    .dcs    "    store   r0, r1[0]"
    .dcs    "__crt_time_done:"
    .dcs    "    return"
    .dcs    "@@clock"
    .dcs    "__crt_clock:"
    .dcs    "    int     iCYCLES"
    .dcs    "    return"
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
    .dcs    "@@__assert_fail"
    .dcs    "#def_func __crt___assert_fail(e, fl, ln)"
    .dcs    "    load    r1, e"
    .dcs    "    load    r2, fl"
    .dcs    "    load    r3, ln"
    .dcs    "    #call   fprintf(STDERR, \"%s:%d: Assertion `%s' failed.\\n\", r2, r3, r1)"
    .dcs    "    #call   exit(134)"
    .dcs    "#end_func"
    .dci    -1

RUNTIME_EMIT_ASM_END:
    nop
