///////////////////////////////////////////////////////////////////////////////
// codegen.asm
//
// Generates CPUSim64 assembly from the typed AST, one function at a time,
// using a simple stack-machine scheme:
//   * an expression's value is left in r0 (integers, pointers, addresses of
//     aggregates) or f0 (double);
//   * temporaries are pushed on the stack, r1-r3 (and r4/f4 briefly) are
//     scratch;
//   * calls push arguments right to left and the callee finds them at
//     SF[3], SF[4], ...  (the same convention as #def_func, so the CPUSim64
//     library functions can be called directly);  locals live at SF[0],
//     SF[-1], ...;  'return' restores SP from SF.
//   * a function returning a struct receives a hidden result pointer at
//     SF[3] and returns it in r0.
//
//   cg_begin(path)    open the output file and write the program header
//   gen_function(sym) emit one function
//   cg_end()          emit startup glue, runtime chunks, literals, globals
///////////////////////////////////////////////////////////////////////////////

#global CG_L:       .dci 0          // label counter (__L<n>)
#global CG_CONSTS:  .dci 0          // vec of 64-bit constants (__K<n>)
#global CG_STRUSE:  .dci 0          // vec: per string literal 1=word 2=packed
#global CG_BRK:     .dci 0          // vec of break labels
#global CG_CONT:    .dci 0          // vec of continue labels
#global CG_GINIT:   .dci 0          // vec of runtime global-address inits
#global CG_FUNC:    .dci 0          // function being generated
#global CG_ADDEND:  .dci 0          // addend from addr_const

// value classes
#define VC_INT      0
#define VC_DBL      1
#define VC_ADDR     2
#define VC_VOID     3

// global init record
#define GI_LABEL    0               // target global label (packed)
#define GI_OFF      1               // word offset in the target (incl. +1)
#define GI_SRC      2               // source label (packed)
#define GI_ADD      3               // source addend
#define GI_SIZE     4

CODEGEN_ASM_START:
    jump    CODEGEN_ASM_END

///////////////////////////////////////////////////////////////////////////////
// output
///////////////////////////////////////////////////////////////////////////////
// emit(fmt, values...): formatted output line(s)
#def_func emit(fmt, values...)
    int     iSPRINTF
    push    r0
    load    r1, OUT_PORT
    move    r2, r0
    int     iPUTS
    pop     r1
    int     iFREE
#end_func

// emit_p(p): output a packed string verbatim
#def_func emit_p(pstr)
    load    r1, OUT_PORT
    load    r2, pstr
    int     iPUTS
#end_func

#def_func new_lbl()
    load    r0, CG_L
    add     r0, 1
    store   r0, CG_L
#end_func

#def_func emit_label(lbl)
    load    r1, lbl
    #call   emit("__L%d:\n", r1)
#end_func

#def_func emit_jump(lbl)
    load    r1, lbl
    #call   emit("    jump    __L%d\n", r1)
#end_func

// emit_jcc(cond_pstr, lbl)
#def_func emit_jcc(cc, lbl)
    load    r1, cc
    load    r2, lbl
    #call   emit("    jump    %s, __L%d\n", r1, r2)
#end_func

#def_func add_const(val)
    load    r1, CG_CONSTS
    load    r2, val
    #call   vec_push(r1, r2)
    load    r1, CG_CONSTS
    #call   vec_len(r1)
#end_func

// fits42(v) -> 1 if v fits a 42-bit signed immediate
#def_func fits42(val)
    load    r0, val
    arshift r0, 41
    #if_cond r0 == 0
        #return 1
    #end_cond
    #if_cond r0 == -1
        #return 1
    #end_cond
    #return 0
#end_func

// emit_imm(reg_pstr, value)
#def_func emit_imm(reg, val)
    #var    v, k
    load    v, val
    #call   fits42(v)
    #if_cond r0 != 0
        load    r1, reg
        #call   emit("    move    %s, %d\n", r1, v)
        #return 0
    #end_cond
    #call   add_const(v)
    move    k, r0
    load    r1, reg
    #call   emit("    load    %s, __K%d\n", r1, k)
#end_func

///////////////////////////////////////////////////////////////////////////////
// helpers
///////////////////////////////////////////////////////////////////////////////
#def_func vclass(tp)
    #var    k
    load    r1, tp
    #call   ty_kind(r1)
    move    k, r0
    #if_cond k == TY_VOID
        #return VC_VOID
    #end_cond
    #if_cond k == TY_DOUBLE
        #return VC_DBL
    #end_cond
    #if_cond k == TY_STRUCT
        #return VC_ADDR
    #end_cond
    #if_cond k == TY_UNION
        #return VC_ADDR
    #end_cond
    #if_cond k == TY_ARRAY
        #return VC_ADDR
    #end_cond
    #if_cond k == TY_FUNC
        #return VC_ADDR
    #end_cond
    #return VC_INT
#end_func

// sym_mem(sym) -> packed memory operand for a variable ("sf[-2]", "__G1_x+1")
#def_func sym_mem(sp_)
    #var    s, m
    load    s, sp_
    load    m, s[S_MEM]
    #if_cond m != 0
        #return m
    #end_cond
    load    r0, s[S_KIND]
    #if_cond r0 == SK_LVAR
        load    r1, s[S_OFFSET]
        #call   sprintf("sf[%d]", r1)
    #else_cond
        load    r1, s[S_LABEL]
        #call   sprintf("%s+1", r1)
    #end_cond
    move    m, r0
    store   m, s[S_MEM]
    #return m
#end_func

// is_simple_var(node) -> 1 for a local or global scalar variable
#def_func is_simple_var(ndp)
    #var    nd, s
    load    nd, ndp
    load    r0, nd[N_KIND]
    #if_cond r0 != ND_VAR
        #return 0
    #end_cond
    load    s, nd[N_VAL]
    load    r0, s[S_KIND]
    #if_cond r0 == SK_LVAR
        jump    ISV_SCALAR
    #end_cond
    #if_cond r0 == SK_GVAR
        jump    ISV_SCALAR
    #end_cond
    #return 0
ISV_SCALAR:
    load    r1, nd[N_TYPE]
    #call   vclass(r1)
    #if_cond r0 == VC_INT
        #return 1
    #end_cond
    #if_cond r0 == VC_DBL
        #return 1
    #end_cond
    #return 0
#end_func

// node_mem(node) -> memory operand of a simple variable node
#def_func node_mem(ndp)
    load    r0, ndp
    load    r1, r0[N_VAL]
    #call   sym_mem(r1)
#end_func

// emit_test(type): set flags from the value of an expression of type
#def_func emit_test(tp)
    load    r1, tp
    #call   vclass(r1)
    #if_cond r0 == VC_DBL
        #call   emit("    test    f0\n")
    #else_cond
        #call   emit("    test    r0\n")
    #end_cond
#end_func

// crt_use(sym): a runtime-provided function is referenced
#def_func crt_use(sp_)
    #var    s
    load    s, sp_
    load    r0, s[S_FLAGS]
    and     r0, SF_BUILTIN
    #if_cond r0 == 0
        #return 0
    #end_cond
    load    r1, s[S_NAME]
    #call   crt_need(r1)
#end_func

// crt_use_p(name_pstr): an internal runtime helper is referenced
#def_func crt_use_p(name)
    load    r1, name
    #call   intern_p(r1)
    #call   crt_need(r0)
#end_func

///////////////////////////////////////////////////////////////////////////////
// addresses
///////////////////////////////////////////////////////////////////////////////
#def_func str_ref(ndp)
    #var    nd, id
    load    nd, ndp
    load    id, nd[N_VAL]
    load    r1, nd[N_TYPE]
    #call   ty_kind(r1)
    #if_cond r0 == TY_PSTR
        #call   mark_str(id, 2)
        #call   emit("    move    r0, __S%d\n", id)
    #else_cond
        #call   mark_str(id, 1)
        #call   emit("    move    r0, __W%d+1\n", id)
    #end_cond
