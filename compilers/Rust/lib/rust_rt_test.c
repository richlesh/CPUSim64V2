/* rust_rt_test.c: tests for rust_rt.c.
 *
 * Host:       cc -std=c99 -Wall -o /tmp/rt_test lib/rust_rt_test.c && /tmp/rt_test
 *             (extra modes: /tmp/rt_test <a|b|c|d|e|f|g>, see rt_mode below)
 * Simulator:  cd compilers/C11 && CPUSIM64_HOME=/tmp/csroot /tmp/csroot/run.sh cc <abs>/lib/rust_rt_test.c
 *
 * The expected strings of the float formatting table were produced by rustc
 * ({} {:?} {:.0} {:.1} {:.2} {:.5} {:e} {:E} {:.0e} {:.3e} of the absolute value).
 * Prints "ALL PASS" when everything succeeds.
 */
#include "rust_rt.c"

#ifdef RS_HOST
#include <string.h>
#include <math.h>
#endif

static long rt_fail = 0;
static long rt_n = 0;

static void rt_check(long ok, const char *name) {
    rt_n++;
    if (!ok) { rt_fail++; printf("FAIL: %s\n", name); }
}
static void rt_checkl(long got, long want, const char *name) {
    rt_n++;
    if (got != want) { rt_fail++; printf("FAIL: %s: got %ld want %ld\n", name, got, want); }
}

static long rt_slen(const char *s) { long n = 0; while (s[n]) n++; return n; }
/* writes the characters of s as bytes at base, returns the length */
static long rt_put(long base, const char *s) {
    long n = 0;
    while (s[n]) { __stb(base + n, (long)s[n]); n++; }
    return n;
}
static long rt_streq(long base, long len, const char *s) {
    long i, n = rt_slen(s);
    if (len != n) return 0;
    for (i = 0; i < n; i++) if (__ldb(base + i) != (long)s[i]) return 0;
    return 1;
}

/* ------------------------------------------------------------- memory */
static void rt_copy_case(long dst_off, long src_off, long n) {
    long b = rs_alloc_bytes(80), i;
    long ref[80], tmp[80];
    for (i = 0; i < 80; i++) { __stb(b + i, i + 1); ref[i] = i + 1; }
    for (i = 0; i < n; i++) tmp[i] = ref[src_off + i];
    for (i = 0; i < n; i++) ref[dst_off + i] = tmp[i];
    rs_copy_bytes(b + dst_off, b + src_off, n);
    for (i = 0; i < 80; i++) {
        if (__ldb(b + i) != ref[i]) {
            rt_fail++;
            printf("FAIL: copy_bytes dst=%ld src=%ld n=%ld at %ld\n", dst_off, src_off, n, i);
            break;
        }
    }
    rt_n++;
    rs_free_bytes(b);
}

static void rt_test_memory(void) {
    long p, i, b, c;
    p = rs_alloc(4);
    for (i = 0; i < 4; i++) RS_W(p + i) = 100 + i;
    p = rs_realloc(p, 4, 100);
    rt_checkl(RS_W(p + 3), 103, "realloc keeps words");
    RS_W(p + 99) = 7;
    rt_checkl(RS_W(p + 99), 7, "realloc grown word");
    rs_free(p);
    b = rs_alloc_bytes(20);
    rt_check(b % 8 == 0, "alloc_bytes aligned");
    rt_put(b, "abcdefghijklmnopqrst");
    rt_checkl(rs_ldb(b + 3), 'd', "ldb");
    rs_stb(b + 3, 'X');
    rt_checkl(rs_ldb(b + 3), 'X', "stb");
    rt_checkl(rs_ldb(b + 2), 'c', "stb neighbour kept");
    rt_checkl(rs_ldb(b + 4), 'e', "stb neighbour kept 2");
    b = rs_realloc_bytes(b, 20, 100);
    rt_checkl(rs_ldb(b + 19), 't', "realloc_bytes keeps bytes");
    rs_free_bytes(b);
    rt_copy_case(8, 0, 40);   /* aligned, forward overlap */
    rt_copy_case(0, 8, 40);   /* aligned, backward */
    rt_copy_case(3, 0, 40);   /* unaligned overlapping up */
    rt_copy_case(0, 3, 40);
    rt_copy_case(1, 17, 33);
    rt_copy_case(50, 5, 25);
    rt_copy_case(16, 0, 7);
    rt_copy_case(0, 16, 64);
    rt_copy_case(5, 5, 20);
    rt_copy_case(24, 8, 0);
    b = rs_alloc(8);
    for (i = 0; i < 8; i++) RS_W(b + i) = i;
    rs_copy_words(b + 2, b, 6);
    rt_checkl(RS_W(b + 7), 5, "copy_words overlap up");
    rt_checkl(RS_W(b + 2), 0, "copy_words overlap up 2");
    rs_copy_words(b, b + 2, 6);
    rt_checkl(RS_W(b + 5), 5, "copy_words overlap down");
    c = rs_alloc_bytes(16);
    rs_fill_bytes(c, 16, 'z');
    rt_checkl(rs_ldb(c + 15), 'z', "fill_bytes");
    rt_put(c, "abc");
    rt_put(b * 8, "abd");
    rt_checkl(rs_cmp_bytes(c, b * 8, 3), -1, "cmp_bytes less");
    rt_checkl(rs_cmp_bytes(b * 8, c, 3), 1, "cmp_bytes greater");
    rt_checkl(rs_cmp_bytes(c, b * 8, 2), 0, "cmp_bytes equal prefix");
}

