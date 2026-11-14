/*
 * mmix_rt.c - run-time support for translated MMIX programs.
 *
 * build.sh compiles this file with the CPUSim64 C11 compiler (together with
 * mmix_stub.c, which only declares the assembly functions) into
 * mmix_rt.asm.  A translated program includes mmix_rt.asm and mmix_io.asm.
 *
 * Program start: main() (here) saves the command line and calls
 * mmx_program(), the translated program, which calls mmx_init() and
 * mmx_load() and then runs the MMIX code.  The translated code calls the
 * functions below for the operations that are too long to expand in line:
 *   mmx_op     multiply, divide, floating point, MOR/MXOR and the like
 *   mmx_exc    an arithmetic exception of an in-line instruction
 *   mmx_trap   TRAP (the MMIXware simulator's system calls)
 *   mmx_save, mmx_unsave
 *
 * The floating-point code is a port of Knuth's mmix-arith (MMIXware) to
 * 64-bit integers, so that results, rounding and exception flags are those
 * of the real MMIX simulator.
 *
 * Portable C without library functions: the code is also compiled natively
 * for unit tests.  CPUSim64 chars, ints and longs are all 64 bits wide.
 */

typedef unsigned long long octa;
typedef long long soct;

/* ---- provided by mmix_io.asm (and the translated program) */
long mmx_program(void);
long mmx_putb(long port, long b);
long mmx_getb(long port);
long mmx_open(octa *packed_name);
long mmx_isfile(octa *packed_name);
long mmx_delete(octa *packed_name);
long mmx_close(long port);
long mmx_seek(long port, long pos);
long mmx_tell(long port);
octa *mmx_alloc(long words);
long mmx_exit(long code);
long mmx_argc(void);
octa *mmx_arg(long i);

/* ---- machine state shared with the translated code */
octa mmx_g[256];        /* global registers $rG..$255 */
octa mmx_sr[32];        /* special registers, numbered as in MMIX */
octa mmx_segbase[8];    /* address of the first word of segment s */
octa mmx_seglim[8];     /* size of segment s in words (0: not present) */
octa mmx_rb;            /* address of local register $0 (rO as a word address) */
octa mmx_rl;            /* rL, for mmx_save and mmx_unsave */
octa mmx_rg;             /* rG (fixed when the program is translated) */
octa mmx_trip;          /* 1 + handler address of a pending trip, else 0 */
octa mmx_stack_end;     /* the register stack must stay below this address */

#define rB 0
#define rD 1
#define rE 2
#define rH 3
#define rJ 4
#define rM 5
#define rR 6
#define rBB 7
#define rC 8
#define rN 9
#define rO 10
#define rS 11
#define rI 12
#define rT 13
#define rTT 14
#define rK 15
#define rQ 16
#define rU 17
#define rV 18
#define rG 19
#define rL 20
#define rA 21
#define rF 22
#define rP 23
#define rW 24
#define rX 25
#define rY 26
#define rZ 27
#define rWW 28
#define rXX 29
#define rYY 30
#define rZZ 31

#define SIGN 0x8000000000000000ULL
#define M32 0xffffffffULL
#define ALL1 0xffffffffffffffffULL

/* exception bits, as in mmix-arith (the event bits of rA shifted left 8) */
#define X_BIT (1 << 8)
#define Z_BIT (1 << 9)
#define U_BIT (1 << 10)
#define O_BIT (1 << 11)
#define I_BIT (1 << 12)
#define W_BIT (1 << 13)
#define V_BIT (1 << 14)
#define D_BIT (1 << 15)
#define H_BIT (1 << 16)
#define E_BIT (1 << 18)

#define ROUND_OFF 1
#define ROUND_UP 2
#define ROUND_DOWN 3
#define ROUND_NEAR 4

/* ------------------------------------------------------------ messages */

static void mmx_put_str(long port, const char *s) {
    while (*s) { mmx_putb(port, *s & 255); s++; }
}

static void mmx_put_hex(long port, octa v) {
    int i, started = 0;
    mmx_putb(port, '#');
    for (i = 60; i >= 0; i -= 4) {
        int d = (int)((v >> i) & 15);
        if (d || started || i == 0) {
            mmx_putb(port, d < 10 ? '0' + d : 'a' + d - 10);
            started = 1;
        }
    }
}

/* Reports a run-time error ("mmix: MSG #ADDR") and stops the program. */
void mmx_fatal(const char *msg, octa addr) {
    mmx_put_str(2, "mmix: ");
    mmx_put_str(2, msg);
    mmx_putb(2, ' ');
    mmx_put_hex(2, addr);
    mmx_putb(2, 10);
    mmx_exit(1);
}

/* An instruction that the MMIX simulator would stop at. */
void mmx_illegal(octa loc) { mmx_fatal("illegal or privileged instruction at", loc); }

void mmx_segv(octa a) { mmx_fatal("memory address out of range:", a); }

/* Something the translation does not support (code), at MMIX address a. */
void mmx_unsupported(octa code, octa a) {
    switch (code) {
    case 0: mmx_fatal("register stack overflow (use a larger --stack); rO =", a); break;
    case 1: mmx_fatal("PUT rG with a new value is not supported, at", a); break;
    case 2: mmx_fatal("RESUME of an instruction in rX is not supported, at", a); break;
    default: mmx_fatal("the program executed data, at", a); break;
    }
}

/* A jump to MMIX address a, outside the translated instructions: memory
   that holds no instruction is a zero tetrabyte, which is TRAP 0,Halt,0 in
   the text segment; elsewhere MMIX would stop. */
void mmx_jump_out(octa a) {
    if (a >> 61) mmx_fatal("jump outside the text segment, to", a);
}

/* ------------------------------------------------------------ memory */

/* The word that holds the byte at MMIX address a. */
static octa *mmx_word(octa a) {
    octa s = a >> 61, w = (a << 3) >> 6;
    if (w >= mmx_seglim[s]) mmx_fatal("memory address out of range:", a);
    return (octa *)mmx_segbase[s] + w;
}

static octa mmx_get8(octa a) { return *mmx_word(a); }
static void mmx_put8(octa a, octa v) { *mmx_word(a) = v; }

