/*
 * vb_rt.c - the run-time library of the VB.NET compiler.
 *
 * Compiled by the CPUSim64 C11 compiler (build.sh) into the linker module
 * rt/vb_rt.asm, which every compiled program includes.  C main() calls
 * vb_program(), which the compiler generates.  The compiled code calls the
 * vbrt_* functions with the C calling convention (arguments pushed right to
 * left, result in r0 or f0).
 *
 * Every value is one 64-bit word (a C11 long or double).  Objects are
 * pointers p with two header words: p[-2] the type tag, p[-1] for the
 * garbage collector.
 *   String: p[0] length, p[1..] one codepoint per word (string literals are
 *           .dcw data with the same layout)
 *   Array:  p[0] number of elements, p[1] rank, p[2 .. 2+rank-1] the
 *           length of each dimension, then the elements in row-major order
 *   Box:    p[0] the value (a boxed Boolean, number or Char)
 * The tag is the TY_* kind of src/vbc.h (strings and boxes), for arrays
 * -(256 * element kind + 65536 * rank), and for the objects of classes,
 * structures and boxed enums the address of the type descriptor:
 *   d[0] kind (D_CLASS, D_STRUCT, D_ENUM, D_INTERFACE), d[1] the full name
 *   (a String), d[2] the words of an instance, d[3] the base type's
 *   descriptor;
 *   classes, structures, interfaces: d[4] the number k of fields of
 *   structure types, d[5] the vtable size, d[6] the address of k pairs
 *   (slot, descriptor), d[7] the address of the vtable (method addresses;
 *   slots 0, 1, 2: ToString, Equals, GetHashCode, 0 for the default),
 *   d[8] the number of interfaces n, d[9] the number of reference fields
 *   m, d[10] the address of their m slots, then from d[11] n pairs
 *   (interface descriptor, itable address);
 *   enums: d[4] the underlying kind, d[5] the number of members n, then n
 *   pairs (value, name).
 * Boolean True is -1.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <time.h>
#include <_Thread.h>

/* must match the TY_* kinds of src/vbc.h */
#define K_BOOLEAN 1
#define K_SBYTE 2
#define K_BYTE 3
#define K_SHORT 4
#define K_USHORT 5
#define K_INTEGER 6
#define K_UINTEGER 7
#define K_LONG 8
#define K_ULONG 9
#define K_DECIMAL 10
#define K_SINGLE 11
#define K_DOUBLE 12
#define K_DATE 13
#define K_CHAR 14
#define K_STRING 15
#define K_OBJECT 16
#define K_NOTHING 17
#define K_ARRAY 18

#define ARR_TAG(ek, rank) (-(256L * (ek) + 65536L * (rank)))
#define D_CLASS 1
#define D_STRUCT 2
#define D_ENUM 3
#define D_INTERFACE 4

/* exceptions (vbrt_throw) */
#define E_OVERFLOW 1
#define E_DIVZERO 2
#define E_INDEX 3
#define E_NULLREF 4
#define E_CAST 5
#define E_FORMAT 6

long vb_program(void);
long *vb_make_exception(long code, long *message);     /* generated: VBRuntimeHelpers.MakeException */
void vb_throw_obj(long *ex);                            /* generated: throws ex */
static long *format_obj(long *o, long *fmt);          /* vb_fmt.c */

static long vb_argc;
static char **vb_argv;

/* threads (vb_thread.c): the number running, and the locks of the allocator and of the console */
static long thr_active;
static spinlock_t thr_alloc_lock;
static spinlock_t thr_out_lock;

/* takes the lock if other threads run; returns whether it did */
static long thr_lock(spinlock_t *l) {
    if (!thr_active) return 0;
    spinlock_lock(l);
    return 1;
}

/* ------------------------------------------------------------ errors */

static long *str_from_c(const char *c);

/* the codes of VBRuntimeHelpers.MakeException (lib/System.vb) */
static const char *exc_names[] = {
    "", "OverflowException", "DivideByZeroException", "IndexOutOfRangeException", "NullReferenceException",
    "InvalidCastException", "FormatException", "ArgumentOutOfRangeException", "ArgumentException",
    "ArgumentNullException", "OutOfMemoryException", "InvalidOperationException", "ArrayTypeMismatchException",
    "ArithmeticException", "KeyNotFoundException", "NotSupportedException", "NotImplementedException",
    "FileNotFoundException", "IOException", "ObjectDisposedException", 0
};

/* throws the .NET exception System.<type> with the message */
static void fail(const char *type, const char *msg) {
    long code = 0, i;
    for (i = 1; exc_names[i]; i++) if (strcmp(exc_names[i], type) == 0) code = i;
    vb_throw_obj(vb_make_exception(code, msg ? str_from_c(msg) : 0));
    exit(1);
}

