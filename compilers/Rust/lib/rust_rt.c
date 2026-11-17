/* rust_rt.c: runtime for the Rust to C translator (see design-codegen.md).
 *
 * Plain C, copied verbatim at the top of every generated program and
 * compiled by the CPUSim64 C11 compiler.  Everything is static.  It also
 * builds with a host compiler (cc -std=c99): then __ldb/__stb and the word
 * memory are simulated by a global array (see the RS_HOST section), so the
 * functions can be unit tested on the host (lib/rust_rt_test.c).
 *
 * Conventions: every scalar is a `long` (one 64-bit word). A byte address is
 * word address * 8 + index (index 0 is the most significant byte).
 */
#ifndef RS_RT_C
#define RS_RT_C

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

struct rs_slice { long base; long len; };
struct rs_dyn { long data; long vt; };

/* ---------------------------------------------------------------- host shim */
#ifndef __CPUSIM64__
#define RS_HOST 1
#define RS_ATTR __attribute__((unused))
#define RS_HOSTMEM_WORDS (1L << 20)
static long rs_hostmem[RS_HOSTMEM_WORDS];
static long rs_host_top = 16; /* words 0..15 reserved: address 0 is null */
#define RS_W(a) (rs_hostmem[(a)])
RS_ATTR static long __ldb(long a) {
    return (long)(((unsigned long)rs_hostmem[a >> 3] >> ((7 - (a & 7)) * 8)) & 255UL);
}
RS_ATTR static void __stb(long a, long v) {
    unsigned long sh = (unsigned long)((7 - (a & 7)) * 8);
    unsigned long w = (unsigned long)rs_hostmem[a >> 3];
    w = (w & ~(255UL << sh)) | (((unsigned long)v & 255UL) << sh);
    rs_hostmem[a >> 3] = (long)w;
}
#else
#define RS_ATTR
#define RS_W(a) (*(long *)(a))
#endif

/* ------------------------------------------------------------------- exit */
RS_ATTR static void rs_exit(long code) { fflush(stdout); exit((int)code); }
RS_ATTR static void rs_abort(void) { fflush(stdout); exit(134); }

/* ----------------------------------------------------------------- memory */
/* sizes are in words; the result is a word address (0 never returned) */
RS_ATTR static long rs_alloc(long words) {
    long p;
    if (words < 1) words = 1;
#ifdef RS_HOST
    p = rs_host_top;
    rs_host_top += words + 1;
    if (rs_host_top >= RS_HOSTMEM_WORDS) rs_abort();
    rs_hostmem[p] = words;
    p++;
#else
    p = (long)malloc(words);
    if (p == 0) rs_abort();
#endif
    return p;
}
RS_ATTR static long rs_realloc(long p, long old_words, long new_words) {
    long q, i, n;
    if (new_words < 1) new_words = 1;
#ifdef RS_HOST
    q = rs_alloc(new_words);
    n = old_words < new_words ? old_words : new_words;
    for (i = 0; i < n; i++) RS_W(q + i) = RS_W(p + i);
#else
    (void)old_words; (void)i; (void)n;
    q = (long)realloc((long *)p, new_words);
    if (q == 0) rs_abort();
#endif
    return q;
}
RS_ATTR static void rs_free(long p) {
#ifndef RS_HOST
    free((long *)p);
#else
    (void)p;
#endif
}
/* packed u8 storage: n bytes need (n + 7) / 8 words; bases are BYTE addresses */
RS_ATTR static long rs_alloc_bytes(long n) { return rs_alloc((n + 7) / 8) * 8; }
RS_ATTR static long rs_realloc_bytes(long base, long old_n, long new_n) {
    return rs_realloc(base / 8, (old_n + 7) / 8, (new_n + 7) / 8) * 8;
}
RS_ATTR static void rs_free_bytes(long base) { rs_free(base / 8); }

RS_ATTR static long rs_ldb(long a) { return __ldb(a); }
RS_ATTR static void rs_stb(long a, long v) { __stb(a, v); }

/* memmove semantics */
RS_ATTR static void rs_copy_words(long dst, long src, long n) {
    long i;
    if (dst == src) return;
    if (dst < src) { for (i = 0; i < n; i++) RS_W(dst + i) = RS_W(src + i); }
    else { for (i = n - 1; i >= 0; i--) RS_W(dst + i) = RS_W(src + i); }
}
RS_ATTR static void rs_copy_bytes(long dst, long src, long n) {
    long i, w;
    if (n <= 0 || dst == src) return;
    w = ((dst | src) & 7) == 0 ? n >> 3 : 0;
    if (dst < src) {
        for (i = 0; i < w; i++) RS_W((dst >> 3) + i) = RS_W((src >> 3) + i);
        for (i = w << 3; i < n; i++) __stb(dst + i, __ldb(src + i));
    } else {
        for (i = n - 1; i >= (w << 3); i--) __stb(dst + i, __ldb(src + i));
        for (i = w - 1; i >= 0; i--) RS_W((dst >> 3) + i) = RS_W((src >> 3) + i);
    }
}
RS_ATTR static void rs_fill_bytes(long base, long n, long v) {
    long i;
    for (i = 0; i < n; i++) __stb(base + i, v);
}
/* -1, 0, 1 as memcmp */
RS_ATTR static long rs_cmp_bytes(long a, long b, long n) {
    long i, x, y;
    for (i = 0; i < n; i++) {
        x = __ldb(a + i);
        y = __ldb(b + i);
        if (x != y) return x < y ? -1 : 1;
    }
    return 0;
}