#end_func

// mark_str(id, bit)
#def_func mark_str(idp, bitp)
    #var    id, v, cnt
    load    id, idp
    load    v, CG_STRUSE
    #macro  VLEN(cnt, v)
    #while  cnt < id
        #call   vec_push(v, 0)
        add     cnt, 1
    #end_while
    #macro  VGET(r1, v, id)
    load    r2, bitp
    or      r1, r2
    #macro  VSET(v, id, r1)
#end_func

#def_func gen_addr(ndp)
    #var    nd, k, s, sk
    load    nd, ndp
    load    k, nd[N_KIND]
    #if_cond k == ND_VAR
        load    s, nd[N_VAL]
        load    sk, s[S_KIND]
        #if_cond sk == SK_LVAR
            load    r1, s[S_OFFSET]
            #call   emit("    add     r0, sf, %d\n", r1)
            #return 0
        #end_cond
        #if_cond sk == SK_GVAR
            load    r1, s[S_LABEL]
            #call   emit("    move    r0, %s+1\n", r1)
            #return 0
        #end_cond
        #call   crt_use(s)
        load    r1, s[S_LABEL]
        #call   emit("    move    r0, %s\n", r1)
        #return 0
    #end_cond
    #if_cond k == ND_DEREF
        load    r1, nd[N_A]
        #call   gen_expr(r1)
        #return 0
    #end_cond
    #if_cond k == ND_MEMBER
        load    r1, nd[N_A]
        #call   gen_addr(r1)
        load    r1, nd[N_VAL]
        load    r1, r1[MB_OFFSET]
        #if_cond r1 != 0
            #call   emit("    add     r0, %d\n", r1)
        #end_cond
        #return 0
    #end_cond
    #if_cond k == ND_STR
        #call   str_ref(nd)
        #return 0
    #end_cond
    #if_cond k == ND_COMMA
        load    r1, nd[N_A]
        #call   gen_expr(r1)
        load    r1, nd[N_B]
        #call   gen_addr(r1)
        #return 0
    #end_cond
    load    r1, nd[N_TYPE]
    #call   vclass(r1)
    #if_cond r0 == VC_ADDR
        #call   gen_expr(nd)
        #return 0
    #end_cond
    #call   error_node(nd, "internal: expression is not addressable")
#end_func

// gen_load(type): replace the address in r0 with the value it designates
#def_func gen_load(tp)
    load    r1, tp
    #call   vclass(r1)
    #if_cond r0 == VC_INT
        #call   emit("    load    r0, r0[0]\n")
    #end_cond
    #if_cond r0 == VC_DBL
        #call   emit("    load    f0, r0[0]\n")
    #end_cond
#end_func

// gen_copy(size): copy size words from [r0] to [r1]; r0 = r1 afterwards
#def_func gen_copy(szp)
    #var    sz, i, l
    load    sz, szp
    #if_cond sz <= 8
        #for    0, i < sz, 1
            #call   emit("    load    r2, r0[%d]\n    store   r2, r1[%d]\n", i, i)
        #end_for
    #else_cond
        #call   new_lbl()
        move    l, r0
        #call   emit("    move    r2, 0\n")
        #call   emit_label(l)
        #call   emit("    load    r3, r0[r2]\n    store   r3, r1[r2]\n    add     r2, 1\n")
        #call   emit("    cmp     r2, %d\n    jump    lt, __L%d\n", sz, l)
    #end_cond
    #call   emit("    move    r0, r1\n")
#end_func

// gen_store(type): store r0/f0 (or copy the aggregate at r0) to [r1]
#def_func gen_store(tp)
    #var    t
    load    t, tp
    #call   vclass(t)
    #if_cond r0 == VC_INT
        #call   emit("    store   r0, r1[0]\n")
        #return 0
    #end_cond
    #if_cond r0 == VC_DBL
        #call   emit("    store   f0, r1[0]\n")
        #return 0
    #end_cond
    load    r1, t[T_SIZE]
    #call   gen_copy(r1)
#end_func

///////////////////////////////////////////////////////////////////////////////
// simple operands for binary operators
///////////////////////////////////////////////////////////////////////////////
// load_simple_r1(node) -> 1 if node's value was loaded into r1 directly
#def_func load_simple_r1(ndp)
    #var    nd, k, v
    load    nd, ndp
    load    k, nd[N_KIND]
    #if_cond k == ND_NUM
        load    v, nd[N_VAL]
        #call   fits42(v)
        #if_cond r0 != 0
            #call   emit("    move    r1, %d\n", v)
            #return 1
        #end_cond
        #return 0
    #end_cond
    #call   is_simple_var(nd)
    #if_cond r0 != 0
        load    r1, nd[N_TYPE]
        #call   vclass(r1)
        #if_cond r0 == VC_INT
            #call   node_mem(nd)
            #call   emit("    load    r1, %s\n", r0)
            #return 1
        #end_cond
    #end_cond
    #return 0
#end_func

// load_simple_f1(node) -> 1 if node's value was loaded into f1 directly
#def_func load_simple_f1(ndp)
    #var    nd, k
    load    nd, ndp
    load    k, nd[N_KIND]
    #if_cond k == ND_FNUM
        load    r1, nd[N_VAL]
        #call   add_const(r1)
        #call   emit("    load    f1, __K%d\n", r0)
        #return 1
    #end_cond
    #call   is_simple_var(nd)
    #if_cond r0 != 0
        load    r1, nd[N_TYPE]
        #call   vclass(r1)
        #if_cond r0 == VC_DBL
            #call   node_mem(nd)
            #call   emit("    load    f1, %s\n", r0)
            #return 1
        #end_cond
    #end_cond
    #return 0
#end_func

// emit_udivmod(dst, src, result_reg): dst = dst / src (result "r2") or
//      dst % src (result "r3"), unsigned, via the runtime helper.  The
//      helper takes r2 / r3 and keeps r0 and r1.
#def_func emit_udivmod(dst, src, res)
    #var    d, s, rr
    load    d, dst
    load    s, src
    load    rr, res
    #call   crt_use_p("__udivmod")
    #macro  STRCMP(d, "r2")
    #if_cond r0 != 0
        #call   emit("    move    r2, %s\n", d)
    #end_cond
    #call   emit("    move    r3, %s\n    call    __crt___udivmod\n", s)
    #macro  STRCMP(d, rr)
    #if_cond r0 != 0
        #call   emit("    move    %s, %s\n", d, rr)
    #end_cond
#end_func

// pow2_shift(value) -> n if value == 2^n (n in 0..62), else -1
#def_func pow2_shift(val)
    #var    v, n
    load    v, val
    #if_cond v <= 0
        #return -1
    #end_cond
    sub     r1, v, 1
    and     r0, v, r1
    #if_cond r0 != 0
        #return -1
    #end_cond
    move    n, 0
    #while  v > 1
        rshift  v, 1
        add     n, 1
    #end_while
    #return n
#end_func

