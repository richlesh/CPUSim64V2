/*
 * gen.c - code generation: writes the CPUSim64 program for the assembled
 * MMIX image.  Included by mmixc.c.
 *
 * The generated program includes the run-time library (rt/mmix_rt.asm and
 * rt/mmix_io.asm).  Its C main() calls __c_mmx_program, written here: it
 * allocates the MMIX memory, copies the image into it, and runs the
 * translated instructions.
 *
 * Every MMIX instruction (every tetrabyte of the text segment that an
 * instruction assembled) becomes a short CPUSim64 sequence with the label
 * M_<address>.  The MMIX registers live in memory:
 *   r27  address of local register $0 (the register stack is the stack
 *        segment, so this is rO as a word address)
 *   r26  rL
 *   r25  address of mmx_seglim[0]   (segment sizes in words)
 *   r24  address of mmx_segbase[0]  (segment addresses)
 *   r23  address of mmx_g[0]        (global registers)
 *   r22  address of mmx_sr[0]       (special registers)
 *   r20  #8000000000000000
 *   r19  limit of the register stack
 * r0-r11 are scratch; the run-time functions change only r0-r4.
 *
 * Local registers at and above rL always hold zero, so reading a register
 * needs no test; writing $X with X >= rL raises rL first, as MMIX does.
 *
 * Indirect jumps (GO, PUSHGO, POP, RESUME, trips) go through MX_GO, which
 * uses a jump table with one entry per tetrabyte of the program's code.
 */

/* ------------------------------------------------------------ output */

static FILE *gout;

static void emit(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(gout, fmt, ap);
    va_end(ap);
    fputc('\n', gout);
}

/* text collected for the end of the program (exception stubs) */
static char *stub_buf;
static size_t stub_len, stub_cap;

static void stub(const char *fmt, ...) {
    char line[400];
    size_t n;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    n = strlen(line);
    if (stub_len + n + 2 > stub_cap) {
        size_t cap = stub_cap ? 2 * stub_cap : 65536;
        char *b;
        while (cap < stub_len + n + 2) cap *= 2;
        b = xalloc(cap);
        if (stub_buf) memcpy(b, stub_buf, stub_len);
        free(stub_buf);
        stub_buf = b;
        stub_cap = cap;
    }
    memcpy(stub_buf + stub_len, line, n);
    stub_len += n;
    stub_buf[stub_len++] = '\n';
    stub_buf[stub_len] = 0;
}

/* 64-bit constants that don't fit in an instruction go to a literal pool */
static octa *lits;
static int nlits, caplits;

static int lit_index(octa v) {
    int i;
    for (i = 0; i < nlits; i++)
        if (lits[i] == v) return i;
    if (nlits == caplits) {
        int cap = caplits ? 2 * caplits : 256;
        octa *b = xalloc(cap * sizeof(octa));
        if (lits) memcpy(b, lits, nlits * sizeof(octa));
        free(lits);
        lits = b;
        caplits = cap;
    }
    lits[nlits] = v;
    return nlits++;
}

static int fits42(octa v) {
    soct s = (soct)v;
    return s >= -((soct)1 << 40) && s < ((soct)1 << 40);
}

/* register r <- constant v (emit_to selects the main code or the stubs) */
static void (*emit_to)(const char *fmt, ...) = emit;

static void konst(int r, octa v) {
    if (fits42(v)) emit_to("\tmove\tr%d, %lld", r, (soct)v);
    else emit_to("\tload\tr%d, MX_K%d", r, lit_index(v));
}

static void push_konst(octa v) {
    if (fits42(v)) emit_to("\tpush\t%lld", (soct)v);
    else {
        emit_to("\tload\tr4, MX_K%d", lit_index(v));
        emit_to("\tpush\tr4");
    }
}

/* ------------------------------------------------------------ registers */

static int G;

static void ldr(int r, int k) {
    emit_to("\tload\tr%d, r%d[%d]", r, k >= G ? 23 : 27, k);
}

static void str_(int r, int k) {
    emit_to("\tstore\tr%d, r%d[%d]", r, k >= G ? 23 : 27, k);
}

/* rL = max(rL, k+1) for a local destination register $k */
static void dest_L(int k) {
    if (k >= G) return;
    emit("\tcmp\tr26, %d", k + 1);
    emit("\tmove\tlt, r26, %d, r26", k + 1);
}

/* ------------------------------------------------------------ the image */

static Tetra **code;            /* instruction tetras sorted by address */
static int ncode;
static octa jt_lo, jt_hi;       /* jump table: tetras jt_lo <= a < jt_hi */
static octa seg_words[4];       /* segment sizes (words) */

static int cmp_tp(const void *a, const void *b) {
    const Tetra *x = *(Tetra * const *)a, *y = *(Tetra * const *)b;
    return x->addr < y->addr ? -1 : x->addr > y->addr;
}

static Tetra *find_inst(octa a) {
    int lo = 0, hi = ncode - 1;
    while (lo <= hi) {
        int m = (lo + hi) / 2;
        if (code[m]->addr == a) return code[m];
        if (code[m]->addr < a) lo = m + 1;
        else hi = m - 1;
    }
    return 0;
}

/* option values (bytes) for the segment sizes beyond the image */
octa opt_data = 1024 * 1024, opt_pool = 512 * 1024, opt_stack = 512 * 1024;

