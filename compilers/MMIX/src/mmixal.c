/*
 * mmixal.c - the MMIXAL assembler front end of the translator.
 *
 * It assembles a program the way Knuth's mmixal does (same symbol rules,
 * PREFIX, local labels, GREG allocation and base addresses, future references
 * in relative addresses and OCTA, alignment, the same operand checks) and
 * produces the memory image that the MMIX loader would build from mmixal's
 * .mmo file: the tetrabytes, rG and the initial global registers.
 *
 * Included by mmixc.c.
 */
#include <ctype.h>

/* ---- operand bits of the opcode table (as in mmixal) */
#define rel_addr_bit  0x1
#define immed_bit     0x2
#define zar_bit       0x4
#define zr_bit        0x8
#define yar_bit       0x10
#define yr_bit        0x20
#define xar_bit       0x40
#define xr_bit        0x80
#define yzar_bit      0x100
#define yzr_bit       0x200
#define xyzar_bit     0x400
#define xyzr_bit      0x800
#define one_arg_bit   0x1000
#define two_arg_bit   0x2000
#define three_arg_bit 0x4000
#define many_arg_bit  0x8000
#define align_bits    0x30000
#define no_label_bit  0x40000
#define mem_bit       0x80000
#define spec_bit      0x100000

#define P_SET    0x100
#define P_IS     0x101
#define P_LOC    0x102
#define P_PREFIX 0x103
#define P_BSPEC  0x104
#define P_ESPEC  0x105
#define P_GREG   0x106
#define P_LOCAL  0x107
#define P_BYTE   0x108
#define P_WYDE   0x109
#define P_TETRA  0x10a
#define P_OCTA   0x10b

#define OP_SETH 0xe0
#define OP_ORH  0xe8
#define OP_ORL  0xeb

typedef struct { const char *name; int code; int bits; } OpSpec;

static OpSpec op_table[] = {
#include "optable.inc"
};

/* ---- symbols */

#define S_UNDEF    0
#define S_DEFINED  1
#define S_REGISTER 2
#define S_PREDEF   3

#define FIX_O   1       /* octabyte at loc gets the value */
#define FIX_YZ  2       /* relative YZ field of the tetra at loc */
#define FIX_XYZ 3       /* relative XYZ field of the tetra at loc */

typedef struct Fixup { int kind; octa loc; struct Fixup *next; } Fixup;

typedef struct Sym {
    char *name;         /* fully qualified, without the leading ':' */
    octa equiv;
    int state;
    int serial;
    Fixup *fix;         /* pending future references (state S_UNDEF) */
    struct Sym *next;   /* hash chain */
} Sym;

#define HASH_SIZE 4093
static Sym *sym_hash[HASH_SIZE];
static int serial_number = 0;

static int hash_name(const char *s) {
    octa h = 0;
    while (*s) { h = (h * 31 + (octa)(*s & 0xff)) % HASH_SIZE; s++; }
    return (int)h;
}

static Sym *sym_find(const char *name, int create) {
    int h = hash_name(name);
    Sym *p;
    for (p = sym_hash[h]; p; p = p->next)
        if (strcmp(p->name, name) == 0) return p;
    if (!create) return 0;
    p = xalloc(sizeof(Sym));
    p->name = xstrdup(name);
    p->state = S_UNDEF;
    p->serial = ++serial_number;
    p->next = sym_hash[h];
    sym_hash[h] = p;
    return p;
}

static void predefine(const char *name, octa v) {
    int h = hash_name(name);
    Sym *p = xalloc(sizeof(Sym));
    p->name = xstrdup(name);
    p->equiv = v;
    p->state = S_PREDEF;
    p->serial = 0;
    p->next = sym_hash[h];
    sym_hash[h] = p;
}

static const char *special_name[32] = {
    "rB", "rD", "rE", "rH", "rJ", "rM", "rR", "rBB", "rC", "rN", "rO", "rS", "rI",
    "rT", "rTT", "rK", "rQ", "rU", "rV", "rG", "rL", "rA", "rF", "rP", "rW", "rX",
    "rY", "rZ", "rWW", "rXX", "rYY", "rZZ"
};

typedef struct { const char *name; octa v; } Predef;