/* -------------------------------------------------------------------- I/O */
/* one codepoint to a port; the simulator's fputc moves codepoints, the host
   has to encode UTF-8 itself */
RS_ATTR static void rs_putcp(long port, long cp) {
    FILE *f = port == 2 ? stderr : stdout;
#ifdef RS_HOST
    if (cp < 0x80) fputc((int)cp, f);
    else if (cp < 0x800) { fputc((int)(0xC0 | (cp >> 6)), f); fputc((int)(0x80 | (cp & 63)), f); }
    else if (cp < 0x10000) {
        fputc((int)(0xE0 | (cp >> 12)), f); fputc((int)(0x80 | ((cp >> 6) & 63)), f);
        fputc((int)(0x80 | (cp & 63)), f);
    } else {
        fputc((int)(0xF0 | (cp >> 18)), f); fputc((int)(0x80 | ((cp >> 12) & 63)), f);
        fputc((int)(0x80 | ((cp >> 6) & 63)), f); fputc((int)(0x80 | (cp & 63)), f);
    }
#else
    fputc((int)cp, f);
#endif
}

/* one byte written as it is (not as a codepoint): a stream in binary mode
   writes the bytes fputc gets (the C11 library's __SIO_BIN mode bit, 8) */
RS_ATTR static void rs_putbyte(long port, long b) {
    FILE *f = port == 2 ? stderr : stdout;
#ifdef RS_HOST
    fputc((int)(b & 255), f);
#else
    long fl = f->__flags;
    f->__flags = fl | 8;
    fputc((int)(b & 255), f);
    f->__flags = fl;
#endif
}

/* Writes bytes like std's Stdout: valid UTF-8 is written as codepoints (the
   port encodes them as UTF-8 again), the bytes of an invalid sequence are
   written unchanged, so binary output (images, write_all of any bytes) is
   exact. */
RS_ATTR static void rs_write_bytes(long port, long base, long len) {
    long i = 0, b, c, need, cp, lo, hi, j, ok, k;
    while (i < len) {
        b = __ldb(base + i);
        if (b < 0x80) { rs_putcp(port, b); i++; continue; }
        if (b >= 0xC2 && b <= 0xDF) { need = 1; cp = b & 0x1F; }
        else if (b >= 0xE0 && b <= 0xEF) { need = 2; cp = b & 0x0F; }
        else if (b >= 0xF0 && b <= 0xF4) { need = 3; cp = b & 7; }
        else { rs_putbyte(port, b); i++; continue; }
        lo = 0x80; hi = 0xBF;
        if (b == 0xE0) lo = 0xA0;
        else if (b == 0xED) hi = 0x9F;
        else if (b == 0xF0) lo = 0x90;
        else if (b == 0xF4) hi = 0x8F;
        ok = 1;
        for (j = 1; j <= need; j++) {
            if (i + j >= len) { ok = 0; break; }
            c = __ldb(base + i + j);
            if (j == 1 ? (c < lo || c > hi) : (c < 0x80 || c > 0xBF)) { ok = 0; break; }
            cp = (cp << 6) | (c & 0x3F);
        }
        if (ok) { rs_putcp(port, cp); i += need + 1; }
        else {
            for (k = 0; k < j && i + k < len; k++) rs_putbyte(port, __ldb(base + i + k));
            i += j;
        }
    }
}

static long rs_rd_buf[4];
static long rs_rd_n = 0;
static long rs_rd_i = 0;
#ifdef RS_HOST
RS_ATTR static long rs_getcp(void) { /* host: decode UTF-8 from the byte stream */
    long b = fgetc(stdin), n, cp, c, i;
    if (b < 0x80) return b;
    if (b >= 0xC2 && b <= 0xDF) { n = 1; cp = b & 0x1F; }
    else if (b >= 0xE0 && b <= 0xEF) { n = 2; cp = b & 0x0F; }
    else if (b >= 0xF0 && b <= 0xF4) { n = 3; cp = b & 7; }
    else return 0xFFFD;
    for (i = 0; i < n; i++) {
        c = fgetc(stdin);
        if (c < 0x80 || c > 0xBF) { if (c >= 0) ungetc((int)c, stdin); return 0xFFFD; }
        cp = (cp << 6) | (c & 0x3F);
    }
    return cp;
}
#else
#define rs_getcp() ((long)fgetc(stdin))
#endif
/* next byte of standard input as UTF-8, -1 at end of input */
RS_ATTR static long rs_read_byte(long port) {
    long cp;
    (void)port;
    if (rs_rd_i < rs_rd_n) return rs_rd_buf[rs_rd_i++];
    cp = rs_getcp();
    if (cp < 0) return -1;
    if (cp < 0x80) return cp;
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) cp = 0xFFFD;
    rs_rd_i = 1;
    if (cp < 0x800) { rs_rd_buf[1] = 0x80 | (cp & 63); rs_rd_n = 2; return 0xC0 | (cp >> 6); }
    if (cp < 0x10000) {
        rs_rd_buf[1] = 0x80 | ((cp >> 6) & 63); rs_rd_buf[2] = 0x80 | (cp & 63); rs_rd_n = 3;
        return 0xE0 | (cp >> 12);
    }
    rs_rd_buf[1] = 0x80 | ((cp >> 12) & 63); rs_rd_buf[2] = 0x80 | ((cp >> 6) & 63);
    rs_rd_buf[3] = 0x80 | (cp & 63); rs_rd_n = 4;
    return 0xF0 | (cp >> 18);
}