/* ------------------------------------------------------------ helpers */

static octa cur_loc, cur_inst;
static int cur_x, cur_y, cur_z;
static int stub_no;

/* the label of an MMIX address: M_<hex> if it is an instruction we
   translated, else 0 */
static const char *code_label(octa a, char *buf) {
    if ((a & 3) || !find_inst(a)) return 0;
    snprintf(buf, 40, "M_%llx", a);
    return buf;
}

/* a jump to MMIX address a (cond: CPUSim64 condition or "" for always) */
static void jump_to(const char *cond, octa a) {
    char buf[40];
    const char *l = code_label(a, buf);
    if (l) {
        if (cond[0]) emit("\tjump\t%s, %s", cond, l);
        else emit("\tjump\t%s", l);
        return;
    }
    /* not code that we know: through MX_GO */
    if (cond[0]) {
        int n = ++stub_no;
        emit("\tjump\t%s, MX_S%d", cond, n);
        stub("MX_S%d:", n);
        emit_to = stub;
        konst(1, a);
        emit_to = emit;
        stub("\tjump\tMX_GO");
    } else {
        konst(1, a);
        emit("\tjump\tMX_GO");
    }
}

/* r1 = $Y + ($Z or Z): the effective address of a load or store */
static void eff_addr(void) {
    ldr(1, cur_y);
    if (cur_inst >> 24 & 1) {
        if (cur_z) emit("\tadd\tr1, %d", cur_z);
    } else {
        ldr(2, cur_z);
        emit("\tadd\tr1, r2");
    }
}

/* r3 = the word that holds the byte at MMIX address r1 (r1 unchanged) */
static void ea_word(void) {
    emit("\trshift\tr2, r1, 61");
    emit("\tlshift\tr3, r1, 3");
    emit("\trshift\tr3, 6");
    emit("\tload\tr4, r25[r2]");
    emit("\tcmp\tr3, r4");
    emit("\tcall\tge, MX_SEGV");
    emit("\tload\tr4, r24[r2]");
    emit("\tadd\tr3, r4");
}

/* r2 = $Z or the immediate Z (op & 1) */
static void op_z(int r) {
    if (cur_inst >> 24 & 1) emit("\tmove\tr%d, %d", r, cur_z);
    else ldr(r, cur_z);
}

/* an exception exc of the current instruction: if store_res, the stub
   first stores r0 into $X; y and z are in the registers ry and rz */
static void exc_stub(const char *cond, int exc, int store_res, int ry, int rz) {
    int n = ++stub_no;
    char next[40];
    emit("\tjump\t%s, MX_S%d", cond, n);
    stub("MX_S%d:", n);
    emit_to = stub;
    if (store_res) str_(0, cur_x);
    stub("\tpush\tr%d", rz);
    stub("\tpush\tr%d", ry);
    push_konst(cur_inst);
    push_konst(cur_loc);
    stub("\tpush\t%d", exc);
    stub("\tcall\t__c_mmx_exc");
    stub("\tadd\tsp, 5");
    stub("\ttest\tr0");
    stub("\tjump\tnz, MX_TRIP0");
    if (code_label(cur_loc + 4, next)) stub("\tjump\t%s", next);
    else {
        konst(1, cur_loc + 4);
        stub("\tjump\tMX_GO");
    }
    emit_to = emit;
}

static void illegal(void) {
    konst(1, cur_loc);
    emit("\tjump\tMX_ILLEGAL");
}

/* call mmx_op(inst, r1, r2, loc); result in r0 */
static void call_op(void) {
    push_konst(cur_loc);
    emit("\tpush\tr2");
    emit("\tpush\tr1");
    push_konst(cur_inst);
    emit("\tcall\t__c_mmx_op");
    emit("\tadd\tsp, 4");
}

static void check_trip(void) {
    emit("\tload\tr1, __c_mmx_trip+1");
    emit("\ttest\tr1");
    emit("\tjump\tnz, MX_TRIP");
}

/* tests the register in r1 for the condition of op bits 1..3 (N Z P OD NN
   NZ NP EV); returns the CPUSim64 condition that holds when it is true */
static const char *cond_of(int op) {
    static const char *c[8] = { "n", "z", "p", "nz", "nn", "nz", "np", "z" };
    int k = (op >> 1) & 7;
    if (k == 3 || k == 7) emit("\tand\tr1, 1");
    emit("\ttest\tr1");
    return c[k];
}

static const char *neg_cond(const char *c) {
    if (!strcmp(c, "n")) return "nn";
    if (!strcmp(c, "nn")) return "n";
    if (!strcmp(c, "z")) return "nz";
    if (!strcmp(c, "nz")) return "z";
    if (!strcmp(c, "p")) return "np";
    return "p";
}

/* stores the low `bytes` bytes of r5 at MMIX address r1 (word in r3) */
static void store_part(int bytes) {
    int bits = 8 * bytes;
    emit("\tand\tr2, r1, %d", 8 - bytes);
    emit("\tlshift\tr2, 3");
    emit("\tmove\tr4, -1");
    emit("\tlshift\tr4, %d", 64 - bits);
    emit("\trshift\tr4, r2");
    emit("\tlshift\tr0, r5, %d", 64 - bits);
    emit("\trshift\tr0, r2");
    emit("\tload\tr6, r3[0]");
    emit("\tcompl\tr4");
    emit("\tand\tr6, r4");
    emit("\tor\tr6, r0");
    emit("\tstore\tr6, r3[0]");
}