/* an exception that no Catch handles */
void vbrt_unhandled(long *ex) {
    long *s, i;
    long *vbrt_obj_str(long *o);
    fflush(stdout);
    s = vbrt_obj_str(ex);
    fputs("Unhandled exception. ", stderr);
    for (i = 0; s && i < s[0]; i++) fputc((int)s[1 + i], stderr);
    fputc('\n', stderr);
    fflush(stderr);
    exit(1);
}

void vbrt_throw(long id) {
    switch (id) {
    case E_OVERFLOW: fail("OverflowException", "Arithmetic operation resulted in an overflow.");
    case E_DIVZERO: fail("DivideByZeroException", "Attempted to divide by zero.");
    case E_INDEX: fail("IndexOutOfRangeException", "Index was outside the bounds of the array.");
    case E_NULLREF: fail("NullReferenceException", "Object reference not set to an instance of an object.");
    case E_CAST: fail("InvalidCastException", "Specified cast is not valid.");
    }
    fail("FormatException", "Input string was not in a correct format.");
}

/* ------------------------------------------------------------ memory */

#include "vb_gc.c"

/* ------------------------------------------------------------ strings */

long *vbrt_str_new(long len) {
    long *s = vbrt_alloc(len + 1, K_STRING);
    s[0] = len;
    return s;
}

static long *str_from_c(const char *c) {
    long n = strlen(c), i;
    long *s = vbrt_str_new(n);
    for (i = 0; i < n; i++) s[i + 1] = c[i];
    return s;
}

static long slen(long *s) { return s ? s[0] : 0; }

long *vbrt_concat(long *a, long *b) {
    long na = slen(a), nb = slen(b), i;
    long *s;
    if (nb == 0 && a) return a;
    if (na == 0 && b) return b;
    s = vbrt_str_new(na + nb);
    for (i = 0; i < na; i++) s[1 + i] = a[1 + i];
    for (i = 0; i < nb; i++) s[1 + na + i] = b[1 + i];
    return s;
}

static long fold(long c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    if (c >= 0xC0 && c <= 0xDE && c != 0xD7) return c + 32;
    return c;
}

/* -1, 0 or 1; Nothing is the empty string; text: Option Compare Text */
long vbrt_strcmp(long *a, long *b, long text) {
    long na = slen(a), nb = slen(b), i;
    for (i = 0; i < na && i < nb; i++) {
        long x = a[1 + i], y = b[1 + i];
        if (text) {
            x = fold(x);
            y = fold(y);
        }
        if (x != y) return x < y ? -1 : 1;
    }
    return na < nb ? -1 : na > nb ? 1 : 0;
}

long *vbrt_str_char(long c) {
    long *s = vbrt_str_new(1);
    s[1] = c;
    return s;
}

long vbrt_char_at(long *s, long i) {
    if (!s) vbrt_throw(E_NULLREF);
    if (i < 0 || i >= s[0]) vbrt_throw(E_INDEX);
    return s[1 + i];
}

long vbrt_str_length(long *s) {
    if (!s) vbrt_throw(E_NULLREF);
    return s[0];
}

/* ------------------------------------------------------------ number formatting */

static double round_even(double d) {
    double f = floor(d), r = d - f;
    if (r > 0.5) return f + 1;
    if (r < 0.5) return f;
    return fmod(f, 2.0) == 0.0 ? f : f + 1;
}

/* rounds a double to the nearest Single (IEEE binary32) value */
double vbrt_to_single(double x) {
    int e, p;
    double m, r;
    if (isnan(x) || isinf(x) || x == 0.0) return x;
    m = frexp(x, &e);           /* x = m * 2^e, 0.5 <= |m| < 1 */
    p = 24;
    if (e - 1 < -126) p = 24 - (-126 - (e - 1));
    if (p <= 0) {
        /* below half the smallest subnormal: zero (or the smallest one) */
        r = ldexp(round_even(ldexp(m, p)), e - p);
        return r;
    }
    r = ldexp(round_even(ldexp(m, p)), e - p);
    if (fabs(r) >= 3.4028235677973366e+38) return x > 0 ? INFINITY : -INFINITY;
    return r;
}

/* .NET Double.ToString() / Single.ToString(): the shortest digits that
   round-trip; scientific notation from 1E+15 (Single: 1E+07) and below
   1E-05 */
