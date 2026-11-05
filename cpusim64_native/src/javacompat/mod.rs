//! Java-compatibility primitives: `String.hashCode`, `String.compareTo`, and a
//! Java-`Formatter`-compatible float/number formatter (`%g`/`%e`/`%f`), plus a
//! general `printf`-style formatter and a `MessageFormat` (`{}`) renderer.
//!
//! The float rules reproduce `java.util.Formatter`:
//!   * `%g` precision = total significant digits (default 6; precision 0 => 1).
//!     Uses scientific notation when the decimal exponent `< -4` or
//!     `>= precision`; otherwise fixed. Trailing zeros are KEPT.
//!   * `%e` precision = digits after the decimal point (default 6); exponent is
//!     printed with a sign and at least two digits.
//!   * `%f` precision = digits after the decimal point (default 6).
//!   * Rounding is round-half-up (Java uses RoundingMode.HALF_UP).

pub mod format;
pub mod regexadapter;
pub mod schubfach;

/// Java `String.hashCode`: s[0]*31^(n-1) + ... + s[n-1], over UTF-16 code units,
/// computed with 32-bit wrapping arithmetic.
pub fn string_hash_code(s: &str) -> i32 {
    let mut h: i32 = 0;
    for u in s.encode_utf16() {
        h = h.wrapping_mul(31).wrapping_add(u as i32);
    }
    h
}

/// Java `String.compareTo`: lexicographic by UTF-16 code unit; if one is a
/// prefix of the other, the shorter is "less"; the result for a differing
/// position is `a - b` of the code units.
pub fn string_compare_to(a: &str, b: &str) -> i32 {
    let av: Vec<u16> = a.encode_utf16().collect();
    let bv: Vec<u16> = b.encode_utf16().collect();
    let n = av.len().min(bv.len());
    for i in 0..n {
        if av[i] != bv[i] {
            return av[i] as i32 - bv[i] as i32;
        }
    }
    av.len() as i32 - bv.len() as i32
}

/// Java `String.compareToIgnoreCase`. Java compares by first upper- then
/// lower-casing each UTF-16 unit; we approximate with Rust char-based case
/// folding over the common prefix, falling back to length diff.
pub fn string_compare_to_ignore_case(a: &str, b: &str) -> i32 {
    // Java's algorithm folds per UTF-16 unit: toUpperCase then toLowerCase.
    let av: Vec<u16> = a.encode_utf16().collect();
    let bv: Vec<u16> = b.encode_utf16().collect();
    let n = av.len().min(bv.len());
    for i in 0..n {
        let mut c1 = av[i];
        let mut c2 = bv[i];
        if c1 != c2 {
            c1 = fold_u16(c1);
            c2 = fold_u16(c2);
            if c1 != c2 {
                return c1 as i32 - c2 as i32;
            }
        }
    }
    av.len() as i32 - bv.len() as i32
}

fn fold_u16(u: u16) -> u16 {
    // Mirror Character.toLowerCase(Character.toUpperCase(c)) for the BMP.
    if let Some(c) = char::from_u32(u as u32) {
        let up = c.to_uppercase().next().unwrap_or(c);
        let lo = up.to_lowercase().next().unwrap_or(up);
        let mut buf = [0u16; 2];
        let enc = lo.encode_utf16(&mut buf);
        if enc.len() == 1 {
            return enc[0];
        }
    }
    u
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn hashcode_matches_java() {
        assert_eq!(string_hash_code("hello"), 99162322);
        assert_eq!(string_hash_code("héllo→✓"), 290067983);
        assert_eq!(string_hash_code(""), 0);
    }

    #[test]
    fn compareto_matches_java() {
        assert_eq!(string_compare_to("abc", "abd"), -1);
        assert_eq!(string_compare_to("abc", "abc"), 0);
        assert_eq!(string_compare_to("abc", "ab"), 1);
    }
}