/* r0 = the `bytes` bytes at MMIX address r1 (word address r3), signed or
   unsigned */
static void load_part(int bytes, int is_signed) {
    int bits = 8 * bytes;
    emit("\tload\tr0, r3[0]");
    if (bytes == 8) return;
    emit("\tand\tr2, r1, %d", 8 - bytes);
    emit("\tlshift\tr2, 3");
    emit("\tlshift\tr0, r2");
    emit("\t%s\tr0, %d", is_signed ? "arshift" : "rshift", 64 - bits);
}

/* the register stack check after a push */
static void stack_check(void) {
    emit("\tcmp\tr27, r19");
    emit("\tcall\tge, MX_STKOVF");
}

/* ------------------------------------------------------------ instructions */

static void gen_inst(Tetra *t) {
    int op, xx, yy, zz;
    octa yz, inst = t->val;
    char lab[40];
    cur_loc = t->addr;
    cur_inst = inst;
    op = (int)(inst >> 24);
    xx = cur_x = (int)((inst >> 16) & 255);
    yy = cur_y = (int)((inst >> 8) & 255);
    zz = cur_z = (int)(inst & 255);
    yz = inst & 0xffff;

    switch (op) {
    case 0x00:                                              /* TRAP */
        if (xx != 0 || yy > 10 || (yy == 0 && zz != 0)) { illegal(); break; }
        ldr(1, yy);
        ldr(2, zz);
        emit("\tpush\tr2");
        emit("\tpush\tr1");
        push_konst(inst);
        push_konst(cur_loc);
        emit("\tcall\t__c_mmx_trap");
        emit("\tadd\tsp, 4");
        break;

    /* floating point, multiply, divide and the bit operations: mmx_op */
    case 0x01: case 0x02: case 0x03: case 0x04: case 0x06:
    case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x16:
    case 0x05: case 0x07: case 0x15: case 0x17:
    case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
    case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
    case 0xd0: case 0xd1: case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
    case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:
        if ((op == 0x05 || op == 0x07 || op == 0x15 || op == 0x17 || (op >= 0x08 && op <= 0x0f)) && yy > 4) {
            illegal();
            break;
        }
        dest_L(xx);
        if (op == 0x05 || op == 0x07 || op == 0x15 || op == 0x17 || (op >= 0x08 && op <= 0x0f))
            emit("\tmove\tr1, %d", yy);
        else ldr(1, yy);
        if ((op >= 0x08 && op <= 0x0f) || op >= 0x18) op_z(2);
        else ldr(2, zz);
        call_op();
        str_(0, xx);
        if (op <= 0x17 || op == 0x18 || op == 0x19 || op == 0x1c || op == 0x1d) check_trip();
        break;

    case 0x20: case 0x21: case 0x24: case 0x25:             /* ADD SUB */
        dest_L(xx);
        ldr(1, yy);
        op_z(2);
        emit("\t%s\tr0, r1, r2", op < 0x24 ? "add" : "sub");
        exc_stub("o", 0x4000, 1, 1, 2);
        str_(0, xx);
        break;
    case 0x22: case 0x23: case 0x26: case 0x27:             /* ADDU SUBU */
        dest_L(xx);
        ldr(1, yy);
        op_z(2);
        emit("\t%s\tr0, r1, r2", op < 0x24 ? "add" : "sub");
        str_(0, xx);
        break;
    case 0x28: case 0x29: case 0x2a: case 0x2b:             /* 2ADDU 4ADDU */
    case 0x2c: case 0x2d: case 0x2e: case 0x2f:             /* 8ADDU 16ADDU */
        dest_L(xx);
        ldr(1, yy);
        op_z(2);
        emit("\tlshift\tr0, r1, %d", ((op - 0x28) >> 1) + 1);
        emit("\tadd\tr0, r2");
        str_(0, xx);
        break;
    case 0x30: case 0x31: case 0x32: case 0x33:             /* CMP CMPU */
        dest_L(xx);
        ldr(1, yy);
        op_z(2);
        if (op >= 0x32) {
            emit("\txor\tr1, r20");
            emit("\txor\tr2, r20");
        }
        /* sign of y - z, correct also when the subtraction overflows */
        emit("\tsub\tr0, r1, r2");
        emit("\txor\tr3, r1, r2");
        emit("\txor\tr4, r1, r0");
        emit("\tand\tr3, r4");
        emit("\txor\tr3, r0");
        emit("\tarshift\tr3, 63");
        emit("\ttest\tr0");
        emit("\tmove\tnz, r0, 1, 0");
        emit("\tor\tr0, r3");
        str_(0, xx);
        break;
    case 0x34: case 0x35: case 0x36: case 0x37:             /* NEG NEGU */
        dest_L(xx);
        emit("\tmove\tr1, %d", yy);
        op_z(2);
        emit("\tsub\tr0, r1, r2");
        if (op < 0x36) exc_stub("o", 0x4000, 1, 1, 2);
        str_(0, xx);
        break;
    case 0x38: case 0x39: case 0x3a: case 0x3b:             /* SL SLU */
    case 0x3c: case 0x3d: case 0x3e: case 0x3f:             /* SR SRU */
    {
        const char *sh = op < 0x3c ? "lshift" : op < 0x3e ? "arshift" : "rshift";
        int check = op == 0x38 || op == 0x39;
        dest_L(xx);
        ldr(1, yy);
        if (op & 1) {
            emit("\tmove\tr2, %d", zz);
            if (zz < 64) emit("\t%s\tr0, r1, %d", sh, zz);
            else if (op == 0x3c || op == 0x3d) emit("\tarshift\tr0, r1, 63");
            else emit("\tmove\tr0, 0");
            if (check) {
                if (zz < 64) {
                    emit("\tarshift\tr3, r0, %d", zz);
                    emit("\tcmp\tr3, r1");
                } else emit("\ttest\tr1");
                exc_stub("nz", 0x4000, 1, 1, 2);
            }
        } else {
            ldr(2, zz);
            snprintf(lab, sizeof lab, "MX_%llx", cur_loc);
            emit("\trshift\tr3, r2, 6");
            emit("\ttest\tr3");
            emit("\tjump\tnz, %s_big", lab);
            emit("\t%s\tr0, r1, r2", sh);
            if (check) {
                emit("\tarshift\tr3, r0, r2");
                emit("\tcmp\tr3, r1");
            }
            emit("\tjump\t%s_done", lab);
            emit("%s_big:", lab);
            if (op == 0x3c) emit("\tarshift\tr0, r1, 63");
            else emit("\tmove\tr0, 0");
            if (check) emit("\ttest\tr1");
            emit("%s_done:", lab);
            if (check) exc_stub("nz", 0x4000, 1, 1, 2);
        }
        str_(0, xx);
        break;
    }

    case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
    case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
    case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
    case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
    {                                                       /* branches */
        octa target = cur_loc + 4 * (yz - ((op & 1) ? 0x10000 : 0));
        ldr(1, xx);
        jump_to(cond_of(op), target);
        break;
    }
    case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
    case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
        dest_L(xx);                                         /* CSxx */
        ldr(1, yy);
        snprintf(lab, sizeof lab, "MX_%llx_no", cur_loc);
        emit("\tjump\t%s, %s", neg_cond(cond_of(op)), lab);
        op_z(2);
        str_(2, xx);
        emit("%s:", lab);
        break;
    case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
    case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
    {                                                       /* ZSxx */
        const char *c;
        dest_L(xx);
        op_z(2);
        ldr(1, yy);
        c = cond_of(op);
        emit("\tmove\t%s, r0, r2, 0", c);
        str_(0, xx);
        break;
    }

    case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
    case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
    case 0x96: case 0x97:                                   /* loads, LDUNC */
    {
        static const int size[8] = { 1, 1, 2, 2, 4, 4, 8, 8 };
        int k = op >= 0x96 ? 6 : (op >> 1) & 7;
        dest_L(xx);
        eff_addr();
        ea_word();
        load_part(size[k], !(op & 2));
        str_(0, xx);
        break;
    }
    case 0x90: case 0x91:                                   /* LDSF */
        dest_L(xx);
        eff_addr();
        ea_word();
        load_part(4, 0);
        emit("\tmove\tr2, r0");
        emit("\tmove\tr1, 0");
        call_op();
        str_(0, xx);
        break;
    case 0x92: case 0x93:                                   /* LDHT */
        dest_L(xx);
        eff_addr();
        ea_word();
        load_part(4, 0);
        emit("\tlshift\tr0, 32");
        str_(0, xx);
        break;
    case 0x94: case 0x95:                                   /* CSWAP */
        snprintf(lab, sizeof lab, "MX_%llx", cur_loc);
        ldr(5, xx);
        dest_L(xx);
        eff_addr();
        ea_word();
        emit("\tload\tr0, r3[0]");
        emit("\tload\tr4, r22[23]");
        emit("\tcmp\tr0, r4");
        emit("\tjump\tne, %s_ne", lab);
        emit("\tstore\tr5, r3[0]");
        emit("\tmove\tr0, 1");
        emit("\tjump\t%s_done", lab);
        emit("%s_ne:", lab);
        emit("\tstore\tr0, r22[23]");
        emit("\tmove\tr0, 0");
        emit("%s_done:", lab);
        str_(0, xx);
        break;
    case 0x98: case 0x99:                                   /* LDVTS */
        illegal();
        break;
    case 0x9a: case 0x9b: case 0x9c: case 0x9d:             /* PRELD PREGO */
    case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd:   /* SYNCD PREST SYNCID */
    case 0xfd:                                              /* SWYM */
        emit("\tnop");
        break;
    case 0x9e: case 0x9f:                                   /* GO */
        eff_addr();
        dest_L(xx);
        konst(0, cur_loc + 4);
        str_(0, xx);
        emit("\tjump\tMX_GO");
        break;

    case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
    case 0xa8: case 0xa9: case 0xaa: case 0xab:             /* STB STW STT */
    {
        int bytes = 1 << ((op - 0xa0) >> 2);
        ldr(5, xx);
        eff_addr();
        ea_word();
        store_part(bytes);
        if (!(op & 2)) {
            emit("\tlshift\tr0, r5, %d", 64 - 8 * bytes);
            emit("\tarshift\tr0, %d", 64 - 8 * bytes);
            emit("\tcmp\tr0, r5");
            exc_stub("ne", 0x4000, 0, 1, 5);
        }
        break;
    }
    case 0xac: case 0xad: case 0xae: case 0xaf:             /* STO STOU */
    case 0xb6: case 0xb7:                                   /* STUNC */
        ldr(5, xx);
        eff_addr();
        ea_word();
        emit("\tstore\tr5, r3[0]");
        break;
    case 0xb0: case 0xb1:                                   /* STSF */
        ldr(5, xx);
        eff_addr();
        emit("\tmove\tr6, r1");
        emit("\tmove\tr2, r5");
        call_op();
        emit("\tmove\tr5, r0");
        emit("\tmove\tr1, r6");
        ea_word();
        store_part(4);
        check_trip();
        break;
    case 0xb2: case 0xb3:                                   /* STHT */
        ldr(5, xx);
        emit("\trshift\tr5, 32");
        eff_addr();
        ea_word();
        store_part(4);
        break;
    case 0xb4: case 0xb5:                                   /* STCO */
        eff_addr();
        ea_word();
        emit("\tmove\tr5, %d", xx);
        emit("\tstore\tr5, r3[0]");
        break;
    case 0xbe: case 0xbf:                                   /* PUSHGO */
        eff_addr();
        emit("\tmove\tr6, r1");
        if (xx < G) {
            dest_L(xx);
            emit("\tmove\tr1, %d", xx);
            emit("\tstore\tr1, r27[%d]", xx);
            konst(1, cur_loc + 4);
            emit("\tstore\tr1, r22[4]");
            emit("\tadd\tr27, %d", xx + 1);
            emit("\tsub\tr26, %d", xx + 1);
        } else {
            emit("\tstore\tr26, r27[r26]");
            konst(1, cur_loc + 4);
            emit("\tstore\tr1, r22[4]");
            emit("\tadd\tr27, r26");
            emit("\tadd\tr27, 1");
            emit("\tmove\tr26, 0");
        }
        stack_check();
        emit("\tmove\tr1, r6");
        emit("\tjump\tMX_GO");
        break;

    case 0xc0: case 0xc1: case 0xc2: case 0xc3: case 0xc4: case 0xc5: case 0xc6: case 0xc7:
    case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd: case 0xce: case 0xcf:
    {                                                       /* logic */
        int k = (op >> 1) & 7;
        dest_L(xx);
        ldr(1, yy);
        op_z(2);
        if (k == 1 || k == 5) emit("\tcompl\tr2");          /* ORN ANDN */
        emit("\t%s\tr0, r1, r2", (k == 0 || k == 1 || k == 2) ? "or" :
             (k == 3 || k == 7) ? "xor" : "and");
        if (k == 2 || k == 6 || k == 7) emit("\tcompl\tr0");  /* NOR NAND NXOR */
        str_(0, xx);
        break;
    }

    case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
    case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
    {                                                       /* wyde immediates */
        octa v = yz << (16 * (3 - (op & 3)));
        int k = (op >> 2) & 3;
        dest_L(xx);
        if (k == 0) konst(0, v);                             /* SET */
        else {
            ldr(0, xx);
            konst(1, k == 3 ? ~v : v);
            emit("\t%s\tr0, r1", k == 1 ? "add" : k == 2 ? "or" : "and");
        }
        str_(0, xx);
        break;
    }

    case 0xf0: case 0xf1:                                   /* JMP */
    {
        octa xyz = inst & 0xffffff;
        jump_to("", cur_loc + 4 * (xyz - ((op & 1) ? 0x1000000 : 0)));
        break;
    }
    case 0xf2: case 0xf3:                                   /* PUSHJ */
    {
        octa target = cur_loc + 4 * (yz - ((op & 1) ? 0x10000 : 0));
        if (xx < G) {
            dest_L(xx);
            emit("\tmove\tr1, %d", xx);
            emit("\tstore\tr1, r27[%d]", xx);
            konst(1, cur_loc + 4);
            emit("\tstore\tr1, r22[4]");
            emit("\tadd\tr27, %d", xx + 1);
            emit("\tsub\tr26, %d", xx + 1);
        } else {
            emit("\tstore\tr26, r27[r26]");
            konst(1, cur_loc + 4);
            emit("\tstore\tr1, r22[4]");
            emit("\tadd\tr27, r26");
            emit("\tadd\tr27, 1");
            emit("\tmove\tr26, 0");
        }
        stack_check();
        jump_to("", target);
        break;
    }
    case 0xf4: case 0xf5:                                   /* GETA */
        dest_L(xx);
        konst(0, cur_loc + 4 * (yz - ((op & 1) ? 0x10000 : 0)));
        str_(0, xx);
        break;
    case 0xf6: case 0xf7:                                   /* PUT */
        if (yy != 0 || xx >= 32 || (xx >= 9 && xx <= 18)) { illegal(); break; }
        op_z(2);
        if (xx == 19) {                                     /* rG */
            konst(1, cur_loc);
            emit("\tcmp\tr2, %d", G);
            emit("\tcall\tne, MX_PUTG");
        } else if (xx == 20) {                              /* rL: can only shrink */
            snprintf(lab, sizeof lab, "MX_%llx", cur_loc);
            emit("\ttest\tr2");
            emit("\tjump\tn, %s_done", lab);
            emit("\tcmp\tr2, r26");
            emit("\tjump\tge, %s_done", lab);
            emit("\tmove\tr0, 0");
            emit("%s_loop:", lab);
            emit("\tsub\tr26, 1");
            emit("\tstore\tr0, r27[r26]");
            emit("\tcmp\tr26, r2");
            emit("\tjump\tgt, %s_loop", lab);
            emit("%s_done:", lab);
        } else {
            if (xx == 21) {                                 /* rA */
                konst(1, cur_loc);
                emit("\trshift\tr3, r2, 18");
                emit("\ttest\tr3");
                emit("\tjump\tnz, MX_ILLEGAL");
            }
            emit("\tstore\tr2, r22[%d]", xx);
        }
        break;
    case 0xf8:                                              /* POP */
        emit("\tmove\tr1, %d", xx);
        emit("\tmove\tr2, %lld", (soct)(yz << 2));
        emit("\tjump\tMX_POP");
        break;
    case 0xf9:                                              /* RESUME */
        if (xx || yy || zz) { illegal(); break; }
        konst(1, cur_loc);
        emit("\tload\tr3, r22[25]");
        emit("\ttest\tr3");
        emit("\tjump\tnn, MX_RESUME");
        emit("\tload\tr1, r22[24]");
        emit("\tjump\tMX_GO");
        break;
    case 0xfa:                                              /* SAVE */
        if (xx < G || yy || zz) { illegal(); break; }
        emit("\tstore\tr27, __c_mmx_rb+1");
        emit("\tstore\tr26, __c_mmx_rl+1");
        emit("\tcall\t__c_mmx_save");
        emit("\tload\tr27, __c_mmx_rb+1");
        emit("\tload\tr26, __c_mmx_rl+1");
        str_(0, xx);
        break;
    case 0xfb:                                              /* UNSAVE */
        if (xx || yy) { illegal(); break; }
        ldr(1, zz);
        push_konst(cur_loc);
        emit("\tpush\tr1");
        emit("\tcall\t__c_mmx_unsave");
        emit("\tadd\tsp, 2");
        emit("\tload\tr27, __c_mmx_rb+1");
        emit("\tload\tr26, __c_mmx_rl+1");
        break;
    case 0xfc:                                              /* SYNC */
        if (xx || yy || zz > 3) illegal();
        else emit("\tnop");
        break;
    case 0xfe:                                              /* GET */
        if (yy != 0 || zz >= 32) { illegal(); break; }
        dest_L(xx);
        if (zz == 20) emit("\tmove\tr0, r26");
        else if (zz == 19) emit("\tmove\tr0, %d", G);
        else if (zz == 10 || zz == 11) {                    /* rO, rS */
            emit("\tload\tr1, r24[3]");
            emit("\tsub\tr0, r27, r1");
            emit("\tlshift\tr0, 3");
            konst(1, 0x6000000000000000ULL);
            emit("\tadd\tr0, r1");
        } else emit("\tload\tr0, r22[%d]", zz);
        str_(0, xx);
        break;
    case 0xff:                                              /* TRIP */
        ldr(1, yy);
        ldr(2, zz);
        emit("\tpush\tr2");
        emit("\tpush\tr1");
        push_konst(inst);
        push_konst(cur_loc);
        emit("\tpush\t%d", 0x10000);
        emit("\tcall\t__c_mmx_exc");
        emit("\tadd\tsp, 5");
        emit("\tjump\tMX_TRIP0");
        break;
    default:
        illegal();
    }
}