/* ----------------------------------------------------------------- panics */
RS_ATTR static void rs_err_str(const char *s) {
    while (*s) { fputc((int)*s, stderr); s++; }
}
/* unsigned decimal to stderr (no unsigned division: halve first) */
RS_ATTR static void rs_err_udec(unsigned long v) {
    char t[24];
    long n = 0, r;
    unsigned long q;
    if (v == 0) { fputc('0', stderr); return; }
    while (v != 0) {
        q = (v >> 1) / 5UL;
        r = (long)(v - q * 10UL);
        t[n++] = (char)('0' + r);
        v = q;
    }
    while (n > 0) { n--; fputc((int)t[n], stderr); }
}
RS_ATTR static void rs_panic_head(long fb, long fl, long line, long col) {
    fflush(stdout);
    rs_err_str("thread 'main' panicked at ");
    rs_write_bytes(2, fb, fl);
    fputc(':', stderr);
    rs_err_udec((unsigned long)line);
    fputc(':', stderr);
    rs_err_udec((unsigned long)col);
    rs_err_str(":\n");
}
RS_ATTR static void rs_panic_tail(void) {
    rs_err_str("\nnote: run with `RUST_BACKTRACE=1` environment variable to display a backtrace\n");
    exit(101);
}
/* file and message are (byte address, byte length) pairs */
RS_ATTR static void rs_panic(long file_base, long file_len, long line, long col,
                             long msg_base, long msg_len) {
    rs_panic_head(file_base, file_len, line, col);
    rs_write_bytes(2, msg_base, msg_len);
    rs_panic_tail();
}
/* message is a C string (one char per word) */
RS_ATTR static void rs_panic_c(long file_base, long file_len, long line, long col,
                               const char *msg) {
    rs_panic_head(file_base, file_len, line, col);
    rs_err_str(msg);
    rs_panic_tail();
}
RS_ATTR static void rs_panic_bounds(long file_base, long file_len, long line, long col,
                                    long index, long len) {
    rs_panic_head(file_base, file_len, line, col);
    rs_err_str("index out of bounds: the len is ");
    rs_err_udec((unsigned long)len);
    rs_err_str(" but the index is ");
    rs_err_udec((unsigned long)index);
    rs_panic_tail();
}
/* slice range a[lo..hi] of a slice of len elements */
RS_ATTR static void rs_check_range(long file_base, long file_len, long line, long col,
                                   long lo, long hi, long len) {
    if ((unsigned long)lo > (unsigned long)hi) {
        rs_panic_head(file_base, file_len, line, col);
        rs_err_str("slice index starts at ");
        rs_err_udec((unsigned long)lo);
        rs_err_str(" but ends at ");
        rs_err_udec((unsigned long)hi);
        rs_panic_tail();
    }
    if ((unsigned long)hi > (unsigned long)len) {
        rs_panic_head(file_base, file_len, line, col);
        rs_err_str("range end index ");
        rs_err_udec((unsigned long)hi);
        rs_err_str(" out of range for slice of length ");
        rs_err_udec((unsigned long)len);
        rs_panic_tail();
    }
}
/* 1 if byte offset i of the str is a char boundary */
RS_ATTR static long rs_is_char_boundary(long base, long len, long i) {
    if (i == 0 || i == len) return 1;
    if (i > len) return 0;
    return (__ldb(base + i) & 0xC0) != 0x80;
}
/* `s` with backquotes for the str messages */
RS_ATTR static void rs_err_quoted(long base, long len) {
    fputc('`', stderr);
    rs_write_bytes(2, base, len);
    fputc('`', stderr);
}
/* str range s[lo..hi]: rustc's messages for out of bounds, lo > hi and
   offsets inside a multi-byte char */
RS_ATTR static void rs_check_str_range(long file_base, long file_len, long line, long col,
                                       long base, long len, long lo, long hi) {
    long bad = -1, s, e, cp;
    if ((unsigned long)lo > (unsigned long)len || (unsigned long)hi > (unsigned long)len) {
        rs_panic_head(file_base, file_len, line, col);
        rs_err_str("byte index ");
        rs_err_udec((unsigned long)((unsigned long)lo > (unsigned long)len ? lo : hi));
        rs_err_str(" is out of bounds of ");
        rs_err_quoted(base, len);
        rs_panic_tail();
    }
    if ((unsigned long)lo > (unsigned long)hi) {
        rs_panic_head(file_base, file_len, line, col);
        rs_err_str("begin <= end (");
        rs_err_udec((unsigned long)lo);
        rs_err_str(" <= ");
        rs_err_udec((unsigned long)hi);
        rs_err_str(") when slicing ");
        rs_err_quoted(base, len);
        rs_panic_tail();
    }
    if (!rs_is_char_boundary(base, len, lo)) bad = lo;
    else if (!rs_is_char_boundary(base, len, hi)) bad = hi;
    if (bad < 0) return;
    s = bad;
    while (s > 0 && (__ldb(base + s) & 0xC0) == 0x80) s--;
    e = s + 1;
    while (e < len && (__ldb(base + e) & 0xC0) == 0x80) e++;
    cp = __ldb(base + s);
    if (cp >= 0xF0) cp &= 7; else if (cp >= 0xE0) cp &= 15; else cp &= 31;
    { long k; for (k = s + 1; k < e; k++) cp = (cp << 6) | (__ldb(base + k) & 63); }
    rs_panic_head(file_base, file_len, line, col);
    rs_err_str("byte index ");
    rs_err_udec((unsigned long)bad);
    rs_err_str(" is not a char boundary; it is inside '");
    fputc((int)cp, stderr);
    rs_err_str("' (bytes ");
    rs_err_udec((unsigned long)s);
    rs_err_str("..");
    rs_err_udec((unsigned long)e);
    rs_err_str(") of ");
    rs_err_quoted(base, len);
    rs_panic_tail();
}