/* ---------------------------------------------------------- arithmetic */
#define I64MAX 9223372036854775807L
static void rt_test_arith(void) {
    long MIN = RS_I64_MIN;
    long u64max = -1;
    rt_checkl(MIN, -I64MAX - 1, "I64_MIN");
    rt_checkl(rs_add_ovf_i64(I64MAX, 1), 1, "add_i64 max+1");
    rt_checkl(rs_add_ovf_i64(MIN, -1), 1, "add_i64 min-1");
    rt_checkl(rs_add_ovf_i64(I64MAX, 0), 0, "add_i64 max+0");
    rt_checkl(rs_add_ovf_i64(-5, 3), 0, "add_i64 -5+3");
    rt_checkl(rs_add_ovf_i64(I64MAX, MIN), 0, "add_i64 max+min");
    rt_checkl(rs_wadd(I64MAX, 1), MIN, "wadd wraps");
    rt_checkl(rs_sub_ovf_i64(MIN, 1), 1, "sub_i64 min-1");
    rt_checkl(rs_sub_ovf_i64(0, MIN), 1, "sub_i64 0-min");
    rt_checkl(rs_sub_ovf_i64(I64MAX, -1), 1, "sub_i64 max+1");
    rt_checkl(rs_sub_ovf_i64(MIN, -1), 0, "sub_i64 min+1");
    rt_checkl(rs_sub_ovf_i64(-1, MIN), 0, "sub_i64 -1-min");
    rt_checkl(rs_wsub(MIN, 1), I64MAX, "wsub wraps");
    rt_checkl(rs_mul_ovf_i64(I64MAX, 2), 1, "mul_i64 max*2");
    rt_checkl(rs_mul_ovf_i64(MIN, -1), 1, "mul_i64 min*-1");
    rt_checkl(rs_mul_ovf_i64(-1, MIN), 1, "mul_i64 -1*min");
    rt_checkl(rs_mul_ovf_i64(MIN, 1), 0, "mul_i64 min*1");
    rt_checkl(rs_mul_ovf_i64(MIN, 0), 0, "mul_i64 min*0");
    rt_checkl(rs_mul_ovf_i64(3037000500L, 3037000500L), 1, "mul_i64 3037000500^2");
    rt_checkl(rs_mul_ovf_i64(3037000499L, 3037000499L), 0, "mul_i64 3037000499^2");
    rt_checkl(rs_mul_ovf_i64(-3037000500L, 3037000500L), 1, "mul_i64 neg big");
    rt_checkl(rs_mul_ovf_i64(I64MAX, -1), 0, "mul_i64 max*-1");
    rt_checkl(rs_mul_ovf_i64(4611686018427387904L, -2), 0, "mul_i64 2^62*-2 = min");
    rt_checkl(rs_mul_ovf_i64(4611686018427387904L, 2), 1, "mul_i64 2^62*2");
    rt_checkl(rs_wmul(4294967296L, 4294967296L), 0, "wmul 2^64");
    rt_checkl(rs_neg_ovf_i64(MIN), 1, "neg_i64 min");
    rt_checkl(rs_neg_ovf_i64(5), 0, "neg_i64 5");
    rt_checkl(rs_wneg(MIN), MIN, "wneg min");
    rt_checkl(rs_div_ovf_i64(MIN, -1), 1, "div_i64 min/-1");
    rt_checkl(rs_div_ovf_i64(MIN, 1), 0, "div_i64 min/1");
    rt_checkl(rs_add_ovf_u64(u64max, 1), 1, "add_u64 max+1");
    rt_checkl(rs_add_ovf_u64(1, 2), 0, "add_u64 1+2");
    rt_checkl(rs_add_ovf_u64(u64max, 0), 0, "add_u64 max+0");
    rt_checkl(rs_add_ovf_u64(MIN, MIN), 1, "add_u64 2^63+2^63");
    rt_checkl(rs_add_ovf_u64(MIN, I64MAX), 0, "add_u64 2^63+(2^63-1)");
    rt_checkl(rs_sub_ovf_u64(1, 2), 1, "sub_u64 1-2");
    rt_checkl(rs_sub_ovf_u64(5, 5), 0, "sub_u64 5-5");
    rt_checkl(rs_sub_ovf_u64(u64max, 1), 0, "sub_u64 max-1");
    rt_checkl(rs_sub_ovf_u64(1, u64max), 1, "sub_u64 1-max");
    rt_checkl(rs_mul_ovf_u64(4294967296L, 4294967296L), 1, "mul_u64 2^32*2^32");
    rt_checkl(rs_mul_ovf_u64(4294967295L, 4294967297L), 0, "mul_u64 = 2^64-1");
    rt_checkl(rs_mul_ovf_u64(MIN, 2), 1, "mul_u64 2^63*2");
    rt_checkl(rs_mul_ovf_u64(MIN, 1), 0, "mul_u64 2^63*1");
    rt_checkl(rs_mul_ovf_u64(0, u64max), 0, "mul_u64 0*max");
    rt_checkl(rs_mul_ovf_u64(u64max, 2), 1, "mul_u64 max*2");
    rt_checkl(rs_ult(1, -1), 1, "ult 1 < max");
    rt_checkl(rs_ult(-1, 1), 0, "ult max < 1");
    rt_checkl(rs_ule(5, 5), 1, "ule");
    rt_checkl(rs_udiv(-1, 2), I64MAX, "udiv");
    rt_checkl(rs_urem(-1, 10), 5, "urem");
    rt_checkl(rs_shr_u(-1, 60), 15, "shr_u");
    rt_checkl(rs_shr_i(-16, 2), -4, "shr_i neg");
    rt_checkl(rs_shr_i(-1, 63), -1, "shr_i -1");
    rt_checkl(rs_shr_i(100, 2), 25, "shr_i pos");
    rt_checkl(rs_shl(1, 63), MIN, "shl 63");
    rt_checkl(rs_shl(3, 4), 48, "shl");
    rt_checkl(rs_wrap(300, 8, 0), 44, "wrap 300 u8");
    rt_checkl(rs_wrap(200, 8, 1), -56, "wrap 200 i8");
    rt_checkl(rs_wrap(-1, 32, 0), 4294967295L, "wrap -1 u32");
    rt_checkl(rs_wrap(4294967295L, 32, 1), -1, "wrap u32max i32");
    rt_checkl(rs_wrap(-129, 8, 1), 127, "wrap -129 i8");
    rt_checkl(rs_wrap(5, 64, 1), 5, "wrap 64");
    rt_checkl(rs_wrap(-3, 16, 0), 65533, "wrap -3 u16");
    rt_checkl(rs_wrap(32768, 16, 1), -32768, "wrap 32768 i16");
    rt_checkl(rs_wrap(1, 1, 0), 1, "wrap 1 bit");
    rt_checkl(rs_in_range(255, 8, 0), 1, "in_range 255 u8");
    rt_checkl(rs_in_range(256, 8, 0), 0, "in_range 256 u8");
    rt_checkl(rs_in_range(-1, 8, 0), 0, "in_range -1 u8");
    rt_checkl(rs_in_range(127, 8, 1), 1, "in_range 127 i8");
    rt_checkl(rs_in_range(128, 8, 1), 0, "in_range 128 i8");
    rt_checkl(rs_in_range(-128, 8, 1), 1, "in_range -128 i8");
    rt_checkl(rs_in_range(-129, 8, 1), 0, "in_range -129 i8");
    rt_checkl(rs_in_range(4294967295L, 32, 0), 1, "in_range u32max");
    rt_checkl(rs_in_range(4294967296L, 32, 0), 0, "in_range 2^32 u32");
    rt_checkl(rs_in_range(2147483648L, 32, 1), 0, "in_range 2^31 i32");
    rt_checkl(rs_in_range(-2147483648L, 32, 1), 1, "in_range -2^31 i32");
    rt_checkl(rs_in_range(-5, 64, 0), 1, "in_range 64");
    /* u32 * u32 through the wide result */
    rt_checkl(rs_in_range(rs_wmul(4294967295L, 4294967295L), 32, 0), 0, "u32 mul overflow");
    rt_checkl(rs_in_range(rs_wmul(65535, 65537), 32, 0), 1, "u32 mul fits");
    rt_checkl(rs_in_range(rs_wmul(-65536, 32768), 32, 1), 1, "i32 mul min fits");
    rt_checkl(rs_in_range(rs_wmul(-65536, 32769), 32, 1), 0, "i32 mul overflow");
}

/* --------------------------------------------------------------- floats */
static double rt_d(unsigned long bits) { return rs_f64_from_bits((long)bits); }