static octa mmx_get1(octa a) {
    octa w = *mmx_word(a);
    return (w >> (56 - 8 * (int)(a & 7))) & 255;
}

static void mmx_put1(octa a, octa b) {
    octa *p = mmx_word(a);
    int sh = 56 - 8 * (int)(a & 7);
    *p = (*p & ~(255ULL << sh)) | ((b & 255) << sh);
}

/* MMIX address of a word of the memory area (the inverse of mmx_word). */
static octa mmx_addr_of(octa *p) {
    int s;
    for (s = 0; s < 4; s++) {
        octa *b = (octa *)mmx_segbase[s];
        if (mmx_seglim[s] && p >= b && p < b + mmx_seglim[s])
            return ((octa)s << 61) + 8 * (octa)(p - b);
    }
    mmx_fatal("register stack outside the memory area:", 0);
    return 0;
}

/* ------------------------------------------------------------ start-up */

static long mmx_nargs;

int main(void) {
    mmx_nargs = mmx_argc();
    return (int)mmx_program();
}

/* Copies a CPUSim64 packed string (byte count, then 8 bytes per word) into
   MMIX memory at a with a terminating zero; returns its length. */
static octa mmx_put_packed(octa a, octa *ps) {
    octa n = ps[0], i;
    for (i = 0; i < n; i++) {
        octa w = ps[1 + (i >> 3)];
        mmx_put1(a + i, (w >> (56 - 8 * (int)(i & 7))) & 255);
    }
    mmx_put1(a + n, 0);
    return n;
}

/* Allocates the memory: the text, data, pool and stack segments get the
   given numbers of words.  Puts the command line into the pool segment as
   the MMIX simulator does and sets the registers for the start of the
   program ($0 = argc, $1 = argv, rL = 2).  argv[0] is name (a packed
   string), the name of the program as MMIX users run it. */
void mmx_init(octa textw, octa dataw, octa poolw, octa stackw, octa G, octa *name) {
    octa total = textw + dataw + poolw + stackw, x, loc, k;
    octa *mem = mmx_alloc((long)total);
    if (!mem) mmx_fatal("not enough memory for the MMIX segments, words:", total);
    mmx_segbase[0] = (octa)mem;
    mmx_segbase[1] = (octa)(mem + textw);
    mmx_segbase[2] = (octa)(mem + textw + dataw);
    mmx_segbase[3] = (octa)(mem + textw + dataw + poolw);
    mmx_seglim[0] = textw;
    mmx_seglim[1] = dataw;
    mmx_seglim[2] = poolw;
    mmx_seglim[3] = stackw;
    mmx_rg = G;
    mmx_rb = mmx_segbase[3];
    mmx_stack_end = (octa)(mem + textw + dataw + poolw + stackw - 260);
    mmx_sr[rN] = 0x010001006aa63ae1ULL;
    mmx_sr[rT] = 0x8000000500000000ULL;
    mmx_sr[rTT] = 0x8000000600000000ULL;
    mmx_sr[rV] = 0x369c200400000000ULL;
    mmx_sr[rG] = G;
    /* command line: M[Pool+8+8k] = argv[k], the strings follow */
    x = 0x4000000000000008ULL;
    loc = x + 8 * (octa)(mmx_nargs + 1);
    for (k = 0; k < (octa)mmx_nargs; k++) {
        octa n;
        mmx_put8(x, loc);
        n = mmx_put_packed(loc, k ? mmx_arg((long)k) : name);
        x += 8;
        loc += 8 + (n & ~7ULL);
    }
    mmx_put8(0x4000000000000000ULL, loc);
    ((octa *)mmx_rb)[0] = (octa)mmx_nargs;
    ((octa *)mmx_rb)[1] = 0x4000000000000008ULL;
    mmx_rl = 2;
}

/* Copies the initial memory image: records of an MMIX address, a word
   count n and n words, ending with a record whose count is 0. */
void mmx_load(octa *t) {
    while (t[1]) {
        octa a = t[0], n = t[1], i;
        octa *p = mmx_word(a);
        mmx_word(a + 8 * (n - 1));          /* checks the end of the run */
        for (i = 0; i < n; i++) p[i] = t[2 + i];
        t += 2 + n;
    }
}

/* ------------------------------------------------------------ exceptions */

/* Handles the exceptions exc (bits as X_BIT..D_BIT, H_BIT for TRIP) of the
   instruction inst at loc with operands y and z: an enabled one trips
   (returns 1 + the handler address and sets rW, rX, rY, rZ, rB, $255),
   the others set event bits in rA.  Returns 0 if there is no trip. */
octa mmx_exc(octa exc, octa loc, octa inst, octa y, octa z) {
    octa j, res = 0;
    /* an exact underflow is not an exception unless underflows trip */
    if ((exc & (U_BIT + X_BIT)) == U_BIT && !(mmx_sr[rA] & U_BIT)) exc &= ~(octa)U_BIT;
    j = exc & (mmx_sr[rA] | H_BIT);
    if (j) {
        int k = 0;
        while (!(j & H_BIT)) { j <<= 1; k++; }
        exc &= ~((octa)H_BIT >> k);
        mmx_sr[rW] = loc + 4;
        mmx_sr[rX] = SIGN | inst;
        mmx_sr[rY] = y;
        mmx_sr[rZ] = z;
        mmx_sr[rB] = mmx_g[255];
        mmx_g[255] = mmx_sr[rJ];
        res = 1 + ((octa)k << 4);
    }
    mmx_sr[rA] |= (exc >> 8) & 0xff;
    return res;
}

/* ------------------------------------------------------------ integer */

static octa aux;            /* high half of a product, remainder */
static int overflow;

/* 128-bit product of y and z: returns the low half, aux = the high half */
static octa omult(octa y, octa z) {
    octa y0 = y & M32, y1 = y >> 32, z0 = z & M32, z1 = z >> 32;
    octa p00 = y0 * z0, p01 = y0 * z1, p10 = y1 * z0, p11 = y1 * z1;
    octa mid = (p00 >> 32) + (p01 & M32) + (p10 & M32);
    aux = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
    return (mid << 32) | (p00 & M32);
}