static void fmt_float(char *out, double d, int single) {
    char tmp[64], digits[32];
    int p, nd = 0, e, i, k = 0, maxp = single ? 9 : 17, big = single ? 7 : 15;
    char *q;
    if (isnan(d)) { strcpy(out, "NaN"); return; }
    if (isinf(d)) { strcpy(out, d > 0 ? "Infinity" : "-Infinity"); return; }
    if (d == 0.0) { strcpy(out, signbit(d) ? "-0" : "0"); return; }
    for (p = 1; p <= maxp; p++) {
        double v;
        snprintf(tmp, sizeof tmp, "%.*e", p - 1, d);
        v = strtod(tmp, 0);
        if (single ? vbrt_to_single(v) == d : v == d) break;
    }
    /* tmp: [-]d.ddde[+-]xx */
    q = tmp;
    if (*q == '-') {
        out[k++] = '-';
        q++;
    }
    for (; *q && *q != 'e'; q++) if (*q != '.') digits[nd++] = *q;
    e = atoi(q + 1);
    while (nd > 1 && digits[nd - 1] == '0') nd--;
    if (e >= big || e < -4) {
        out[k++] = digits[0];
        if (nd > 1) {
            out[k++] = '.';
            for (i = 1; i < nd; i++) out[k++] = digits[i];
        }
        k += sprintf(out + k, "E%c%02d", e < 0 ? '-' : '+', e < 0 ? -e : e);
        return;
    }
    if (e < 0) {
        out[k++] = '0';
        out[k++] = '.';
        for (i = 0; i < -e - 1; i++) out[k++] = '0';
        for (i = 0; i < nd; i++) out[k++] = digits[i];
    } else {
        for (i = 0; i <= e; i++) out[k++] = i < nd ? digits[i] : '0';
        if (nd > e + 1) {
            out[k++] = '.';
            for (i = e + 1; i < nd; i++) out[k++] = digits[i];
        }
    }
    out[k] = 0;
}

/* the text of a value of a primitive kind */
static void fmt_value(char *buf, long kind, long bits) {
    double d;
    switch (kind) {
    case K_BOOLEAN: strcpy(buf, bits ? "True" : "False"); return;
    case K_BYTE: case K_USHORT: case K_UINTEGER: case K_ULONG:
        sprintf(buf, "%llu", (unsigned long long)bits);
        return;
    case K_SINGLE: case K_DOUBLE:
        memcpy(&d, &bits, sizeof d);
        fmt_float(buf, d, kind == K_SINGLE);
        return;
    case K_CHAR: buf[0] = (char)bits; buf[1] = 0; return;
    }
    sprintf(buf, "%lld", (long long)bits);
}

/* a value of a primitive kind converted to String */
long *vbrt_to_str(long kind, long bits) {
    char buf[64];
    if (kind == K_STRING || kind == K_OBJECT) return 0;
    fmt_value(buf, kind, bits);
    return str_from_c(buf);
}

/* ------------------------------------------------------------ boxes */

long *vbrt_box(long kind, long bits) {
    long *o = vbrt_alloc(1, kind);
    o[0] = bits;
    return o;
}

static long tag_of(long *o) { return o[-2]; }

static const char *sys_name(long k) {
    static const char *n[] = {
        "Void", "Boolean", "SByte", "Byte", "Int16", "UInt16", "Int32", "UInt32", "Int64", "UInt64",
        "Decimal", "Single", "Double", "DateTime", "Char", "String", "Object", "Object", "Array"
    };
    return (k >= 0 && k <= K_ARRAY) ? n[k] : "Object";
}

static const char *vb_name(long k) {
    static const char *n[] = {
        "Void", "Boolean", "SByte", "Byte", "Short", "UShort", "Integer", "UInteger", "Long", "ULong",
        "Decimal", "Single", "Double", "Date", "Char", "String", "Object", "Object", "Array"
    };
    return (k >= 0 && k <= K_ARRAY) ? n[k] : "Object";
}

/* the text of an object with a type descriptor: an enum's member name (or
   its number), else the type name */
static long *desc_str(long *d, long *o) {
    if (d[0] != D_ENUM && ((long *)d[7])[0]) {
        /* an overridden ToString() */
        long *(*f)(long *) = (long *(*)(long *))((long *)d[7])[0];
        return f(o);
    }
    if (d[0] == D_ENUM) {
        long i, n = d[5];
        char buf[64];
        for (i = 0; i < n; i++)
            if (d[6 + 2 * i] == o[0]) return (long *)d[7 + 2 * i];
        fmt_value(buf, d[4], o[0]);
        return str_from_c(buf);
    }
    return (long *)d[1];
}

/* an enum value as its name (Color.Red.ToString()) */
long *vbrt_enum_str(long *d, long v) {
    long box[3];
    box[0] = (long)d;
    box[2] = v;
    return desc_str(d, box + 2);
}