static void rt_test_casts(void) {
    double nan = rt_d(0x7ff8000000000000UL), inf = rt_d(0x7ff0000000000000UL);
    double ninf = rt_d(0xfff0000000000000UL);
    rt_checkl(rs_f64_to_bits(1.0), (long)0x3ff0000000000000UL, "to_bits 1.0");
    rt_checkl(rs_f64_to_bits(rt_d(0xc00921fb54442d18UL)), (long)0xc00921fb54442d18UL, "bits roundtrip");
    rt_check(rs_f64_is_nan(nan), "is_nan");
    rt_check(!rs_f64_is_nan(inf), "inf is not nan");
    rt_check(!rs_f64_is_nan(1.5), "1.5 is not nan");
    rt_checkl(rs_f64_to_int(3.99, 64, 1), 3, "3.99 as i64");
    rt_checkl(rs_f64_to_int(-3.99, 64, 1), -3, "-3.99 as i64");
    rt_checkl(rs_f64_to_int(0.99, 64, 1), 0, "0.99 as i64");
    rt_checkl(rs_f64_to_int(-0.5, 64, 1), 0, "-0.5 as i64");
    rt_checkl(rs_f64_to_int(300.0, 8, 0), 255, "300 as u8");
    rt_checkl(rs_f64_to_int(-5.0, 8, 0), 0, "-5 as u8");
    rt_checkl(rs_f64_to_int(200.0, 8, 1), 127, "200 as i8");
    rt_checkl(rs_f64_to_int(-200.0, 8, 1), -128, "-200 as i8");
    rt_checkl(rs_f64_to_int(-128.0, 8, 1), -128, "-128 as i8");
    rt_checkl(rs_f64_to_int(127.9, 8, 1), 127, "127.9 as i8");
    rt_checkl(rs_f64_to_int(2147483648.0, 32, 1), 2147483647L, "2^31 as i32");
    rt_checkl(rs_f64_to_int(-2147483649.0, 32, 1), -2147483647L - 1, "-2^31-1 as i32");
    rt_checkl(rs_f64_to_int(4294967296.0, 32, 0), 4294967295L, "2^32 as u32");
    rt_checkl(rs_f64_to_int(4294967295.5, 32, 0), 4294967295L, "u32max.5 as u32");
    rt_checkl(rs_f64_to_int(nan, 64, 1), 0, "nan as i64");
    rt_checkl(rs_f64_to_int(nan, 8, 0), 0, "nan as u8");
    rt_checkl(rs_f64_to_int(inf, 64, 1), I64MAX, "inf as i64");
    rt_checkl(rs_f64_to_int(ninf, 64, 1), RS_I64_MIN, "-inf as i64");
    rt_checkl(rs_f64_to_int(inf, 64, 0), -1, "inf as u64");
    rt_checkl(rs_f64_to_int(1e19, 64, 1), I64MAX, "1e19 as i64");
    rt_checkl(rs_f64_to_int(-1e19, 64, 1), RS_I64_MIN, "-1e19 as i64");
    rt_checkl(rs_f64_to_int(1e19, 64, 0), (long)10000000000000000000UL, "1e19 as u64");
    rt_checkl(rs_f64_to_int(18446744073709551616.0, 64, 0), -1, "2^64 as u64");
    rt_checkl(rs_f64_to_int(9223372036854775808.0, 64, 0), RS_I64_MIN, "2^63 as u64");
    rt_checkl(rs_f64_to_int(-9223372036854775808.0, 64, 1), RS_I64_MIN, "-2^63 as i64");
    rt_checkl(rs_f64_to_int(4503599627370497.0, 64, 1), 4503599627370497L, "2^52+1 as i64");
    rt_checkl(rs_f64_to_int(1e15, 64, 1), 1000000000000000L, "1e15 as i64");
    rt_checkl(rs_f64_to_bits(rs_u64_to_f64(-1)), (long)0x43f0000000000000UL, "u64max to f64");
    rt_checkl(rs_f64_to_bits(rs_u64_to_f64(RS_I64_MIN)), (long)0x43e0000000000000UL, "2^63 to f64");
    rt_checkl(rs_f64_to_bits(rs_u64_to_f64(5)), (long)0x4014000000000000UL, "5 to f64");
    /* a u64 whose low bit must stick: 2^63 + 2^10 + 1 rounds up */
    rt_checkl(rs_f64_to_bits(rs_u64_to_f64((long)(0x8000000000000401UL))), (long)0x43e0000000000001UL,
              "u64 sticky bit");
    /* f32 rounding */
    rt_checkl(rs_f64_to_bits(rs_round_f32(0.1)), (long)0x3fb99999a0000000UL, "f32 0.1");
    rt_checkl(rs_f64_to_bits(rs_round_f32(16777217.0)), (long)0x4170000000000000UL, "f32 2^24+1 ties even");
    rt_checkl(rs_f64_to_bits(rs_round_f32(16777219.0)), (long)0x4170000040000000UL, "f32 2^24+3 ties even");
    rt_checkl(rs_f64_to_bits(rs_round_f32(3.5e38)), (long)0x7ff0000000000000UL, "f32 overflow");
    rt_checkl(rs_f64_to_bits(rs_round_f32(3.4028234663852886e38)), (long)0x47efffffe0000000UL, "f32 max");
    rt_checkl(rs_f64_to_bits(rs_round_f32(3.4028235677973366e38)), (long)0x7ff0000000000000UL, "f32 max rounds to inf");
    rt_checkl(rs_f64_to_bits(rs_round_f32(1.4e-45)), (long)0x36a0000000000000UL, "f32 min subnormal");
    rt_checkl(rs_f64_to_bits(rs_round_f32(1e-46)), 0, "f32 underflow");
    rt_checkl(rs_f64_to_bits(rs_round_f32(-1.5)), (long)0xbff8000000000000UL, "f32 -1.5");
    rt_checkl(rs_f64_to_bits(rs_round_f32(7.0e-45)), (long)0x36c4000000000000UL, "f32 5*2^-149");
}