static octa signed_omult(octa y, octa z) {
    octa acc = omult(y, z);
    if (y & SIGN) aux -= z;
    if (z & SIGN) aux -= y;
    overflow = (aux != ((acc & SIGN) ? ALL1 : 0));
    return acc;
}

/* (x:y) / z, unsigned; aux = remainder.  If x >= z: aux = y, returns x. */
static octa odiv(octa x, octa y, octa z) {
    octa q = 0, r = x;
    int i;
    if (x >= z) { aux = y; return x; }
    if (x == 0 && !((y | z) & SIGN)) {
        q = (octa)((soct)y / (soct)z);
        aux = y - q * z;
        return q;
    }
    for (i = 0; i < 64; i++) {
        octa top = r >> 63;
        r = (r << 1) | (y >> 63);
        y <<= 1;
        q <<= 1;
        if (top || r >= z) { r -= z; q |= 1; }
    }
    aux = r;
    return q;
}

static octa signed_odiv(octa y, octa z) {
    octa yy, zz, q;
    int sy, sz;
    if (y & SIGN) sy = 2, yy = 0 - y; else sy = 0, yy = y;
    if (z & SIGN) sz = 1, zz = 0 - z; else sz = 0, zz = z;
    q = odiv(0, yy, zz);
    overflow = 0;
    switch (sy + sz) {
    case 2 + 1:
        aux = 0 - aux;
        if (q == SIGN) overflow = 1;
        return q;
    case 0 + 0: default:
        return q;
    case 2 + 0:
        if (aux) aux = zz - aux;
        break;
    case 0 + 1:
        if (aux) aux = aux - zz;
        break;
    }
    if (aux) return ALL1 - q;
    return 0 - q;
}

static octa count_bits(octa x) {
    octa n = 0;
    while (x) { x &= x - 1; n++; }
    return n;
}

static octa bool_mult(octa y, octa z, int xor) {
    octa x = 0;
    int k;
    for (k = 0; k < 8; k++) {
        octa yb = (y >> (8 * k)) & 255;
        if (yb) {
            octa m = ((z >> k) & 0x0101010101010101ULL) * 255;
            if (xor) x ^= m & (yb * 0x0101010101010101ULL);
            else x |= m & (yb * 0x0101010101010101ULL);
        }
    }
    return x;
}

static octa part_diff(octa y, octa z, int bits) {
    octa x = 0, mask = bits == 64 ? ALL1 : (1ULL << bits) - 1;
    int k;
    for (k = 0; k < 64; k += bits) {
        octa a = (y >> k) & mask, b = (z >> k) & mask;
        if (a > b) x |= (a - b) << k;
    }
    return x;
}

/* ------------------------------------------------------------ floating point */

#define ZRO 0
#define NUM 1
#define INF 2
#define NAN_ 3
#define ZERO_EXPONENT (-1000)
#define INF_OCTA 0x7ff0000000000000ULL
#define STANDARD_NAN 0x7ff8000000000000ULL
#define QUIET 0x0008000000000000ULL
#define H22 0x0040000000000000ULL     /* f.h >= 0x400000 */
#define H23 0x0080000000000000ULL     /* f.h >= 0x800000 */

static int cur_round;
static int exceptions;

static octa shl(octa y, int s) {
    if (s <= 0) return y;
    if (s >= 64) return 0;
    return y << s;
}

static octa shr(octa y, int s, int u) {
    if (s <= 0) return y;
    if (s >= 64) return (u || !(y & SIGN)) ? 0 : ALL1;
    if (u || !(y & SIGN)) return y >> s;
    return (y >> s) | ~(ALL1 >> s);
}

static octa fpack(octa f, int e, int s, int r) {
    octa o;
    if (e > 0x7fd) e = 0x7ff, o = 0;
    else {
        if (e < 0) {
            if (e < -54) o = 1;
            else {
                octa oo;
                o = shr(f, -e, 1);
                oo = shl(o, -e);
                if (oo != f) o |= 1;
            }
            e = 0;
        } else o = f;
    }
    if (o & 3) exceptions |= X_BIT;
    switch (r) {
    case ROUND_DOWN: if (s == '-') o += 3; break;
    case ROUND_UP: if (s != '-') o += 3; break;
    case ROUND_OFF: break;
    case ROUND_NEAR: o += (o & 4) ? 2 : 1; break;
    }
    o = o >> 2;
    o += (octa)e << 52;
    if (o >= INF_OCTA) exceptions |= O_BIT + X_BIT;
    else if (o < 0x0010000000000000ULL) exceptions |= U_BIT;
    if (s == '-') o |= SIGN;
    return o;
}

static octa sfpack(octa f, int e, int s, int r) {
    octa o;
    if (e > 0x47d) e = 0x47f, o = 0;
    else {
        o = (shl(f, 3) >> 32) & M32;
        if (f & 0x1fffffff) o |= 1;
        if (e < 0x380) {
            if (e < 0x380 - 25) o = 1;
            else {
                octa o0 = o, oo;
                o = o >> (0x380 - e);
                oo = (o << (0x380 - e)) & M32;
                if (oo != o0) o |= 1;
            }
            e = 0x380;
        }
    }
    if (o & 3) exceptions |= X_BIT;
    switch (r) {
    case ROUND_DOWN: if (s == '-') o += 3; break;
    case ROUND_UP: if (s != '-') o += 3; break;
    case ROUND_OFF: break;
    case ROUND_NEAR: o += (o & 4) ? 2 : 1; break;
    }
    o = o >> 2;
    o += (octa)(e - 0x380) << 23;
    if (o >= 0x7f800000) exceptions |= O_BIT + X_BIT;
    else if (o < 0x100000) exceptions |= U_BIT;
    if (s == '-') o |= 0x80000000ULL;
    return o & M32;
}