static Predef predefs[] = {
    {"ROUND_CURRENT", 0}, {"ROUND_OFF", 1}, {"ROUND_UP", 2}, {"ROUND_DOWN", 3}, {"ROUND_NEAR", 4},
    {"Inf", 0x7ff0000000000000ULL},
    {"Data_Segment", 0x2000000000000000ULL}, {"Pool_Segment", 0x4000000000000000ULL},
    {"Stack_Segment", 0x6000000000000000ULL},
    {"D_BIT", 0x80}, {"V_BIT", 0x40}, {"W_BIT", 0x20}, {"I_BIT", 0x10},
    {"O_BIT", 0x08}, {"U_BIT", 0x04}, {"Z_BIT", 0x02}, {"X_BIT", 0x01},
    {"D_Handler", 0x10}, {"V_Handler", 0x20}, {"W_Handler", 0x30}, {"I_Handler", 0x40},
    {"O_Handler", 0x50}, {"U_Handler", 0x60}, {"Z_Handler", 0x70}, {"X_Handler", 0x80},
    {"StdIn", 0}, {"StdOut", 1}, {"StdErr", 2},
    {"TextRead", 0}, {"TextWrite", 1}, {"BinaryRead", 2}, {"BinaryWrite", 3}, {"BinaryReadWrite", 4},
    {"Halt", 0}, {"Fopen", 1}, {"Fclose", 2}, {"Fread", 3}, {"Fgets", 4}, {"Fgetws", 5},
    {"Fwrite", 6}, {"Fputs", 7}, {"Fputws", 8}, {"Fseek", 9}, {"Ftell", 10}
};

/* local symbols 0H..9H */
static Sym forward_local[10];       /* future references to dF */
static Sym backward_local[10];      /* value of dB */

/* ---- the assembled image */
Tetra *tetras = 0;
int ntetras = 0;
static int tetra_cap = 0;
#define TETRA_HASH 8191
static int *tetra_index = 0;        /* open addressing on tetra addresses */
static int tetra_index_cap = 0;

static void tetra_rehash(void) {
    int i, cap = tetra_index_cap ? tetra_index_cap * 2 : TETRA_HASH * 2 + 1;
    int *idx = xalloc(sizeof(int) * cap);
    for (i = 0; i < cap; i++) idx[i] = -1;
    for (i = 0; i < ntetras; i++) {
        int h = (int)((tetras[i].addr >> 2) % (octa)cap);
        while (idx[h] >= 0) h = (h + 1) % cap;
        idx[h] = i;
    }
    if (tetra_index) free(tetra_index);
    tetra_index = idx;
    tetra_index_cap = cap;
}

Tetra *tetra_at(octa addr, int create) {
    int h;
    addr &= ~3ULL;
    if (!tetra_index) tetra_rehash();
    h = (int)((addr >> 2) % (octa)tetra_index_cap);
    while (tetra_index[h] >= 0) {
        if (tetras[tetra_index[h]].addr == addr) return &tetras[tetra_index[h]];
        h = (h + 1) % tetra_index_cap;
    }
    if (!create) return 0;
    if (ntetras >= tetra_cap) {
        int ncap = tetra_cap ? tetra_cap * 2 : 1024;
        Tetra *t = xalloc(sizeof(Tetra) * ncap);
        if (tetras) { memcpy(t, tetras, sizeof(Tetra) * ntetras); free(tetras); }
        tetras = t;
        tetra_cap = ncap;
    }
    tetras[ntetras].addr = addr;
    tetras[ntetras].val = 0;
    tetras[ntetras].line = 0;
    tetras[ntetras].is_inst = 0;
    tetra_index[h] = ntetras;
    ntetras++;
    if (ntetras * 2 > tetra_index_cap) tetra_rehash();
    return &tetras[ntetras - 1];
}

/* ---- assembler state */
int greg = 255;
octa greg_val[256];
int lreg = 32;
octa main_addr = 0;
int main_defined = 0;

static octa cur_loc = 0;
static char *cur_prefix = 0;        /* "" for ":" */
static int spec_mode = 0;
static int cur_greg = 0;

/* xor `k` bytes of value v into the image at cur_loc (big-endian) */
static void assemble(int k, octa v, int is_inst) {
    int i;
    for (i = k - 1; i >= 0; i--) {
        octa b = (v >> (8 * i)) & 0xff;
        if (!spec_mode) {
            Tetra *t = tetra_at(cur_loc, 1);
            int shift = (int)(3 - (cur_loc & 3)) * 8;
            t->val ^= b << shift;
            if (!t->line) t->line = line_no;
            if (is_inst) t->is_inst = 1;
        }
        cur_loc++;
    }
}

static void set_tetra(octa addr, octa v) {
    Tetra *t = tetra_at(addr, 1);
    t->val = v & M32;
}

/* ---- values of expressions */
#define V_PURE  0
#define V_REG   1
#define V_UNDEF 2

typedef struct { octa v; int status; Sym *sym; } Val;