/* ----------------------------------------------------------- formatting */
struct rt_row { unsigned long bits; const char *s[10]; };
static struct rt_row rt_rows[] = {
{0x0000000000000000UL,{"0","0.0","0","0.0","0.00","0.00000","0e0","0E0","0e0","0.000e0"}},
{0x3ff0000000000000UL,{"1","1.0","1","1.0","1.00","1.00000","1e0","1E0","1e0","1.000e0"}},
{0x3fb999999999999aUL,{"0.1","0.1","0","0.1","0.10","0.10000","1e-1","1E-1","1e-1","1.000e-1"}},
{0x3fc999999999999aUL,{"0.2","0.2","0","0.2","0.20","0.20000","2e-1","2E-1","2e-1","2.000e-1"}},
{0x3fd3333333333333UL,{"0.3","0.3","0","0.3","0.30","0.30000","3e-1","3E-1","3e-1","3.000e-1"}},
{0x3fe0000000000000UL,{"0.5","0.5","0","0.5","0.50","0.50000","5e-1","5E-1","5e-1","5.000e-1"}},
{0x3ff8000000000000UL,{"1.5","1.5","2","1.5","1.50","1.50000","1.5e0","1.5E0","2e0","1.500e0"}},
{0x4004000000000000UL,{"2.5","2.5","2","2.5","2.50","2.50000","2.5e0","2.5E0","2e0","2.500e0"}},
{0x400c000000000000UL,{"3.5","3.5","4","3.5","3.50","3.50000","3.5e0","3.5E0","4e0","3.500e0"}},
{0x3fc0000000000000UL,{"0.125","0.125","0","0.1","0.12","0.12500","1.25e-1","1.25E-1","1e-1","1.250e-1"}},
{0x3fd8000000000000UL,{"0.375","0.375","0","0.4","0.38","0.37500","3.75e-1","3.75E-1","4e-1","3.750e-1"}},
{0x4059000000000000UL,{"100","100.0","100","100.0","100.00","100.00000","1e2","1E2","1e2","1.000e2"}},
{0x40fe240c9fbe76c9UL,{"123456.789","123456.789","123457","123456.8","123456.79","123456.78900","1.23456789e5","1.23456789E5","1e5","1.235e5"}},
{0x430c6bf526340000UL,{"1000000000000000","1000000000000000.0","1000000000000000","1000000000000000.0","1000000000000000.00","1000000000000000.00000","1e15","1E15","1e15","1.000e15"}},
{0x4341c37937e08000UL,{"10000000000000000","1e16","10000000000000000","10000000000000000.0","10000000000000000.00","10000000000000000.00000","1e16","1E16","1e16","1.000e16"}},
{0x4376345785d8a000UL,{"100000000000000000","1e17","100000000000000000","100000000000000000.0","100000000000000000.00","100000000000000000.00000","1e17","1E17","1e17","1.000e17"}},
{0x444b1ae4d6e2ef50UL,{"1000000000000000000000","1e21","1000000000000000000000","1000000000000000000000.0","1000000000000000000000.00","1000000000000000000000.00000","1e21","1E21","1e21","1.000e21"}},
{0x4480f0cf064dd592UL,{"10000000000000000000000","1e22","10000000000000000000000","10000000000000000000000.0","10000000000000000000000.00","10000000000000000000000.00000","1e22","1E22","1e22","1.000e22"}},
{0x44b52d02c7e14af6UL,{"100000000000000000000000","1e23","99999999999999991611392","99999999999999991611392.0","99999999999999991611392.00","99999999999999991611392.00000","1e23","1E23","1e23","1.000e23"}},
{0x3e8421f5f40d8376UL,{"0.00000015","1.5e-7","0","0.0","0.00","0.00000","1.5e-7","1.5E-7","1e-7","1.500e-7"}},
{0x3ee4f8b588e368f1UL,{"0.00001","1e-5","0","0.0","0.00","0.00001","1e-5","1E-5","1e-5","1.000e-5"}},
{0x3f1a36e2eb1c432dUL,{"0.0001","0.0001","0","0.0","0.00","0.00010","1e-4","1E-4","1e-4","1.000e-4"}},
{0x3f202c9dedbc309dUL,{"0.0001234","0.0001234","0","0.0","0.00","0.00012","1.234e-4","1.234E-4","1e-4","1.234e-4"}},
{0x3e7ad7f29abcaf48UL,{"0.0000001","1e-7","0","0.0","0.00","0.00000","1e-7","1E-7","1e-7","1.000e-7"}},
{0x0000000000000001UL,{"?","5e-324","0","0.0","0.00","0.00000","5e-324","5E-324","5e-324","4.941e-324"}},
{0x0010000000000000UL,{"?","2.2250738585072014e-308","0","0.0","0.00","0.00000","2.2250738585072014e-308","2.2250738585072014E-308","2e-308","2.225e-308"}},
{0x7fefffffffffffffUL,{"?","1.7976931348623157e308","?","?","?","?","1.7976931348623157e308","1.7976931348623157E308","2e308","1.798e308"}},
{0x400921fb54442d18UL,{"3.141592653589793","3.141592653589793","3","3.1","3.14","3.14159","3.141592653589793e0","3.141592653589793E0","3e0","3.142e0"}},
{0x4005bf0a8b145769UL,{"2.718281828459045","2.718281828459045","3","2.7","2.72","2.71828","2.718281828459045e0","2.718281828459045E0","3e0","2.718e0"}},
{0x3fd5555555555555UL,{"0.3333333333333333","0.3333333333333333","0","0.3","0.33","0.33333","3.333333333333333e-1","3.333333333333333E-1","3e-1","3.333e-1"}},
{0x3fe5555555555555UL,{"0.6666666666666666","0.6666666666666666","1","0.7","0.67","0.66667","6.666666666666666e-1","6.666666666666666E-1","7e-1","6.667e-1"}},
{0x3fd3333333333334UL,{"0.30000000000000004","0.30000000000000004","0","0.3","0.30","0.30000","3.0000000000000004e-1","3.0000000000000004E-1","3e-1","3.000e-1"}},
{0x4340000000000000UL,{"9007199254740992","9007199254740992.0","9007199254740992","9007199254740992.0","9007199254740992.00","9007199254740992.00000","9.007199254740992e15","9.007199254740992E15","9e15","9.007e15"}},
{0x4330000000000000UL,{"4503599627370496","4503599627370496.0","4503599627370496","4503599627370496.0","4503599627370496.00","4503599627370496.00000","4.503599627370496e15","4.503599627370496E15","5e15","4.504e15"}},
{0x3fa70a3d70a3d70aUL,{"0.045","0.045","0","0.0","0.04","0.04500","4.5e-2","4.5E-2","4e-2","4.500e-2"}},
{0x3fac28f5c28f5c29UL,{"0.055","0.055","0","0.1","0.06","0.05500","5.5e-2","5.5E-2","6e-2","5.500e-2"}},
{0x3ff0147ae147ae14UL,{"1.005","1.005","1","1.0","1.00","1.00500","1.005e0","1.005E0","1e0","1.005e0"}},
{0x4005666666666666UL,{"2.675","2.675","3","2.7","2.67","2.67500","2.675e0","2.675E0","3e0","2.675e0"}},
{0x54b249ad2594c37dUL,{"?","1e100","?","?","?","?","1e100","1E100","1e100","1.000e100"}},
{0x2b2bff2ee48e0530UL,{"?","1e-100","0","0.0","0.00","0.00000","1e-100","1E-100","1e-100","1.000e-100"}},
{0x437b69b4ba630f35UL,{"123456789012345680","1.2345678901234568e17","123456789012345680","123456789012345680.0","123456789012345680.00","123456789012345680.00000","1.2345678901234568e17","1.2345678901234568E17","1e17","1.235e17"}},
{0x3eb0c6f7a0b5ed8dUL,{"0.000001","1e-6","0","0.0","0.00","0.00000","1e-6","1E-6","1e-6","1.000e-6"}},
{0x3eb4b66dc015e19bUL,{"0.00000123456789","1.23456789e-6","0","0.0","0.00","0.00000","1.23456789e-6","1.23456789E-6","1e-6","1.235e-6"}},
{0x406fe00000000000UL,{"255","255.0","255","255.0","255.00","255.00000","2.55e2","2.55E2","3e2","2.550e2"}},
{0x40f0000000000000UL,{"65536","65536.0","65536","65536.0","65536.00","65536.00000","6.5536e4","6.5536E4","7e4","6.554e4"}},
{0x41f0000000000000UL,{"4294967296","4294967296.0","4294967296","4294967296.0","4294967296.00","4294967296.00000","4.294967296e9","4.294967296E9","4e9","4.295e9"}},
{0x4202a05f20000000UL,{"10000000000","10000000000.0","10000000000","10000000000.0","10000000000.00","10000000000.00000","1e10","1E10","1e10","1.000e10"}},
{0x3fefffeb074a771dUL,{"0.99999","0.99999","1","1.0","1.00","0.99999","9.9999e-1","9.9999E-1","1e0","1.000e0"}},
{0x4023fff2e48e8a72UL,{"9.9999","9.9999","10","10.0","10.00","9.99990","9.9999e0","9.9999E0","1e1","1.000e1"}},
{0x4058ffae147ae148UL,{"99.995","99.995","100","100.0","100.00","99.99500","9.9995e1","9.9995E1","1e2","1.000e2"}},
{0x3f40624dd2f1a9fcUL,{"0.0005","0.0005","0","0.0","0.00","0.00050","5e-4","5E-4","5e-4","5.000e-4"}},
{0x4021000000000000UL,{"8.5","8.5","8","8.5","8.50","8.50000","8.5e0","8.5E0","8e0","8.500e0"}},
{0x4023000000000000UL,{"9.5","9.5","10","9.5","9.50","9.50000","9.5e0","9.5E0","1e1","9.500e0"}},
{0x3fa999999999999aUL,{"0.05","0.05","0","0.1","0.05","0.05000","5e-2","5E-2","5e-2","5.000e-2"}},
{0x3fee666666666666UL,{"0.95","0.95","1","0.9","0.95","0.95000","9.5e-1","9.5E-1","9e-1","9.500e-1"}},
{0x7e37e43c8800759cUL,{"?","1e300","?","?","?","?","1e300","1E300","1e300","1.000e300"}},
{0x4011666666666666UL,{"4.35","4.35","4","4.3","4.35","4.35000","4.35e0","4.35E0","4e0","4.350e0"}},
{0x4011cccccccccccdUL,{"4.45","4.45","4","4.5","4.45","4.45000","4.45e0","4.45E0","4e0","4.450e0"}},
{0x3fe6666666666666UL,{"0.7","0.7","1","0.7","0.70","0.70000","7e-1","7E-1","7e-1","7.000e-1"}},
{0x408f400000000000UL,{"1000","1000.0","1000","1000.0","1000.00","1000.00000","1e3","1E3","1e3","1.000e3"}},
{0x3f60624dd2f1a9fcUL,{"0.002","0.002","0","0.0","0.00","0.00200","2e-3","2E-3","2e-3","2.000e-3"}},
{0x7ff0000000000000UL,{"inf","inf","inf","inf","inf","inf","inf","inf","inf","inf"}},
{0x7ff8000000000000UL,{"NaN","NaN","NaN","NaN","NaN","NaN","NaN","NaN","NaN","NaN"}},
{0x000fffffffffffffUL,{"?","2.225073858507201e-308","0","0.0","0.00","0.00000","2.225073858507201e-308","2.225073858507201E-308","2e-308","2.225e-308"}},
{0x50b8599c39263059UL,{"?","7.218027539944233e80","?","?","?","?","7.218027539944233e80","7.218027539944233E80","7e80","7.218e80"}},
{0x7d765e9c9be4bcfcUL,{"?","2.285880515036382e296","?","?","?","?","2.285880515036382e296","2.285880515036382E296","2e296","2.286e296"}},
{0x5502f08db2715945UL,{"?","3.3140631768523595e101","?","?","?","?","3.3140631768523595e101","3.3140631768523595E101","3e101","3.314e101"}},
{0x59ca7d9bbb2d420fUL,{"?","3.502361454010842e124","?","?","?","?","3.502361454010842e124","3.502361454010842E124","4e124","3.502e124"}},
{0x6f8ac158eab477d2UL,{"?","2.0282241158133682e229","?","?","?","?","2.0282241158133682e229","2.0282241158133682E229","2e229","2.028e229"}},
{0x008623ac482c9cbcUL,{"?","3.9409346806608853e-306","0","0.0","0.00","0.00000","3.9409346806608853e-306","3.9409346806608853E-306","4e-306","3.941e-306"}},
{0x408b5c08ec32c7ecUL,{"875.5043567626249","875.5043567626249","876","875.5","875.50","875.50436","8.755043567626249e2","8.755043567626249E2","9e2","8.755e2"}},
{0x7f1dfd6d7cf20724UL,{"?","2.0566074621228124e304","?","?","?","?","2.0566074621228124e304","2.0566074621228124E304","2e304","2.057e304"}},
};