/* Object.ToString() */
long *vbrt_obj_str(long *o) {
    long t;
    char buf[80];
    if (!o) return 0;
    t = tag_of(o);
    if (t == K_STRING) return o;
    if (t > 255) return desc_str((long *)t, o);
    if (t < 0) {
        long ek = ((-t) >> 8) & 255, rank = (-t) >> 16, i, k;
        k = sprintf(buf, "System.%s[", sys_name(ek));
        for (i = 1; i < rank; i++) buf[k++] = ',';
        buf[k++] = ']';
        buf[k] = 0;
        return str_from_c(buf);
    }
    fmt_value(buf, t, o[0]);
    return str_from_c(buf);
}

/* ------------------------------------------------------------ structures */

/* a new structure value of the type d (zero, with its structure fields)
   structure descriptors: d[4] the number k of fields of structure types,
   then k pairs (slot, descriptor) */
long *vbrt_struct_new(long *d) {
    long *o = vbrt_alloc(d[2] ? d[2] : 1, (long)d), i;
    long *pairs = (long *)d[6];
    for (i = 0; i < d[4]; i++) o[pairs[2 * i]] = (long)vbrt_struct_new((long *)pairs[2 * i + 1]);
    return o;
}

/* a copy of a structure value (a deep copy of its structure fields) */
long *vbrt_struct_copy(long *src) {
    long *d, *o, i;
    if (!src) return 0;
    d = (long *)tag_of(src);
    o = vbrt_alloc(d[2] ? d[2] : 1, (long)d);
    for (i = 0; i < d[2]; i++) o[i] = src[i];
    {
        long *pairs = (long *)d[6];
        for (i = 0; i < d[4]; i++) o[pairs[2 * i]] = (long)vbrt_struct_copy((long *)src[pairs[2 * i]]);
    }
    return o;
}

/* the elements of a new array of a structure type */
void vbrt_array_fill(long *a, long *d) {
    long i, *e = a + 2 + a[1];
    for (i = 0; i < a[0]; i++)
        if (!e[i]) e[i] = (long)vbrt_struct_new(d);
}

/* a shallow copy of an object of a class (Delegate.Combine) */
long *vbrt_clone(long *o) {
    long *d, *c, i;
    if (!o) return 0;
    d = (long *)tag_of(o);
    if ((long)d <= 255) vbrt_throw(E_CAST);
    c = vbrt_alloc(d[2] ? d[2] : 1, (long)d);
    for (i = 0; i < d[2]; i++) c[i] = o[i];
    return c;
}

/* the name of the type of an object, for messages */
static const char *type_of_obj(long *o) {
    static char buf[200];
    long t = tag_of(o), *n, i, k = 0;
    if (t > 255) {
        n = (long *)((long *)t)[1];
        for (i = 0; i < n[0] && k < 190; i++) buf[k++] = (char)n[1 + i];
        buf[k] = 0;
        return buf;
    }
    if (t < 0) return "System.Array";
    sprintf(buf, "System.%s", sys_name(t));
    return buf;
}

/* the itable of the interface d in the object o's class (interface calls) */
long *vbrt_itable(long *o, long *d) {
    long *c, i;
    if (!o) vbrt_throw(E_NULLREF);
    for (c = (long *)tag_of(o); c; c = (long *)c[3])
        for (i = 0; i < c[8]; i++)
            if ((long *)c[11 + 2 * i] == d) return (long *)c[12 + 2 * i];
    vbrt_throw(E_CAST);
    return 0;
}

/* Object.Equals: an override, else reference equality (structures: equal
   fields) */
long vbrt_obj_equals(long *o, long *other) {
    long t, *d, i;
    if (!o) vbrt_throw(E_NULLREF);
    t = tag_of(o);
    if (t > 255 && ((long *)t)[0] != D_ENUM && ((long *)((long *)t)[7])[1]) {
        /* Equals(obj): the method takes Me after its parameter */
        long (*f)(long *, long *) = (long (*)(long *, long *))((long *)((long *)t)[7])[1];
        return f(other, o) ? -1 : 0;
    }
    if (o == other) return -1;
    if (!other || tag_of(other) != t) return 0;
    if (t == K_STRING) return vbrt_strcmp(o, other, 0) == 0 ? -1 : 0;
    if (t <= 255) return o[0] == other[0] ? -1 : 0;
    d = (long *)t;
    if (d[0] == D_ENUM) return o[0] == other[0] ? -1 : 0;
    if (d[0] != D_STRUCT) return 0;
    for (i = 0; i < d[2]; i++) {
        long *a = (long *)o[i], *b2 = (long *)other[i];
        int nested = 0, k;
        long *pairs = (long *)d[6];
        for (k = 0; k < d[4]; k++) if (pairs[2 * k] == i) nested = 1;
        if (nested ? !vbrt_obj_equals(a, b2) : o[i] != other[i]) return 0;
    }
    return -1;
}