#define MAXVALS 2000
static Val vals[MAXVALS];
static int nvals;

static int isletter(int c) { return isalpha(c) || c == '_' || c == ':' || c > 126; }

static char *scan_p;                /* scanning position in the operand list */
static int expr_error;

/* 128-bit by 64-bit unsigned division of (hi:lo) by d: quotient (low 64 bits) and remainder */
static octa div128(octa hi, octa lo, octa d, octa *rem) {
    octa q = 0, r = hi;
    int i;
    if (d == 0) { if (rem) *rem = lo; return 0; }
    if (r >= d) r %= d;             /* the quotient's high part is not needed */
    for (i = 63; i >= 0; i--) {
        int carry = (r >> 63) != 0;
        r = (r << 1) | ((lo >> i) & 1);
        if (carry || r >= d) { r -= d; q |= 1ULL << i; }
    }
    if (rem) *rem = r;
    return q;
}

static void lookup_symbol(char *start, char *end, Val *out) {
    char name[1000];
    size_t n = (size_t)(end - start);
    Sym *pp;
    if (n >= 900) n = 900;
    if (start[0] == ':') {
        memcpy(name, start + 1, n - 1);
        name[n - 1] = 0;
    } else {
        strcpy(name, cur_prefix);
        strncat(name, start, n);
    }
    pp = sym_find(name, 1);
    if (pp->state == S_PREDEF) pp->state = S_DEFINED;
    out->sym = pp;
    out->v = pp->equiv;
    out->status = pp->state == S_DEFINED ? V_PURE : pp->state == S_REGISTER ? V_REG : V_UNDEF;
}

static void push_val(Val v) {
    if (nvals >= MAXVALS) fatal("Capacity exceeded: too many operands");
    vals[nvals++] = v;
}

static void parse_expr(Val *r);

static void parse_primary(Val *r) {
    char *p = scan_p;
    r->sym = 0;
    r->status = V_PURE;
    r->v = 0;
    if (isletter(*p)) {
        char *q = p;
        while (isletter(*q) || isdigit(*q)) q++;
        lookup_symbol(p, q, r);
        scan_p = q;
        return;
    }
    if (isdigit(*p)) {
        if (p[1] == 'F') { *r = (Val){0}; r->sym = &forward_local[*p - '0']; r->status = V_UNDEF; scan_p = p + 2; return; }
        if (p[1] == 'B') {
            Sym *b = &backward_local[*p - '0'];
            r->sym = b; r->v = b->equiv; r->status = b->state == S_REGISTER ? V_REG : V_PURE;
            scan_p = p + 2;
            return;
        }
        while (isdigit(*p)) { r->v = r->v * 10 + (octa)(*p - '0'); p++; }
        scan_p = p;
        return;
    }
    switch (*p) {
    case '#':
        p++;
        if (!isxdigit(*p)) { err("illegal hexadecimal constant"); expr_error = 1; }
        while (isxdigit(*p)) {
            int d = isdigit(*p) ? *p - '0' : (tolower(*p) - 'a' + 10);
            r->v = (r->v << 4) + (octa)d;
            p++;
        }
        scan_p = p;
        return;
    case '\'':
        r->v = (octa)(p[1] & 0xff);
        scan_p = p + 3;
        return;
    case '"':
        /* "abc" stands for 'a',"bc": the first character ends the current
           operand and the rest of the string is rescanned as the next
           operands (mmixal rewrites its operand buffer the same way) */
        p++;
        if (*p == '"') { warn("null string is treated as zero"); r->v = 0; scan_p = p + 1; return; }
        r->v = (octa)(*p & 0xff);
        if (p[1] == '"') { scan_p = p + 2; return; }
        if (!p[1]) { scan_p = p + 1; return; }
        p[0] = '"';
        p[-1] = ',';
        scan_p = p - 1;
        return;
    case '@':
        r->v = cur_loc;
        scan_p = p + 1;
        return;
    case '(':
        scan_p = p + 1;
        parse_expr(r);
        if (*scan_p == ')') scan_p++;
        else warn("missing right parenthesis");
        return;
    }
    if (*p) errs("syntax error at character `%s'", (char[]){*p, 0});
    else err("syntax error at end of operand");
    expr_error = 1;
    scan_p = p + (*p ? 1 : 0);
}

