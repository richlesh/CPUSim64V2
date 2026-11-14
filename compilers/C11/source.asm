///////////////////////////////////////////////////////////////////////////////
// source.asm
//
// Source file loading and path helpers.
//   read_file(path)        -> wstr of the file's codepoints (or 0)
//   path_dir(path)         -> packed directory prefix ("" or "dir/")
//   path_join(dir, name)   -> packed dir+name
//   path_out(path)         -> packed output path (x.c -> x.asm)
///////////////////////////////////////////////////////////////////////////////

SOURCE_ASM_START:
    jump    SOURCE_ASM_END

///////////////////////////////////////////////////////////////////////////////
// read_file(path) -> wstr with a guaranteed trailing newline, or 0 on failure
///////////////////////////////////////////////////////////////////////////////
#def_func read_file(path)
    #var    port, v, c, a, cnt
    load    r1, path
    #call   fileExists(r1)
    #if_cond r0 == 0
        #return 0
    #end_cond
    load    r1, path
    #call   isFile(r1)
    #if_cond r0 == 0
        #return 0
    #end_cond
    load    r1, path
    #call   openTextFile(r1, READ_MODE)
    move    port, r0
    #if_cond port == -1
        #return 0
    #end_cond
    #call   vec_new()
    move    v, r0
    #macro  IN0(c, port)
    #while  c != -1
        #call   vec_push(v, c)
        #macro  IN0(c, port)
    #end_while
    #call   closeFile(port)
    load    a, v[VEC_ARR]
    load    cnt, a[0]
    move    c, 0
    #if_cond cnt > 0
        load    c, a[cnt]
    #end_cond
    #if_cond c != '\n'
        #call   vec_push(v, '\n')
    #end_cond
    load    a, v[VEC_ARR]
    move    r1, v
    int     iFREE
    #return a
#end_func

///////////////////////////////////////////////////////////////////////////////
// path_dir(path) -> packed directory prefix including the trailing '/'
///////////////////////////////////////////////////////////////////////////////
#def_func path_dir(path)
    #var    w, cnt, i, c, r
    load    r1, path
    int     iGET_CODEPOINTS
    move    w, r0
    load    cnt, w[0]
    move    i, cnt
    #while  i > 0
        load    c, w[i]
        #if_cond c == '/'
            #break
        #end_cond
        sub     i, 1
    #end_while
    store   i, w[0]
    move    r1, w
    int     iFROM_CODEPOINTS
    move    r, r0
    move    r1, w
    int     iFREE
    #return r
#end_func

///////////////////////////////////////////////////////////////////////////////
// display_path(path) -> packed path for messages: an absolute path relative to
//      the working directory (environment variable PWD) when that is shorter,
//      otherwise the path as given
///////////////////////////////////////////////////////////////////////////////
#def_func display_path(path)
    #var    w, c, wl, cl, m, j, ups, k, sb, r, ch, st
    load    r1, path
    int     iGET_CODEPOINTS
    move    w, r0
    load    wl, w[0]
    move    r, 0
    #if_cond wl < 1
        jump    DP_DONE
    #end_cond
    load    ch, w[1]
    #if_cond ch != '/'
        jump    DP_DONE
    #end_cond
    move    r1, "PWD"
    int     iENV
    #if_cond r0 == 0
        jump    DP_DONE
    #end_cond
    move    r1, r0
    int     iGET_CODEPOINTS
    move    c, r0
    load    cl, c[0]
    // strip a trailing '/' (but keep "/")
    #if_cond cl > 1
        load    ch, c[cl]
        #if_cond ch == '/'
            sub     cl, 1
        #end_cond
    #end_cond
    #if_cond cl < 1
        jump    DP_FREE_C                   // empty: keep the path
    #end_cond
    // m = length of the common prefix
    move    m, 0
    #while  m < cl
        #if_cond m >= wl
            #break
        #end_cond
        add     r1, m, 1
        load    r2, w[r1]
        load    r3, c[r1]
        #if_cond r2 != r3
            #break
        #end_cond
        add     m, 1
    #end_while
    // j = end of the common directory part; st = index in w where the rest starts
    move    j, -1
    #if_cond m == cl
        #if_cond m == wl
            move    j, cl
            add     st, wl, 1
        #else_cond
            add     r1, m, 1
            load    r2, w[r1]
            #if_cond r2 == '/'
                move    j, cl
                add     st, cl, 2           // skip the '/' after the working directory
            #end_cond
        #end_cond
    #end_cond
    #if_cond j < 0
        // back up to the last '/' of the common prefix
        move    j, m
        #while  j > 1
            load    r2, c[j]
            #if_cond r2 == '/'
                #break
            #end_cond
            sub     j, 1
        #end_while
        add     st, j, 1
    #end_cond
    // ups = number of cwd directories below the common part
    move    ups, 0
    #if_cond j < cl
        move    ups, 1
        add     k, j, 1
        #while  k <= cl
            load    r2, c[k]
            #if_cond r2 == '/'
                add     ups, 1
            #end_cond
            add     k, 1
        #end_while
    #end_cond
    #call   vec_new()
    move    sb, r0
    #while  ups > 0
        #call   sb_add_p(sb, "../")
        sub     ups, 1
    #end_while
    move    k, st
    #while  k <= wl
        load    r1, w[k]
        #call   vec_push(sb, r1)
        add     k, 1
    #end_while
    #call   vec_len(sb)
    #if_cond r0 == 0
        #call   sb_add_p(sb, ".")
    #end_cond
    #call   vec_len(sb)
    #if_cond r0 < wl
        #call   sb_to_p(sb)
        move    r, r0
    #end_cond
