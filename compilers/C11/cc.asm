///////////////////////////////////////////////////////////////////////////////
// cc.asm
//
// CPUSim64 C compiler - main program.
//
// Usage:   run.sh cc input.c [more.c ...] [-lstd] [--pp | --tokens | --ast] [--lib=DIR]
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
//   -lstd     link the C library as whole modules (obj/lib/<name>.asm)
// With several input files or -lstd every file becomes a relocatable module
// (see cc_main); assemble.sh -r first.asm -o obj/first.o64 links them.
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
#global CC_FILES:   .dci 0          // vec of packed input file paths
#global CC_LSTD:    .dci 0          // 1: -lstd (link the C library as whole modules)
#global CC_MODULE:  .dci 0          // MOD_NONE, MOD_PROGRAM or MOD_LIBRARY
#global CC_FILENO:  .dci 0          // index (1-based) of the program file being compiled
#global CC_LIBQ:    .dci 0          // vec of packed library .c paths to compile as modules
#global CC_OUTLIB:  .dci 0          // packed directory for library modules ("dir/obj/lib/")
#global CC_HASMAIN: .dci 0          // 1 once a module defines main
#global CC_EXPORTS: .dci 0          // vec of packed labels exported by the modules
#global CC_IMPORTS: .dci 0          // vec of packed labels imported by the modules
#global CC_IMPNAMES: .dci 0         // vec of packed C names of CC_IMPORTS

#define MODE_COMPILE    0
#define MODE_PP         1
#define MODE_TOKENS     2
#define MODE_AST        3

#define MOD_NONE        0       // one program, library appended (no modules)
#define MOD_PROGRAM     1       // module of a program file
#define MOD_LIBRARY     2       // module of a C library source (-lstd)

    #call   cc_main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// cc_main() -> exit code
//
// One input file without -lstd is compiled as one program (the C library
// functions it uses are appended).  Several input files, or -lstd, select
// module mode: every file is compiled into its own relocatable module
// (x.asm with #export / #extern), library sources needed with -lstd are
// compiled whole into obj/lib/<name>.asm, and the first file's module
// #includes the others, so that "assemble.sh -r first.asm -o obj/first.o64"
// links the program.
///////////////////////////////////////////////////////////////////////////////
#def_func cc_main()
    #var    argc, opt, i, cnt, path
    int     iARGC
    move    argc, r0
    #call   vec_new()
    store   r0, CC_FILES
    #call   vec_new()
    store   r0, CC_LIBQ
    #call   vec_new()
    store   r0, CC_EXPORTS
    #call   vec_new()
    store   r0, CC_IMPORTS
    #call   vec_new()
    store   r0, CC_IMPNAMES
    #for    1, i < argc, 1
        #call   args(i)
        move    opt, r0
        #call   p_first_char(opt)
        #if_cond r0 == '-'
            #call   parse_option(opt)
            #if_cond r0 == 0
                #call   cc_fatal2("unknown option", opt)
            #end_cond
        #else_cond
            load    r1, CC_FILES
            #call   vec_push(r1, opt)
        #end_cond
    #end_for
    load    r1, CC_FILES
    #call   vec_len(r1)
    move    cnt, r0
    #if_cond cnt == 0
        #call   fprintf(STDERR, "usage: cc input.c [more.c ...] [-lstd] [--pp|--tokens|--ast] [--lib=DIR]\n")
        #return 2
    #end_cond
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
    // banner (compile mode only: --pp, --tokens and --ast print machine-readable output)
    load    r0, CC_MODE
    #if_cond r0 == MODE_COMPILE
        #call   putline("================================================================================")
        #call   putline("CPUSim64 C11 Compiler 1.0")
        #call   putline("©2026 Richard Lesh")
        #call   putline("Compiles .c source files into CPUSim64 .asm files.")
        #call   putline("================================================================================")
        #call   puts("Arguments:")
        #for    1, i < argc, 1
            #call   args(i)
            move    opt, r0
            #call   printf(" %s", opt)
        #end_for
        #call   putline("")
    #end_cond
    load    r0, CC_LSTD
    #if_cond cnt == 1
        #if_cond r0 == 0
            load    r1, CC_FILES
            #macro  VGET(path, r1, 1)
            #call   compile_file(path)
            #return r0
        #end_cond
    #end_cond
    // ---- module mode
    load    r0, CC_MODE
    #if_cond r0 != MODE_COMPILE
        #call   cc_fatal("--pp, --tokens and --ast need a single input file without -lstd")
    #end_cond
    move    r0, MOD_PROGRAM
    store   r0, CC_MODULE
    load    r1, CC_FILES
    #macro  VGET(path, r1, 1)
    #call   path_dir(path)
    move    opt, r0
    #call   vec_new()
    move    i, r0
    #call   sb_add_p(i, opt)
    #call   sb_add_p(i, "obj/lib/")
    #call   sb_to_p(i)
    store   r0, CC_OUTLIB
    load    r0, CC_LSTD
    #if_cond r0 != 0
        load    r1, CC_OUTLIB
        #call   path_parent(r1)
        #call   makeDirectory(r0)           // dir/obj/
        load    r1, CC_OUTLIB
        #call   makeDirectory(r1)           // dir/obj/lib/
    #end_cond
    #for    1, i <= cnt, 1
        load    r1, CC_FILES
        #macro  VGET(path, r1, i)
        store   i, CC_FILENO
        #call   compile_file(path)
    #end_for
    // ---- library modules (-lstd); compiling one may queue more
    move    r0, MOD_LIBRARY
    store   r0, CC_MODULE
    move    r0, 0
    store   r0, CC_FILENO
    move    i, 1
    load    r1, CC_LIBQ
    #call   vec_len(r1)
    #while  i <= r0
        load    r1, CC_LIBQ
        #macro  VGET(path, r1, i)
        #call   compile_file(path)
        add     i, 1
        load    r1, CC_LIBQ
        #call   vec_len(r1)
    #end_while
    load    r0, CC_HASMAIN
    #if_cond r0 == 0
        #call   cc_fatal("no 'main' function")
    #end_cond
    #call   check_imports()
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// lib_module_use_all(): a library module contains the whole library source,
//      so every function body (recorded lazily) and every object is marked used
///////////////////////////////////////////////////////////////////////////////
#def_func lib_module_use_all()
    #var    v, cnt, i, s
    load    v, FUNCS
    #macro  VLEN(cnt, v)
    #for    1, i <= cnt, 1
        #macro  VGET(s, v, i)
        load    r0, s[S_FLAGS]
        and     r1, r0, SF_LAZY
        #if_cond r1 != 0
            or      r0, SF_USED
            store   r0, s[S_FLAGS]
        #end_cond
    #end_for
    load    v, GLOBALS
    #macro  VLEN(cnt, v)
    #for    1, i <= cnt, 1
        #macro  VGET(s, v, i)
        load    r0, s[S_FLAGS]
        or      r0, SF_USED
        store   r0, s[S_FLAGS]
    #end_for