static void parse_unary(Val *r) {
    char c = *scan_p;
    if (c == '+') { scan_p++; parse_unary(r); return; }
    if (c == '-' || c == '~' || c == '$' || c == '&') {
        scan_p++;
        parse_unary(r);
        if (c == '&') {
            if (!r->sym) err("can take serial number of symbol only");
            else r->v = (octa)r->sym->serial;
            r->status = V_PURE;
        } else {
            if (r->status != V_PURE) {
                errs("can %s pure values only", c == '-' ? "negate" : c == '~' ? "complement" : "registerize");
            }
            if (c == '-') r->v = 0 - r->v;
            else if (c == '~') r->v = ~r->v;
            else r->status = V_REG;
        }
        r->sym = 0;
        return;
    }
    parse_primary(r);
}

static void binary_check(Val *a, Val *b, const char *what) {
    if (a->status != V_PURE || b->status != V_PURE) errs("can %s pure values only", what);
}

static void parse_term(Val *r) {
    parse_unary(r);
    for (;;) {
        char c = *scan_p;
        int op;
        Val b;
        if (c == '*') op = '*';
        else if (c == '/' && scan_p[1] == '/') op = 'f';
        else if (c == '/') op = '/';
        else if (c == '%') op = '%';
        else if (c == '<' && scan_p[1] == '<') op = '<';
        else if (c == '>' && scan_p[1] == '>') op = '>';
        else if (c == '&') op = '&';
        else return;
        scan_p += (op == 'f' || op == '<' || op == '>') ? 2 : 1;
        parse_unary(&b);
        switch (op) {
        case '*': binary_check(r, &b, "multiply"); r->v = r->v * b.v; break;
        case '/': case '%': {
            octa rem;
            binary_check(r, &b, "divide");
            if (b.v == 0) warn("division by zero");
            r->v = div128(0, r->v, b.v, &rem);
            if (op == '%') r->v = rem;
            break;
        }
        case 'f':
            binary_check(r, &b, "compute a ratio of");
            if (r->v >= b.v) warn("illegal fraction");
            r->v = div128(r->v, 0, b.v, 0);
            break;
        case '<': case '>':
            binary_check(r, &b, "compute a bitwise shift of");
            if (b.v > 63) r->v = 0;
            else if (op == '<') r->v = r->v << b.v;
            else r->v = r->v >> b.v;
            break;
        case '&': binary_check(r, &b, "compute bitwise and of"); r->v &= b.v; break;
        }
        r->status = (r->status == b.status) ? V_PURE : V_REG;
        r->sym = 0;
    }
}

static void parse_expr(Val *r) {
    parse_term(r);
    for (;;) {
        char c = *scan_p;
        Val b;
        if (c != '+' && c != '-' && c != '|' && c != '^') return;
        scan_p++;
        parse_term(&b);
        if (c == '+') {
            if (b.status == V_UNDEF) err("cannot add an undefined quantity");
            if (r->status == V_UNDEF) err("cannot add to an undefined quantity");
            if (r->status == V_REG && b.status == V_REG) err("cannot add two register numbers");
            r->v = r->v + b.v;
        } else if (c == '-') {
            if (b.status == V_UNDEF) err("cannot subtract an undefined quantity");
            if (r->status == V_UNDEF) err("cannot subtract from an undefined quantity");
            if (b.status == V_REG && r->status != V_REG) err("cannot subtract register number from pure value");
            r->v = r->v - b.v;
        } else if (c == '|') {
            binary_check(r, &b, "compute bitwise or of");
            r->v |= b.v;
        } else {
            binary_check(r, &b, "compute bitwise xor of");
            r->v ^= b.v;
        }
        r->status = (r->status == b.status) ? V_PURE : V_REG;
        r->sym = 0;
    }
}

/* Evaluates the comma-separated operand list into vals[0..nvals-1]. */
static void parse_operands(char *list) {
    nvals = 0;
    expr_error = 0;
    /* an operand field that cannot start an expression is a remark: the
       operand is 0 (mmixal: "a GREG % comment") */
    if (!(isletter(*list) || isdigit(*list) || strchr("#'\"@-+&~$(", *list))) {
        list[0] = '0';
        list[1] = 0;
    }
    scan_p = list;
    for (;;) {
        Val v;
        parse_expr(&v);
        if (v.status == V_REG && v.v > 0xff) {
            warn("register number too large, will be reduced mod 256");
            v.v &= 0xff;
        }
        push_val(v);
        if (*scan_p == ',') { scan_p++; continue; }
        if (*scan_p == ')') { warn("missing left parenthesis"); scan_p++; continue; }
        if (*scan_p && !expr_error) errs("syntax error at `%s'", (char[]){*scan_p, 0});
        break;
    }
}

/* ---- fixups of future references */

