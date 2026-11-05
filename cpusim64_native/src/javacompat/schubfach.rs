//! Java-compatible `Double.toString` digit generation via the Schubfach
//! algorithm (ports `jdk.internal.math.DoubleToDecimal` + the parts of
//! `MathUtils` it needs). This yields the exact significand digits `f`, the
//! power-of-ten exponent `e`, and digit count `len` that Java's `Formatter`
//! uses as the source for `%g`/`%e`/`%f`. It matches Java byte-for-byte,
//! including the subnormal edge cases where Rust's native shortest formatter
//! diverges.
//!
//! The large `g` lookup table of `MathUtils` is replaced by computing the two
//! 63-bit halves `g1(e)`/`g0(e)` on the fly with 256-bit integer arithmetic,
//! which is exact over the required exponent range.

const P: i32 = 53; // double precision (significand bits incl. implicit)
const W: i32 = 11; // exponent width
const Q_MIN: i32 = (-1 << (W - 1)) - P + 3;
const C_MIN: i64 = 1i64 << (P - 1);
#[allow(dead_code)]
const H: i32 = 17;

const T_MASK: i64 = (1i64 << (P - 1)) - 1;
const BQ_MASK: i32 = (1 << W) - 1;
const MASK_63: i64 = i64::MAX; // (1<<63)-1
const C_TINY: i64 = 3;

// log approximations (ports MathUtils constants).
const Q_10: i64 = 41;
const C_10: i64 = 661_971_961_083;
const A_10: i64 = -274_743_187_321;
const Q_2: i64 = 38;
const C_2: i64 = 913_124_641_741;

fn flog10pow2(q: i32) -> i32 {
    ((q as i64 * C_10) >> Q_10) as i32
}
fn flog10three_quarters_pow2(q: i32) -> i32 {
    ((q as i64 * C_10 + A_10) >> Q_10) as i32
}
fn flog2pow10(e: i32) -> i32 {
    ((e as i64 * C_2) >> Q_2) as i32
}

/// The decimal result: value = f * 10^e, with `len` = number of digits in f.
#[derive(Debug, Clone, Copy)]
pub struct FpDecimal {
    pub f: u64,
    pub e: i32,
    pub len: i32,
}

/// Compute g = floor(10^e * 2^-r) + 1 where r = flog2pow10(e) - 125, as a
/// 126-bit value split into g1 (top 63 bits) and g0 (low 63 bits). Exact via
/// 256-bit arithmetic over the range of `e` used by doubles.
fn g1g0(e: i32) -> (i64, i64) {
    // Represent 10^|e| exactly in base 2^32 limbs, then scale by 2^-r (r may be
    // negative, i.e. a left shift) and take floor, add 1, split.
    let r = flog2pow10(e) - 125;
    // numerator = 10^e, we want floor(10^e * 2^-r).
    // Build 10^|e| as a big unsigned integer (limbs little-endian u32).
    let mut num = pow10_big(e.unsigned_abs());
    if e < 0 {
        // 10^e = 1 / 10^|e|; floor(2^-r / 10^|e|) with r<0 => 2^(-r) / 10^|e|.
        // beta in [2^125, 2^126): g = floor(10^e 2^-r)+1 = floor(2^(-r)/10^|e|)+1.
        let shift = (-r) as u32; // -r > 0
        let numerator = shl_big(&vec![1u32], shift); // 2^(-r)
        let q = div_big(&numerator, &num); // floor
        let g = add1_big(&q);
        split_g(&g)
    } else {
        // e >= 0: g = floor(10^e * 2^-r) + 1.
        if r >= 0 {
            let q = shr_big(&num, r as u32);
            let g = add1_big(&q);
            split_g(&g)
        } else {
            num = shl_big(&num, (-r) as u32);
            let g = add1_big(&num);
            split_g(&g)
        }
    }
}