// emit_i2d(ireg, freg, unsigned): freg = (double)ireg   (uses r3, r4)
//      An unsigned value >= 2^63 is halved (keeping the low bit so that
//      rounding is correct), converted, and doubled.
#def_func emit_i2d(ireg, freg, uns)
    #var    ir, fr, l1, l2
    load    ir, ireg
    load    fr, freg
    load    r0, uns
    #if_cond r0 == 0
        #call   emit("    move    %s, %s\n", fr, ir)
        #return 0
    #end_cond
    #call   new_lbl()
    move    l1, r0
    #call   new_lbl()
    move    l2, r0
    #call   emit("    test    %s\n    jump    nn, __L%d\n", ir, l1)
    #call   emit("    move    r4, %s\n    and     r4, 1\n    move    r3, %s\n", ir, ir)
    #call   emit("    rshift  r3, 1\n    or      r3, r4\n    move    %s, r3\n", fr)
    #call   emit("    add     %s, %s\n    jump    __L%d\n", fr, fr, l2)
    #call   emit("__L%d:\n    move    %s, %s\n__L%d:\n", l1, fr, ir, l2)
#end_func

// emit_d2i(freg, ireg, unsigned): ireg = (integer)freg   (uses r3, f3; freg
//      is clobbered).  Values >= 2^63 convert to unsigned by subtracting
//      2^63 first and setting the top bit afterwards.
#def_func emit_d2i(freg, ireg, uns)
    #var    ir, fr, l1, l2, k
    load    ir, ireg
    load    fr, freg
    load    r0, uns
    #if_cond r0 == 0
        #call   emit("    move    %s, %s\n", ir, fr)
        #return 0
    #end_cond
    #call   new_lbl()
    move    l1, r0
    #call   new_lbl()
    move    l2, r0
    move    r1, 0x43E
    lshift  r1, 52
    #call   add_const(r1)
    move    k, r0
    #call   emit("    load    f3, __K%d\n    cmp     %s, f3\n    jump    lt, __L%d\n", k, fr, l1)
    #call   emit("    sub     %s, f3\n    move    %s, %s\n", fr, ir, fr)
    #call   emit("    move    r3, 1\n    lshift  r3, 63\n    xor     %s, r3\n    jump    __L%d\n", ir, l2)
    #call   emit("__L%d:\n    move    %s, %s\n__L%d:\n", l1, ir, fr, l2)
#end_func

// int_op(kind, dst, src, unsigned): dst = dst op src  (r3/r4 may be used)
#def_func int_op(kind, dst, src, uns)
    #var    k, d, s
    load    k, kind
    load    d, dst
    load    s, src
    #if_cond k == ND_ADD
        #call   emit("    add     %s, %s\n", d, s)
    #end_cond
    #if_cond k == ND_SUB
        #call   emit("    sub     %s, %s\n", d, s)
    #end_cond
    #if_cond k == ND_MUL
        #call   emit("    mult    %s, %s\n", d, s)
    #end_cond
    #if_cond k == ND_DIV
        load    r0, uns
        #if_cond r0 != 0
            #call   emit_udivmod(d, s, "r2")
        #else_cond
            #call   emit("    div     %s, %s\n", d, s)
        #end_cond
    #end_cond
    #if_cond k == ND_MOD
        load    r0, uns
        #if_cond r0 != 0
            #call   emit_udivmod(d, s, "r3")
        #else_cond
            #call   emit("    div     r4, r3, %s, %s\n    move    %s, r3\n", d, s, d)
        #end_cond
    #end_cond
    #if_cond k == ND_BAND
        #call   emit("    and     %s, %s\n", d, s)
    #end_cond
    #if_cond k == ND_BOR
        #call   emit("    or      %s, %s\n", d, s)
    #end_cond
    #if_cond k == ND_BXOR
        #call   emit("    xor     %s, %s\n", d, s)
    #end_cond
    #if_cond k == ND_SHL
        #call   emit("    lshift  %s, %s\n", d, s)
    #end_cond
    #if_cond k == ND_SHR
        load    r0, uns
        #if_cond r0 != 0
            #call   emit("    rshift  %s, %s\n", d, s)
        #else_cond
            #call   emit("    arshift %s, %s\n", d, s)
        #end_cond
    #end_cond
#end_func

// dbl_op(kind, dst, src): double arithmetic
#def_func dbl_op(kind, dst, src)
    #var    k, d, s
    load    k, kind
    load    d, dst
    load    s, src
    #if_cond k == ND_ADD
        #call   emit("    add     %s, %s\n", d, s)
    #end_cond
    #if_cond k == ND_SUB
        #call   emit("    sub     %s, %s\n", d, s)
    #end_cond
    #if_cond k == ND_MUL
        #call   emit("    mult    %s, %s\n", d, s)
    #end_cond
    #if_cond k == ND_DIV
        #call   emit("    div     %s, %s\n", d, s)
    #end_cond
#end_func

// is_uns_int(type) -> 1 for unsigned integer types (not pointers)
#def_func is_uns_int(tp)
    #var    t
    load    t, tp
    #call   ty_is_integer(t)
    #if_cond r0 == 0
        #return 0
    #end_cond
    load    r0, t[T_FLAGS]
    and     r0, TYF_UNSIGNED
    #if_cond r0 != 0
        #return 1
    #end_cond
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// binary operators
///////////////////////////////////////////////////////////////////////////////
#def_func gen_binary(ndp)
    #var    nd, k, a, b, ot, uns, l1, l2
    load    nd, ndp
    load    k, nd[N_KIND]
    load    a, nd[N_A]
    load    b, nd[N_B]
    #call   rtype(a)
    move    ot, r0
    #call   ty_is_double(ot)
    #if_cond r0 != 0
        #call   gen_expr(a)
        #call   load_simple_f1(b)
        #if_cond r0 == 0
            #call   emit("    push    f0\n")
            #call   gen_expr(b)
            #call   emit("    move    f1, f0\n    pop     f0\n")
        #end_cond
        #if_cond k <= ND_DIV
            #call   dbl_op(k, "f0", "f1")
            #return 0
        #end_cond
        #call   emit("    cmp     f0, f1\n")
        #if_cond k == ND_EQ
            #call   emit("    move    eq, r0, 1, 0\n")
        #end_cond
        #if_cond k == ND_NE
            #call   emit("    move    ne, r0, 1, 0\n")
        #end_cond
        #if_cond k == ND_LT
            #call   emit("    move    lt, r0, 1, 0\n")
        #end_cond
        #if_cond k == ND_LE
            #call   emit("    move    le, r0, 1, 0\n")
        #end_cond
        #return 0
    #end_cond
    #call   is_uns_int(ot)
    move    uns, r0
    // unsigned division by a power of two is a shift or a mask
    #if_cond uns != 0
        #call   is_const(b)
        #if_cond r0 != 0
            #call   const_eval(b)
            move    l2, r0
            #call   pow2_shift(l2)
            move    l1, r0
            #if_cond l1 >= 0
                #if_cond k == ND_DIV
                    #call   gen_expr(a)
                    #if_cond l1 > 0
                        #call   emit("    rshift  r0, %d\n", l1)
                    #end_cond
                    #return 0
                #end_cond
                #if_cond k == ND_MOD
                    #call   gen_expr(a)
                    sub     l2, 1
                    #call   emit_imm("r1", l2)
                    #call   emit("    and     r0, r1\n")
                    #return 0
                #end_cond
            #end_cond
        #end_cond
    #end_cond
    #call   gen_expr(a)
    #call   load_simple_r1(b)
    #if_cond r0 == 0
        #call   emit("    push    r0\n")
        #call   gen_expr(b)
        #call   emit("    move    r1, r0\n    pop     r0\n")
    #end_cond
    #if_cond k < ND_EQ
        #call   int_op(k, "r0", "r1", uns)
        #return 0
    #end_cond
    #if_cond k == ND_EQ
        #call   emit("    cmp     r0, r1\n    move    eq, r0, 1, 0\n")
        #return 0
    #end_cond
    #if_cond k == ND_NE
        #call   emit("    cmp     r0, r1\n    move    ne, r0, 1, 0\n")
        #return 0
    #end_cond
    // signed < and <= must account for overflow of the subtraction
    #if_cond uns != 0
        #call   emit("    move    r2, 1\n    lshift  r2, 63\n    xor     r0, r2\n    xor     r1, r2\n")
    #end_cond
    #call   new_lbl()
    move    l1, r0
    #call   new_lbl()
    move    l2, r0
    #call   emit("    cmp     r0, r1\n    jump    o, __L%d\n", l1)
    #if_cond k == ND_LT
        #call   emit("    move    lt, r0, 1, 0\n")
    #else_cond
        #call   emit("    move    le, r0, 1, 0\n")
    #end_cond
    #call   emit_jump(l2)
    #call   emit_label(l1)
    #call   emit("    move    lt, r0, 0, 1\n")
    #call   emit_label(l2)