/* ------------------------------------------------------ integer arithmetic */
/* wrapping operations (no signed overflow in C) */
RS_ATTR static long rs_wadd(long a, long b) { return (long)((unsigned long)a + (unsigned long)b); }
RS_ATTR static long rs_wsub(long a, long b) { return (long)((unsigned long)a - (unsigned long)b); }
RS_ATTR static long rs_wmul(long a, long b) { return (long)((unsigned long)a * (unsigned long)b); }
RS_ATTR static long rs_wneg(long a) { return (long)(0UL - (unsigned long)a); }
#define RS_I64_MIN ((long)(1UL << 63))
/* overflow predicates: 1 when the mathematical result is outside the range;
   the wrapped result is rs_wadd / rs_wsub / rs_wmul */
RS_ATTR static long rs_add_ovf_i64(long a, long b) {
    long r = rs_wadd(a, b);
    return ((a ^ r) & (b ^ r)) < 0;
}
RS_ATTR static long rs_sub_ovf_i64(long a, long b) {
    long r = rs_wsub(a, b);
    return ((a ^ b) & (a ^ r)) < 0;
}
RS_ATTR static long rs_mul_ovf_i64(long a, long b) {
    long r;
    if (a == 0 || b == 0) return 0;
    if (a == -1) return b == RS_I64_MIN;
    if (b == -1) return a == RS_I64_MIN;
    r = rs_wmul(a, b);
    return r / b != a;
}
RS_ATTR static long rs_neg_ovf_i64(long a) { return a == RS_I64_MIN; }
RS_ATTR static long rs_div_ovf_i64(long a, long b) { return a == RS_I64_MIN && b == -1; }
RS_ATTR static long rs_add_ovf_u64(long a, long b) { return (unsigned long)rs_wadd(a, b) < (unsigned long)a; }
RS_ATTR static long rs_sub_ovf_u64(long a, long b) { return (unsigned long)a < (unsigned long)b; }
RS_ATTR static long rs_mul_ovf_u64(long a, long b) {
    if (a == 0) return 0;
    return (long)((unsigned long)rs_wmul(a, b) / (unsigned long)a != (unsigned long)b);
}
/* unsigned compare, division, remainder, shifts on the u64 bit pattern */
RS_ATTR static long rs_ult(long a, long b) { return (unsigned long)a < (unsigned long)b; }
RS_ATTR static long rs_ule(long a, long b) { return (unsigned long)a <= (unsigned long)b; }
RS_ATTR static long rs_udiv(long a, long b) { return (long)((unsigned long)a / (unsigned long)b); }
RS_ATTR static long rs_urem(long a, long b) { return (long)((unsigned long)a % (unsigned long)b); }
RS_ATTR static long rs_shl(long v, long n) { return (long)((unsigned long)v << n); }
RS_ATTR static long rs_shr_u(long v, long n) { return (long)((unsigned long)v >> n); }
RS_ATTR static long rs_shr_i(long v, long n) { return v >= 0 ? v >> n : ~((~v) >> n); }

/* narrow types: normalize to `bits` (1..64) as an `as` cast does */
RS_ATTR static long rs_wrap(long value, long bits, long is_signed) {
    unsigned long m, v;
    if (bits >= 64) return value;
    m = (1UL << bits) - 1UL;
    v = (unsigned long)value & m;
    if (is_signed && ((v >> (bits - 1)) & 1UL)) v |= ~m;
    return (long)v;
}
/* 1 when value (a normalized or a plain wide result) is inside the type */
RS_ATTR static long rs_in_range(long value, long bits, long is_signed) {
    long lim;
    if (bits >= 64) return 1;
    if (is_signed) {
        lim = 1L << (bits - 1);
        return value >= -lim && value < lim;
    }
    return (unsigned long)value <= (1UL << bits) - 1UL;
}

/* ------------------------------------------------------------------ floats */
union rs_fbits { double d; unsigned long u; };
RS_ATTR static long rs_f64_to_bits(double v) {
    union rs_fbits b;
    b.d = v;
    return (long)b.u;
}
RS_ATTR static double rs_f64_from_bits(long u) {
    union rs_fbits b;
    b.u = (unsigned long)u;
    return b.d;
}
RS_ATTR static long rs_f64_is_nan(double v) {
    unsigned long u = (unsigned long)rs_f64_to_bits(v) & 0x7fffffffffffffffUL;
    return u > 0x7ff0000000000000UL;
}
/* `v as iN/uN` for bits 1..64: truncates toward zero, saturates, NaN is 0.
   Works on the bit pattern (the simulator's float compare is unreliable for NaN). */