static int funpack(octa x, octa *f, int *e, int *s) {
    int ee;
    exceptions = 0;
    *s = (x & SIGN) ? '-' : '+';
    *f = (x << 2) & 0x003fffffffffffffULL;
    ee = (int)((x >> 52) & 0x7ff);
    if (ee) {
        *e = ee - 1;
        *f |= H22;
        return ee < 0x7ff ? NUM : (*f == H22 ? INF : NAN_);
    }
    if (!(x & M32) && !(*f >> 32)) { *e = ZERO_EXPONENT; return ZRO; }
    do { ee--; *f <<= 1; } while (!(*f & H22));
    *e = ee;
    return NUM;
}

static int sfunpack(octa x, octa *f, int *e, int *s) {
    int ee;
    exceptions = 0;
    *s = (x & 0x80000000ULL) ? '-' : '+';
    *f = (x & 0x7fffff) << 31;
    ee = (int)((x >> 23) & 0xff);
    if (ee) {
        *e = ee + 0x380 - 1;
        *f |= H22;
        return ee < 0xff ? NUM : ((x & 0x7fffffff) == 0x7f800000 ? INF : NAN_);
    }
    if (!(x & 0x7fffffff)) { *e = ZERO_EXPONENT; return ZRO; }
    do { ee--; *f <<= 1; } while (!(*f & H22));
    *e = ee + 0x380;
    return NUM;
}

static octa load_sf(octa z) {
    octa f, x = 0;
    int e, s, t = sfunpack(z, &f, &e, &s);
    switch (t) {
    case ZRO: x = 0; break;
    case NUM: return fpack(f, e, s, ROUND_OFF);
    case INF: x = INF_OCTA; break;
    case NAN_: x = shr(f, 2, 1) | INF_OCTA; break;
    }
    if (s == '-') x |= SIGN;
    return x;
}

static octa store_sf(octa x) {
    octa f, z = 0;
    int e, s, t = funpack(x, &f, &e, &s);
    switch (t) {
    case ZRO: z = 0; break;
    case NUM: return sfpack(f, e, s, cur_round);
    case INF: z = 0x7f800000; break;
    case NAN_:
        if (!(f & 0x0020000000000000ULL)) { f |= 0x0020000000000000ULL; exceptions |= I_BIT; }
        z = 0x7f800000 | ((f >> 31) & M32);
        break;
    }
    if (s == '-') z |= 0x80000000ULL;
    return z & M32;
}

/* the NaN cases common to the binary operations; returns 1 with *x set */
static int nan_case(int yt, int zt, octa y, octa z, octa *x) {
    if (zt == NAN_) {
        if (yt == NAN_ && !(y & QUIET)) exceptions |= I_BIT;
        if (!(z & QUIET)) exceptions |= I_BIT, z |= QUIET;
        *x = z;
        return 1;
    }
    if (yt == NAN_) {
        if (!(y & QUIET)) exceptions |= I_BIT, y |= QUIET;
        *x = y;
        return 1;
    }
    return 0;
}

static octa fmult(octa y, octa z) {
    int yt, zt, ye, ze, ys, zs, xs, xe;
    octa x, xf, yf, zf;
    yt = funpack(y, &yf, &ye, &ys);
    zt = funpack(z, &zf, &ze, &zs);
    xs = ys == zs ? '+' : '-';
    if (nan_case(yt, zt, y, z, &x)) return x;
    if (yt == ZRO || zt == ZRO) {
        if (yt == INF || zt == INF) { x = STANDARD_NAN; exceptions |= I_BIT; }
        else x = 0;
    } else if (yt == INF || zt == INF) x = INF_OCTA;
    else {
        xe = ye + ze - 0x3fd;
        x = omult(yf, shl(zf, 9));
        if (aux >= H22) xf = aux;
        else xf = shl(aux, 1), xe--;
        if (x) xf |= 1;
        return fpack(xf, xe, xs, cur_round);
    }
    if (xs == '-') x |= SIGN;
    return x;
}

static octa fdivide(octa y, octa z) {
    int yt, zt, ye, ze, ys, zs, xs, xe;
    octa x, xf, yf, zf;
    yt = funpack(y, &yf, &ye, &ys);
    zt = funpack(z, &zf, &ze, &zs);
    xs = ys == zs ? '+' : '-';
    if (nan_case(yt, zt, y, z, &x)) return x;
    if ((yt == ZRO && zt != ZRO) || (yt == NUM && zt == INF)) x = 0;
    else if (yt == NUM && zt == ZRO) { exceptions |= Z_BIT; x = INF_OCTA; }
    else if (yt == INF && zt != INF) x = INF_OCTA;
    else if (yt == NUM && zt == NUM) {
        xe = ye - ze + 0x3fd;
        xf = odiv(yf, 0, shl(zf, 9));
        if (xf >= H23) {
            aux |= xf & 1;
            xf = shr(xf, 1, 1);
            xe++;
        }
        if (aux) xf |= 1;
        return fpack(xf, xe, xs, cur_round);
    } else { x = STANDARD_NAN; exceptions |= I_BIT; }
    if (xs == '-') x |= SIGN;
    return x;
}

static octa fplus(octa y, octa z) {
    int yt, zt, ye, ze, ys, zs, xs = '+', xe, d;
    octa x, xf, yf, zf;
    yt = funpack(y, &yf, &ye, &ys);
    zt = funpack(z, &zf, &ze, &zs);
    if (nan_case(yt, zt, y, z, &x)) return x;
    if (yt == ZRO && zt == NUM) return fpack(zf, ze, zs, ROUND_OFF);
    if (yt == NUM && zt == ZRO) return fpack(yf, ye, ys, ROUND_OFF);
    if (yt == INF && zt == INF && ys != zs) {
        exceptions |= I_BIT; x = STANDARD_NAN; xs = zs;
    } else if (zt == INF) { x = INF_OCTA; xs = zs; }
    else if (yt == INF) { x = INF_OCTA; xs = ys; }
    else if (yt == NUM && zt == NUM && y != (z ^ SIGN)) {
        octa o, oo;
        if (ye < ze || (ye == ze && yf < zf)) {
            o = yf, yf = zf, zf = o;
            d = ye, ye = ze, ze = d;
            d = ys, ys = zs, zs = d;
        }
        d = ye - ze;
        xs = ys, xe = ye;
        if (d) {
            if (d <= 2) zf = shr(zf, d, 1);
            else if (d > 54) zf = 1;
            else {
                if (ys != zs) d--, xe--, yf = shl(yf, 1);
                o = zf;
                zf = shr(o, d, 1);
                oo = shl(zf, d);
                if (oo != o) zf |= 1;
            }
        }
        if (ys == zs) {
            xf = yf + zf;
            if (xf >= H23) xe++, d = (int)(xf & 1), xf = shr(xf, 1, 1), xf |= d;
        } else {
            xf = yf - zf;
            if (xf >= H23) xe++, d = (int)(xf & 1), xf = shr(xf, 1, 1), xf |= d;
            else while (xf < H22) xe--, xf = shl(xf, 1);
        }
        return fpack(xf, xe, xs, cur_round);
    } else {
        x = 0;
        xs = ys == zs ? ys : (cur_round == ROUND_DOWN ? '-' : '+');
    }
    if (xs == '-') x |= SIGN;
    return x;
}