/* (mode, prec, has_prec) of the ten columns */
static long rt_cols[10][3] = {
    {0, 0, 0}, {1, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 2, 1},
    {0, 5, 1}, {2, 0, 0}, {3, 0, 0}, {2, 0, 1}, {2, 3, 1}
};

static long rt_buf = 0;

static void rt_test_fmt_table(void) {
    long r, c, n, i, nrows = (long)(sizeof(rt_rows) / sizeof(rt_rows[0]));
    for (r = 0; r < nrows; r++) {
        double v = rt_d(rt_rows[r].bits);
        for (c = 0; c < 10; c++) {
            const char *want = rt_rows[r].s[c];
            if (want[0] == '?' && want[1] == 0) continue;
            n = rs_fmt_f64(v, rt_cols[c][1], rt_cols[c][2], rt_cols[c][0], rt_buf);
            rt_n++;
            if (!rt_streq(rt_buf, n, want)) {
                rt_fail++;
                printf("FAIL fmt %lx col %ld: want '%s' got '", (unsigned long)rt_rows[r].bits, c, want);
                for (i = 0; i < n && i < 60; i++) printf("%c", (int)__ldb(rt_buf + i));
                printf("'\n");
            }
        }
        /* Debug with precision is the fixed form */
        if (rt_rows[r].s[4][0] != '?') {
            n = rs_fmt_f64(v, 2, 1, 1, rt_buf);
            rt_check(rt_streq(rt_buf, n, rt_rows[r].s[4]), "debug with precision");
        }
        /* the sign is the caller's: a negative input prints its magnitude */
        if (rt_rows[r].bits < 0x7ff0000000000000UL && rt_rows[r].s[0][0] != '?') {
            n = rs_fmt_f64(rt_d(rt_rows[r].bits | 0x8000000000000000UL), 0, 0, 0, rt_buf);
            rt_check(rt_streq(rt_buf, n, rt_rows[r].s[0]), "negative input magnitude");
        }
    }
}