long vbrt_ref_equals(long *a, long *b2) { return a == b2 ? -1 : 0; }

/* Object.GetHashCode: an override, else from the address (or value) */
long vbrt_obj_hash(long *o) {
    long t;
    if (!o) vbrt_throw(E_NULLREF);
    t = tag_of(o);
    if (t > 255 && ((long *)t)[0] != D_ENUM && ((long *)((long *)t)[7])[2]) {
        long (*f)(long *) = (long (*)(long *))((long *)((long *)t)[7])[2];
        return f(o);
    }
    if (t == K_STRING) {
        long h = 5381, i;
        for (i = 0; i < o[0]; i++) h = (h * 33 + o[1 + i]) & 0x7FFFFFFF;
        return h;
    }
    if (t <= 255 || ((long *)t)[0] == D_ENUM) return (o[0] ^ (o[0] >> 32)) & 0x7FFFFFFF;
    return ((long)o >> 1) & 0x7FFFFFFF;
}

/* is the object o of the type d: a descriptor (a class and its base
   classes, an interface it implements) or a primitive kind */
static int is_instance(long *o, long *d) {
    long t = tag_of(o), *c;
    if ((long)d <= 255) return t == (long)d;
    if (t <= 255) return 0;
    for (c = (long *)t; c; c = (long *)c[3]) {
        long i;
        if (c == d) return 1;
        if (c[0] == D_ENUM) break;
        for (i = 0; i < c[8]; i++) if ((long *)c[11 + 2 * i] == d) return 1;
    }
    return 0;
}

/* TypeOf o Is T */
long vbrt_isinst(long *o, long *d) { return o && is_instance(o, d) ? -1 : 0; }

/* TryCast(o, T) */
long *vbrt_trycast(long *o, long *d) { return o && is_instance(o, d) ? o : 0; }

/* the full name of the type of an object (Exception messages) */
long *vbrt_type_fullname(long *o) {
    if (!o) vbrt_throw(E_NULLREF);
    if (tag_of(o) > 255) return (long *)((long *)tag_of(o))[1];
    return str_from_c(type_of_obj(o));
}

/* DirectCast(o, T) / CType(o, T) to a class or structure: o if it is an
   instance of the type d (or Nothing) */
long *vbrt_cast(long *o, long *d) {
    char msg[500];
    long *n = (long *)d[1], i, k;
    if (!o || is_instance(o, d)) return o;
    k = sprintf(msg, "Unable to cast object of type '%s' to type '", type_of_obj(o));
    for (i = 0; i < n[0] && k < 480; i++) msg[k++] = (char)n[1 + i];
    sprintf(msg + k, "'.");
    fail("InvalidCastException", msg);
    return 0;
}

/* ------------------------------------------------------------ conversions */

static void range_error(void) { vbrt_throw(E_OVERFLOW); }

/* checks that v fits the integral kind and returns it */
static long check_int(long kind, long v) {
    switch (kind) {
    case K_SBYTE: if (v < -128 || v > 127) range_error(); break;
    case K_BYTE: if (v < 0 || v > 255) range_error(); break;
    case K_SHORT: if (v < -32768 || v > 32767) range_error(); break;
    case K_USHORT: if (v < 0 || v > 65535) range_error(); break;
    case K_INTEGER: if (v < -2147483647 - 1 || v > 2147483647) range_error(); break;
    case K_UINTEGER: if (v < 0 || v > 4294967295) range_error(); break;
    case K_BOOLEAN: return v ? -1 : 0;
    }
    return v;
}

/* a Double converted to an integral kind: rounded to even (CInt), range
   checked */
long vbrt_d2i(double d, long kind) {
    double r;
    if (isnan(d)) range_error();
    if (kind == K_BOOLEAN) return d != 0.0 ? -1 : 0;
    r = round_even(d);
    if (kind == K_ULONG) {
        if (r < 0 || r >= 18446744073709551616.0) range_error();
        if (r >= 9223372036854775808.0) return (long)(r - 9223372036854775808.0) + (-9223372036854775807 - 1);
        return (long)r;
    }
    if (r < -9223372036854775808.0 || r >= 9223372036854775808.0) range_error();
    return check_int(kind, (long)r);
}

double vbrt_u2d(long u) {
    unsigned long long v = (unsigned long long)u;
    return (double)v;
}