static void add_fixup(Sym *s, int kind, octa loc) {
    Fixup *f = xalloc(sizeof(Fixup));
    f->kind = kind;
    f->loc = loc;
    f->next = s->fix;
    s->fix = f;
}

static void resolve_fixups(Sym *s, octa value) {
    Fixup *f;
    for (f = s->fix; f; f = f->next) {
        if (f->kind == FIX_O) {
            set_tetra(f->loc, value >> 32);
            set_tetra(f->loc + 4, value & M32);
        } else {
            Tetra *t = tetra_at(f->loc, 1);
            soct delta;
            if ((value - f->loc) & 3) warn("relative address not divisible by 4");
            delta = (soct)(value - f->loc) >> 2;
            if (f->kind == FIX_YZ) {
                if (delta >= 0 && delta < 0x10000) t->val = (t->val & ~0xffffULL) | (octa)delta;
                else if (delta < 0 && delta >= -0x10000) t->val = ((t->val & ~0xffffULL) | (octa)(delta + 0x10000)) ^ 0x01000000ULL;
                else err("relative address is too far away");
            } else {
                if (delta >= 0 && delta < 0x1000000) t->val = (t->val & ~0xffffffULL) | (octa)delta;
                else if (delta < 0 && delta >= -0x1000000) t->val = ((t->val & ~0xffffffULL) | (octa)(delta + 0x1000000)) ^ 0x01000000ULL;
                else err("relative address is too far away");
            }
        }
    }
    s->fix = 0;
}

/* ---- one statement */

static OpSpec *find_op(const char *name) {
    int i, n = (int)(sizeof op_table / sizeof op_table[0]);
    for (i = 0; i < n; i++) if (strcmp(op_table[i].name, name) == 0) return &op_table[i];
    return 0;
}

static octa rel_field(octa target, int bits, int *opcode, const char *what) {
    soct delta;
    if (target & 3) warn("relative address is not divisible by 4");
    delta = (soct)((target >> 2) - (cur_loc >> 2));
    if (delta >= 0) {
        if (delta >= (1LL << bits)) err(what);
        return (octa)delta & ((1ULL << bits) - 1);
    }
    (*opcode)++;
    if (delta < -(1LL << bits)) err(what);
    return (octa)(delta + (1LL << bits)) & ((1ULL << bits) - 1);
}

