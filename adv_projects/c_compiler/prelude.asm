///////////////////////////////////////////////////////////////////////////////
// prelude.asm
//
// Built-in C source preprocessed before every translation unit.  It defines
// only the predefined macros (__CPUSIM64__, __STDC__, ...); all types,
// macros and library declarations live in the headers in lib/.
// Tokens from the prelude carry the file name "<built-in>".
//
//   prelude_source() -> wstr of the prelude text
///////////////////////////////////////////////////////////////////////////////

PRELUDE_ASM_START:
    jump    PRELUDE_ASM_END

// dcs_lines_to_w(addr) -> wstr joining .dcs lines (terminated by .dci -1)
#def_func dcs_lines_to_w(ap)
    #var    a, sb
    load    a, ap
    #call   vec_new()
    move    sb, r0
    #while  TRUE
        load    r0, a[0]
        #if_cond r0 < 0
            #break
        #end_cond
        #call   sb_add_p(sb, a)
        #call   vec_push(sb, '\n')
        #call   dcs_next(a)
        move    a, r0
    #end_while
    #call   sb_to_w(sb)
#end_func

#def_func prelude_source()
    #call   dcs_lines_to_w(PRELUDE_SRC)
#end_func

PRELUDE_SRC:
    .dcs    "#define __CPUSIM64__ 1"
    .dcs    "#define __STDC__ 1"
    .dcs    "#define __STDC_VERSION__ 201112L"
    .dcs    "#define __STDC_HOSTED__ 1"
    .dci    -1

PRELUDE_ASM_END:
    nop