/* ------------------------------------------------------------ fixed code */

static void gen_support(void) {
    int i;
    emit("");
    emit("// ---- shared code of the translated program");
    emit("// MX_GO: jumps to the MMIX address in r1");
    emit("MX_GO:");
    emit("\tand\tr1, -4");
    konst(2, jt_lo);
    emit("\tsub\tr2, r1, r2");
    emit("\ttest\tr2");
    emit("\tjump\tn, MX_GO_OUT");
    konst(3, jt_hi - jt_lo);
    emit("\tcmp\tr2, r3");
    emit("\tjump\tge, MX_GO_OUT");
    emit("\trshift\tr2, 2");
    emit("\tmove\tr3, MX_JT");
    emit("\tjump\tr3, r2");
    emit("MX_GO_OUT:");
    emit("\tpush\tr1");
    emit("\tcall\t__c_mmx_jump_out");
    emit("\tadd\tsp, 1");
    emit("\tjump\tMX_HALT");
    emit("");
    emit("// a trip: r0 = 1 + the handler address");
    emit("MX_TRIP0:");
    emit("\tsub\tr1, r0, 1");
    emit("\tjump\tMX_GO");
    emit("// a trip of mmx_op: r1 = 1 + the handler address");
    emit("MX_TRIP:");
    emit("\tmove\tr0, 0");
    emit("\tstore\tr0, __c_mmx_trip+1");
    emit("\tsub\tr1, 1");
    emit("\tjump\tMX_GO");
    emit("");
    emit("// executing a zero tetrabyte (TRAP 0,Halt,0)");
    emit("MX_HALT:");
    emit("\tpush\t0");
    emit("\tpush\t0");
    emit("\tpush\t0");
    emit("\tpush\t0");
    emit("\tcall\t__c_mmx_trap");
    emit("\tstop");
    emit("");
    emit("// errors (r1 = the MMIX address)");
    emit("MX_SEGV:");
    emit("\tpush\tr1");
    emit("\tcall\t__c_mmx_segv");
    emit("MX_ILLEGAL:");
    emit("\tpush\tr1");
    emit("\tcall\t__c_mmx_illegal");
    emit("MX_STKOVF:");
    emit("\tpush\t0");
    emit("\tpush\t0");
    emit("\tcall\t__c_mmx_unsupported");
    emit("MX_PUTG:");
    emit("\tpush\tr1");
    emit("\tpush\t1");
    emit("\tcall\t__c_mmx_unsupported");
    emit("MX_RESUME:");
    emit("\tpush\tr1");
    emit("\tpush\t2");
    emit("\tcall\t__c_mmx_unsupported");
    emit("MX_DATA:");
    emit("\tpush\tr1");
    emit("\tpush\t3");
    emit("\tcall\t__c_mmx_unsupported");
    emit("");
    emit("// MX_POP: POP r1,YZ with r2 = 4*YZ");
    emit("MX_POP:");
    emit("\tmove\tr5, 0");
    emit("\ttest\tr1");
    emit("\tjump\tz, MX_POP_1");
    emit("\tcmp\tr1, r26");
    emit("\tjump\tgt, MX_POP_1");
    emit("\tsub\tr3, r1, 1");
    emit("\tload\tr5, r27[r3]");
    emit("MX_POP_1:");
    emit("\tload\tr6, r27[-1]");
    emit("\tand\tr6, 255");
    emit("\tadd\tr7, r26, 1");
    emit("\tcmp\tr1, r7");
    emit("\tmove\tlt, r7, r1, r7");
    emit("\tadd\tr7, r6");
    emit("\tcmp\tr7, %d", G);
    emit("\tmove\tgt, r7, %d, r7", G);
    emit("\tsub\tr8, r27, r6");
    emit("\tsub\tr8, 1");
    emit("\tcmp\tr7, r6");
    emit("\tjump\tle, MX_POP_2");
    emit("\tstore\tr5, r27[-1]");
    emit("MX_POP_2:");
    emit("\tadd\tr9, r8, r7");
    emit("\tadd\tr10, r27, r26");
    emit("\tmove\tr11, 0");
    emit("MX_POP_3:");
    emit("\tcmp\tr9, r10");
    emit("\tjump\tge, MX_POP_4");
    emit("\tstore\tr11, r9[0]");
    emit("\tadd\tr9, 1");
    emit("\tjump\tMX_POP_3");
    emit("MX_POP_4:");
    emit("\tmove\tr27, r8");
    emit("\tmove\tr26, r7");
    emit("\tload\tr1, r22[4]");
    emit("\tadd\tr1, r2");
    emit("\tjump\tMX_GO");
    emit("");
    emit("// jump table: one entry per tetrabyte from #%llx to #%llx", jt_lo, jt_hi);
    emit("MX_JT:");
    {
        octa a;
        int k = 0;
        for (a = jt_lo; a < jt_hi; a += 4) {
            while (k < ncode && code[k]->addr < a) k++;
            if (k < ncode && code[k]->addr == a) emit("\tjump\tM_%llx", a);
            else emit("\tjump\tMX_HALT");
        }
    }
    emit("");
    emit("// constants");
    emit("MX_SIGN:\t.dci\t0x8000000000000000");
    {
        /* argv[0]: the source file name without directory and extension */
        const char *b = src_name, *p;
        char name[256];
        size_t n;
        for (p = src_name; *p; p++)
            if (*p == '/') b = p + 1;
        n = strlen(b);
        for (p = b + n; p > b; p--)
            if (p[-1] == '.') { n = (size_t)(p - 1 - b); break; }
        if (n > 200) n = 200;
        memcpy(name, b, n);
        name[n] = 0;
        emit("MX_NAME:\t.dcs\t\"%s\"", name);
    }
    for (i = 0; i < nlits; i++) emit("MX_K%d:\t.dci\t0x%llx", i, lits[i]);
}