static void do_statement(char *label, char *opname, char *operands) {
    OpSpec *op;
    int opcode, bits, j;
    octa x = 0, y = 0, z = 0, yz = 0, xyz = 0;
    if (!opname[0]) {
        if (label[0]) { char m[300]; snprintf(m, sizeof m, "no opcode; label `%s' will be ignored", label); warn(m); }
        return;
    }
    op = find_op(opname);
    if (!op) { errs("unknown operation code `%s'", opname); return; }
    opcode = op->code;
    bits = op->bits;
    if (spec_mode && !(bits & spec_bit)) { errs("cannot use `%s' in special mode", opname); return; }
    if ((bits & no_label_bit) && label[0]) {
        char m[300]; snprintf(m, sizeof m, "label field of `%s' instruction is ignored", opname); warn(m);
        label[0] = 0;
    }
    if (bits & align_bits) {
        int k = (bits & align_bits) >> 16;
        cur_loc = (cur_loc + (1ULL << k) - 1) & ~((1ULL << k) - 1);
    }
    parse_operands(operands);

    if (opcode == P_GREG) {
        int found = 0;
        if (vals[0].v) {
            for (j = greg; j < 255; j++) if (greg_val[j] == vals[0].v) { cur_greg = j; found = 1; break; }
        }
        if (!found) {
            if (greg == 32) err("too many global registers");
            greg--;
            greg_val[greg] = vals[0].v;
            cur_greg = greg;
        }
    }

    /* ---- the label */
    if (label[0]) {
        int new_state = S_DEFINED;
        octa value = cur_loc;
        Sym *pp;
        if (opcode == P_IS) {
            if (vals[0].status == V_UNDEF) err("the operand is undefined");
            value = vals[0].v;
            if (vals[0].status == V_REG) new_state = S_REGISTER;
        } else if (opcode == P_GREG) {
            value = (octa)cur_greg;
            new_state = S_REGISTER;
        }
        if (isdigit(label[0])) pp = &forward_local[label[0] - '0'];
        else {
            char name[1000];
            if (label[0] == ':') strcpy(name, label + 1);
            else { strcpy(name, cur_prefix); strcat(name, label); }
            pp = sym_find(name, 1);
        }
        if (pp->state == S_DEFINED || pp->state == S_REGISTER) {
            if (pp->equiv != value || pp->state != new_state) {
                if (pp->serial) errs("symbol `%s' is already defined", label);
                else { char m[300]; snprintf(m, sizeof m, "redefinition of predefined symbol `%s'", label); warn(m); }
                pp->serial = ++serial_number;
            }
        } else if (pp->state == S_PREDEF) {
            pp->serial = ++serial_number;
        } else if (pp->fix) {
            if (new_state == S_REGISTER) err("future reference cannot be to a register");
            resolve_fixups(pp, value);
        }
        if (isdigit(label[0])) pp = &backward_local[label[0] - '0'];
        pp->equiv = value;
        pp->state = new_state;
        if (!isdigit(label[0])) {
            for (j = 0; j < nvals; j++)
                if (vals[j].status == V_UNDEF && vals[j].sym == pp) {
                    vals[j].status = new_state == S_REGISTER ? V_REG : V_PURE;
                    vals[j].v = value;
                }
        }
    }

    /* ---- data directives */
    if (bits & many_arg_bit) {
        int k = 1 << (opcode - P_BYTE);
        for (j = 0; j < nvals; j++) {
            if (vals[j].status == V_REG) warn("register number used as a constant");
            if (vals[j].status == V_UNDEF && opcode != P_OCTA) { err("undefined constant"); continue; }
            if ((vals[j].v >> 32 && opcode < P_OCTA) || ((vals[j].v & M32) > 0xffff && opcode < P_TETRA)
                || ((vals[j].v & M32) > 0xff && opcode < P_WYDE)) {
                if (k == 1) warn("constant doesn't fit in one byte");
                else { char m[80]; snprintf(m, sizeof m, "constant doesn't fit in %d bytes", k); warn(m); }
            }
            if (k < 8) assemble(k, vals[j].v & M32, 0);
            else if (vals[j].status == V_UNDEF) {
                add_fixup(vals[j].sym, FIX_O, cur_loc);
                assemble(4, 0, 0);
                assemble(4, 0, 0);
            } else assemble(8, vals[j].v, 0);
        }
        return;
    }

    switch (nvals) {
    case 1:
        if (!(bits & one_arg_bit)) { errs("opcode `%s' needs more than one operand", opname); return; }
        if (vals[0].status == V_UNDEF) {
            if (bits & rel_addr_bit) {
                add_fixup(vals[0].sym, FIX_XYZ, cur_loc);
                xyz = 0;
                goto assemble_inst;
            } else if (opcode != P_PREFIX) { err("the operand is undefined"); return; }
        } else if (vals[0].status == V_REG) {
            if (!(bits & (xyzr_bit + xyzar_bit))) { char m[300]; snprintf(m, sizeof m, "operand of `%s' should not be a register number", opname); warn(m); }
        } else {
            if (bits & xyzr_bit) { char m[300]; snprintf(m, sizeof m, "operand of `%s' should be a register number", opname); warn(m); }
            if (bits & rel_addr_bit) {
                xyz = rel_field(vals[0].v, 24, &opcode, "relative address is too far away");
                goto assemble_inst;
            }
        }
        if (opcode > 0xff) {
            switch (opcode) {
            case P_LOC: cur_loc = vals[0].v; return;
            case P_IS: return;
            case P_PREFIX:
                if (!vals[0].sym) { err("not a valid prefix"); return; }
                free(cur_prefix);
                cur_prefix = xstrdup(vals[0].sym->name);
                return;
            case P_GREG: return;
            case P_LOCAL: if (vals[0].v > (octa)lreg) lreg = (int)vals[0].v; return;
            case P_BSPEC:
                if (vals[0].v > 0xffff) warn("operand of `BSPEC' doesn't fit in two bytes");
                spec_mode = 1;
                return;
            case P_ESPEC: spec_mode = 0; return;
            }
        }
        if (vals[0].v > 0xffffff) warn("XYZ field doesn't fit in three bytes");
        xyz = vals[0].v & 0xffffff;
        goto assemble_inst;
    case 2:
        if (!(bits & two_arg_bit)) {
            if (bits & one_arg_bit) errs("opcode `%s' must not have two operands", opname);
            else errs("opcode `%s' must have more than two operands", opname);
            return;
        }
        if ((bits & (three_arg_bit + mem_bit)) == three_arg_bit) goto make_two_three;
        if (vals[1].status == V_UNDEF) {
            if (bits & rel_addr_bit) {
                add_fixup(vals[1].sym, FIX_YZ, cur_loc);
                yz = 0;
                goto assemble_X;
            }
            err("YZ field is undefined");
            return;
        } else if (vals[1].status == V_REG) {
            if (!(bits & (immed_bit + yzr_bit + yzar_bit))) { char m[300]; snprintf(m, sizeof m, "YZ field of `%s' should not be a register number", opname); warn(m); }
            if (opcode == P_SET) vals[1].v <<= 8, opcode = 0xc1;
            else if (bits & mem_bit) vals[1].v <<= 8, opcode++;
        } else {
            if (bits & mem_bit) {
                octa o = vals[1].v, acc;
                int k = 0;
                for (j = greg; j < 255; j++) {
                    if (greg_val[j]) {
                        acc = vals[1].v - greg_val[j];
                        if (acc <= o) o = acc, k = j;
                    }
                }
                if (o <= 0xff && k) { yz = ((octa)k << 8) + o; opcode++; }
                else { err("no base address is close enough to the address A"); return; }
                goto assemble_X;
            }
            if (opcode == P_SET) opcode = 0xe3;
            else if (bits & immed_bit) opcode++;
            else if (bits & yzr_bit) { char m[300]; snprintf(m, sizeof m, "YZ field of `%s' should be a register number", opname); warn(m); }
            if (bits & rel_addr_bit) {
                yz = rel_field(vals[1].v, 16, &opcode, "relative address is too far away");
                goto assemble_X;
            }
        }
        if (vals[1].v > 0xffff) warn("YZ field doesn't fit in two bytes");
        yz = vals[1].v & 0xffff;
        goto assemble_X;
    make_two_three:
        vals[2] = vals[1];
        nvals = 3;
        vals[1].v = 0;
        vals[1].sym = 0;
        vals[1].status = V_PURE;
        /* fall through */
    case 3:
        if (!(bits & three_arg_bit)) { errs("opcode `%s' must not have three operands", opname); return; }
        if (vals[2].status == V_UNDEF) { err("Z field is undefined"); return; }
        if (vals[2].status == V_REG) {
            if (!(bits & (immed_bit + zr_bit + zar_bit))) { char m[300]; snprintf(m, sizeof m, "Z field of `%s' should not be a register number", opname); warn(m); }
        } else if (bits & immed_bit) opcode++;
        else if (bits & zr_bit) { char m[300]; snprintf(m, sizeof m, "Z field of `%s' should be a register number", opname); warn(m); }
        if (vals[2].v > 0xff) warn("Z field doesn't fit in one byte");
        z = vals[2].v & 0xff;
        if (vals[1].status == V_UNDEF) { err("Y field is undefined"); return; }
        if (vals[1].status == V_REG) {
            if (!(bits & (yr_bit + yar_bit))) { char m[300]; snprintf(m, sizeof m, "Y field of `%s' should not be a register number", opname); warn(m); }
        } else if (bits & yr_bit) { char m[300]; snprintf(m, sizeof m, "Y field of `%s' should be a register number", opname); warn(m); }
        if (vals[1].v > 0xff) warn("Y field doesn't fit in one byte");
        y = vals[1].v & 0xff;
        yz = (y << 8) + z;
        goto assemble_X;
    default:
        errs("too many operands for opcode `%s'", opname);
        return;
    }
assemble_X:
    if (vals[0].status == V_UNDEF) { err("X field is undefined"); return; }
    if (vals[0].status == V_REG) {
        if (!(bits & (xr_bit + xar_bit))) { char m[300]; snprintf(m, sizeof m, "X field of `%s' should not be a register number", opname); warn(m); }
    } else if (bits & xr_bit) { char m[300]; snprintf(m, sizeof m, "X field of `%s' should be a register number", opname); warn(m); }
    if (vals[0].v > 0xff) warn("X field doesn't fit in one byte");
    x = vals[0].v & 0xff;
    xyz = (x << 16) + yz;
assemble_inst:
    if (opcode > 0xff) { errs("operation `%s' not allowed here", opname); return; }
    assemble(4, ((octa)opcode << 24) + xyz, 1);
}

