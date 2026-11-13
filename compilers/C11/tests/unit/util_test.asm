///////////////////////////////////////////////////////////////////////////////
// util_test.asm - unit tests for util.asm (vectors, string builder, intern)
///////////////////////////////////////////////////////////////////////////////
#include <system/io.asm>
#include <system/string.asm>
#include <system/system.asm>
#include "../../defs.def"

#global FAILS: .dci 0

    #call   test_main()
    #call   exit(r0)

#def_func check(cond, msg)
    #var    c
    load    c, cond
    #if_cond c == 0
        load    r1, msg
        #call   printf("FAIL %s\n", r1)
        load    r0, FAILS
        add     r0, 1
        store   r0, FAILS
    #end_cond
#end_func

#def_func test_main()
    #var    v, i, x, e1, e2, e3, s, w, cnt
    #call   intern_init()

    // ---- vector push / grow / get / pop
    #call   vec_new()
    move    v, r0
    #for    1, i <= 1000, 1
        mult    x, i, 3
        #call   vec_push(v, x)
    #end_for
    #call   vec_len(v)
    #macro  COMPARE(r0, eq, 1000)
    #call   check(r0, "vec_len after 1000 pushes")
    #call   vec_get(v, 500)
    #macro  COMPARE(r0, eq, 1500)
    #call   check(r0, "vec_get(500)")
    #macro  VGET(x, v, 1000)
    #macro  COMPARE(x, eq, 3000)
    #call   check(r0, "VGET macro")
    #call   vec_pop(v)
    #macro  COMPARE(r0, eq, 3000)
    #call   check(r0, "vec_pop value")
    #macro  VLEN(x, v)
    #macro  COMPARE(x, eq, 999)
    #call   check(r0, "VLEN after pop")
    #call   vec_last(v)
    #macro  COMPARE(r0, eq, 2997)
    #call   check(r0, "vec_last")

    // ---- string builder
    #call   vec_new()
    move    s, r0
    #call   sb_add_p(s, "abc")
    #call   sb_add_int(s, -42)
    #call   vec_push(s, 'Z')
    #call   sb_to_p(s)
    move    w, r0
    #macro  STRCMP(w, "abc-42Z")
    #macro  COMPARE(r0, eq, 0)
    #call   check(r0, "string builder")
    #call   w_from_p("h\u{e9}llo")
    load    cnt, r0[0]
    #macro  COMPARE(cnt, eq, 5)
    #call   check(r0, "w_from_p unicode length")

    // ---- intern
    #call   intern_p("hello")
    move    e1, r0
    #call   intern_p("hello")
    move    e2, r0
    #call   intern_p("world")
    move    e3, r0
    #macro  COMPARE(e1, eq, e2)
    #call   check(r0, "intern same pointer")
    #macro  COMPARE(e1, ne, e3)
    #call   check(r0, "intern different pointer")
    #call   ent_p(e3)
    #macro  STRCMP(r0, "world")
    #macro  COMPARE(r0, eq, 0)
    #call   check(r0, "ent_p round trip")
    // intern from a sub-range of a codepoint array
    #call   w_from_p("xxhelloyy")
    #call   intern(r0, 3, 5)
    #macro  COMPARE(r0, eq, e1)
    #call   check(r0, "intern sub-range")
    // many entries
    #for    1, i <= 300, 1
        #call   vec_new()
        move    s, r0
        #call   sb_add_p(s, "id")
        #call   sb_add_int(s, i)
        load    r1, s[VEC_ARR]
        #call   intern_w(r1)
    #end_for
    #call   vec_new()
    move    s, r0
    #call   sb_add_p(s, "id")
    #call   sb_add_int(s, 123)
    load    r1, s[VEC_ARR]
    #call   intern_w(r1)
    move    x, r0
    #call   intern_p("id123")
    #macro  COMPARE(r0, eq, x)
    #call   check(r0, "intern many")

    // ---- paths
    #call   path_out("dir/sub/prog.c")
    #macro  STRCMP(r0, "dir/sub/prog.asm")
    #macro  COMPARE(r0, eq, 0)
    #call   check(r0, "path_out")
    #call   path_dir("dir/sub/prog.c")
    #macro  STRCMP(r0, "dir/sub/")
    #macro  COMPARE(r0, eq, 0)
    #call   check(r0, "path_dir")
    #call   path_dir("prog.c")
    #macro  STRCMP(r0, "")
    #macro  COMPARE(r0, eq, 0)
    #call   check(r0, "path_dir no dir")
    #call   path_join("dir/", "x.h")
    #macro  STRCMP(r0, "dir/x.h")
    #macro  COMPARE(r0, eq, 0)
    #call   check(r0, "path_join")
    #call   path_base("a/b/prog.c")
    #macro  STRCMP(r0, "prog")
    #macro  COMPARE(r0, eq, 0)
    #call   check(r0, "path_base")

    load    r0, FAILS
    #if_cond r0 == 0
        #call   printf("ALL PASS\n")
    #end_cond
    #return 0
#end_func

#include "../../util.asm"
#include "../../errors.asm"
#include "../../source.asm"
    stop
    stop