/* the initial memory image: records of address, word count and words */
static void gen_image(void) {
    Tetra **all = xalloc((ntetras + 1) * sizeof(Tetra *));
    int i, n = 0;
    for (i = 0; i < ntetras; i++)
        if (tetras[i].val) all[n++] = &tetras[i];
    qsort(all, n, sizeof(Tetra *), cmp_tp);
    emit("");
    emit("// initial memory image: address, number of octabytes, octabytes");
    emit("MX_IMAGE:");
    i = 0;
    while (i < n) {
        octa start = all[i]->addr & ~7ULL, a = start, end;
        int j = i, k;
        /* a run: octabytes with gaps of at most 16 octabytes */
        while (j < n && (all[j]->addr >> 61) == (start >> 61) && (all[j]->addr & ~7ULL) <= a + 8 * 16) {
            a = all[j]->addr & ~7ULL;
            j++;
        }
        end = a + 8;
        emit("\t.dci\t0x%llx", start);
        emit("\t.dci\t%lld", (soct)((end - start) >> 3));
        k = i;
        for (a = start; a < end; a += 8) {
            octa v = 0;
            while (k < j && (all[k]->addr & ~7ULL) == a) {
                if (all[k]->addr & 4) v |= all[k]->val & M32;
                else v |= (all[k]->val & M32) << 32;
                k++;
            }
            emit("\t.dci\t0x%llx", v);
        }
        i = j;
    }
    emit("\t.dci\t0");
    emit("\t.dci\t0");
    free(all);
}