RS_ATTR static long rs_f64_to_int(double v, long bits, long is_signed) {
    unsigned long u = (unsigned long)rs_f64_to_bits(v);
    unsigned long mag, lim;
    long e = (long)((u >> 52) & 0x7ffUL), neg = (long)(u >> 63), ue;
    if (e == 0x7ff && (u & 0xfffffffffffffUL) != 0) return 0;
    if (e < 1023) return 0;
    ue = e - 1023;
    if (is_signed) {
        lim = 1UL << (bits - 1); /* magnitude limit of the negative side */
        if (ue >= 64) mag = ~0UL;
        else if (ue >= 52) mag = ((u & 0xfffffffffffffUL) | (1UL << 52)) << (ue - 52);
        else mag = ((u & 0xfffffffffffffUL) | (1UL << 52)) >> (52 - ue);
        if (neg) return mag >= lim ? (long)(0UL - lim) : (long)(0UL - mag);
        return mag >= lim ? (long)(lim - 1UL) : (long)mag;
    }
    if (neg) return 0;
    lim = bits >= 64 ? ~0UL : (1UL << bits) - 1UL;
    if (ue >= 64) mag = ~0UL;
    else if (ue >= 52) mag = ((u & 0xfffffffffffffUL) | (1UL << 52)) << (ue - 52);
    else mag = ((u & 0xfffffffffffffUL) | (1UL << 52)) >> (52 - ue);
    return (long)(mag > lim ? lim : mag);
}
/* u64 bit pattern to f64 (round to nearest even) */
RS_ATTR static double rs_u64_to_f64(long x) {
    unsigned long h;
    if (x >= 0) return (double)x;
    h = ((unsigned long)x >> 1) | ((unsigned long)x & 1UL);
    return (double)(long)h * 2.0;
}
/* round an f64 to the nearest f32 value (kept in a double); `x as f32` */
RS_ATTR static double rs_round_f32(double v) {
    unsigned long u = (unsigned long)rs_f64_to_bits(v), sig, q, rem, half, sgn = u & (1UL << 63);
    long e = (long)((u >> 52) & 0x7ffUL), E, drop, k;
    if (e == 0x7ff) return v;
    if (e == 0) return rs_f64_from_bits((long)sgn); /* f64 subnormal: below f32 range */
    E = e - 1023;
    if (E > 127) return rs_f64_from_bits((long)(sgn | 0x7ff0000000000000UL));
    sig = (u & 0xfffffffffffffUL) | (1UL << 52);
    drop = E >= -126 ? 29 : 29 + (-126 - E);
    if (drop > 53) return rs_f64_from_bits((long)sgn);
    if (drop == 53) { q = 0; rem = sig; }
    else { q = sig >> drop; rem = sig & ((1UL << drop) - 1UL); }
    half = 1UL << (drop - 1);
    if (rem > half || (rem == half && (q & 1UL))) q++;
    if (E >= -126 && (q >> 24) != 0) { /* mantissa carried into a new bit */
        q >>= 1; drop++;
        if (E + 1 > 127) return rs_f64_from_bits((long)(sgn | 0x7ff0000000000000UL));
    }
    k = drop + e - 1075; /* value = q * 2^k, k >= -149 */
    return rs_f64_from_bits((long)(sgn | (unsigned long)((k + 1023)) << 52)) * (double)(long)q;
}

/* ---------------------------------------------------------------- bignums */
/* non negative integers in 32 bit limbs held in longs, little endian */
#define RS_BN 100
struct rs_big { long n; long d[RS_BN]; };
#define RS_M32 0xffffffffL

static void rs_bn_set(struct rs_big *a, unsigned long v) {
    a->n = 0;
    while (v != 0) { a->d[a->n++] = (long)(v & 0xffffffffUL); v >>= 32; }
}
static void rs_bn_copy(struct rs_big *a, struct rs_big *b) {
    long i;
    a->n = b->n;
    for (i = 0; i < b->n; i++) a->d[i] = b->d[i];
}
static void rs_bn_mul_small(struct rs_big *a, long m) { /* m < 2^31 */
    long i, t, c = 0;
    for (i = 0; i < a->n; i++) {
        t = a->d[i] * m + c;
        a->d[i] = t & RS_M32;
        c = t >> 32;
    }
    while (c != 0) { a->d[a->n++] = c & RS_M32; c >>= 32; }
}
static void rs_bn_mul_pow2(struct rs_big *a, long bits) {
    long ws = bits >> 5, bs = bits & 31, i, t, c = 0;
    if (a->n == 0) return;
    if (bs != 0) {
        for (i = 0; i < a->n; i++) {
            t = (a->d[i] << bs) | c;
            a->d[i] = t & RS_M32;
            c = t >> 32;
        }
        if (c != 0) a->d[a->n++] = c;
    }
    if (ws != 0) {
        for (i = a->n - 1; i >= 0; i--) a->d[i + ws] = a->d[i];
        for (i = 0; i < ws; i++) a->d[i] = 0;
        a->n += ws;
    }
}
static void rs_bn_mul_pow10(struct rs_big *a, long k) {
    long p;
    while (k >= 9) { rs_bn_mul_small(a, 1000000000L); k -= 9; }
    if (k > 0) { p = 1; while (k > 0) { p *= 10; k--; } rs_bn_mul_small(a, p); }
}
static void rs_bn_mul_pow5(struct rs_big *a, long k) {
    long p;
    while (k >= 13) { rs_bn_mul_small(a, 1220703125L); k -= 13; }
    if (k > 0) { p = 1; while (k > 0) { p *= 5; k--; } rs_bn_mul_small(a, p); }
}
static void rs_bn_add(struct rs_big *a, struct rs_big *b) {
    long i, t, c = 0, n = a->n > b->n ? a->n : b->n;
    for (i = 0; i < n; i++) {
        t = (i < a->n ? a->d[i] : 0) + (i < b->n ? b->d[i] : 0) + c;
        a->d[i] = t & RS_M32;
        c = t >> 32;
    }
    a->n = n;
    if (c != 0) a->d[a->n++] = c;
}
static long rs_bn_cmp(struct rs_big *a, struct rs_big *b) {
    long i;
    if (a->n != b->n) return a->n < b->n ? -1 : 1;
    for (i = a->n - 1; i >= 0; i--) {
        if (a->d[i] != b->d[i]) return a->d[i] < b->d[i] ? -1 : 1;
    }
    return 0;
}
static void rs_bn_sub(struct rs_big *a, struct rs_big *b) { /* a >= b */
    long i, t, br = 0;
    for (i = 0; i < a->n; i++) {
        t = a->d[i] - (i < b->n ? b->d[i] : 0) - br;
        if (t < 0) { t += 4294967296L; br = 1; } else br = 0;
        a->d[i] = t;
    }
    while (a->n > 0 && a->d[a->n - 1] == 0) a->n--;
}
static long rs_bn_divmod_small(struct rs_big *a, long m) { /* m < 2^31; returns the remainder */
    long i, t, r = 0;
    for (i = a->n - 1; i >= 0; i--) {
        t = (r << 32) | a->d[i];
        a->d[i] = t / m;
        r = t % m;
    }
    while (a->n > 0 && a->d[a->n - 1] == 0) a->n--;
    return r;
}
/* floor(m / scale) < 10; m becomes the remainder.  s2, s4, s8 are 2, 4, 8 * scale */
static long rs_bn_digit(struct rs_big *m, struct rs_big *s, struct rs_big *s2,
                        struct rs_big *s4, struct rs_big *s8) {
    long d = 0;
    if (rs_bn_cmp(m, s8) >= 0) { rs_bn_sub(m, s8); d += 8; }
    if (rs_bn_cmp(m, s4) >= 0) { rs_bn_sub(m, s4); d += 4; }
    if (rs_bn_cmp(m, s2) >= 0) { rs_bn_sub(m, s2); d += 2; }
    if (rs_bn_cmp(m, s) >= 0) { rs_bn_sub(m, s); d += 1; }
    return d;
}