static void rt_test_fmt_extra(void) {
    long n, i, ok;
    /* DBL_MAX Display: 17976931348623157 followed by 292 zeros */
    n = rs_fmt_f64(rt_d(0x7fefffffffffffffUL), 0, 0, 0, rt_buf);
    ok = n == 309 && rt_streq(rt_buf, 17, "17976931348623157");
    ok = n == 309;
    for (i = 0; i < 17 && ok; i++) if (__ldb(rt_buf + i) != "17976931348623157"[i]) ok = 0;
    for (i = 17; i < 309 && ok; i++) if (__ldb(rt_buf + i) != '0') ok = 0;
    rt_check(ok, "DBL_MAX display");
    /* 5e-324 Display: 0. then 323 digits ending in 5 */
    n = rs_fmt_f64(rt_d(1UL), 0, 0, 0, rt_buf);
    ok = n == 326 && __ldb(rt_buf) == '0' && __ldb(rt_buf + 1) == '.';
    for (i = 2; i < 325 && ok; i++) if (__ldb(rt_buf + i) != '0') ok = 0;
    rt_check(ok && __ldb(rt_buf + 325) == '5', "5e-324 display");
    /* DBL_MAX with {:.0}: exact digits */
    n = rs_fmt_f64(rt_d(0x7fefffffffffffffUL), 0, 1, 0, rt_buf);
    ok = n == 309;
    for (i = 0; i < 20 && ok; i++) if (__ldb(rt_buf + i) != "17976931348623157081"[i]) ok = 0;
    rt_check(ok, "DBL_MAX exact digits");
    /* output is cut at 400 bytes */
    n = rs_fmt_f64(1.0, 1000, 1, 0, rt_buf);
    rt_checkl(n, 400, "cap 400");
    /* negative zero / negative nan carry no sign in the text */
    n = rs_fmt_f64(rt_d(0x8000000000000000UL), 0, 0, 0, rt_buf);
    rt_check(rt_streq(rt_buf, n, "0"), "-0 display");
    n = rs_fmt_f64(rt_d(0x8000000000000000UL), 0, 0, 1, rt_buf);
    rt_check(rt_streq(rt_buf, n, "0.0"), "-0 debug");
    n = rs_fmt_f64(rt_d(0xfff8000000000000UL), 0, 0, 0, rt_buf);
    rt_check(rt_streq(rt_buf, n, "NaN"), "-nan");
    n = rs_fmt_f64(rt_d(0xfff0000000000000UL), 0, 0, 0, rt_buf);
    rt_check(rt_streq(rt_buf, n, "inf"), "-inf");
    /* f32: digits of the shortest f32 representation */
    n = rs_fmt_f32(rs_round_f32(0.3), 0, 0, 0, rt_buf);
    rt_check(rt_streq(rt_buf, n, "0.3"), "f32 0.3");
    n = rs_fmt_f64(rs_round_f32(0.3), 0, 0, 0, rt_buf);
    rt_check(rt_streq(rt_buf, n, "0.30000001192092896"), "f64 of f32 0.3");
    n = rs_fmt_f32(rs_round_f32(1.0 / 3.0), 0, 0, 0, rt_buf);
    rt_check(rt_streq(rt_buf, n, "0.33333334"), "f32 1/3");
    n = rs_fmt_f32(rs_round_f32(16777216.0), 0, 0, 0, rt_buf);
    rt_check(rt_streq(rt_buf, n, "16777216"), "f32 2^24");
    n = rs_fmt_f32(rs_round_f32(1e10), 0, 0, 0, rt_buf);
    rt_check(rt_streq(rt_buf, n, "10000000000"), "f32 1e10");
    n = rs_fmt_f32(rs_round_f32(1.4e-45), 0, 0, 2, rt_buf);
    rt_check(rt_streq(rt_buf, n, "1e-45"), "f32 min subnormal exp");
    n = rs_fmt_f32(rs_round_f32(3.4028234663852886e38), 0, 0, 2, rt_buf);
    rt_check(rt_streq(rt_buf, n, "3.4028235e38"), "f32 max exp");
    n = rs_fmt_f32(rs_round_f32(1.17549435e-38), 0, 0, 2, rt_buf);
    rt_check(rt_streq(rt_buf, n, "1.1754944e-38"), "f32 min normal exp");
    n = rs_fmt_f32(rs_round_f32(0.1), 0, 0, 1, rt_buf);
    rt_check(rt_streq(rt_buf, n, "0.1"), "f32 debug 0.1");
    n = rs_fmt_f32(rs_round_f32(1e16), 0, 0, 1, rt_buf);
    rt_check(rt_streq(rt_buf, n, "1e16"), "f32 debug 1e16");
}

/* -------------------------------------------------------------- parsing */
static long rt_parse(const char *s, double *v) {
    long b = rs_alloc_bytes(rt_slen(s) + 1), len = rt_put(b, s), o = rs_alloc(1), ok;
    ok = rs_parse_f64(b, len, o);
    if (ok) *v = rs_f64_from_bits(RS_W(o));
    rs_free(o);
    rs_free_bytes(b);
    return ok;
}
static void rt_pbits(const char *s, unsigned long want) {
    double v = 0.0;
    long ok = rt_parse(s, &v);
    rt_n++;
    if (!ok || (unsigned long)rs_f64_to_bits(v) != want) {
        rt_fail++;
        printf("FAIL parse '%s': ok=%ld bits=%lx want %lx\n", s, ok, (unsigned long)rs_f64_to_bits(v), want);
    }
}
static void rt_pbad(const char *s) {
    double v = 0.0;
    rt_n++;
    if (rt_parse(s, &v)) { rt_fail++; printf("FAIL parse '%s' should be rejected\n", s); }
}
static void rt_test_parse(void) {
    double v = 0.0;
    rt_pbits("1", 0x3ff0000000000000UL);
    rt_pbits("-1.5e3", 0xc097700000000000UL);
    rt_pbits("+.5", 0x3fe0000000000000UL);
    rt_pbits("5.", 0x4014000000000000UL);
    rt_pbits("0", 0UL);
    rt_pbits("-0", 0x8000000000000000UL);
    rt_pbits("0.1", 0x3fb999999999999aUL);
    rt_pbits("1e400", 0x7ff0000000000000UL);
    rt_pbits("-1e400", 0xfff0000000000000UL);
    rt_pbits("1e-400", 0UL);
    rt_pbits("4.9e-324", 1UL);
    rt_pbits("1.7976931348623157e308", 0x7fefffffffffffffUL);
    rt_pbits("2.2250738585072014e-308", 0x0010000000000000UL);
    rt_pbits("9007199254740993", 0x4340000000000000UL);
    rt_pbits("1E3", 0x408f400000000000UL);
    rt_pbits("1e+3", 0x408f400000000000UL);
    rt_pbits("1e-1", 0x3fb999999999999aUL);
    rt_pbits("inf", 0x7ff0000000000000UL);
    rt_pbits("-inf", 0xfff0000000000000UL);
    rt_pbits("+Infinity", 0x7ff0000000000000UL);
    rt_pbits("INF", 0x7ff0000000000000UL);
    rt_pbits("NaN", 0x7ff8000000000000UL);
    rt_pbits("nan", 0x7ff8000000000000UL);
    rt_pbits("0.30000000000000004", 0x3fd3333333333334UL);
    rt_pbits("123456789012345678901234567890", 0x45f8ee90ff6c373eUL);
    rt_pbad("");
    rt_pbad(".");
    rt_pbad("+");
    rt_pbad("-");
    rt_pbad("e5");
    rt_pbad("1e");
    rt_pbad("1e+");
    rt_pbad("1.5x");
    rt_pbad(" 1");
    rt_pbad("1 ");
    rt_pbad("infinit");
    rt_pbad("infx");
    rt_pbad("nanx");
    rt_pbad("0x10");
    rt_pbad("1_000");
    rt_pbad("--1");
    rt_pbad("1.2.3");
    rt_pbad(".e5");
    rt_n++;
    if (rt_parse("nan", &v) && !rs_f64_is_nan(v)) { rt_fail++; printf("FAIL parse nan is nan\n"); }
}