/* segment sizes: the image plus the room given by the options */
static void segment_sizes(void) {
    octa ext[4] = { 0, 0, 0, 0 };
    int i;
    for (i = 0; i < ntetras; i++) {
        octa a = tetras[i].addr, s = a >> 61, off = (a << 3) >> 3;
        if (s < 4 && off + 8 > ext[s]) ext[s] = off + 8;
    }
    seg_words[0] = (ext[0] >> 3) + 1024;
    seg_words[1] = (ext[1] >> 3) + (opt_data >> 3);
    seg_words[2] = (ext[2] >> 3) + (opt_pool >> 3);
    seg_words[3] = (ext[3] >> 3) + (opt_stack >> 3);
    if (seg_words[3] < 4096) seg_words[3] = 4096;
}

/* The source file for .source directives: its name as a string literal.
   The .asm is written next to the .mms, so the name is relative to it. */
static char *source_lit;
static int source_line;

static char *source_literal(void) {
    const char *b = src_name, *p;
    char *lit, *q;
    for (p = src_name; *p; p++)
        if (*p == '/') b = p + 1;
    lit = xalloc(2 * strlen(b) + 3);
    q = lit;
    *q++ = '"';
    for (p = b; *p; p++) {
        if (*p == '"' || *p == '\\') *q++ = '\\';
        *q++ = *p;
    }
    *q++ = '"';
    *q = 0;
    return lit;
}