/* ---- lines */

#define BUF_SIZE 72

static int read_line(FILE *f, char *buf, int *truncated) {
    int c, n = 0;
    *truncated = 0;
    c = getc(f);
    if (c == EOF) return 0;
    while (c != EOF && c != '\n') {
        if (n < BUF_SIZE) buf[n++] = (char)c;
        else *truncated = 1;
        c = getc(f);
    }
    if (n > 0 && buf[n - 1] == '\r') n--;
    buf[n] = 0;
    return 1;
}

static void process_line(char *buffer) {
    char *p = buffer, *q;
    char lab[BUF_SIZE + 2], opf[BUF_SIZE + 2], operands[BUF_SIZE * 2 + 4];
    while (1) {
        if (!*p) return;
        q = lab;
        if (!isspace(*p)) {
            if (!isdigit(*p) && !isletter(*p)) return;      /* a comment */
            *q++ = *p++;
            while (isdigit(*p) || isletter(*p)) *q++ = *p++;
            if (*p && !isspace(*p)) { errs("label syntax error at `%s'", (char[]){*p, 0}); }
        }
        *q = 0;
        if (isdigit(lab[0]) && (lab[1] != 'H' || lab[2])) errs("improper local label `%s'", lab);
        if (*p) p++;
        while (isspace(*p)) p++;
        q = opf;
        while (isletter(*p) || isdigit(*p)) *q++ = *p++;
        *q = 0;
        if (!isspace(*p) && *p && opf[0]) { errs("opcode syntax error at `%s'", (char[]){*p, 0}); return; }
        while (isspace(*p)) p++;
        q = operands;
        while (*p) {
            if (*p == ';') break;
            if (*p == '\'') {
                *q++ = *p++;
                if (!*p) err("incomplete character constant");
                *q++ = *p++;
                if (*p != '\'') err("illegal character constant");
            } else if (*p == '"') {
                for (*q++ = *p++; *p && *p != '"'; p++, q++) *q = *p;
                if (!*p) err("incomplete string constant");
            }
            if (!*p) break;
            *q++ = *p++;
            if (isspace(*p)) break;
        }
        while (isspace(*p)) p++;
        if (*p == ';') p++;
        else p = "";
        if (q == operands) *q++ = '0';
        *q = 0;
        do_statement(lab, opf, operands);
    }
}