fn split_g(g: &[u32]) -> (i64, i64) {
    // g is 126 bits. g1 = top 63 bits, g0 = low 63 bits.
    let v = big_to_u128(g);
    let g1 = (v >> 63) as i64;
    let g0 = (v & ((1u128 << 63) - 1)) as i64;
    (g1, g0)
}

fn big_to_u128(limbs: &[u32]) -> u128 {
    let mut v: u128 = 0;
    for (i, &l) in limbs.iter().enumerate() {
        if i >= 4 {
            break;
        }
        v |= (l as u128) << (32 * i);
    }
    v
}

fn pow10_big(e: u32) -> Vec<u32> {
    let mut result = vec![1u32];
    for _ in 0..e {
        result = mul_small(&result, 10);
    }
    normalize(result)
}

fn mul_small(a: &[u32], m: u32) -> Vec<u32> {
    let mut out = Vec::with_capacity(a.len() + 1);
    let mut carry: u64 = 0;
    for &limb in a {
        let p = limb as u64 * m as u64 + carry;
        out.push((p & 0xFFFF_FFFF) as u32);
        carry = p >> 32;
    }
    while carry > 0 {
        out.push((carry & 0xFFFF_FFFF) as u32);
        carry >>= 32;
    }
    normalize(out)
}

fn shl_big(a: &[u32], bits: u32) -> Vec<u32> {
    let word_shift = (bits / 32) as usize;
    let bit_shift = bits % 32;
    let mut out = vec![0u32; word_shift];
    if bit_shift == 0 {
        out.extend_from_slice(a);
    } else {
        let mut carry: u32 = 0;
        for &limb in a {
            let v = ((limb as u64) << bit_shift) | carry as u64;
            out.push((v & 0xFFFF_FFFF) as u32);
            carry = (v >> 32) as u32;
        }
        if carry > 0 {
            out.push(carry);
        }
    }
    normalize(out)
}

fn shr_big(a: &[u32], bits: u32) -> Vec<u32> {
    let word_shift = (bits / 32) as usize;
    let bit_shift = bits % 32;
    if word_shift >= a.len() {
        return vec![0];
    }
    let mut tmp = a[word_shift..].to_vec();
    if bit_shift != 0 {
        let mut carry: u32 = 0;
        for i in (0..tmp.len()).rev() {
            let v = tmp[i];
            tmp[i] = (v >> bit_shift) | (carry << (32 - bit_shift));
            carry = v & ((1 << bit_shift) - 1);
        }
    }
    normalize(tmp)
}

fn add1_big(a: &[u32]) -> Vec<u32> {
    let mut out = a.to_vec();
    let mut carry = 1u64;
    let mut i = 0;
    while carry > 0 {
        if i == out.len() {
            out.push(0);
        }
        let v = out[i] as u64 + carry;
        out[i] = (v & 0xFFFF_FFFF) as u32;
        carry = v >> 32;
        i += 1;
    }
    normalize(out)
}

// Long division: floor(num / den), schoolbook on u32 limbs.
fn div_big(num: &[u32], den: &[u32]) -> Vec<u32> {
    if cmp_big(num, den) == std::cmp::Ordering::Less {
        return vec![0];
    }
    // Bit-by-bit long division.
    let nbits = bit_len(num);
    let mut quotient = vec![0u32; (nbits / 32 + 1) as usize];
    let mut rem: Vec<u32> = vec![0];
    for i in (0..nbits).rev() {
        rem = shl_big(&rem, 1);
        if get_bit(num, i) {
            rem = add1_big_at_bit0(&rem);
        }
        if cmp_big(&rem, den) != std::cmp::Ordering::Less {
            rem = sub_big(&rem, den);
            set_bit(&mut quotient, i);
        }
    }
    normalize(quotient)
}

fn add1_big_at_bit0(a: &[u32]) -> Vec<u32> {
    add1_big(a)
}

