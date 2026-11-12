///////////////////////////////////////////////////////////////////////////////
// cc.asm
//
// CPUSim64 C compiler - main program.
//
// Usage:   run.sh cc input.c [--pp | --tokens | --ast]
//
// Reads the C source file named by the first command line argument and
// writes CPUSim64 assembly to input.asm next to it.  The optional second
// argument selects a debugging dump instead of code generation:
//   --pp      print the preprocessed token stream
//   --tokens  print the token list
//   --ast     print the abstract syntax tree
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
    #var    argc, path, src, opt, ptoks, mtoks, toks, mode
    int     iARGC
    move    argc, r0
    #if_cond argc < 2
        #call   fprintf(STDERR, "usage: cc input.c [--pp|--tokens|--ast]\n")
        #return 2
    #end_cond
    #call   args(1)
    move    path, r0
    store   path, CC_INPATH
    #if_cond argc >= 3
        #call   args(2)
        move    opt, r0
        #macro  STRCMP(opt, "--pp")
        #if_cond r0 == 0
            move    r1, MODE_PP
            store   r1, CC_MODE
        #end_cond
        #macro  STRCMP(opt, "--tokens")
        #if_cond r0 == 0
            move    r1, MODE_TOKENS
            store   r1, CC_MODE
        #end_cond
        #macro  STRCMP(opt, "--ast")
        #if_cond r0 == 0
            move    r1, MODE_AST
            store   r1, CC_MODE
        #end_cond
        load    r1, CC_MODE
        #if_cond r1 == MODE_COMPILE
            #call   cc_fatal2("unknown option", opt)
        #end_cond
    #end_cond
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
// strip_prelude(toks) -> vec of the tokens that do not come from the prelude
///////////////////////////////////////////////////////////////////////////////
#def_func strip_prelude(toksp)
    #var    toks, outv, cnt, i, t, pf
    load    toks, toksp
    load    pf, CC_PRELUDE
    #call   vec_new()
    move    outv, r0
    #macro  VLEN(cnt, toks)
    #for    1, i <= cnt, 1
        #macro  VGET(t, toks, i)
        load    r0, t[TK_FILE]
        #if_cond r0 != pf
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