#end_func

// gen_logic(node): && and ||
#def_func gen_logic(ndp)
    #var    nd, k, l1, l2, a, b
    load    nd, ndp
    load    k, nd[N_KIND]
    load    a, nd[N_A]
    load    b, nd[N_B]
    #call   new_lbl()
    move    l1, r0
    #call   new_lbl()
    move    l2, r0
    #call   gen_expr(a)
    load    r1, a[N_TYPE]
    #call   emit_test(r1)
    #if_cond k == ND_LAND
        #call   emit_jcc("z", l1)
    #else_cond
        #call   emit_jcc("nz", l1)
    #end_cond
    #call   gen_expr(b)
    load    r1, b[N_TYPE]
    #call   emit_test(r1)
    #if_cond k == ND_LAND
        #call   emit_jcc("z", l1)
        #call   emit("    move    r0, 1\n")
        #call   emit_jump(l2)
        #call   emit_label(l1)
        #call   emit("    move    r0, 0\n")
    #else_cond
        #call   emit_jcc("nz", l1)
        #call   emit("    move    r0, 0\n")
        #call   emit_jump(l2)
        #call   emit_label(l1)
        #call   emit("    move    r0, 1\n")
    #end_cond
    #call   emit_label(l2)
#end_func

///////////////////////////////////////////////////////////////////////////////
// assignment
///////////////////////////////////////////////////////////////////////////////
// norm_bool(type, reg): normalize reg to 0/1 if the type is _Bool
#def_func norm_bool(tp, reg)
    load    r1, tp
    #call   ty_kind(r1)
    #if_cond r0 == TY_BOOL
        load    r1, reg
        #call   emit("    test    %s\n    move    nz, %s, 1, 0\n", r1, r1)
    #end_cond
#end_func

#def_func gen_assign(ndp)
    #var    nd, a, b, ta, vc
    load    nd, ndp
    load    a, nd[N_A]
    load    b, nd[N_B]
    load    ta, nd[N_TYPE]
    #call   vclass(ta)
    move    vc, r0
    #if_cond vc != VC_ADDR
        #call   is_simple_var(a)
        #if_cond r0 != 0
            #call   gen_expr(b)
            #call   node_mem(a)
            #if_cond vc == VC_DBL
                #call   emit("    store   f0, %s\n", r0)
            #else_cond
                #call   emit("    store   r0, %s\n", r0)
            #end_cond
            #return 0
        #end_cond
    #end_cond
    #call   gen_addr(a)
    #call   emit("    push    r0\n")
    #call   gen_expr(b)
    #call   emit("    pop     r1\n")
    #call   gen_store(ta)
#end_func

#def_func gen_opassign(ndp)
    #var    nd, a, b, op, ot, ta, dop, dl
    load    nd, ndp
    load    a, nd[N_A]
    load    b, nd[N_B]
    load    op, nd[N_VAL]
    load    ot, nd[N_VAL2]
    load    ta, nd[N_TYPE]
    #call   ty_is_double(ot)
    move    dop, r0
    #call   ty_is_double(ta)
    move    dl, r0
    #call   gen_expr(b)
    #if_cond dop != 0
        #call   emit("    push    f0\n")
    #else_cond
        #call   emit("    push    r0\n")
    #end_cond
    #call   gen_addr(a)
    #if_cond dop != 0
        #call   emit("    pop     f1\n")
        #if_cond dl != 0
            #call   emit("    load    f2, r0[0]\n")
        #else_cond
            #call   emit("    load    r2, r0[0]\n")
            #call   is_uns_int(ta)
            #call   emit_i2d("r2", "f2", r0)
        #end_cond
        #call   dbl_op(op, "f2", "f1")
        #if_cond dl != 0
            #call   emit("    store   f2, r0[0]\n    move    f0, f2\n")
        #else_cond
            #call   is_uns_int(ta)
            #call   emit_d2i("f2", "r2", r0)
            #call   norm_bool(ta, "r2")
            #call   emit("    store   r2, r0[0]\n    move    r0, r2\n")
        #end_cond
        #return 0
    #end_cond
    #call   emit("    pop     r1\n    load    r2, r0[0]\n")
    #call   is_uns_int(ot)
    #call   int_op(op, "r2", "r1", r0)
    #call   norm_bool(ta, "r2")
    #call   emit("    store   r2, r0[0]\n    move    r0, r2\n")
#end_func

#def_func gen_incdec(ndp)
    #var    nd, a, d, post, ta, m
    load    nd, ndp
    load    a, nd[N_A]
    load    d, nd[N_VAL]
    load    post, nd[N_VAL2]
    load    ta, nd[N_TYPE]
    #call   ty_is_double(ta)
    #if_cond r0 != 0
        #call   gen_addr(a)
        #call   emit("    load    f1, r0[0]\n    add     f2, f1, %d\n    store   f2, r0[0]\n", d)
        #if_cond post != 0
            #call   emit("    move    f0, f1\n")
        #else_cond
            #call   emit("    move    f0, f2\n")
        #end_cond
        #return 0
    #end_cond
    #call   is_simple_var(a)
    #if_cond r0 != 0
        #call   node_mem(a)
        move    m, r0
        #call   emit("    load    r1, %s\n    add     r2, r1, %d\n", m, d)
        #call   norm_bool(ta, "r2")
        #call   emit("    store   r2, %s\n", m)
    #else_cond
        #call   gen_addr(a)
        #call   emit("    load    r1, r0[0]\n    add     r2, r1, %d\n", d)
        #call   norm_bool(ta, "r2")
        #call   emit("    store   r2, r0[0]\n")
    #end_cond
    #if_cond post != 0
        #call   emit("    move    r0, r1\n")
    #else_cond
        #call   emit("    move    r0, r2\n")
    #end_cond
#end_func

///////////////////////////////////////////////////////////////////////////////
// casts
///////////////////////////////////////////////////////////////////////////////
#def_func gen_cast(ndp)
    #var    nd, a, to, from, tk, fk, fd, td
    load    nd, ndp
    load    a, nd[N_A]
    load    to, nd[N_TYPE]
    #call   gen_expr(a)
    #call   ty_kind(to)
    move    tk, r0
    #if_cond tk == TY_VOID
        #return 0
    #end_cond
    #call   rtype(a)
    move    from, r0
    #call   ty_kind(from)
    move    fk, r0
    // packed string -> char*
    #if_cond fk == TY_PSTR
        #call   ty_is_char_ptr(to)
        #if_cond r0 != 0
            #call   crt_use_p("p2c")
            #call   emit("    push    r0\n    call    __crt_p2c\n    add     sp, 1\n")
        #end_cond
        #return 0
    #end_cond
    #call   ty_is_double(from)
    move    fd, r0
    #call   ty_is_double(to)
    move    td, r0
    #if_cond fd != 0
        #if_cond td != 0
            #return 0
        #end_cond
        #if_cond tk == TY_BOOL
            #call   emit("    test    f0\n    move    nz, r0, 1, 0\n")
        #else_cond
            #call   is_uns_int(to)
            #call   emit_d2i("f0", "r0", r0)
        #end_cond
        #return 0
    #end_cond
    #if_cond td != 0
        #call   is_uns_int(from)
        #call   emit_i2d("r0", "f0", r0)
        #return 0
    #end_cond
    #if_cond tk == TY_BOOL
        #if_cond fk != TY_BOOL
            #call   emit("    test    r0\n    move    nz, r0, 1, 0\n")
        #end_cond
    #end_cond
