///////////////////////////////////////////////////////////////////////////////
// cc.asm
//
// CPUSim64 C compiler - main program.
//
// Usage:   run.sh cc input.c [--pp | --tokens | --ast] [--lib=DIR]
//
// Reads the C source file named by the first command line argument and
// writes CPUSim64 assembly to input.asm next to it.  The optional second
// argument selects a debugging dump instead of code generation:
//   --pp      print the preprocessed token stream
//   --tokens  print the token list
//   --ast     print the abstract syntax tree
//   --lib=DIR additional library directory for <name.h> headers, searched
//             before the standard lib/ directory next to the compiler's
//             obj/ directory (may be repeated)
//
// CPUSim64 has no linker, so this file includes every compiler module.
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/string.asm>
#include <system/system.asm>
#include "defs.def"

#global CC_MODE:    .dci 0          // 0 = compile, 1 = pp, 2 = tokens, 3 = ast
#global CC_INPATH:  .dci 0          // packed input path
#global CC_INDIR:   .dci 0          // packed input directory prefix
#global CC_PRELUDE: .dci 0          // intern entry "<built-in>"
#global CC_LIBDIRS: .dci 0          // vec of packed library directories ("dir/")

#define MODE_COMPILE    0
#define MODE_PP         1
#define MODE_TOKENS     2
#define MODE_AST        3

    #call   cc_main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// cc_main() -> exit code
///////////////////////////////////////////////////////////////////////////////
#def_func cc_main()
    #var    argc, path, src, opt, ptoks, mtoks, toks, mode, i
    int     iARGC
    move    argc, r0
    #if_cond argc < 2
        #call   fprintf(STDERR, "usage: cc input.c [--pp|--tokens|--ast] [--lib=DIR]\n")
        #return 2
    #end_cond
    #call   args(1)
    move    path, r0
    store   path, CC_INPATH
    #for    2, i < argc, 1
        #call   args(i)
        move    opt, r0
        #call   parse_option(opt)
        #if_cond r0 == 0
            #call   cc_fatal2("unknown option", opt)
        #end_cond
    #end_for
    // --lib directories are searched first, then the standard library
    #call   find_libdir()
    move    r1, r0
    load    r0, CC_LIBDIRS
    #if_cond r0 == 0
        push    r1
        #call   vec_new()
        store   r0, CC_LIBDIRS
        pop     r1
    #end_cond
    load    r0, CC_LIBDIRS
    #call   vec_push(r0, r1)
    #call   intern_init()
    #call   intern_p(path)
    store   r0, ERR_FILE
    #call   path_dir(path)
    store   r0, CC_INDIR
    #call   read_file(path)
    move    src, r0
    #if_cond src == 0
        #call   cc_fatal2("cannot open file", path)
    #end_cond
    #call   lex_init()
    #call   pp_init()
    // tokenize the prelude and the main file
    #call   intern_p("<built-in>")
    store   r0, CC_PRELUDE
    #call   mark_lib_file(r0)
    #call   prelude_source()
    load    r1, CC_PRELUDE
    #call   lex_file(r0, r1)
    move    ptoks, r0
    load    r1, ERR_FILE
    #call   lex_file(src, r1)
    move    mtoks, r0
    #call   pp_run(ptoks, mtoks)
    move    toks, r0
    load    mode, CC_MODE
    #if_cond mode == MODE_PP
        #call   strip_prelude(toks)
        #call   dump_tokens(r0, 1)
        #return 0
    #end_cond
    #if_cond mode == MODE_TOKENS
        #call   strip_prelude(toks)
        #call   dump_tokens(r0, 2)
        #return 0
    #end_cond
    // parse (and generate code for each function as it is parsed)
    #call   types_init()
    #call   ast_init()
    #call   symtab_init()
    #call   decl_init()
    #call   parser_init(toks)
    #if_cond mode == MODE_AST
        #call   parse_tu()
        #return 0
    #end_cond
    #call   cg_begin(path)
    #call   parse_tu()
    #call   intern_p("main")
    load    r0, r0[E_GLOBAL]
    #if_cond r0 == 0
        #call   cc_fatal("no 'main' function")
    #end_cond
    load    r0, r0[S_FLAGS]
    and     r0, SF_DEFINED
    #if_cond r0 == 0
        #call   cc_fatal("no 'main' function")
    #end_cond
    #call   compile_lazy()
    #call   cg_end()
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// parse_option(opt) -> 1 if opt is a recognized option (and applies it)
///////////////////////////////////////////////////////////////////////////////
#def_func parse_option(optp)
    #var    opt, w, cnt, sb, i, c
    load    opt, optp
    #macro  STRCMP(opt, "--pp")
    #if_cond r0 == 0
        move    r1, MODE_PP
        store   r1, CC_MODE
        #return 1
    #end_cond
    #macro  STRCMP(opt, "--tokens")
    #if_cond r0 == 0
        move    r1, MODE_TOKENS
        store   r1, CC_MODE
        #return 1
    #end_cond
    #macro  STRCMP(opt, "--ast")
    #if_cond r0 == 0
        move    r1, MODE_AST
        store   r1, CC_MODE
        #return 1
    #end_cond
    // --lib=DIR
    #call   w_from_p(opt)
    move    w, r0
    load    cnt, w[0]
    #if_cond cnt < 7
        #return 0
    #end_cond
    #call   vec_new()
    move    sb, r0
    #for    1, i <= 6, 1
        load    c, w[i]
        #call   vec_push(sb, c)
    #end_for
    #call   sb_to_p(sb)
    #macro  STRCMP(r0, "--lib=")
    #if_cond r0 != 0
        #return 0
    #end_cond
    #call   vec_new()
    move    sb, r0
    #for    7, i <= cnt, 1
        load    c, w[i]
        #call   vec_push(sb, c)
    #end_for
    #if_cond c != '/'
        #call   vec_push(sb, '/')
    #end_cond
    #call   sb_to_p(sb)
    move    w, r0
    load    r0, CC_LIBDIRS
    #if_cond r0 == 0
        #call   vec_new()
        store   r0, CC_LIBDIRS
    #end_cond
    load    r0, CC_LIBDIRS
    #call   vec_push(r0, w)
    #return 1