/* ------------------------------------------------------- float formatting */
#define RS_FMT_CAP 400
#define RS_NDIG 800

/* Decodes the magnitude bits of a finite float: value = mant * 2^exp2.  minnorm
   is 1 for a normal number whose mantissa is the minimum (power of two). */
static void rs_decode(unsigned long a, long is32, long *mant, long *exp2, long *minnorm) {
    long e = (long)(a >> 52), E;
    unsigned long m = a & 0xfffffffffffffUL, sig;
    *minnorm = 0;
    if (e == 0) { *mant = (long)m; *exp2 = -1074; return; }
    sig = m | (1UL << 52);
    if (!is32) { *mant = (long)sig; *exp2 = e - 1075; *minnorm = (m == 0); return; }
    E = e - 1023;
    if (E >= -126) { *mant = (long)(sig >> 29); *exp2 = E - 23; *minnorm = (m == 0); }
    else { *mant = (long)(sig >> (-97 - E)); *exp2 = -149; }
}

/* Exact decimal digits: value = 0.D * 10^dp, D without trailing zeros.
   Returns the digit count (0 for zero). */
static long rs_exact(long mant, long exp2, char *dig, long *dp) {
    struct rs_big n;
    char t[RS_NDIG + 16];
    long nt = 0, i, r, e10 = 0, nd;
    rs_bn_set(&n, (unsigned long)mant);
    if (exp2 >= 0) rs_bn_mul_pow2(&n, exp2);
    else { rs_bn_mul_pow5(&n, -exp2); e10 = exp2; }
    while (n.n > 0) {
        r = rs_bn_divmod_small(&n, 1000000000L);
        for (i = 0; i < 9; i++) { t[nt++] = (char)('0' + r % 10); r /= 10; }
    }
    while (nt > 0 && t[nt - 1] == '0') nt--;
    for (i = 0; i < nt; i++) dig[i] = t[nt - 1 - i];
    nd = nt;
    *dp = nd + e10;
    while (nd > 0 && dig[nd - 1] == '0') nd--;
    if (nd == 0) *dp = 0;
    return nd;
}

/* Shortest digits that round trip (Steele-White / Dragon4 as Rust's flt2dec):
   value = 0.d1..dn * 10^k.  mant/minus/plus/exp as Rust's Decoded. */
static long rs_shortest(long mant, long minus, long plus, long exp, long inclusive,
                        char *dig, long *kout) {
    struct rs_big m, mi, pl, sc, s2, s4, s8, t;
    long k, n = 0, up, down, c, nbits = 0, x, i, j;
    unsigned long ux = (unsigned long)(mant + plus - 1);
    while (ux != 0) { nbits++; ux >>= 1; }
    x = (nbits + exp) * 1292913986L; /* floor(x / 2^32) */
    k = x >= 0 ? (x >> 32) : -(((-x) + 4294967295L) >> 32);
    rs_bn_set(&m, (unsigned long)mant);
    rs_bn_set(&mi, (unsigned long)minus);
    rs_bn_set(&pl, (unsigned long)plus);
    rs_bn_set(&sc, 1UL);
    if (exp < 0) rs_bn_mul_pow2(&sc, -exp);
    else { rs_bn_mul_pow2(&m, exp); rs_bn_mul_pow2(&mi, exp); rs_bn_mul_pow2(&pl, exp); }
    if (k >= 0) rs_bn_mul_pow10(&sc, k);
    else { rs_bn_mul_pow10(&m, -k); rs_bn_mul_pow10(&mi, -k); rs_bn_mul_pow10(&pl, -k); }
    rs_bn_copy(&t, &m);
    rs_bn_add(&t, &pl);
    c = rs_bn_cmp(&sc, &t);
    if (inclusive ? c <= 0 : c < 0) k++;
    else { rs_bn_mul_small(&m, 10); rs_bn_mul_small(&mi, 10); rs_bn_mul_small(&pl, 10); }
    rs_bn_copy(&s2, &sc); rs_bn_mul_pow2(&s2, 1);
    rs_bn_copy(&s4, &sc); rs_bn_mul_pow2(&s4, 2);
    rs_bn_copy(&s8, &sc); rs_bn_mul_pow2(&s8, 3);
    for (;;) {
        dig[n++] = (char)('0' + rs_bn_digit(&m, &sc, &s2, &s4, &s8));
        c = rs_bn_cmp(&m, &mi);
        down = inclusive ? c <= 0 : c < 0;
        rs_bn_copy(&t, &m);
        rs_bn_add(&t, &pl);
        c = rs_bn_cmp(&sc, &t);
        up = inclusive ? c <= 0 : c < 0;
        if (down || up) break;
        rs_bn_mul_small(&m, 10); rs_bn_mul_small(&mi, 10); rs_bn_mul_small(&pl, 10);
    }
    if (up) {
        j = 1;
        if (down) {
            rs_bn_copy(&t, &m);
            rs_bn_mul_pow2(&t, 1);
            j = rs_bn_cmp(&t, &sc) >= 0;
        }
        if (j) {
            for (i = n - 1; i >= 0 && dig[i] == '9'; i--) { }
            if (i >= 0) { dig[i]++; for (j = i + 1; j < n; j++) dig[j] = '0'; }
            else { dig[0] = '1'; for (j = 1; j < n; j++) dig[j] = '0'; dig[n++] = '0'; k++; }
        }
    }
    *kout = k;
    return n;
}