static int fepscomp(octa y, octa z, octa e, int s) {
    octa yf, zf, ef, o, oo;
    int ye, ze, ee, ys, zs, es, yt, zt, et, d;
    et = funpack(e, &ef, &ee, &es);
    if (es == '-') return 2;
    switch (et) {
    case NAN_: return 2;
    case INF: ee = 10000; break;
    default: break;
    }
    yt = funpack(y, &yf, &ye, &ys);
    zt = funpack(z, &zf, &ze, &zs);
    if (yt == NAN_ || zt == NAN_) return 2;
    if (yt == INF && zt == INF) return ys == zs || ee >= 1023;
    if (yt == INF || zt == INF) return s && ee >= 1022;
    if (yt == ZRO && zt == ZRO) return 1;
    if ((yt == ZRO || zt == ZRO) && !s) return 0;
    if (ye < 0 && yt != ZRO) yf = shl(y, 2), ye = 0;
    if (ze < 0 && zt != ZRO) zf = shl(z, 2), ze = 0;
    if (ye < ze || (ye == ze && yf < zf)) {
        o = yf, yf = zf, zf = o;
        d = ye, ye = ze, ze = d;
        d = ys, ys = zs, zs = d;
    }
    if (ze == ZERO_EXPONENT) ze = ye;
    d = ye - ze;
    if (!s) ee -= d;
    if (ee >= 1023) return 1;
    if (d > 54) o = 0, oo = zf;
    else o = shr(zf, d, 1), oo = shl(o, d);
    if (oo != zf) {
        if (ee < 1020) return 0;
        o += ys == zs ? 0 : 1;
    }
    o = ys == zs ? yf - o : yf + o;
    if (!o) return 1;
    if (ee < 968) return 0;
    if (ee >= 1021) ef = shl(ef, ee - 1021);
    else ef = shr(ef, 1021 - ee, 1);
    return o <= ef;
}

static int fcomp(octa y, octa z) {
    int yt, zt, ye, ze, ys, zs, x;
    octa yf, zf;
    yt = funpack(y, &yf, &ye, &ys);
    zt = funpack(z, &zf, &ze, &zs);
    if (yt == NAN_ || zt == NAN_) return 2;
    if (yt == ZRO && zt == ZRO) return 0;
    if (ys != zs) x = 1;
    else if (y > z) x = 1;
    else if (y < z) x = -1;
    else return 0;
    return ys == '-' ? -x : x;
}

static octa fintegerize(octa z, int r) {
    int zt, ze, zs;
    octa xf, zf;
    zt = funpack(z, &zf, &ze, &zs);
    if (!r) r = cur_round;
    if (zt == NAN_) {
        if (!(z & QUIET)) { exceptions |= I_BIT; z |= QUIET; }
        return z;
    }
    if (zt != NUM) return z;
    if (ze >= 1074) return fpack(zf, ze, zs, ROUND_OFF);
    if (ze <= 1020) xf = 1;
    else {
        octa oo;
        xf = shr(zf, 1074 - ze, 1);
        oo = shl(xf, 1074 - ze);
        if (oo != zf) xf |= 1;
    }
    switch (r) {
    case ROUND_DOWN: if (zs == '-') xf += 3; break;
    case ROUND_UP: if (zs != '-') xf += 3; break;
    case ROUND_OFF: break;
    case ROUND_NEAR: xf += (xf & 4) ? 2 : 1; break;
    }
    xf &= ~3ULL;
    if (ze >= 1022) return fpack(shl(xf, 1074 - ze), ze, zs, ROUND_OFF);
    if (xf & M32) xf = 0x3ff0000000000000ULL;
    if (zs == '-') xf |= SIGN;
    return xf;
}

static octa fixit(octa z, int r) {
    int zt, ze, zs;
    octa zf, o;
    zt = funpack(z, &zf, &ze, &zs);
    if (!r) r = cur_round;
    if (zt == NAN_ || zt == INF) { exceptions |= I_BIT; return z; }
    if (zt == ZRO) return 0;
    if (funpack(fintegerize(z, r), &zf, &ze, &zs) == ZRO) return 0;
    if (ze <= 1076) o = shr(zf, 1076 - ze, 1);
    else {
        if (ze > 1085 || (ze == 1085 && (zf > H22 || (zf == H22 && zs != '-'))))
            exceptions |= W_BIT;
        if (ze >= 1140) return 0;
        o = shl(zf, ze - 1076);
    }
    return zs == '-' ? 0 - o : o;
}

static octa floatit(octa z, int r, int u, int p) {
    int e, s, t;
    exceptions = 0;
    if (!z) return 0;
    if (!r) r = cur_round;
    if (!u && (z & SIGN)) s = '-', z = 0 - z; else s = '+';
    e = 1076;
    while (z < H22) e--, z = shl(z, 1);
    while (z >= H23) {
        e++;
        t = (int)(z & 1);
        z = shr(z, 1, 1);
        z |= t;
    }
    if (p) {
        int ex;
        octa st = sfpack(z, e, s, r);
        ex = exceptions;
        sfunpack(st, &z, &e, &s);
        exceptions = ex;
    }
    return fpack(z, e, s, r);
}