#end_func

///////////////////////////////////////////////////////////////////////////////
// calls
///////////////////////////////////////////////////////////////////////////////
#def_func gen_call(ndp)
    #var    nd, fn, margs, cnt, i, a, t, vc, nw, sz, j, rt, tmp, direct, s, pos, nfree
    load    nd, ndp
    load    fn, nd[N_A]
    load    margs, nd[N_VAL]
    load    rt, nd[N_TYPE]
    load    tmp, nd[N_C]
    #macro  VLEN(cnt, margs)
    move    nw, 0
    move    nfree, 0
    move    i, cnt
    #while  i >= 1
        #macro  VGET(a, margs, i)
        #call   rtype(a)
        move    t, r0
        #call   vclass(t)
        move    vc, r0
        load    r0, a[N_KIND]
        #if_cond r0 == ND_C2P
            add     nfree, 1
        #end_cond
        #call   gen_expr(a)
        #if_cond vc == VC_DBL
            #call   emit("    push    f0\n")
            add     nw, 1
        #else_cond
            #if_cond vc == VC_ADDR
                #call   ty_is_struct(t)
                #if_cond r0 != 0
                    load    sz, t[T_SIZE]
                    sub     j, sz, 1
                    #while  j >= 0
                        #call   emit("    load    r1, r0[%d]\n    push    r1\n", j)
                        sub     j, 1
                    #end_while
                    add     nw, sz
                #else_cond
                    #call   emit("    push    r0\n")
                    add     nw, 1
                #end_cond
            #else_cond
                #if_cond vc == VC_VOID
                    #call   error_node(a, "invalid use of void expression")
                #end_cond
                #call   emit("    push    r0\n")
                add     nw, 1
            #end_cond
        #end_cond
        sub     i, 1
    #end_while
    // hidden struct result pointer
    #if_cond tmp != 0
        load    r1, tmp[S_OFFSET]
        #call   emit("    add     r0, sf, %d\n    push    r0\n", r1)
        add     nw, 1
    #end_cond
    move    direct, 0
    load    r0, fn[N_KIND]
    #if_cond r0 == ND_VAR
        load    s, fn[N_VAL]
        load    r0, s[S_KIND]
        #if_cond r0 == SK_FUNC
            move    direct, 1
        #end_cond
    #end_cond
    #if_cond direct != 0
        #call   crt_use(s)
        load    r1, s[S_LABEL]
        #call   emit("    call    %s\n", r1)
    #else_cond
        #call   gen_expr(fn)
        #call   emit("    call    r0\n")
    #end_cond
    // free temporary packed strings made for the arguments
    #if_cond nfree != 0
        #call   emit("    move    r4, r0\n    move    f4, f0\n")
        move    pos, 1
        #if_cond tmp != 0
            add     pos, 1
        #end_cond
        #for    1, i <= cnt, 1
            #macro  VGET(a, margs, i)
            load    r0, a[N_KIND]
            #if_cond r0 == ND_C2P
                #call   emit("    load    r1, sp[%d]\n    int     iFREE\n", pos)
            #end_cond
            #call   rtype(a)
            move    t, r0
            move    sz, 1
            #call   ty_is_struct(t)
            #if_cond r0 != 0
                load    sz, t[T_SIZE]
            #end_cond
            add     pos, sz
        #end_for
        #call   emit("    move    r0, r4\n    move    f0, f4\n")
    #end_cond
    #if_cond nw != 0
        #call   emit("    add     sp, %d\n", nw)
    #end_cond
    #if_cond tmp != 0
        load    r1, tmp[S_OFFSET]
        #call   emit("    add     r0, sf, %d\n", r1)
    #end_cond
#end_func

// gen_memzero(sym): clear a local object
#def_func gen_memzero(sp_)
    #var    s, sz, off, i
    load    s, sp_
    load    r1, s[S_TYPE]
    load    sz, r1[T_SIZE]
    load    off, s[S_OFFSET]
    #if_cond sz <= 8
        #for    0, i < sz, 1
            add     r1, off, i
            #call   emit("    store   0, sf[%d]\n", r1)
        #end_for
    #else_cond
        // iMEMCLEAR cannot address the stack (a separate memory area in
        // the simulator), so locals are cleared with a loop
        #call   new_lbl()
        move    i, r0
        #call   emit("    add     r1, sf, %d\n    move    r2, %d\n", off, sz)
        #call   emit("__L%d:\n    sub     r2, 1\n    store   0, r1[r2]\n    test    r2\n    jump    nz, __L%d\n", i, i)
    #end_cond
#end_func

///////////////////////////////////////////////////////////////////////////////
// expressions
///////////////////////////////////////////////////////////////////////////////
#def_func gen_expr(ndp)
    #var    nd, k, t, vc, l1, l2, a
    load    nd, ndp
    load    k, nd[N_KIND]
    load    t, nd[N_TYPE]
    #if_cond k == ND_NUM
        load    r2, nd[N_VAL]
        #call   emit_imm("r0", r2)
        #return 0
    #end_cond
    #if_cond k == ND_FNUM
        load    r1, nd[N_VAL]
        #call   add_const(r1)
        #call   emit("    load    f0, __K%d\n", r0)
        #return 0
    #end_cond
    #if_cond k == ND_VAR
        #call   vclass(t)
        move    vc, r0
        #if_cond vc == VC_ADDR
            #call   gen_addr(nd)
            #return 0
        #end_cond
        #call   node_mem(nd)
        #if_cond vc == VC_DBL
            #call   emit("    load    f0, %s\n", r0)
        #else_cond
            #call   emit("    load    r0, %s\n", r0)
        #end_cond
        #return 0
    #end_cond
    #if_cond k == ND_STR
        #call   str_ref(nd)
        #return 0
    #end_cond
    #if_cond k == ND_ADDR
        load    r1, nd[N_A]
        #call   gen_addr(r1)
        #return 0
    #end_cond
    #if_cond k == ND_DEREF
        #call   gen_addr(nd)
        #call   gen_load(t)
        #return 0
    #end_cond
    #if_cond k == ND_MEMBER
        #call   gen_addr(nd)
        #call   gen_load(t)
        #return 0
    #end_cond
    #if_cond k == ND_ASSIGN
        #call   gen_assign(nd)
        #return 0
    #end_cond
    #if_cond k == ND_OPASSIGN
        #call   gen_opassign(nd)
        #return 0
    #end_cond
    #if_cond k == ND_INCDEC
        #call   gen_incdec(nd)
        #return 0
    #end_cond
    #if_cond k == ND_CAST
        #call   gen_cast(nd)
        #return 0
    #end_cond
    #if_cond k == ND_C2P
        load    r1, nd[N_A]
        #call   gen_expr(r1)
        #call   crt_use_p("c2p")
        #call   emit("    push    r0\n    call    __crt_c2p\n    add     sp, 1\n")
        #return 0
    #end_cond
    #if_cond k == ND_NEG
        load    r1, nd[N_A]
        #call   gen_expr(r1)
        #call   ty_is_double(t)
        #if_cond r0 != 0
            #call   emit("    neg     f0\n")
        #else_cond
            #call   emit("    neg     r0\n")
        #end_cond
        #return 0
    #end_cond
    #if_cond k == ND_BNOT
        load    r1, nd[N_A]
        #call   gen_expr(r1)
        #call   emit("    compl   r0\n")
        #return 0
    #end_cond
    #if_cond k == ND_NOT
        load    a, nd[N_A]
        #call   gen_expr(a)
        load    r1, a[N_TYPE]
        #call   emit_test(r1)
        #call   emit("    move    z, r0, 1, 0\n")
        #return 0
    #end_cond
    #if_cond k == ND_LAND
        #call   gen_logic(nd)
        #return 0
    #end_cond
    #if_cond k == ND_LOR
        #call   gen_logic(nd)
        #return 0
    #end_cond
    #if_cond k == ND_COND
        #call   new_lbl()
        move    l1, r0
        #call   new_lbl()
        move    l2, r0
        load    a, nd[N_A]
        #call   gen_expr(a)
        load    r1, a[N_TYPE]
        #call   emit_test(r1)
        #call   emit_jcc("z", l1)
        load    r1, nd[N_B]
        #call   gen_expr(r1)
        #call   emit_jump(l2)
        #call   emit_label(l1)
        load    r1, nd[N_C]
        #call   gen_expr(r1)
        #call   emit_label(l2)
        #return 0
    #end_cond
    #if_cond k == ND_COMMA
        load    r1, nd[N_A]
        #call   gen_expr(r1)
        load    r1, nd[N_B]
        #call   gen_expr(r1)
        #return 0
    #end_cond
    #if_cond k == ND_CALL
        #call   gen_call(nd)
        #return 0
    #end_cond
    #if_cond k == ND_MEMZERO
        load    r1, nd[N_VAL]
        #call   gen_memzero(r1)
        #return 0
    #end_cond
    #if_cond k >= ND_ADD
        #if_cond k <= ND_LE
            #call   gen_binary(nd)
            #return 0
        #end_cond
    #end_cond
    #call   error_node(nd, "internal: cannot generate code for expression")