/* ------------------------------------------------- pseudo random checks */
static unsigned long rt_seed = 88172645463325252UL;
static unsigned long rt_rand(void) {
    rt_seed ^= rt_seed << 13;
    rt_seed ^= rt_seed >> 7;
    rt_seed ^= rt_seed << 17;
    return rt_seed;
}
/* the text of every mode parses back to the same bits */
static void rt_test_roundtrip(long count) {
    long i, n, mode, ok, o = rs_alloc(1);
    unsigned long bits;
    double v, w;
    for (i = 0; i < count; i++) {
        bits = rt_rand() & 0x7fffffffffffffffUL;
        if ((bits >> 52) == 0x7ff) continue;
        if (i % 3 == 0) bits = (bits & 0x800fffffffffffffUL) | ((0x3e0UL + (rt_rand() % 64UL)) << 52);
        v = rt_d(bits);
        for (mode = 0; mode < 4; mode++) {
            n = rs_fmt_f64(v, 0, 0, mode, rt_buf);
            ok = rs_parse_f64(rt_buf, n, o);
            w = rs_f64_from_bits(RS_W(o));
            rt_n++;
            if (!ok || (unsigned long)rs_f64_to_bits(w) != bits) {
                rt_fail++;
                printf("FAIL roundtrip %lx mode %ld\n", bits, mode);
            }
        }
    }
    rs_free(o);
}

/* ------------------------------------------------------- host-only tests */
#ifdef RS_HOST
static void rt_host_str(char *dst, long n) {
    long i;
    for (i = 0; i < n; i++) dst[i] = (char)__ldb(rt_buf + i);
    dst[n] = 0;
}
/* C "d.dddde-05" -> Rust "d.ddde-5" */
static void rt_c_to_rust_exp(const char *c, char *out) {
    const char *e = strchr(c, 'e');
    long ex = strtol(e + 1, 0, 10);
    long m = (long)(e - c);
    memcpy(out, c, (size_t)m);
    sprintf(out + m, "e%ld", ex);
}
static long rt_digits(const char *s) { /* digits of the mantissa of d.ddde<x> */
    long n = 0;
    while (*s && *s != 'e') { if (*s != '.') n++; s++; }
    return n;
}
static void rt_test_host_mass(long count) {
    long i, n, p, bad = 0, j;
    unsigned long bits;
    double v;
    char got[1200], want[1200], c1[1200], c2[1200];
    static const long precs[] = {0, 1, 2, 3, 5, 10, 17};
    for (i = 0; i < count; i++) {
        bits = rt_rand() & 0x7fffffffffffffffUL;
        if (i % 4 == 0) bits = (bits & 0x800fffffffffffffUL) | ((0x3b0UL + (rt_rand() % 0x100UL)) << 52);
        if (i % 16 == 1) bits &= 0x800fffffffffffffUL | ((rt_rand() % 4UL) << 52);
        if ((bits >> 52) == 0x7ff) continue;
        v = rt_d(bits);
        /* exact notation against libc (exact, round half even) */
        for (j = 0; j < 7; j++) {
            p = precs[j];
            if (v < 1e25) {
                n = rs_fmt_f64(v, p, 1, 0, rt_buf);
                rt_host_str(got, n);
                snprintf(want, sizeof want, "%.*f", (int)p, v);
                rt_n++;
                if (strcmp(got, want) != 0) { if (bad++ < 10) printf("FAIL fixed %lx .%ld: '%s' vs '%s'\n", bits, p, got, want); rt_fail++; }
            }
            n = rs_fmt_f64(v, p, 1, 2, rt_buf);
            rt_host_str(got, n);
            snprintf(c1, sizeof c1, "%.*e", (int)p, v);
            rt_c_to_rust_exp(c1, want);
            rt_n++;
            if (strcmp(got, want) != 0) { if (bad++ < 10) printf("FAIL exp %lx .%ld: '%s' vs '%s'\n", bits, p, got, want); rt_fail++; }
        }
        /* shortest: round trips, and no shorter correctly rounded form does */
        n = rs_fmt_f64(v, 0, 0, 2, rt_buf);
        rt_host_str(got, n);
        rt_n++;
        if (strtod(got, 0) != v) { if (bad++ < 10) printf("FAIL shortest roundtrip %lx '%s'\n", bits, got); rt_fail++; }
        for (p = 0; p < 17; p++) {
            snprintf(c1, sizeof c1, "%.*e", (int)p, v);
            if (strtod(c1, 0) == v) break;
        }
        rt_c_to_rust_exp(c1, c2);
        rt_n++;
        if (strcmp(got, c2) != 0) {
            if (rt_digits(got) != p + 1) { if (bad++ < 10) printf("FAIL shortest %lx '%s' vs '%s'\n", bits, got, c2); rt_fail++; }
            /* same length, different digits: exact ties, where Rust rounds up and libc to even */
        }
        n = rs_fmt_f64(v, 0, 0, 0, rt_buf);
        rt_host_str(got, n);
        rt_n++;
        if (strtod(got, 0) != v) { if (bad++ < 10) printf("FAIL display roundtrip %lx '%s'\n", bits, got); rt_fail++; }
    }
}
static void rt_test_host_f32(long count) {
    long i, n, p, bad = 0;
    unsigned long bits;
    double v, r;
    float f, g;
    char got[400], c1[400], c2[400];
    for (i = 0; i < count; i++) {
        bits = rt_rand();
        v = rt_d(bits);
        if (i % 2 == 0) v = (double)(float)((double)(long)(rt_rand() % 2000001UL - 1000000UL) / (double)(1L << (rt_rand() % 40UL)));
        if (i % 5 == 0) { bits = (rt_rand() & 0x800fffffffffffffUL) | ((0x370UL + rt_rand() % 0x100UL) << 52); v = rt_d(bits); }
        if (v != v || v > 1e39 || v < -1e39) continue;
        r = rs_round_f32(v);
        f = (float)v;
        rt_n++;
        if (rs_f64_to_bits(r) != rs_f64_to_bits((double)f)) {
            if (bad++ < 10) printf("FAIL round_f32 %lx: %lx vs %lx\n", (unsigned long)rs_f64_to_bits(v), (unsigned long)rs_f64_to_bits(r), (unsigned long)rs_f64_to_bits((double)f));
            rt_fail++;
            continue;
        }
        if (isinf(r)) continue;
        n = rs_fmt_f32(r, 0, 0, 2, rt_buf);
        rt_host_str(got, n);
        g = strtof(got, 0);
        rt_n++;
        if (g != fabsf(f)) { if (bad++ < 10) printf("FAIL f32 roundtrip %g '%s'\n", (double)f, got); rt_fail++; }
        for (p = 0; p < 9; p++) {
            snprintf(c1, sizeof c1, "%.*e", (int)p, fabs((double)f));
            if (strtof(c1, 0) == fabsf(f)) break;
        }
        rt_c_to_rust_exp(c1, c2);
        rt_n++;
        if (rt_digits(got) != p + 1) { if (bad++ < 10) printf("FAIL f32 digits '%s' vs '%s'\n", got, c2); rt_fail++; }
    }
}
static void rt_test_host_casts(long count) {
    long i, want, bad = 0;
    double v;
    unsigned long bits;
    for (i = 0; i < count; i++) {
        bits = rt_rand();
        v = rt_d(bits);
        if (i % 2) v = (double)(long)rt_rand() / (double)(1L << (rt_rand() % 62UL));
        if (v != v) continue;
        want = v >= 9223372036854775808.0 ? I64MAX : v <= -9223372036854775808.0 ? RS_I64_MIN : (long)v;
        rt_n++;
        if (rs_f64_to_int(v, 64, 1) != want) { if (bad++ < 10) printf("FAIL to_int i64 %lx\n", bits); rt_fail++; }
        want = v >= 2147483647.0 ? 2147483647L : v <= -2147483648.0 ? -2147483647L - 1 : (long)v;
        rt_n++;
        if (rs_f64_to_int(v, 32, 1) != want) { if (bad++ < 10) printf("FAIL to_int i32 %lx\n", bits); rt_fail++; }
        want = v >= 255.0 ? 255 : v <= 0.0 ? 0 : (long)v;
        rt_n++;
        if (rs_f64_to_int(v, 8, 0) != want) { if (bad++ < 10) printf("FAIL to_int u8 %lx\n", bits); rt_fail++; }
        want = v >= 18446744073709551615.0 ? -1 : v <= 0.0 ? 0 : (long)(unsigned long)v;
        rt_n++;
        if (rs_f64_to_int(v, 64, 0) != want) { if (bad++ < 10) printf("FAIL to_int u64 %lx\n", bits); rt_fail++; }
        bits = rt_rand();
        want = (long)bits;
        rt_n++;
        if (rs_f64_to_bits(rs_u64_to_f64(want)) != rs_f64_to_bits((double)bits)) { if (bad++ < 10) printf("FAIL u64_to_f64 %lx\n", bits); rt_fail++; }
    }
}
static void rt_test_host_arith(long count) {
    long i, a, b, bad = 0;
    __int128 x;
    unsigned __int128 ux;
    unsigned long ua, ub;
    for (i = 0; i < count; i++) {
        a = (long)rt_rand(); b = (long)rt_rand();
        if (i % 3 == 0) { a >>= (rt_rand() % 64UL); b >>= (rt_rand() % 64UL); }
        if (i % 7 == 0) b = (long)(rt_rand() % 5UL) - 2;
        x = (__int128)a + b;
        rt_n++; if (rs_add_ovf_i64(a, b) != (x > I64MAX || x < RS_I64_MIN)) { if (bad++ < 10) printf("FAIL add_i64 %ld %ld\n", a, b); rt_fail++; }
        x = (__int128)a - b;
        rt_n++; if (rs_sub_ovf_i64(a, b) != (x > I64MAX || x < RS_I64_MIN)) { if (bad++ < 10) printf("FAIL sub_i64 %ld %ld\n", a, b); rt_fail++; }
        x = (__int128)a * b;
        rt_n++; if (rs_mul_ovf_i64(a, b) != (x > I64MAX || x < RS_I64_MIN)) { if (bad++ < 10) printf("FAIL mul_i64 %ld %ld\n", a, b); rt_fail++; }
        ua = (unsigned long)a; ub = (unsigned long)b;
        ux = (unsigned __int128)ua + ub;
        rt_n++; if (rs_add_ovf_u64(a, b) != (ux > (unsigned __int128)0xffffffffffffffffUL)) { if (bad++ < 10) printf("FAIL add_u64 %lu %lu\n", ua, ub); rt_fail++; }
        rt_n++; if (rs_sub_ovf_u64(a, b) != (ua < ub)) { if (bad++ < 10) printf("FAIL sub_u64 %lu %lu\n", ua, ub); rt_fail++; }
        ux = (unsigned __int128)ua * ub;
        rt_n++; if (rs_mul_ovf_u64(a, b) != (ux > (unsigned __int128)0xffffffffffffffffUL)) { if (bad++ < 10) printf("FAIL mul_u64 %lu %lu\n", ua, ub); rt_fail++; }
    }
}
#endif