fn sub_big(a: &[u32], b: &[u32]) -> Vec<u32> {
    let mut out = Vec::with_capacity(a.len());
    let mut borrow: i64 = 0;
    for i in 0..a.len() {
        let av = a[i] as i64;
        let bv = if i < b.len() { b[i] as i64 } else { 0 };
        let mut v = av - bv - borrow;
        if v < 0 {
            v += 1 << 32;
            borrow = 1;
        } else {
            borrow = 0;
        }
        out.push(v as u32);
    }
    normalize(out)
}

fn cmp_big(a: &[u32], b: &[u32]) -> std::cmp::Ordering {
    let a = trim(a);
    let b = trim(b);
    if a.len() != b.len() {
        return a.len().cmp(&b.len());
    }
    for i in (0..a.len()).rev() {
        if a[i] != b[i] {
            return a[i].cmp(&b[i]);
        }
    }
    std::cmp::Ordering::Equal
}

fn trim(a: &[u32]) -> Vec<u32> {
    let mut v = a.to_vec();
    while v.len() > 1 && *v.last().unwrap() == 0 {
        v.pop();
    }
    v
}

fn normalize(mut a: Vec<u32>) -> Vec<u32> {
    while a.len() > 1 && *a.last().unwrap() == 0 {
        a.pop();
    }
    if a.is_empty() {
        a.push(0);
    }
    a
}

fn bit_len(a: &[u32]) -> u32 {
    let a = trim(a);
    let top = a.len() as u32 - 1;
    let hb = 32 - a[a.len() - 1].leading_zeros();
    top * 32 + hb
}

fn get_bit(a: &[u32], i: u32) -> bool {
    let w = (i / 32) as usize;
    if w >= a.len() {
        return false;
    }
    (a[w] >> (i % 32)) & 1 == 1
}

fn set_bit(a: &mut Vec<u32>, i: u32) {
    let w = (i / 32) as usize;
    while a.len() <= w {
        a.push(0);
    }
    a[w] |= 1 << (i % 32);
}

fn multiply_high(x: i64, y: i64) -> i64 {
    (((x as i128) * (y as i128)) >> 64) as i64
}

fn rop(g1: i64, g0: i64, cp: i64) -> i64 {
    let x1 = multiply_high(g0, cp);
    let y0 = g1.wrapping_mul(cp);
    let y1 = multiply_high(g1, cp);
    let z = (((y0 as u64) >> 1) as i64).wrapping_add(x1);
    let vbp = y1.wrapping_add(((z as u64) >> 63) as i64);
    vbp | ((((z & MASK_63).wrapping_add(MASK_63)) as u64) >> 63) as i64
}

/// Main Schubfach digit generation (ports `DoubleToDecimal.toDecimal`/`toChars`
/// up to the point of producing `FpDecimal`). Caller handles sign; pass |v|.
pub fn to_decimal(v: f64) -> Option<FpDecimal> {
    let bits = v.to_bits() as i64;
    let t = bits & T_MASK;
    let bq = ((bits >> (P - 1)) as i32) & BQ_MASK;
    if bq < BQ_MASK {
        if bq != 0 {
            let mq = -Q_MIN + 1 - bq;
            let c = C_MIN | t;
            if 0 < mq && mq < P {
                let f = c >> mq;
                if f << mq == c {
                    return Some(to_chars(f, 0));
                }
            }
            return Some(to_decimal_core(-mq, c, 0));
        }
        if t != 0 {
            return Some(if t < C_TINY {
                to_decimal_core(Q_MIN, 10 * t, -1)
            } else {
                to_decimal_core(Q_MIN, t, 0)
            });
        }
        // zero
        return Some(FpDecimal { f: 0, e: 0, len: 0 });
    }
    None // NaN/Inf
}