#end_func

///////////////////////////////////////////////////////////////////////////////
// statements
///////////////////////////////////////////////////////////////////////////////
// gen_cond_false(expr, label): jump to label if expr is false
#def_func gen_cond_false(ndp, lbl)
    #var    nd
    load    nd, ndp
    #call   gen_expr(nd)
    load    r1, nd[N_TYPE]
    #call   emit_test(r1)
    load    r2, lbl
    #call   emit_jcc("z", r2)
#end_func

#def_func gen_stmt(ndp)
    #var    nd, k, l1, l2, l3, v, cnt, i, c, val
    load    nd, ndp
    #if_cond nd == 0
        #return 0
    #end_cond
    load    k, nd[N_KIND]
    #if_cond k == ND_EXPR
        load    r1, nd[N_A]
        #call   gen_expr(r1)
        #return 0
    #end_cond
    #if_cond k == ND_BLOCK
        load    v, nd[N_VAL]
        #macro  VLEN(cnt, v)
        #for    1, i <= cnt, 1
            #macro  VGET(r1, v, i)
            #call   gen_stmt(r1)
        #end_for
        #return 0
    #end_cond
    #if_cond k == ND_NULL
        #return 0
    #end_cond
    #if_cond k == ND_RETURN
        #call   gen_return(nd)
        #return 0
    #end_cond
    #if_cond k == ND_IF
        #call   new_lbl()
        move    l1, r0
        #call   new_lbl()
        move    l2, r0
        load    r1, nd[N_A]
        #call   gen_cond_false(r1, l1)
        load    r1, nd[N_B]
        #call   gen_stmt(r1)
        load    r0, nd[N_C]
        #if_cond r0 != 0
            #call   emit_jump(l2)
            #call   emit_label(l1)
            load    r1, nd[N_C]
            #call   gen_stmt(r1)
            #call   emit_label(l2)
        #else_cond
            #call   emit_label(l1)
        #end_cond
        #return 0
    #end_cond
    #if_cond k == ND_FOR
        #call   new_lbl()
        move    l1, r0                  // top
        #call   new_lbl()
        move    l2, r0                  // continue
        #call   new_lbl()
        move    l3, r0                  // break
        load    r1, nd[N_A]
        #call   gen_stmt(r1)
        #call   emit_label(l1)
        load    r0, nd[N_B]
        #if_cond r0 != 0
            load    r1, nd[N_B]
            #call   gen_cond_false(r1, l3)
        #end_cond
        load    r1, CG_BRK
        #call   vec_push(r1, l3)
        load    r1, CG_CONT
        #call   vec_push(r1, l2)
        load    r1, nd[N_D]
        #call   gen_stmt(r1)
        load    r1, CG_BRK
        #call   vec_pop(r1)
        load    r1, CG_CONT
        #call   vec_pop(r1)
        #call   emit_label(l2)
        load    r0, nd[N_C]
        #if_cond r0 != 0
            load    r1, nd[N_C]
            #call   gen_expr(r1)
        #end_cond
        #call   emit_jump(l1)
        #call   emit_label(l3)
        #return 0
    #end_cond
    #if_cond k == ND_DO
        #call   new_lbl()
        move    l1, r0
        #call   new_lbl()
        move    l2, r0
        #call   new_lbl()
        move    l3, r0
        #call   emit_label(l1)
        load    r1, CG_BRK
        #call   vec_push(r1, l3)
        load    r1, CG_CONT
        #call   vec_push(r1, l2)
        load    r1, nd[N_A]
        #call   gen_stmt(r1)
        load    r1, CG_BRK
        #call   vec_pop(r1)
        load    r1, CG_CONT
        #call   vec_pop(r1)
        #call   emit_label(l2)
        load    r1, nd[N_B]
        #call   gen_expr(r1)
        load    r1, nd[N_B]
        load    r1, r1[N_TYPE]
        #call   emit_test(r1)
        #call   emit_jcc("nz", l1)
        #call   emit_label(l3)
        #return 0
    #end_cond
    #if_cond k == ND_SWITCH
        #call   new_lbl()
        move    l3, r0
        load    r1, nd[N_A]
        #call   gen_expr(r1)
        load    v, nd[N_VAL]
        #macro  VLEN(cnt, v)
        #for    1, i <= cnt, 1
            #macro  VGET(c, v, i)
            #call   new_lbl()
            store   r0, c[N_D]
            load    val, c[N_VAL]
            move    r0, val
            arshift r0, 20
            #if_cond r0 == 0
                #call   emit("    cmp     r0, %d\n", val)
            #else_cond
                #if_cond r0 == -1
                    #call   emit("    cmp     r0, %d\n", val)
                #else_cond
                    #call   emit_imm("r1", val)
                    #call   emit("    cmp     r0, r1\n")
                #end_cond
            #end_cond
            load    r2, c[N_D]
            #call   emit_jcc("eq", r2)
        #end_for
        load    c, nd[N_VAL2]
        #if_cond c != 0
            #call   new_lbl()
            store   r0, c[N_D]
            #call   emit_jump(r0)
        #else_cond
            #call   emit_jump(l3)
        #end_cond
        load    r1, CG_BRK
        #call   vec_push(r1, l3)
        load    r1, nd[N_B]
        #call   gen_stmt(r1)
        load    r1, CG_BRK
        #call   vec_pop(r1)
        #call   emit_label(l3)
        #return 0
    #end_cond
    #if_cond k == ND_CASE
        load    r1, nd[N_D]
        #call   emit_label(r1)
        load    r1, nd[N_A]
        #call   gen_stmt(r1)
        #return 0
    #end_cond
    #if_cond k == ND_BREAK
        load    r1, CG_BRK
        #call   vec_last(r1)
        #call   emit_jump(r0)
        #return 0
    #end_cond
    #if_cond k == ND_CONTINUE
        load    r1, CG_CONT
        #call   vec_last(r1)
        #call   emit_jump(r0)
        #return 0
    #end_cond
    #if_cond k == ND_GOTO
        load    r1, nd[N_VAL2]
        load    r1, r1[LB_LABEL]
        #call   emit("    jump    %s\n", r1)
        #return 0
    #end_cond
    #if_cond k == ND_LABEL
        load    r1, nd[N_VAL2]
        load    r1, r1[LB_LABEL]
        #call   emit("%s:\n", r1)
        load    r1, nd[N_A]
        #call   gen_stmt(r1)
        #return 0
    #end_cond
    #call   error_node(nd, "internal: cannot generate code for statement")