int generate(const char *out_path, const char *rt_path) {
    int i;
    octa a;
    gout = fopen(out_path, "w");
    if (!gout) {
        fprintf(stderr, "mmixc: can't create %s\n", out_path);
        return 1;
    }
    G = greg;
    if (!main_defined) {
        fprintf(stderr, "\"%s\": the program does not define Main!\n", src_name);
        fclose(gout);
        remove(out_path);
        return 1;
    }

    /* the code: instruction tetras of the text segment */
    code = xalloc((ntetras + 1) * sizeof(Tetra *));
    ncode = 0;
    for (i = 0; i < ntetras; i++)
        if (tetras[i].is_inst && (tetras[i].addr >> 61) == 0) code[ncode++] = &tetras[i];
    qsort(code, ncode, sizeof(Tetra *), cmp_tp);
    jt_lo = ncode ? code[0]->addr : 0;
    jt_hi = ncode ? code[ncode - 1]->addr + 4 : 4;
    if ((jt_hi - jt_lo) >> 2 > 2000000) {
        fprintf(stderr, "\"%s\": the instructions are spread over too wide a range!\n", src_name);
        fclose(gout);
        remove(out_path);
        return 1;
    }
    segment_sizes();

    emit("// Generated by the CPUSim64 MMIX translator from %s", src_name);
    emit("#include \"%s/mmix_rt.asm\"", rt_path);
    emit("#include \"%s/mmix_io.asm\"", rt_path);
    emit("");
    emit("// ---- start: memory, image, registers");
    emit("__c_mmx_program:");
    emit("\tmove\tr0, MX_NAME");
    emit("\tpush\tr0");
    emit("\tpush\t%d", G);
    for (i = 3; i >= 0; i--) push_konst(seg_words[i]);
    emit("\tcall\t__c_mmx_init");
    emit("\tadd\tsp, 6");
    emit("\tmove\tr0, MX_IMAGE");
    emit("\tpush\tr0");
    emit("\tcall\t__c_mmx_load");
    emit("\tadd\tsp, 1");
    emit("\tmove\tr25, __c_mmx_seglim+1");
    emit("\tmove\tr24, __c_mmx_segbase+1");
    emit("\tmove\tr23, __c_mmx_g+1");
    emit("\tmove\tr22, __c_mmx_sr+1");
    emit("\tload\tr27, __c_mmx_rb+1");
    emit("\tload\tr26, __c_mmx_rl+1");
    emit("\tload\tr19, __c_mmx_stack_end+1");
    emit("\tload\tr20, MX_SIGN");
    for (i = G; i < 256; i++)
        if (greg_val[i]) {
            konst(0, greg_val[i]);
            str_(0, i);
        }
    emit("\tmove\tr0, %d", G);
    emit("\tstore\tr0, r22[19]");
    jump_to("", main_addr);

    emit("");
    emit("// ---- the program");
    source_lit = source_literal();
    for (i = 0; i < ncode; i++) {
        Tetra *t = code[i];
        /* the debugger shows the .mms line of each instruction */
        if (t->line > 0 && t->line != source_line) {
            emit(".source %s, %d", source_lit, t->line);
            source_line = t->line;
        }
        emit("M_%llx:\t\t\t\t// #%08llx  line %d", t->addr, t->val, t->line);
        gen_inst(t);
        a = t->addr + 4;
        if (i + 1 < ncode && code[i + 1]->addr == a) continue;
        /* the next tetrabyte is not an instruction */
        konst(1, a);
        if (tetra_at(a, 0)) emit("\tjump\tMX_DATA");
        else emit("\tjump\tMX_HALT");
    }
    if (source_line) emit(".source_end");
    if (stub_len) {
        emit("");
        emit("// ---- exceptions");
        fputs(stub_buf, gout);
    }
    gen_support();
    gen_image();
    fclose(gout);
    return 0;
}