fn to_decimal_core(q: i32, c: i64, dk: i32) -> FpDecimal {
    let out = (c as i32) & 0x1;
    let cb = c << 2;
    let cbr = cb + 2;
    let cbl;
    let k;
    if c != C_MIN || q == Q_MIN {
        cbl = cb - 2;
        k = flog10pow2(q);
    } else {
        cbl = cb - 1;
        k = flog10three_quarters_pow2(q);
    }
    let h = q + flog2pow10(-k) + 2;
    let (g1, g0) = g1g0(-k);
    let vb = rop(g1, g0, cb.wrapping_shl(h as u32));
    let vbl = rop(g1, g0, cbl.wrapping_shl(h as u32));
    let vbr = rop(g1, g0, cbr.wrapping_shl(h as u32));
    let s = vb >> 2;
    if s >= 100 {
        let sp10 = 10i64.wrapping_mul(multiply_high(s, 115_292_150_460_684_698i64 << 4));
        let tp10 = sp10 + 10;
        let upin = vbl + out as i64 <= sp10 << 2;
        let wpin = (tp10 << 2) + out as i64 <= vbr;
        if upin != wpin {
            return to_chars(if upin { sp10 } else { tp10 }, k);
        }
    }
    let t = s + 1;
    let uin = vbl + out as i64 <= s << 2;
    let win = (t << 2) + out as i64 <= vbr;
    if uin != win {
        return to_chars(if uin { s } else { t }, k + dk);
    }
    let cmp = vb - ((s + t) << 1);
    let away = cmp > 0 || (cmp == 0 && (s & 0x1) != 0);
    to_chars(if away { t } else { s }, k + dk)
}

fn pow10_u64(e: i32) -> u64 {
    const POW10: [u64; 20] = [
        1,
        10,
        100,
        1_000,
        10_000,
        100_000,
        1_000_000,
        10_000_000,
        100_000_000,
        1_000_000_000,
        10_000_000_000,
        100_000_000_000,
        1_000_000_000_000,
        10_000_000_000_000,
        100_000_000_000_000,
        1_000_000_000_000_000,
        10_000_000_000_000_000,
        100_000_000_000_000_000,
        1_000_000_000_000_000_000,
        10_000_000_000_000_000_000,
    ];
    POW10[e as usize]
}

/// Ports the core of `toChars`: produce f (as integer significand digits) and
/// decimal exponent e, with len = digit count. We stop before the string
/// rendering (the Formatter does that).
fn to_chars(f: i64, e: i32) -> FpDecimal {
    let mut len = flog10pow2(64 - (f as u64).leading_zeros() as i32);
    if (f as u64) >= pow10_u64(len) {
        len += 1;
    }
    FpDecimal {
        f: f as u64,
        e,
        len,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn digits(v: f64) -> (u64, i32, i32) {
        let d = to_decimal(v).unwrap();
        (d.f, d.e, d.len)
    }

    #[test]
    fn subnormals_match_java() {
        // Java Double.toString: bits=1 -> 4.9E-324, bits=2 -> 9.9E-324,
        // bits=3 -> 1.5E-323, bits=10 -> 4.9E-323, bits=52 -> 2.57E-322.
        // FpDecimal gives f and e where value = f * 10^e.
        // 4.9E-324 => f=49, e=-325, len=2.
        assert_eq!(digits(f64::from_bits(1)), (49, -325, 2));
        assert_eq!(digits(f64::from_bits(2)), (99, -325, 2));
        assert_eq!(digits(f64::from_bits(3)), (15, -324, 2));
        assert_eq!(digits(f64::from_bits(10)), (49, -324, 2));
        assert_eq!(digits(f64::from_bits(52)), (257, -324, 3));
    }

    #[test]
    fn normals_match() {
        // pi: Schubfach yields 31415926535897930 * 10^-16 (fast-path trailing 0).
        assert_eq!(digits(std::f64::consts::PI), (31415926535897930, -16, 17));
        assert_eq!(digits(2.0), (2, 0, 1));
        assert_eq!(digits(26.0), (26, 0, 2));
    }
}