/* ----------------------------------------------------------------- I/O */
static void rt_test_output(void) {
    long b = rs_alloc_bytes(64), n;
    /* valid text: "h", e-acute, euro sign, grinning face, newline */
    __stb(b, 'h'); __stb(b + 1, 0xC3); __stb(b + 2, 0xA9); __stb(b + 3, 0xE2); __stb(b + 4, 0x82);
    __stb(b + 5, 0xAC); __stb(b + 6, 0xF0); __stb(b + 7, 0x9F); __stb(b + 8, 0x98); __stb(b + 9, 0x80);
    __stb(b + 10, '\n');
    printf("OUT1:");
    rs_write_bytes(1, b, 11);
    /* invalid: a FF b, truncated euro (E2 82) c, C0 80 d, ED A0 80 e, F4 90 80 80 f, E2 82 at the end */
    n = 0;
    __stb(b + n, 'a'); n++; __stb(b + n, 0xFF); n++; __stb(b + n, 'b'); n++;
    __stb(b + n, 0xE2); n++; __stb(b + n, 0x82); n++; __stb(b + n, 'c'); n++;
    __stb(b + n, 0xC0); n++; __stb(b + n, 0x80); n++; __stb(b + n, 'd'); n++;
    __stb(b + n, 0xED); n++; __stb(b + n, 0xA0); n++; __stb(b + n, 0x80); n++; __stb(b + n, 'e'); n++;
    __stb(b + n, 0xF4); n++; __stb(b + n, 0x90); n++; __stb(b + n, 0x80); n++; __stb(b + n, 0x80); n++;
    __stb(b + n, 'f'); n++; __stb(b + n, 0xE2); n++; __stb(b + n, 0x82); n++; __stb(b + n, '\n'); n++;
    printf("OUT2:");
    rs_write_bytes(1, b, n);
    printf("OUT3:");
    rs_write_bytes(1, b, 0);
    rs_write_bytes(1, b + 10, 1);
    printf("ERR-STREAM-TEST\n");
    rs_write_bytes(2, b, 1);
    rs_write_bytes(2, b + 10, 1);
    rs_free_bytes(b);
}

/* first argument: a panic, b panic_c, c bounds, d read stdin bytes, e exit(7), f abort */
static long rt_mode(int argc, char **argv) {
    long b, f, m;
    long c = 0;
    if (argc > 1) c = (long)argv[1][0];
    if (c == 0) return 0;
    b = rs_alloc_bytes(64);
    f = rt_put(b, "src/main.rs");
    m = rt_put(b + 16, "custom message");
    printf("start\n");
    if (c == 'a') rs_panic(b, f, 4, 5, b + 16, m);
    if (c == 'b') rs_panic_c(b, f, 12, 34, "attempt to add with overflow");
    if (c == 'c') rs_panic_bounds(b, f, 7, 9, 5, 3);
    if (c == 'g') rs_panic_bounds(b, f, 1, 2, -1, -2);
    if (c == 'd') {
        long x;
        while ((x = rs_read_byte(0)) >= 0) printf("%ld ", x);
        printf("EOF %ld\n", x);
    }
    if (c == 'e') rs_exit(7);
    if (c == 'f') rs_abort();
    return 1;
}

int main(int argc, char **argv) {
    long n;
    rt_buf = rs_alloc_bytes(1024);
    if (rt_mode(argc, argv)) return 0;
    rt_test_memory();
    rt_test_arith();
    rt_test_casts();
    rt_test_fmt_table();
    rt_test_fmt_extra();
    rt_test_parse();
#ifdef RS_HOST
    rt_test_roundtrip(20000);
    rt_test_host_mass(200000);
    rt_test_host_f32(100000);
    rt_test_host_casts(100000);
    rt_test_host_arith(200000);
#else
    rt_test_roundtrip(12);
#endif
    rt_test_output();
    n = rt_fail;
    printf("%ld checks, %ld failed\n", rt_n, n);
    if (n == 0) printf("ALL PASS\n");
    return n == 0 ? 0 : 1;
}