static octa froot(octa z, int r) {
    int zt, ze, zs, xe, k;
    octa x = 0, xf, rf, zf;
    if (!r) r = cur_round;
    zt = funpack(z, &zf, &ze, &zs);
    if (zs == '-' && zt != ZRO) { exceptions |= I_BIT; x = STANDARD_NAN; }
    else switch (zt) {
    case NAN_:
        if (!(z & QUIET)) exceptions |= I_BIT, z |= QUIET;
        return z;
    case NUM:
        xf = 2;
        xe = (ze + 0x3fe) >> 1;
        if (ze & 1) zf = shl(zf, 1);
        rf = ((zf >> 32) >> 22) - 1;
        for (k = 53; k; k--) {
            rf = shl(rf, 2);
            xf = shl(xf, 1);
            if (k >= 43) rf += ((zf >> 32) >> (2 * (k - 43))) & 3;
            else if (k >= 27) rf += ((zf & M32) >> (2 * (k - 27))) & 3;
            if (rf > xf) {
                xf++;
                rf -= xf;
                xf++;
            }
        }
        if (rf) xf++;
        return fpack(xf, xe, '+', r);
    default:
        x = z;
        break;
    }
    if (zs == '-') x |= SIGN;
    return x;
}

static octa fremstep(octa y, octa z, int delta) {
    int yt, zt, ye, ze, xs, ys, zs, xe, thresh, odd;
    octa x, xf, yf, zf;
    yt = funpack(y, &yf, &ye, &ys);
    zt = funpack(z, &zf, &ze, &zs);
    if (nan_case(yt, zt, y, z, &x)) return x;
    if (zt == ZRO || yt == INF) { x = STANDARD_NAN; exceptions |= I_BIT; }
    else if (yt == ZRO || zt == INF) return y;
    else {
        odd = 0;
        thresh = ye - delta;
        if (thresh < ze) thresh = ze;
        while (ye >= thresh) {
            if (yf == zf) goto zero_out;
            if (yf < zf) {
                if (ye == ze) goto try_complement;
                ye--, yf = shl(yf, 1);
            }
            yf = yf - zf;
            if (ye == ze) odd = 1;
            while (yf < H22) ye--, yf = shl(yf, 1);
        }
        if (ye >= ze) {
            exceptions |= E_BIT;
            return fpack(yf, ye, ys, ROUND_OFF);
        }
        if (ye < ze - 1) return fpack(yf, ye, ys, ROUND_OFF);
        yf = shr(yf, 1, 1);
try_complement:
        xf = zf - yf, xe = ze, xs = ys == '+' ? '-' : '+';
        if (xf > yf || (xf == yf && !odd)) xf = yf, xs = ys;
        while (xf < H22) xe--, xf = shl(xf, 1);
        return fpack(xf, xe, xs, ROUND_OFF);
zero_out:
        x = 0;
    }
    if (ys == '-') x |= SIGN;
    return x;
}

/* ------------------------------------------------------------ mmx_op */

static octa mmx_exc_of(octa exc, octa loc, octa inst, octa y, octa z) {
    if (exc) mmx_trip = mmx_exc(exc, loc, inst, y, z);
    return 0;
}

/* Performs the instruction inst (at loc) on the operands y and z (for the
   stores LDSF/STSF: z is the tetrabyte loaded or the register stored) and
   returns the result.  An exception that trips sets mmx_trip. */
octa mmx_op(octa inst, octa y, octa z, octa loc) {
    int op = (int)((inst >> 24) & 255);
    octa x = 0, exc = 0, a;
    int k;
    cur_round = (int)((mmx_sr[rA] >> 16) & 3);
    if (!cur_round) cur_round = ROUND_NEAR;
    exceptions = 0;
    switch (op) {
    case 0x04: x = fplus(y, z); exc = exceptions; break;                /* FADD */
    case 0x06:                                                            /* FSUB */
        a = z;
        if (fcomp(a, 0) != 2) a ^= SIGN;
        x = fplus(y, a); exc = exceptions; break;
    case 0x10: x = fmult(y, z); exc = exceptions; break;                 /* FMUL */
    case 0x14: x = fdivide(y, z); exc = exceptions; break;               /* FDIV */
    case 0x16: x = fremstep(y, z, 2500); exc = exceptions; break;        /* FREM */
    case 0x15: case 0x17: case 0x05: case 0x07:                          /* FSQRT FINT FIX FIXU */
    case 0x08: case 0x09: case 0x0a: case 0x0b:                          /* FLOT FLOTU */
    case 0x0c: case 0x0d: case 0x0e: case 0x0f:                          /* SFLOT SFLOTU */
        if (y > 4) mmx_illegal(loc);
        if (op == 0x15) x = froot(z, (int)y);
        else if (op == 0x17) x = fintegerize(z, (int)y);
        else if (op == 0x05) x = fixit(z, (int)y);
        else if (op == 0x07) { x = fixit(z, (int)y); exceptions &= ~W_BIT; }
        else x = floatit(z, (int)y, op & 2, op & 4);
        exc = exceptions;
        break;
    case 0x01:                                                            /* FCMP */
        k = fcomp(y, z);
        goto cmp_fin;
    case 0x11:                                                            /* FCMPE */
        k = fepscomp(y, z, mmx_sr[rE], 1);
        if (k) goto cmp_zero_or_invalid;
        k = fcomp(y, z);
cmp_fin:
        if (k < 0) { x = ALL1; break; }
        if (k == 1) { x = 1; break; }
cmp_zero_or_invalid:
        if (k == 2) exc |= I_BIT;
        x = 0;
        break;
    case 0x02: x = fcomp(y, z) == 2; break;                              /* FUN */
    case 0x03: x = fcomp(y, z) == 0; break;                              /* FEQL */
    case 0x13:                                                            /* FEQLE */
        k = fepscomp(y, z, mmx_sr[rE], 0);
        if (k == 1) x = 1;
        else if (k == 2) exc |= I_BIT;
        break;
    case 0x12: x = fepscomp(y, z, mmx_sr[rE], 1) == 2; break;            /* FUNE */
    case 0x18: case 0x19:                                                 /* MUL */
        x = signed_omult(y, z);
        if (overflow) exc |= V_BIT;
        break;
    case 0x1a: case 0x1b:                                                 /* MULU */
        x = omult(y, z);
        mmx_sr[rH] = aux;
        break;
    case 0x1c: case 0x1d:                                                 /* DIV */
        if (!z) { aux = y; exc |= D_BIT; overflow = 0; x = 0; }
        else x = signed_odiv(y, z);
        mmx_sr[rR] = aux;
        if (overflow) exc |= V_BIT;
        break;
    case 0x1e: case 0x1f:                                                 /* DIVU */
        x = odiv(mmx_sr[rD], y, z);
        mmx_sr[rR] = aux;
        break;
    case 0xd0: case 0xd1: x = part_diff(y, z, 8); break;                  /* BDIF */
    case 0xd2: case 0xd3: x = part_diff(y, z, 16); break;                 /* WDIF */
    case 0xd4: case 0xd5: x = part_diff(y, z, 32); break;                 /* TDIF */
    case 0xd6: case 0xd7: x = part_diff(y, z, 64); break;                 /* ODIF */
    case 0xd8: case 0xd9:                                                 /* MUX */
        x = (y & mmx_sr[rM]) | (z & ~mmx_sr[rM]);
        break;
    case 0xda: case 0xdb: x = count_bits(y & ~z); break;                  /* SADD */
    case 0xdc: case 0xdd: x = bool_mult(y, z, 0); break;                  /* MOR */
    case 0xde: case 0xdf: x = bool_mult(y, z, 1); break;                  /* MXOR */
    case 0x90: case 0x91: x = load_sf(z); break;                          /* LDSF */
    case 0xb0: case 0xb1: x = store_sf(z); exc = exceptions; break;       /* STSF */
    default:
        mmx_illegal(loc);
    }
    mmx_exc_of(exc, loc, inst, y, z);
    return x;
}