#end_func

#def_func gen_return(ndp)
    #var    nd, a, rt
    load    nd, ndp
    load    a, nd[N_A]
    #if_cond a != 0
        load    r0, CG_FUNC
        load    r0, r0[S_TYPE]
        load    rt, r0[T_BASE]
        #call   gen_expr(a)
        #call   ty_is_struct(rt)
        #if_cond r0 != 0
            #call   emit("    load    r1, sf[3]\n")
            load    r1, rt[T_SIZE]
            #call   gen_copy(r1)
        #end_cond
    #end_cond
    #call   emit("    return\n")
#end_func

///////////////////////////////////////////////////////////////////////////////
// functions
///////////////////////////////////////////////////////////////////////////////
#def_func gen_function(sp_)
    #var    s, fr
    load    s, sp_
    store   s, CG_FUNC
    load    r1, s[S_NAME]
    #call   ent_p(r1)
    #call   emit("\n// %s\n", r0)
    load    r1, s[S_LABEL]
    #call   emit("%s:\n", r1)
    load    fr, s[S_FRAME]
    #if_cond fr > 0
        #call   emit("    add     sp, -%d\n", fr)
    #end_cond
    load    r1, s[S_INIT]
    #call   gen_stmt(r1)
    #call   emit("    move    r0, 0\n    return\n")
#end_func

///////////////////////////////////////////////////////////////////////////////
// program
///////////////////////////////////////////////////////////////////////////////
#def_func cg_begin(path)
    #var    outv
    #call   vec_new()
    store   r0, CG_CONSTS
    #call   vec_new()
    store   r0, CG_STRUSE
    #call   vec_new()
    store   r0, CG_BRK
    #call   vec_new()
    store   r0, CG_CONT
    #call   vec_new()
    store   r0, CG_GINIT
    #call   crt_init()
    load    r1, path
    #call   path_out(r1)
    move    outv, r0
    store   outv, OUT_PATH
    #call   openTextFile(outv, WRITE_MODE)
    #if_cond r0 == -1
        #call   cc_fatal2("cannot create output file", outv)
    #end_cond
    store   r0, OUT_PORT
    load    r1, path
    #call   emit("// Generated by the CPUSim64 C compiler from %s\n", r1)
    #call   emit("#include <system/io.asm>\n#include <system/string.asm>\n")
    #call   emit("#include <system/system.asm>\n#include <system/math.asm>\n\n")
    #call   emit("    call    __crt_ginit\n    call    __crt_main\n    #call   exit(r0)\n")
#end_func

// addr_const(node) -> packed label (CG_ADDEND = addend) or 0
#def_func addr_const(ndp)
    #var    nd, k, l, adv, t
    load    nd, ndp
    load    k, nd[N_KIND]
    #if_cond k == ND_CAST
        load    r1, nd[N_A]
        #call   addr_const(r1)
        #return r0
    #end_cond
    #if_cond k == ND_ADDR
        load    r1, nd[N_A]
        #call   lval_const(r1)
        #return r0
    #end_cond
    #if_cond k == ND_STR
        #call   lval_const(nd)
        #return r0
    #end_cond
    #if_cond k == ND_VAR
        load    r1, nd[N_TYPE]
        #call   vclass(r1)
        #if_cond r0 == VC_ADDR
            #call   lval_const(nd)
            #return r0
        #end_cond
        #return 0
    #end_cond
    #if_cond k == ND_ADD
        jump    AC_ADDSUB
    #end_cond
    #if_cond k == ND_SUB
        jump    AC_ADDSUB
    #end_cond
    #if_cond k == ND_COMMA
        // compound literal at file scope
        load    r1, nd[N_B]
        #call   addr_const(r1)
        #return r0
    #end_cond
    #return 0
AC_ADDSUB:
    load    t, nd[N_B]
    #call   is_const(t)
    #if_cond r0 == 0
        #return 0
    #end_cond
    load    r1, nd[N_A]
    #call   addr_const(r1)
    move    l, r0
    #if_cond l == 0
        #return 0
    #end_cond
    load    adv, CG_ADDEND
    #call   const_eval(t)
    #if_cond k == ND_SUB
        neg     r0
    #end_cond
    add     adv, r0
    store   adv, CG_ADDEND
    #return l
#end_func

// lval_const(node) -> packed label of an lvalue with static storage
#def_func lval_const(ndp)
    #var    nd, k, s, l, id
    load    nd, ndp
    load    k, nd[N_KIND]
    #if_cond k == ND_VAR
        load    s, nd[N_VAL]
        load    r0, s[S_KIND]
        #if_cond r0 == SK_GVAR
            move    r0, 1
            store   r0, CG_ADDEND
            load    r0, s[S_LABEL]
            #return r0
        #end_cond
        #if_cond r0 == SK_FUNC
            #call   crt_use(s)
            move    r0, 0
            store   r0, CG_ADDEND
            load    r0, s[S_LABEL]
            #return r0
        #end_cond
        #return 0
    #end_cond
    #if_cond k == ND_STR
        load    id, nd[N_VAL]
        load    r1, nd[N_TYPE]
        #call   ty_kind(r1)
        #if_cond r0 == TY_PSTR
            #call   mark_str(id, 2)
            move    r0, 0
            store   r0, CG_ADDEND
            #call   sprintf("__S%d", id)
            #return r0
        #end_cond
        #call   mark_str(id, 1)
        move    r0, 1
        store   r0, CG_ADDEND
        #call   sprintf("__W%d", id)
        #return r0
    #end_cond
    #if_cond k == ND_MEMBER
        load    r1, nd[N_A]
        #call   lval_const(r1)
        move    l, r0
        #if_cond l == 0
            #return 0
        #end_cond
        load    r1, nd[N_VAL]
        load    r1, r1[MB_OFFSET]
        load    r0, CG_ADDEND
        add     r0, r1
        store   r0, CG_ADDEND
        #return l
    #end_cond
    #if_cond k == ND_DEREF
        load    r1, nd[N_A]
        #call   addr_const(r1)
        #return r0
    #end_cond
    #return 0
#end_func

// emit_word(value): one initialized data word
#def_func emit_word(val)
    #var    v
    load    v, val
    #call   fits42(v)
    #if_cond r0 != 0
        #call   emit("#global .dci %d\n", v)
    #else_cond
        #call   emit("#global .dci 0x%x\n", v)
    #end_cond
#end_func

