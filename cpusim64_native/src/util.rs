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
    // Version is injected at build time from the Maven project version via
    // build.rs (env CPUSIM64_VERSION). Falls back to the Cargo package version
    // for a plain `cargo build` outside the Maven wrapper.
    pub const VERSION: &str = env!("CPUSIM64_BUILD_VERSION");
    pub const COPYRIGHT: &str = "©2025–2026";
}

/// Monotonic nanosecond clock mirroring Java's `System.nanoTime()`.
///
/// Values are relative to a fixed, lazily-initialized process baseline, so the
/// absolute magnitude is meaningless but deltas are accurate. Used by the
/// simulator to measure wall-clock run time and system (I/O / interrupt) time.
pub fn nano_time() -> i64 {
    use std::sync::OnceLock;
    use std::time::Instant;
    static BASELINE: OnceLock<Instant> = OnceLock::new();
    let base = BASELINE.get_or_init(Instant::now);
    base.elapsed().as_nanos() as i64
}

/// Uniform pseudo-random double in `[0.0, 1.0)`, mirroring Java's
/// `Math.random()`.
///
/// Backed by a process-global SplitMix64 generator seeded once from system
/// entropy (monotonic clock plus the address of a stack local). This avoids an
/// external `rand` crate dependency while providing good-quality, non-repeating
/// values. The top 53 bits of each 64-bit output form the mantissa, giving a
/// uniform double in `[0, 1)` exactly as the usual `(x >> 11) / 2^53` mapping.
pub fn random() -> f64 {
    use std::sync::{Mutex, OnceLock};

    static STATE: OnceLock<Mutex<u64>> = OnceLock::new();
    let state = STATE.get_or_init(|| {
        // Seed from the monotonic clock and a stack address for entropy.
        let seed_marker = 0u8;
        let seed = (nano_time() as u64)
            ^ (&seed_marker as *const u8 as u64).rotate_left(17)
            ^ 0x9E37_79B9_7F4A_7C15;
        Mutex::new(seed)
    });

    let mut s = state.lock().unwrap();
    // SplitMix64 step.
    *s = s.wrapping_add(0x9E37_79B9_7F4A_7C15);
    let mut z = *s;
    z = (z ^ (z >> 30)).wrapping_mul(0xBF58_476D_1CE4_E5B9);
    z = (z ^ (z >> 27)).wrapping_mul(0x94D0_49BB_1331_11EB);
    z ^= z >> 31;

    // Top 53 bits -> uniform double in [0, 1).
    (z >> 11) as f64 * (1.0 / (1u64 << 53) as f64)
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