/* ------------------------------------------------------------ TRAP */

#define MAX_FILES 256
static long port_of[MAX_FILES];     /* 0: not open; else CPUSim64 port + 1 */
static int mode_of[MAX_FILES];      /* as mmix-io: 1 read, 2 write, 4 binary, 8 read/write */
static int files_ready;

static void files_init(void) {
    int i;
    for (i = 0; i < MAX_FILES; i++) port_of[i] = 0, mode_of[i] = 0;
    port_of[0] = 1, mode_of[0] = 1;
    port_of[1] = 2, mode_of[1] = 2;
    port_of[2] = 3, mode_of[2] = 2;
    files_ready = 1;
}

/* the string at MMIX address a as a CPUSim64 packed string */
static octa *packed_from(octa a) {
    octa n = 0, i;
    octa *ps;
    while (mmx_get1(a + n)) {
        n++;
        if (n >= 4096) return 0;
    }
    ps = mmx_alloc((long)(2 + (n >> 3)));
    if (!ps) return 0;
    ps[0] = n;
    for (i = 0; i < n; i++)
        ps[1 + (i >> 3)] |= mmx_get1(a + i) << (56 - 8 * (int)(i & 7));
    return ps;
}

static octa f_open(int h, octa name, octa mode) {
    static const int mode_code[5] = { 0x1, 0x2, 0x5, 0x6, 0xf };
    octa *ps;
    long port;
    if (mode > 4) goto failure;
    if (h > 2 && mode_of[h]) mmx_close(port_of[h] - 1);
    mode_of[h] = 0;
    ps = packed_from(name);
    if (!ps) goto failure;
    if (mode == 0 || mode == 2) {               /* reading: the file must exist */
        if (!mmx_isfile(ps)) goto failure;
    } else mmx_delete(ps);                      /* writing: start empty */
    port = mmx_open(ps);
    if (port < 0) goto failure;
    port_of[h] = port + 1;
    mode_of[h] = mode_code[mode];
    return 0;
failure:
    mode_of[h] = 0;
    return ALL1;
}

static octa f_close(int h) {
    if (!mode_of[h]) return ALL1;
    if (h > 2) mmx_close(port_of[h] - 1);
    mode_of[h] = 0;
    return 0;
}

static long getb(int h) { return mmx_getb(port_of[h] - 1); }

static octa f_read(int h, octa buf, octa size) {
    octa n = 0;
    if (!(mode_of[h] & 1)) return ALL1 - size;
    if (mode_of[h] & 8) mode_of[h] &= ~2;
    if (size >> 32) return ALL1 - size;
    while (n < size) {
        long c = getb(h);
        if (c < 0) break;
        mmx_put1(buf + n, (octa)c);
        n++;
    }
    return n - size;
}

static octa f_gets(int h, octa buf, octa size) {
    octa n = 0;
    if (!(mode_of[h] & 1)) return ALL1;
    if (!size) return ALL1;
    if (mode_of[h] & 8) mode_of[h] &= ~2;
    while (n < size - 1) {
        long c = getb(h);
        if (c < 0) {
            if (n == 0) return ALL1;
            break;
        }
        mmx_put1(buf + n, (octa)c);
        n++;
        if (c == '\n') break;
    }
    mmx_put1(buf + n, 0);
    return n;
}

static octa f_getws(int h, octa buf, octa size) {
    octa n = 0;
    if (!(mode_of[h] & 1)) return ALL1;
    if (!size) return ALL1;
    if (mode_of[h] & 8) mode_of[h] &= ~2;
    buf &= ~1ULL;
    while (n < size - 1) {
        long c1 = getb(h), c2;
        if (c1 < 0) break;
        c2 = getb(h);
        if (c2 < 0) break;
        mmx_put1(buf + 2 * n, (octa)c1);
        mmx_put1(buf + 2 * n + 1, (octa)c2);
        n++;
        if (c1 == 0 && c2 == '\n') break;
    }
    mmx_put1(buf + 2 * n, 0);
    mmx_put1(buf + 2 * n + 1, 0);
    return n;
}

