///////////////////////////////////////////////////////////////////////////////
// errors.asm
//
// Fail-on-first-error reporting.  Every error prints
//     file:line: error: message
// to STDERR and terminates the compiler with exit code 1.
///////////////////////////////////////////////////////////////////////////////

#global ERR_FILE:   .dci 0          // intern entry of the current main file
#global OUT_PORT:   .dci 0          // output port while generating code
#global OUT_PATH:   .dci 0          // output file path (packed)

ERRORS_ASM_START:
    jump    ERRORS_ASM_END

///////////////////////////////////////////////////////////////////////////////
// cc_die(): remove a partially written output file and exit with status 1
///////////////////////////////////////////////////////////////////////////////
#def_func cc_die()
    load    r1, OUT_PORT
    #if_cond r1 != 0
        #call   closeFile(r1)
        move    r0, 0
        store   r0, OUT_PORT
        load    r1, OUT_PATH
        #call   deleteFile(r1)
    #end_cond
    #call   exit(1)
#end_func

///////////////////////////////////////////////////////////////////////////////
// cc_fatal(msg): error without a source location
///////////////////////////////////////////////////////////////////////////////
#def_func cc_fatal(msg)
    load    r1, msg
    #call   fprintf(STDERR, "cc: error: %s\n", r1)
    #call   cc_die()
#end_func

///////////////////////////////////////////////////////////////////////////////
// cc_fatal2(msg, detail): "cc: error: msg: detail"
///////////////////////////////////////////////////////////////////////////////
#def_func cc_fatal2(msg, detail)
    load    r1, msg
    load    r2, detail
    #call   fprintf(STDERR, "cc: error: %s: %s\n", r1, r2)
    #call   cc_die()
#end_func

///////////////////////////////////////////////////////////////////////////////
// error_at(file_entry, line, msg)
///////////////////////////////////////////////////////////////////////////////
#def_func error_at(fe, ln, msg)
    #var    f, l, m
    load    f, fe
    load    l, ln
    load    m, msg
    #if_cond f == 0
        load    f, ERR_FILE
    #end_cond
    #call   ent_p(f)
    #call   fprintf(STDERR, "%s:%d: error: %s\n", r0, l, m)
    #call   cc_die()
#end_func

///////////////////////////////////////////////////////////////////////////////
// error_at2(file_entry, line, msg, name_entry): "...: msg 'name'"
///////////////////////////////////////////////////////////////////////////////
#def_func error_at2(fe, ln, msg, nme)
    #var    f, l, m, cnt
    load    f, fe
    load    l, ln
    load    m, msg
    #if_cond f == 0
        load    f, ERR_FILE
    #end_cond
    load    r1, nme
    #call   ent_p(r1)
    move    cnt, r0
    #call   ent_p(f)
    #call   fprintf(STDERR, "%s:%d: error: %s '%s'\n", r0, l, m, cnt)
    #call   cc_die()
#end_func

///////////////////////////////////////////////////////////////////////////////
// error_tok(tok, msg)
///////////////////////////////////////////////////////////////////////////////
#def_func error_tok(tk, msg)
    #var    t
    load    t, tk
    load    r1, t[TK_FILE]
    load    r2, t[TK_LINE]
    load    r3, msg
    #call   error_at(r1, r2, r3)
#end_func

///////////////////////////////////////////////////////////////////////////////
// error_tok2(tok, msg, name_entry)
///////////////////////////////////////////////////////////////////////////////
#def_func error_tok2(tk, msg, nme)
    #var    t, cnt
    load    t, tk
    load    cnt, nme
    load    r1, t[TK_FILE]
    load    r2, t[TK_LINE]
    load    r3, msg
    #call   error_at2(r1, r2, r3, cnt)
#end_func

///////////////////////////////////////////////////////////////////////////////
// error_node(node, msg)
///////////////////////////////////////////////////////////////////////////////
#def_func error_node(nd, msg)
    #var    cnt
    load    cnt, nd
    load    r1, cnt[N_FILE]
    load    r2, cnt[N_LINE]
    load    r3, msg
    #call   error_at(r1, r2, r3)
#end_func

ERRORS_ASM_END:
    nop