#end_func

///////////////////////////////////////////////////////////////////////////////
// find_libdir() -> packed library directory: the lib/ directory next to the
//      directory holding the compiler object file (args(0) is
//      ".../obj/cc.o64"), else "lib/" relative to the current directory
///////////////////////////////////////////////////////////////////////////////
#def_func find_libdir()
    #var    d, sb
    #call   args(0)
    #if_cond r0 != 0
        #call   path_dir(r0)
        #call   path_parent(r0)
        move    d, r0
        #call   vec_new()
        move    sb, r0
        #call   sb_add_p(sb, d)
        #call   sb_add_p(sb, "lib/")
        #call   sb_to_p(sb)
        move    d, r0
        #call   isDirectory(d)
        #if_cond r0 != 0
            #return d
        #end_cond
    #end_cond
    #return "lib/"
#end_func

///////////////////////////////////////////////////////////////////////////////
// strip_prelude(toks) -> vec of the tokens that do not come from the prelude
//                        or from library files
///////////////////////////////////////////////////////////////////////////////
#def_func strip_prelude(toksp)
    #var    toks, outv, cnt, i, t
    load    toks, toksp
    #call   vec_new()
    move    outv, r0
    #macro  VLEN(cnt, toks)
    #for    1, i <= cnt, 1
        #macro  VGET(t, toks, i)
        load    r1, t[TK_FILE]
        #call   is_lib_file(r1)
        #if_cond r0 == 0
            #call   vec_push(outv, t)
        #end_cond
    #end_for
    #return outv
#end_func

#include "util.asm"
#include "errors.asm"
#include "source.asm"
#include "lexer.asm"
#include "pp.asm"
#include "prelude.asm"
#include "types.asm"
#include "symtab.asm"
#include "ast.asm"
#include "sema.asm"
#include "parse_expr.asm"
#include "parse_decl.asm"
#include "parse_stmt.asm"
#include "codegen.asm"
#include "runtime_emit.asm"

    stop
    stop