/* Round D (nd digits, value 0.D * 10^dp) to `keep` digits, ties to even.
   Returns the new digit count; dp may grow by one. */
static long rs_round_digits(char *d, long nd, long *dp, long keep) {
    long up = 0, i;
    if (keep >= nd) return nd;
    if (keep < 0) { *dp = 0; return 0; }
    if (d[keep] > '5') up = 1;
    else if (d[keep] == '5') up = keep + 1 < nd ? 1 : (keep > 0 && ((d[keep - 1] - '0') & 1));
    nd = keep;
    if (up) {
        for (i = keep - 1; i >= 0 && d[i] == '9'; i--) { }
        if (i < 0) { d[0] = '1'; nd = 1; *dp = *dp + 1; }
        else { d[i]++; nd = i + 1; }
    }
    while (nd > 0 && d[nd - 1] == '0') nd--;
    if (nd == 0) *dp = 0;
    return nd;
}

static void rs_em(long buf, long *pos, long c) {
    if (*pos < RS_FMT_CAP) __stb(buf + *pos, c);
    (*pos)++;
}
static void rs_em_str(long buf, long *pos, const char *s) {
    while (*s) { rs_em(buf, pos, (long)*s); s++; }
}
/* integer part, then p decimals */
static void rs_put_fixed(long buf, long *pos, const char *d, long nd, long dp, long p) {
    long i, idx;
    if (dp <= 0) rs_em(buf, pos, '0');
    else for (i = 0; i < dp && *pos < RS_FMT_CAP; i++) rs_em(buf, pos, i < nd ? d[i] : '0');
    if (p > 0) {
        rs_em(buf, pos, '.');
        for (i = 0; i < p && *pos < RS_FMT_CAP; i++) {
            idx = dp + i;
            rs_em(buf, pos, (idx < 0 || idx >= nd) ? '0' : d[idx]);
        }
    }
}
/* d1[.d2..]e<exp>; p < 0: all digits, else exactly p decimals */
static void rs_put_exp(long buf, long *pos, const char *d, long nd, long dp, long p, long upper) {
    long i, e, n = 0;
    char t[24];
    rs_em(buf, pos, nd > 0 ? d[0] : '0');
    if (p < 0) {
        if (nd > 1) { rs_em(buf, pos, '.'); for (i = 1; i < nd; i++) rs_em(buf, pos, d[i]); }
    } else if (p > 0) {
        rs_em(buf, pos, '.');
        for (i = 1; i <= p && *pos < RS_FMT_CAP; i++) rs_em(buf, pos, i < nd ? d[i] : '0');
    }
    rs_em(buf, pos, upper ? 'E' : 'e');
    e = nd > 0 ? dp - 1 : 0;
    if (e < 0) { rs_em(buf, pos, '-'); e = -e; }
    if (e == 0) t[n++] = '0';
    while (e > 0) { t[n++] = (char)('0' + e % 10); e /= 10; }
    while (n > 0) { n--; rs_em(buf, pos, t[n]); }
}

static long rs_fmt_impl(unsigned long u, long is32, long prec, long has_prec, long mode, long buf) {
    long pos = 0, nd, dp = 0, mant, exp2, minn, keep, p, e, inclusive, minus, plus;
    unsigned long a = u & 0x7fffffffffffffffUL;
    char dig[RS_NDIG + 16];
    e = (long)(a >> 52);
    if (e == 0x7ff) {
        rs_em_str(buf, &pos, (a & 0xfffffffffffffUL) != 0 ? "NaN" : "inf");
        return pos;
    }
    if (prec > 100000) prec = 100000;
    rs_decode(a, is32, &mant, &exp2, &minn);
    if (mant == 0) { nd = 0; dp = 0; }
    else if (has_prec) {
        nd = rs_exact(mant, exp2, dig, &dp);
        keep = mode >= 2 ? prec + 1 : dp + prec;
        nd = rs_round_digits(dig, nd, &dp, keep);
    } else {
        inclusive = (mant & 1) == 0;
        if (minn) { mant <<= 2; minus = 1; plus = 2; exp2 -= 2; }
        else { mant <<= 1; minus = 1; plus = 1; exp2 -= 1; }
        nd = rs_shortest(mant, minus, plus, exp2, inclusive, dig, &dp);
    }
    if (mode >= 2) {
        rs_put_exp(buf, &pos, dig, nd, dp, has_prec ? prec : -1, mode == 3);
    } else if (mode == 1 && !has_prec && (dp >= 17 || (nd > 0 && dp <= -4))) {
        rs_put_exp(buf, &pos, dig, nd, dp, -1, 0);
    } else {
        if (has_prec) p = prec;
        else { p = nd - dp; if (p < (mode == 1 ? 1 : 0)) p = mode == 1 ? 1 : 0; }
        rs_put_fixed(buf, &pos, dig, nd, dp, p);
    }
    return pos > RS_FMT_CAP ? RS_FMT_CAP : pos;
}

