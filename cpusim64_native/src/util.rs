//! Small interpreter-relevant helpers ported from `Utils.java` plus build info.

/// Ports `Utils.decodeSI`: parse an integer with an optional SI suffix
/// (K/M/G/T/P/E, case-insensitive). Returns the value as i32.
pub fn decode_si(s: &str) -> Result<i32, String> {
    let s = s.trim();
    if s.is_empty() {
        return Err("empty input".to_string());
    }
    let last = s.chars().last().unwrap();
    let (multiplier, number_part) = match last.to_ascii_uppercase() {
        'K' => (1e3, &s[..s.len() - 1]),
        'M' => (1e6, &s[..s.len() - 1]),
        'G' => (1e9, &s[..s.len() - 1]),
        'T' => (1e12, &s[..s.len() - 1]),
        'P' => (1e15, &s[..s.len() - 1]),
        'E' => (1e18, &s[..s.len() - 1]),
        _ => (1.0, s),
    };
    let value: f64 = number_part
        .trim()
        .parse()
        .map_err(|_| format!("invalid number: {s}"))?;
    let result = (value * multiplier).round();
    if result > i32::MAX as f64 || result < 0.0 {
        return Err(format!("Value out of range for integer: {s}"));
    }
    Ok(result as i32)
}

pub mod buildinfo {
    // Matches the Java reference output banner for cpusim64 2.8.0.
    pub const VERSION: &str = "2.8.0";
    pub const COPYRIGHT: &str = "©2025–2026";
}

/// Inverse hyperbolic functions (ports `Utils.asinh/acosh/atanh`).
pub fn asinh(x: f64) -> f64 {
    (x + (x * x + 1.0).sqrt()).ln()
}
pub fn acosh(x: f64) -> f64 {
    (x + (x * x - 1.0).sqrt()).ln()
}
pub fn atanh(x: f64) -> f64 {
    0.5 * ((1.0 + x) / (1.0 - x)).ln()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn decode_si_cases() {
        assert_eq!(decode_si("1K").unwrap(), 1000);
        assert_eq!(decode_si("2.5M").unwrap(), 2_500_000);
        assert_eq!(decode_si("42").unwrap(), 42);
        assert_eq!(decode_si("1M").unwrap(), 1_000_000);
    }
}