// emit_global(sym)
#def_func emit_global(sp_)
    #var    s, ty, sz, lv, words, cnt, i, l, e, lt, lab, g, v
    load    s, sp_
    load    lab, s[S_LABEL]
    // library globals are emitted only if something references them
    load    r0, s[S_FLAGS]
    and     r1, r0, SF_PRELUDE
    #if_cond r1 != 0
        and     r1, r0, SF_USED
        #if_cond r1 == 0
            #return 0
        #end_cond
    #end_cond
    load    r0, s[S_FLAGS]
    and     r0, SF_EXTERN
    #if_cond r0 != 0
        load    r0, s[S_FLAGS]
        and     r0, SF_USED
        #if_cond r0 != 0
            load    r1, s[S_NAME]
            #call   ent_p(r1)
            #call   cc_fatal2("undefined reference to", r0)
        #end_cond
        #return 0
    #end_cond
    load    ty, s[S_TYPE]
    load    sz, ty[T_SIZE]
    #call   ty_is_struct(ty)
    #if_cond r0 != 0
        load    r0, ty[T_FLAGS]
        and     r0, TYF_COMPLETE
        #if_cond r0 == 0
            load    r1, s[S_NAME]
            #call   ent_p(r1)
            #call   cc_fatal2("storage size isn't known for", r0)
        #end_cond
    #end_cond
    #if_cond sz < 1
        move    sz, 1
    #end_cond
    load    lv, s[S_INIT]
    #if_cond lv == 0
        #call   emit("#global %s: .dca %d\n", lab, sz)
        #return 0
    #end_cond
    #call   zalloc(sz)
    move    words, r0
    #macro  VLEN(cnt, lv)
    #for    1, i <= cnt, 1
        #macro  VGET(l, lv, i)
        load    e, l[L_EXPR]
        load    lt, l[L_TYPE]
        load    r0, l[L_OFF]
        #if_cond r0 >= sz
            #continue
        #end_cond
        #call   ty_is_double(lt)
        #if_cond r0 != 0
            #call   const_eval_d(e)
            load    r1, l[L_OFF]
            store   f0, words[r1]
            #continue
        #end_cond
        #call   is_const(e)
        #if_cond r0 != 0
            #call   const_eval(e)
            move    v, r0
            #call   ty_kind(lt)
            #if_cond r0 == TY_BOOL
                #if_cond v != 0
                    move    v, 1
                #end_cond
            #end_cond
            load    r1, l[L_OFF]
            store   v, words[r1]
            #continue
        #end_cond
        #call   addr_const(e)
        #if_cond r0 == 0
            #call   error_node(e, "initializer element is not constant")
        #end_cond
        move    v, r0
        #call   zalloc(GI_SIZE)
        move    g, r0
        store   lab, g[GI_LABEL]
        load    r1, l[L_OFF]
        add     r1, 1
        store   r1, g[GI_OFF]
        store   v, g[GI_SRC]
        load    r1, CG_ADDEND
        store   r1, g[GI_ADD]
        load    r1, CG_GINIT
        #call   vec_push(r1, g)
    #end_for
    #call   emit("#global %s: .dci 0\n", lab)
    #for    0, i < sz, 1
        load    r1, words[i]
        #call   emit_word(r1)
    #end_for
    move    r1, words
    int     iFREE
#end_func

// escape_asm(wstr) -> packed string usable inside an assembler "..." literal
#def_func escape_asm(wp)
    #var    w, cnt, i, c, sb
    load    w, wp
    #call   vec_new()
    move    sb, r0
    load    cnt, w[0]
    #for    1, i <= cnt, 1
        load    c, w[i]
        #if_cond c == '"'
            #call   sb_add_p(sb, "\\\"")
            #continue
        #end_cond
        #if_cond c == '\\'
            #call   sb_add_p(sb, "\\\\")
            #continue
        #end_cond
        #if_cond c >= 32
            #if_cond c < 127
                #call   vec_push(sb, c)
                #continue
            #end_cond
        #end_cond
        #call   sprintf("\\u{%x}", c)
        move    c, r0
        #call   sb_add_p(sb, c)
        move    r1, c
        int     iFREE
    #end_for
    #call   sb_to_p(sb)
#end_func

#def_func cg_end()
    #var    s, e, cnt, i, v, g, f, np, use, w
    // ---- every used function must be defined somewhere
    load    v, FUNCS
    #macro  VLEN(cnt, v)
    #for    1, i <= cnt, 1
        #macro  VGET(s, v, i)
        load    f, s[S_FLAGS]
        and     r0, f, SF_USED
        #if_cond r0 == 0
            #continue
        #end_cond
        and     r0, f, SF_DEFINED
        #if_cond r0 != 0
            #continue
        #end_cond
        and     r0, f, SF_BUILTIN
        #if_cond r0 != 0
            load    r1, s[S_NAME]
            load    r0, r1[E_CRT]
            #if_cond r0 != 0
                #continue
            #end_cond
        #end_cond
        load    r1, s[S_NAME]
        #call   ent_p(r1)
        #call   cc_fatal2("undefined reference to", r0)
    #end_for
    // ---- startup: call main
    #call   intern_p("main")
    load    s, r0[E_GLOBAL]
    load    r1, s[S_TYPE]
    load    r1, r1[T_PARAMS]
    #call   vec_len(r1)
    move    np, r0
    #call   emit("\n// startup\n__crt_main:\n")
    #if_cond np > 0
        #call   crt_use_p("args")
        #if_cond np > 2
            #call   emit("    push    0\n")
        #end_cond
        #call   emit("    #call   __crt_args()\n    push    r0\n    int     iARGC\n    push    r0\n")
    #end_cond
    load    r1, s[S_LABEL]
    #call   emit("    call    %s\n", r1)
    // returning from main is equivalent to calling exit() (runs the atexit
    // handlers) when the library exit() was compiled into the program
    #call   intern_p("exit")
    load    r1, r0[E_GLOBAL]
    #if_cond r1 != 0
        load    r0, r1[S_FLAGS]
        and     r0, SF_DEFINED
        #if_cond r0 != 0
            load    r1, r1[S_LABEL]
            #call   emit("    push    r0\n    call    %s\n", r1)
        #end_cond
    #end_cond
    #call   emit("    return\n")
    // ---- globals (collects address initializers)
    load    v, GLOBALS
    #macro  VLEN(cnt, v)
    #for    1, i <= cnt, 1
        #macro  VGET(r1, v, i)
        #call   emit_global(r1)
    #end_for
    // ---- runtime address initialization of globals
    #call   emit("\n__crt_ginit:\n")
    load    v, CG_GINIT
    #macro  VLEN(cnt, v)
    #for    1, i <= cnt, 1
        #macro  VGET(g, v, i)
        load    r1, g[GI_SRC]
        load    r2, g[GI_ADD]
        #call   emit("    move    r0, %s+%d\n", r1, r2)
        load    r1, g[GI_LABEL]
        load    r2, g[GI_OFF]
        #call   emit("    store   r0, %s+%d\n", r1, r2)
    #end_for
    #call   emit("    return\n")
    // ---- runtime library chunks
    #call   crt_emit()
    // ---- string literals
    load    v, CG_STRUSE
    #macro  VLEN(cnt, v)
    #for    1, i <= cnt, 1
        #macro  VGET(use, v, i)
        #if_cond use == 0
            #continue
        #end_cond
        load    r1, STRLITS
        #macro  VGET(w, r1, i)
        #call   escape_asm(w)
        move    e, r0
        and     r0, use, 1
        #if_cond r0 != 0
            #call   emit("__W%d: .dcw \"%s\\u{0}\"\n", i, e)
        #end_cond
        and     r0, use, 2
        #if_cond r0 != 0
            #call   emit("__S%d: .dcs \"%s\"\n", i, e)
        #end_cond
    #end_for
    // ---- 64-bit constants
    load    v, CG_CONSTS
    #macro  VLEN(cnt, v)
    #for    1, i <= cnt, 1
        #macro  VGET(r1, v, i)
        #call   emit("__K%d: .dci 0x%x\n", i, r1)
    #end_for
    #call   emit("    stop\n    stop\n")
    load    r1, OUT_PORT
    #call   closeFile(r1)
    move    r0, 0
    store   r0, OUT_PORT
#end_func

CODEGEN_ASM_END:
    nop