/* Writes the text of |v| at byte address buf (room for 400 bytes; longer output
   is cut) and returns its length.  The caller prints the sign (bit 63).
   mode 0 Display, 1 Debug, 2 LowerExp, 3 UpperExp. */
RS_ATTR static long rs_fmt_f64(double v, long prec, long has_prec, long mode, long buf) {
    return rs_fmt_impl((unsigned long)rs_f64_to_bits(v), 0, prec, has_prec, mode, buf);
}
/* same for an f32 (v must already be an f32 value, see rs_round_f32) */
RS_ATTR static long rs_fmt_f32(double v, long prec, long has_prec, long mode, long buf) {
    return rs_fmt_impl((unsigned long)rs_f64_to_bits(v), 1, prec, has_prec, mode, buf);
}

/* ---------------------------------------------------------------- parsing */
static long rs_lower(long c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }
static long rs_match_word(long base, long len, long i, const char *w) {
    long j = 0;
    while (w[j]) {
        if (i + j >= len || rs_lower(__ldb(base + i + j)) != w[j]) return 0;
        j++;
    }
    return j;
}
/* Parses the bytes as a Rust f64 literal (sign, digits[.digits][e[sign]digits],
   inf, infinity, nan, ignoring case).  Stores the value at word address `out`
   (as the 64 bit pattern) and returns 1; returns 0 if invalid. */
RS_ATTR static long rs_parse_f64(long base, long len, long out) {
    long i = 0, neg = 0, c, nd = 0, j, k, fl, ex, ngx;
    unsigned long bits;
    char *s;
    char t[24];
    double v;
    if (len <= 0) return 0;
    c = __ldb(base);
    if (c == '+' || c == '-') { neg = c == '-'; i = 1; }
    if (i >= len) return 0;
    j = rs_match_word(base, len, i, "infinity");
    if (j == 0) j = rs_match_word(base, len, i, "inf");
    if (j != 0 && i + j == len) {
        bits = 0x7ff0000000000000UL;
    } else if ((j = rs_match_word(base, len, i, "nan")) != 0 && i + j == len) {
        bits = 0x7ff8000000000000UL;
    } else {
        k = i;
        while (k < len && (c = __ldb(base + k)) >= '0' && c <= '9') { k++; nd++; }
        if (k < len && __ldb(base + k) == '.') {
            k++;
            while (k < len && (c = __ldb(base + k)) >= '0' && c <= '9') { k++; nd++; }
        }
        if (nd == 0) return 0;
        if (k < len && (__ldb(base + k) == 'e' || __ldb(base + k) == 'E')) {
            k++;
            if (k < len && (__ldb(base + k) == '+' || __ldb(base + k) == '-')) k++;
            j = 0;
            while (k < len && (c = __ldb(base + k)) >= '0' && c <= '9') { k++; j++; }
            if (j == 0) return 0;
        }
        if (k != len) return 0;
        /* Rewrite as <significant digits>e<exponent>: the C library's strtod is
           exact for that form (it loses long runs of leading zeros). */
        s = (char *)malloc(len + 32);
        if (s == 0) rs_abort();
        nd = 0; fl = 0; j = 0;
        for (k = i; k < len; k++) {
            c = __ldb(base + k);
            if (c == '.') { j = 1; continue; }
            if (c < '0' || c > '9') break;
            if (nd == 0 && c == '0') { if (j) fl++; continue; }
            s[nd++] = (char)c;
            if (j) fl++;
        }
        ex = 0; ngx = 0;
        if (k < len) { /* at e or E */
            k++;
            c = __ldb(base + k);
            if (c == '+' || c == '-') { ngx = c == '-'; k++; }
            for (; k < len; k++) {
                if (ex < 100000000L) ex = ex * 10 + (__ldb(base + k) - '0');
            }
            if (ngx) ex = -ex;
        }
        ex -= fl;
        while (nd > 0 && s[nd - 1] == '0') { nd--; ex++; }
        if (nd == 0) bits = 0UL;
        else {
            s[nd++] = 'e';
            if (ex < 0) { s[nd++] = '-'; ex = -ex; }
            j = 0;
            do { t[j++] = (char)('0' + ex % 10); ex /= 10; } while (ex > 0);
            while (j > 0) s[nd++] = t[--j];
            s[nd] = 0;
            v = strtod(s, 0);
            bits = (unsigned long)rs_f64_to_bits(v);
        }
        free(s);
    }
    if (neg) bits |= 1UL << 63;
    RS_W(out) = (long)bits;
    return 1;
}

#endif /* RS_RT_C */

/* ------------------------------------------------------ f64 functions */
RS_ATTR static double rs_sqrt(double v) { return sqrt(v); }
RS_ATTR static double rs_floor(double v) { return floor(v); }
RS_ATTR static double rs_ceil(double v) { return ceil(v); }
/* Rust `%` on floats: the remainder with the sign of the dividend (C fmod) */
RS_ATTR static double rs_fmod(double a, double b) { return fmod(a, b); }