static octa f_write(int h, octa buf, octa size) {
    octa i;
    long port = port_of[h] - 1;
    if (!(mode_of[h] & 2)) return 0 - size;
    if (mode_of[h] & 8) mode_of[h] &= ~1;
    for (i = 0; i < size; i++) mmx_putb(port, (long)mmx_get1(buf + i));
    return 0;
}

static octa f_puts(int h, octa s) {
    octa n = 0, c;
    long port = port_of[h] - 1;
    if (!(mode_of[h] & 2)) return ALL1;
    if (mode_of[h] & 8) mode_of[h] &= ~1;
    while ((c = mmx_get1(s + n)) != 0) { mmx_putb(port, (long)c); n++; }
    return n;
}

static octa f_putws(int h, octa s) {
    octa n = 0;
    long port = port_of[h] - 1;
    if (!(mode_of[h] & 2)) return ALL1;
    if (mode_of[h] & 8) mode_of[h] &= ~1;
    s &= ~1ULL;
    for (;;) {
        octa c1 = mmx_get1(s + 2 * n), c2 = mmx_get1(s + 2 * n + 1);
        if (!c1 && !c2) break;
        mmx_putb(port, (long)c1);
        mmx_putb(port, (long)c2);
        n++;
    }
    return n;
}

static octa f_seek(int h, octa off) {
    long port = port_of[h] - 1;
    if (!(mode_of[h] & 4)) return ALL1;
    if (mode_of[h] & 8) mode_of[h] = 0xf;
    if (off & SIGN) {
        if (off < 0xffffffff80000000ULL) return ALL1;
        /* -1 is the end of the file, -1-k is k bytes before the end; a
           CPUSim64 position -k is k bytes before the end */
        if (mmx_seek(port, off == ALL1 ? 0x7fffffffffffffffLL : (long)(off + 1)) < 0) return ALL1;
    } else {
        if (off >> 31) return ALL1;
        if (mmx_seek(port, (long)off) < 0) return ALL1;
    }
    return 0;
}

static octa f_tell(int h) {
    long x;
    if (!(mode_of[h] & 4)) return ALL1;
    x = mmx_tell(port_of[h] - 1);
    if (x < 0) return ALL1;
    return (octa)x;
}

/* TRAP inst at loc ($Y = y, $Z = z): the MMIXware simulator's system calls.
   The result goes to $255.  Halt ends the program with exit code $255. */
void mmx_trap(octa loc, octa inst, octa y, octa z) {
    int xx = (int)((inst >> 16) & 255), yy = (int)((inst >> 8) & 255), zz = (int)(inst & 255);
    octa b = mmx_g[255], mb = 0, ma = 0, r = 0;
    if (!files_ready) files_init();
    if (xx != 0 || yy > 10) mmx_illegal(loc);
    mmx_sr[rWW] = loc + 4;
    mmx_sr[rXX] = SIGN | inst;
    mmx_sr[rYY] = y;
    mmx_sr[rZZ] = z;
    if (yy == 1 || yy == 3 || yy == 4 || yy == 5 || yy == 6) {
        mb = mmx_get8(b);
        ma = mmx_get8(b + 8);
    }
    switch (yy) {
    case 0:
        if (zz) mmx_illegal(loc);
        b &= M32;                           /* the exit code is $255 as an int */
        if (b & 0x80000000ULL) b |= ~M32;
        mmx_exit((long)b);
        break;
    case 1: r = f_open(zz, mb, ma); break;
    case 2: r = f_close(zz); break;
    case 3: r = f_read(zz, mb, ma); break;
    case 4: r = f_gets(zz, mb, ma); break;
    case 5: r = f_getws(zz, mb, ma); break;
    case 6: r = f_write(zz, mb, ma); break;
    case 7: r = f_puts(zz, b); break;
    case 8: r = f_putws(zz, b); break;
    case 9: r = f_seek(zz, b); break;
    case 10: r = f_tell(zz); break;
    }
    mmx_sr[rBB] = r;
    mmx_g[255] = r;
}

/* ------------------------------------------------------------ SAVE, UNSAVE */

/* SAVE: stores the local registers' count, the global registers and the
   special registers above the local registers (from mmx_rb, mmx_rl) and
   returns the address of the last octabyte stored.  rL becomes 0. */
octa mmx_save(void) {
    octa *p = (octa *)mmx_rb;
    octa L = mmx_rl;
    int k;
    p[L] = L;
    L++;
    p += L;
    if ((octa)(p + 300) >= mmx_stack_end + 260) mmx_fatal("register stack overflow at SAVE, rO =", mmx_addr_of(p));
    for (k = (int)mmx_rg;;) {
        if (k == rZ + 1) *p = (mmx_rg << 56) | (mmx_sr[rA] & 0x3ffff);
        else if (k >= 32) *p = mmx_g[k];
        else *p = mmx_sr[k];
        p++;
        if (k == 255) k = rB;
        else if (k == rR) k = rP;
        else if (k == rZ + 1) break;
        else k++;
    }
    mmx_rb = (octa)p;
    mmx_rl = 0;
    return mmx_addr_of(p) - 8;
}

/* UNSAVE z: the inverse of SAVE; sets mmx_rb and mmx_rl. */
void mmx_unsave(octa z, octa loc) {
    octa *top = mmx_word(z & ~7ULL), *p = top, *q;
    octa L;
    int k;
    for (k = rZ + 1;;) {
        if (k == rZ + 1) {
            octa G = *p >> 56;
            if (G < 32) G = 32;
            if (G != mmx_rg) mmx_fatal("UNSAVE would change rG (not supported) at", loc);
            mmx_sr[rA] = *p & 0x3ffff;
        } else if (k >= 32) mmx_g[k] = *p;
        else mmx_sr[k] = *p;
        if (k == rP) k = rR;
        else if (k == rB) k = 255;
        else if (k == (int)mmx_rg) break;
        else k--;
        p--;
    }
    p--;                    /* the count of local registers */
    L = *p & 255;
    if (L > mmx_rg) L = mmx_rg;
    mmx_rl = L;
    mmx_rb = (octa)(p - (*p & 255));
    /* registers at and above rL read as zero */
    for (q = (octa *)mmx_rb + L; q <= top; q++) *q = 0;
}