static int ws(long c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

/* parses a VB numeric string ("12", " -1.5e3 ", "&HFF"); 0 on success */
static int parse_num(long *s, double *out, long *iout, int *is_int) {
    char buf[400];
    long n = slen(s), i = 0, k = 0;
    char *end;
    while (i < n && ws(s[1 + i])) i++;
    while (n > i && ws(s[n])) n--;
    if (i < n && s[1 + i] == '&' && i + 1 < n && (s[2 + i] == 'H' || s[2 + i] == 'h' || s[2 + i] == 'O' || s[2 + i] == 'o')) {
        int base = (s[2 + i] == 'H' || s[2 + i] == 'h') ? 16 : 8;
        unsigned long long v = 0;
        i += 2;
        if (i >= n) return 1;
        for (; i < n; i++) {
            long c = s[1 + i], dg;
            if (c >= '0' && c <= '9') dg = c - '0';
            else if (c >= 'a' && c <= 'f') dg = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') dg = c - 'A' + 10;
            else return 1;
            if (dg >= base) return 1;
            v = v * base + dg;
        }
        *iout = (long)v;
        *out = (double)(long)v;
        *is_int = 1;
        return 0;
    }
    if (n - i <= 0 || n - i >= 390) return 1;
    for (; i < n; i++) buf[k++] = (char)s[1 + i];
    buf[k] = 0;
    *out = strtod(buf, &end);
    *is_int = 0;
    return *end != 0;
}

static void cast_error(long *s, long kind) {
    char msg[300];
    long n = slen(s), i, k = 0;
    k = sprintf(msg, "Conversion from string \"");
    for (i = 0; i < n && k < 250; i++) msg[k++] = (char)s[1 + i];
    sprintf(msg + k, "\" to type '%s' is not valid.", vb_name(kind));
    fail("InvalidCastException", msg);
}

static long d_bits(double d) {
    long b;
    memcpy(&b, &d, sizeof b);
    return b;
}

static double bits_d(long b) {
    double d;
    memcpy(&d, &b, sizeof d);
    return d;
}

/* the value of kind `from` converted to kind `to` (both primitive, not
   String); bits in, bits out */
static long convert_prim(long from, long bits, long to) {
    double d;
    if (from == to) return bits;
    if (to == K_SINGLE || to == K_DOUBLE) {
        if (from == K_SINGLE || from == K_DOUBLE) d = bits_d(bits);
        else if (from == K_ULONG) d = vbrt_u2d(bits);
        else if (from == K_CHAR) vbrt_throw(E_CAST);
        else d = (double)bits;
        return d_bits(to == K_SINGLE ? vbrt_to_single(d) : d);
    }
    if (from == K_SINGLE || from == K_DOUBLE) return vbrt_d2i(bits_d(bits), to);
    if (to == K_CHAR || from == K_CHAR) {
        if (to == from) return bits;
        vbrt_throw(E_CAST);
    }
    if (to == K_BOOLEAN) return bits ? -1 : 0;
    if (from == K_ULONG && bits < 0 && to != K_ULONG) range_error();
    if (to == K_ULONG) {
        if (bits < 0 && from != K_BOOLEAN) range_error();
        return bits;
    }
    if (from == K_BOOLEAN) {
        /* True is -1, or the largest value of an unsigned type */
        if (!bits) return 0;
        switch (to) {
        case K_BYTE: return 255;
        case K_USHORT: return 65535;
        case K_UINTEGER: return 4294967295;
        }
        return -1;
    }
    return check_int(to, bits);
}

/* an Object (or String) converted to a primitive kind (CInt(o), CDbl(s),
   ...): bits; Nothing gives the default value */
long vbrt_unbox(long *o, long kind) {
    long t;
    if (!o) return 0;
    t = tag_of(o);
    if (t == K_STRING) {
        double d;
        long iv;
        int isint;
        if (kind == K_CHAR) return o[0] ? o[1] : 0;
        if (kind == K_BOOLEAN) {
            static const char *tf[] = { "true", "false" };
            int w;
            for (w = 0; w < 2; w++) {
                long n = slen(o), i, j = 0, ok = 1, a = 0;
                while (a < n && ws(o[1 + a])) a++;
                while (n > a && ws(o[n])) n--;
                if (n - a != (long)strlen(tf[w])) continue;
                for (i = a; i < n; i++, j++) if (fold(o[1 + i]) != tf[w][j]) ok = 0;
                if (ok) return w == 0 ? -1 : 0;
            }
        }
        if (parse_num(o, &d, &iv, &isint)) cast_error(o, kind);
        if (isint) return convert_prim(K_LONG, iv, kind);
        return convert_prim(K_DOUBLE, d_bits(d), kind);
    }
    if (t > 255) {
        long *d = (long *)t;
        if (d[0] != D_ENUM) vbrt_throw(E_CAST);
        return convert_prim(d[4], o[0], kind);
    }
    if (t < 0) vbrt_throw(E_CAST);
    return convert_prim(t, o[0], kind);
}

/* ------------------------------------------------------------ arithmetic */

long vbrt_l_mul(long a, long b) {
    long p;
    if (a == 0 || b == 0) return 0;
    if ((a == -1 && b == -9223372036854775807 - 1) || (b == -1 && a == -9223372036854775807 - 1)) range_error();
    p = (long)((unsigned long long)a * (unsigned long long)b);
    if (p / b != a) range_error();
    return p;
}

/* ULong arithmetic (op: '+', '-', '*', '/', '%') with overflow checks */
long vbrt_ul_op(long op, long a, long b) {
    unsigned long long x = (unsigned long long)a, y = (unsigned long long)b, r = 0;
    switch (op) {
    case '+': r = x + y; if (r < x) range_error(); break;
    case '-': if (y > x) range_error(); r = x - y; break;
    case '*': r = x * y; if (x && r / x != y) range_error(); break;
    case '/': if (!y) vbrt_throw(E_DIVZERO); r = x / y; break;
    case '%': if (!y) vbrt_throw(E_DIVZERO); r = x % y; break;
    }
    return (long)r;
}

long vbrt_ul_cmp(long a, long b) {
    unsigned long long x = (unsigned long long)a, y = (unsigned long long)b;
    return x < y ? -1 : x > y ? 1 : 0;
}

double vbrt_fmod(double a, double b) {
    return fmod(a, b);
}

/* ------------------------------------------------------------ arrays */

static long *new_array(long tag, long rank, long *lens) {
    long total = 1, i, *a;
    for (i = 0; i < rank; i++) {
        if (lens[i] < 0) range_error();
        total *= lens[i];
    }
    a = vbrt_alloc(2 + rank + total, tag);
    a[0] = total;
    a[1] = rank;
    for (i = 0; i < rank; i++) a[2 + i] = lens[i];
    return a;
}

/* New T(ub0, ub1, ...): the upper bounds of the dimensions */
long *vbrt_array_new(long tag, long rank, ...) {
    long lens[32], i;
    va_list ap;
    va_start(ap, rank);
    for (i = 0; i < rank; i++) lens[i] = va_arg(ap, long) + 1;
    va_end(ap);
    return new_array(tag, rank, lens);
}

/* the address of the element a(i0, i1, ...) */
long *vbrt_index(long *a, long rank, ...) {
    long lin = 0, i;
    va_list ap;
    if (!a) vbrt_throw(E_NULLREF);
    va_start(ap, rank);
    for (i = 0; i < rank; i++) {
        long ix = va_arg(ap, long);
        if (ix < 0 || ix >= a[2 + i]) vbrt_throw(E_INDEX);
        lin = lin * a[2 + i] + ix;
    }
    va_end(ap);
    return a + 2 + rank + lin;
}

/* ReDim [Preserve] a(ub0, ...) */
long *vbrt_redim(long *old, long preserve, long tag, long rank, ...) {
    long lens[32], i, *a;
    va_list ap;
    va_start(ap, rank);
    for (i = 0; i < rank; i++) lens[i] = va_arg(ap, long) + 1;
    va_end(ap);
    a = new_array(tag, rank, lens);
    if (preserve && old) {
        long j, n;
        for (i = 0; i < rank - 1; i++)
            if (old[2 + i] != lens[i])
                fail("ArrayTypeMismatchException",
                     "'ReDim' can only change the rightmost dimension.");
        /* copy row by row over the last dimension */
        n = lens[rank - 1] < old[2 + rank - 1] ? lens[rank - 1] : old[2 + rank - 1];
        for (j = 0; j < (rank > 1 ? old[0] / old[2 + rank - 1] : 1); j++)
            for (i = 0; i < n; i++)
                a[2 + rank + j * lens[rank - 1] + i] = old[2 + rank + j * old[2 + rank - 1] + i];
    }
    return a;
}

long vbrt_array_length(long *a) {
    if (!a) vbrt_throw(E_NULLREF);
    return a[0];
}

/* ------------------------------------------------------------ console */

static void put_str(long *s) {
    long n = slen(s), i;
    for (i = 0; i < n; i++) putchar((int)s[1 + i]);
}

/* the composite format of String.Format and Console.WriteLine(format, ...):
   {index[,alignment][:format]} */
long *vbrt_format(long *fmt, long *args) {
    long n = slen(fmt), i = 0, cap = 64, len = 0;
    long *buf = malloc(sizeof(long) * cap), *s;
    if (!fmt) vbrt_throw(E_NULLREF);
#define PUT(c) do { if (len == cap) { cap *= 2; buf = realloc(buf, sizeof(long) * cap); } buf[len++] = (c); } while (0)
    while (i < n) {
        long c = fmt[1 + i];
        if (c == '{') {
            long idx = 0, align = 0, neg = 0, nd = 0, k;
            long *v;
            if (i + 1 < n && fmt[2 + i] == '{') { PUT('{'); i += 2; continue; }
            i++;
            while (i < n && fmt[1 + i] >= '0' && fmt[1 + i] <= '9') { idx = idx * 10 + fmt[1 + i] - '0'; i++; nd++; }
            if (!nd) vbrt_throw(E_FORMAT);
            while (i < n && fmt[1 + i] == ' ') i++;
            if (i < n && fmt[1 + i] == ',') {
                i++;
                while (i < n && fmt[1 + i] == ' ') i++;
                if (i < n && fmt[1 + i] == '-') { neg = 1; i++; }
                nd = 0;
                while (i < n && fmt[1 + i] >= '0' && fmt[1 + i] <= '9') { align = align * 10 + fmt[1 + i] - '0'; i++; nd++; }
                if (!nd) vbrt_throw(E_FORMAT);
                while (i < n && fmt[1 + i] == ' ') i++;
            }
            {
                long *spec = 0;
                if (i < n && fmt[1 + i] == ':') {
                    long st = ++i, kk;
                    while (i < n && fmt[1 + i] != '}') i++;
                    spec = vbrt_str_new(i - st);
                    for (kk = 0; kk < i - st; kk++) spec[1 + kk] = fmt[1 + st + kk];
                }
                if (i >= n || fmt[1 + i] != '}') vbrt_throw(E_FORMAT);
                i++;
                if (!args || idx >= args[0])
                    fail("FormatException", "Index (zero based) must be greater than or equal to zero and less than the size of the argument list.");
                v = spec ? format_obj((long *)args[3 + idx], spec) : vbrt_obj_str((long *)args[3 + idx]);
            }
            if (!neg) for (k = slen(v); k < align; k++) PUT(' ');
            for (k = 0; k < slen(v); k++) PUT(v[1 + k]);
            if (neg) for (k = slen(v); k < align; k++) PUT(' ');
            continue;
        }
        if (c == '}') {
            if (i + 1 < n && fmt[2 + i] == '}') { PUT('}'); i += 2; continue; }
            vbrt_throw(E_FORMAT);
        }
        PUT(c);
        i++;
    }
#undef PUT
    s = vbrt_str_new(len);
    for (i = 0; i < len; i++) s[1 + i] = buf[i];
    free(buf);
    return s;
}

/* Console.Write/WriteLine of a value of a kind (String, Object, Char()
   arrays and the primitive kinds) */
void vbrt_write(long kind, long bits, long nl) {
    long locked = thr_lock(&thr_out_lock);
    if (kind == K_STRING) put_str((long *)bits);
    else if (kind == K_OBJECT) put_str(vbrt_obj_str((long *)bits));
    else if (kind == K_ARRAY) {
        long *a = (long *)bits, i;
        if (a) for (i = 0; i < a[0]; i++) putchar((int)a[3 + i]);
    } else if (kind == K_CHAR) putchar((int)bits);
    else if (kind != 0) {
        char buf[64];
        fmt_value(buf, kind, bits);
        fputs(buf, stdout);
    }
    if (nl) putchar('\n');
    if (locked) spinlock_unlock(&thr_out_lock);
}

void vbrt_write_format(long *fmt, long *args, long nl) {
    long *s = vbrt_format(fmt, args);
    long locked = thr_lock(&thr_out_lock);
    put_str(s);
    if (nl) putchar('\n');
    if (locked) spinlock_unlock(&thr_out_lock);
}

long *vbrt_readline(void) {
    char *line = 0;
    size_t cap = 0;
    long n, i, *s;
    fflush(stdout);
    n = getline(&line, &cap, stdin);
    if (n < 0) return 0;
    if (n > 0 && line[n - 1] == '\n') n--;
    if (n > 0 && line[n - 1] == '\r') n--;
    s = vbrt_str_new(n);
    for (i = 0; i < n; i++) s[1 + i] = line[i];
    free(line);
    return s;
}

/* ------------------------------------------------------------ program */

/* the command line arguments as a String() (without the program name) */
long *vbrt_args(void) {
    long lens[1], i;
    long *a;
    lens[0] = vb_argc > 1 ? vb_argc - 1 : 0;
    a = new_array(ARR_TAG(K_STRING, 1), 1, lens);
    for (i = 1; i < vb_argc; i++) a[3 + i - 1] = (long)str_from_c(vb_argv[i]);
    return a;
}

void vbrt_end(void) {
    fflush(stdout);
    exit(0);
}

#include "vb_fmt.c"
#include "vb_strings.c"
#include "vb_io.c"
#include "vb_thread.c"

int main(int argc, char **argv) {
    long r;
    vb_argc = argc;
    vb_argv = argv;
    r = vb_program();
    fflush(stdout);
    return (int)r;
}