DP_FREE_C:
    move    r1, c
    int     iFREE
DP_DONE:
    move    r1, w
    int     iFREE
    #if_cond r == 0
        load    r, path
    #end_cond
    #return r
#end_func

///////////////////////////////////////////////////////////////////////////////
// path_parent(dir) -> packed parent of a directory prefix ("a/b/" -> "a/",
//                     "b/" -> "", "./obj/" -> "./")
///////////////////////////////////////////////////////////////////////////////
#def_func path_parent(dirp)
    #var    w, cnt, c, r
    load    r1, dirp
    int     iGET_CODEPOINTS
    move    w, r0
    load    cnt, w[0]
    #if_cond cnt > 0
        load    c, w[cnt]
        #if_cond c == '/'
            sub     cnt, 1
            store   cnt, w[0]
        #end_cond
    #end_cond
    move    r1, w
    int     iFROM_CODEPOINTS
    move    r, r0
    move    r1, w
    int     iFREE
    #call   path_dir(r)
#end_func

///////////////////////////////////////////////////////////////////////////////
// path_join(dir, name) -> packed dir + name (name returned as-is if absolute)
///////////////////////////////////////////////////////////////////////////////
#def_func path_join(dirp, namep)
    #var    v, nm, c
    load    nm, namep
    #call   w_from_p(nm)
    load    c, r0[1]
    move    r1, r0
    int     iFREE
    #if_cond c == '/'
        #call   vec_new()
        move    v, r0
        #call   sb_add_p(v, nm)
        #call   sb_to_p(v)
        #return r0
    #end_cond
    #call   vec_new()
    move    v, r0
    load    r1, dirp
    #call   sb_add_p(v, r1)
    #call   sb_add_p(v, nm)
    #call   sb_to_p(v)
#end_func

///////////////////////////////////////////////////////////////////////////////
// path_out(path) -> packed output path: strip a trailing ".c", add ".asm"
///////////////////////////////////////////////////////////////////////////////
#def_func path_out(path)
    #var    w, cnt, c, v
    load    r1, path
    int     iGET_CODEPOINTS
    move    w, r0
    load    cnt, w[0]
    #if_cond cnt >= 2
        load    c, w[cnt]
        #if_cond c == 'c'
            sub     r1, cnt, 1
            load    c, w[r1]
            #if_cond c == '.'
                sub     cnt, 2
                store   cnt, w[0]
            #end_cond
        #end_cond
    #end_cond
    #call   vec_new()
    move    v, r0
    #call   sb_add_w(v, w)
    #call   sb_add_p(v, ".asm")
    move    r1, w
    int     iFREE
    #call   sb_to_p(v)
#end_func

///////////////////////////////////////////////////////////////////////////////
// path_base(path) -> packed base name without directory and ".c" extension
///////////////////////////////////////////////////////////////////////////////
#def_func path_base(path)
    #var    w, cnt, i, c, v, s
    load    r1, path
    int     iGET_CODEPOINTS
    move    w, r0
    load    cnt, w[0]
    #if_cond cnt >= 2
        load    c, w[cnt]
        #if_cond c == 'c'
            sub     r1, cnt, 1
            load    c, w[r1]
            #if_cond c == '.'
                sub     cnt, 2
            #end_cond
        #end_cond
    #end_cond
    move    i, cnt
    #while  i > 0
        load    c, w[i]
        #if_cond c == '/'
            #break
        #end_cond
        sub     i, 1
    #end_while
    #call   vec_new()
    move    v, r0
    add     s, i, 1
    #while  s <= cnt
        load    c, w[s]
        #call   vec_push(v, c)
        add     s, 1
    #end_while
    move    r1, w
    int     iFREE
    #call   sb_to_p(v)
#end_func

SOURCE_ASM_END:
    nop