#end_func

///////////////////////////////////////////////////////////////////////////////
// defines_main() -> 1 if the translation unit defines main
///////////////////////////////////////////////////////////////////////////////
#def_func defines_main()
    #call   intern_p("main")
    load    r0, r0[E_GLOBAL]
    #if_cond r0 == 0
        #return 0
    #end_cond
    load    r0, r0[S_FLAGS]
    and     r0, SF_DEFINED
    #if_cond r0 == 0
        #return 0
    #end_cond
    #return 1
#end_func

///////////////////////////////////////////////////////////////////////////////
// p_first_char(packed) -> first character of a packed string (0 if empty)
///////////////////////////////////////////////////////////////////////////////
#def_func p_first_char(sp_)
    #var    w, c
    load    w, sp_
    #call   w_from_p(w)
    move    w, r0
    move    c, 0
    load    r0, w[0]
    #if_cond r0 > 0
        load    c, w[1]
    #end_cond
    move    r1, w
    int     iFREE
    #return c
#end_func

///////////////////////////////////////////////////////////////////////////////
// compile_file(path) -> exit code: compiles one translation unit, starting
//      from a fresh compiler state (so every module is compiled on its own)
///////////////////////////////////////////////////////////////////////////////
#def_func compile_file(pathp)
    #var    path, src, ptoks, mtoks, toks, mode
    load    path, pathp
    store   path, CC_INPATH
    #call   intern_init()
    #call   intern_p(path)
    store   r0, ERR_FILE
    // a library module is compiled like the appended library (the same labels
    // and builtin declarations), then everything in it is compiled (see below)
    load    r1, CC_MODULE
    #if_cond r1 == MOD_LIBRARY
        #call   mark_lib_file(r0)
    #end_cond
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
    load    r0, CC_MODULE
    #if_cond r0 == MOD_LIBRARY
        #call   lib_module_use_all()
    #end_cond
    #call   defines_main()
    #if_cond r0 != 0
        store   r0, CC_HASMAIN
    #else_cond
        load    r0, CC_MODULE
        #if_cond r0 == MOD_NONE
            #call   cc_fatal("no 'main' function")
        #end_cond
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
    #macro  STRCMP(opt, "-lstd")
    #if_cond r0 == 0
        move    r1, 1
        store   r1, CC_LSTD
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