int assemble_file(FILE *f) {
    char buffer[BUF_SIZE + 2];
    int i, truncated, warned = 0;
    int n = (int)(sizeof predefs / sizeof predefs[0]);
    cur_prefix = xstrdup("");
    sym_find("Main", 1);                    /* serial number 1 */
    for (i = 0; i < 32; i++) predefine(special_name[i], (octa)i);
    for (i = 0; i < n; i++) predefine(predefs[i].name, predefs[i].v);
    for (i = 0; i < 10; i++) {
        backward_local[i].state = S_DEFINED;
        backward_local[i].equiv = 0;
        forward_local[i].state = S_UNDEF;
    }
    while (read_line(f, buffer, &truncated)) {
        line_no++;
        if (truncated) {
            if (!warned) { warn("trailing characters of long input line have been dropped"); warned = 1; }
            else warn("trailing characters dropped");
        }
        if (buffer[0] == '#') {             /* line directive: # <line> "file" */
            char *p = buffer + 1;
            int k = 0;
            while (isspace(*p)) p++;
            while (isdigit(*p)) k = k * 10 + (*p++ - '0');
            if (k) line_no = k - 1;
            continue;
        }
        process_line(buffer);
    }
    if (lreg >= greg) {
        char m[100];
        snprintf(m, sizeof m, "Danger: Must reduce the number of GREGs by %d", lreg - greg + 1);
        fatal(m);
    }
    {
        Sym *m = sym_find("Main", 0);
        if (m && m->state == S_DEFINED) { main_addr = m->equiv; main_defined = 1; }
        greg_val[255] = main_defined ? main_addr : 0;
    }
    /* undefined symbols */
    for (i = 0; i < HASH_SIZE; i++) {
        Sym *s;
        for (s = sym_hash[i]; s; s = s->next) {
            if (s->state == S_UNDEF && (s->fix || s->serial == 1)) {
                fprintf(stderr, "undefined symbol: %s\n", s->name);
                err_count++;
            }
        }
    }
    for (i = 0; i < 10; i++) if (forward_local[i].fix) {
        fprintf(stderr, "undefined local symbol %dF\n", i);
        err_count++;
    }
    return err_count;
}

static int cmp_tetra(const void *a, const void *b) {
    const Tetra *x = a, *y = b;
    return x->addr < y->addr ? -1 : x->addr > y->addr ? 1 : 0;
}

void dump_image(FILE *out) {
    int i;
    qsort(tetras, ntetras, sizeof(Tetra), cmp_tetra);
    tetra_rehash();
    fprintf(out, "G %d\n", greg);
    for (i = greg; i < 256; i++) fprintf(out, "$%d %016llx\n", i, greg_val[i]);
    for (i = 0; i < ntetras; i++)
        if (tetras[i].val) fprintf(out, "%016llx %08llx\n", tetras[i].addr, tetras[i].val);
}
