//! Small interpreter-relevant helpers ported from `Utils.java` plus build info.

/// Ports `Utils.formatSI`: format a count with a binary (1024-based) suffix
/// Ki/Mi/Gi/Ti/Pi/Ei, up to two decimals, no trailing zeros (134217728 -> "128Mi").
pub fn format_si(value: u64) -> String {
    const UNITS: [&str; 7] = ["", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei"];
    let mut d = value as f64;
    let mut i = 0;
    while d >= 1024.0 && i < UNITS.len() - 1 {
        d /= 1024.0;
        i += 1;
    }
    let s = if d == d.round() {
        format!("{}", d as u64)
    } else {
        format!("{d:.2}")
            .trim_end_matches('0')
            .trim_end_matches('.')
            .to_string()
    };
    format!("{s}{}", UNITS[i])
}

/// Ports `Utils.decodeSI`: parse an integer with an optional suffix, case-
/// insensitive: decimal SI K/M/G/T/P/E (10^3 .. 10^18) or binary IEC
/// Ki/Mi/Gi/Ti/Pi/Ei (2^10 .. 2^60). Returns the value as i64 (the Java
/// version is limited to `int`; the native simulator allows 64-bit sizes).
pub fn decode_si(s: &str) -> Result<i64, String> {
    let s = s.trim();
    if s.is_empty() {
        return Err("empty input".to_string());
    }
    // Suffix letter and the text before the suffix; a trailing 'i' makes it binary.
    let (binary, body) = match s.strip_suffix(['i', 'I']) {
        Some(rest) if rest.ends_with(|c: char| "KMGTPEkmgtpe".contains(c)) => (true, rest),
        _ => (false, s),
    };
    let last = body.chars().last().unwrap_or(' ');
    let power: i32 = match last.to_ascii_uppercase() {
        'K' => 1,
        'M' => 2,
        'G' => 3,
        'T' => 4,
        'P' => 5,
        'E' => 6,
        _ => 0,
    };
    let number_part = if power > 0 { &body[..body.len() - 1] } else { body };
    let multiplier = if binary {
        (1u64 << (10 * power)) as f64
    } else {
        10f64.powi(3 * power)
    };
    let value: f64 = number_part
        .trim()
        .parse()
        .map_err(|_| format!("invalid number: {s}"))?;
    let result = (value * multiplier).round();
    if result >= i64::MAX as f64 || result < 0.0 {
        return Err(format!("Value out of range for integer: {s}"));
    }
    Ok(result as i64)
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

/// The words of a command line as Java's `Runtime.exec(String)` splits it: at
/// runs of space, tab, newline, carriage return and form feed, with no quoting.
pub fn split_command_words(cmd: &str) -> Vec<&str> {
    cmd.split([' ', '\t', '\n', '\r', '\u{c}'])
        .filter(|w| !w.is_empty())
        .collect()
}

/// The lines of `text` as Java's `BufferedReader.readLine` returns them: ended
/// by `\n`, `\r` or `\r\n`; a last line without a terminator is still a line.
pub fn java_read_lines(text: &str) -> Vec<&str> {
    let mut lines = Vec::new();
    let bytes = text.as_bytes();
    let mut start = 0;
    let mut i = 0;
    while i < bytes.len() {
        match bytes[i] {
            b'\n' => {
                lines.push(&text[start..i]);
                start = i + 1;
            }
            b'\r' => {
                lines.push(&text[start..i]);
                if i + 1 < bytes.len() && bytes[i + 1] == b'\n' {
                    i += 1;
                }
                start = i + 1;
            }
            _ => {}
        }
        i += 1;
    }
    if start < bytes.len() {
        lines.push(&text[start..]);
    }
    lines
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
        assert_eq!(decode_si("4G").unwrap(), 4_000_000_000);
        assert_eq!(decode_si("3000000000").unwrap(), 3_000_000_000);
        assert!(decode_si("10E").is_err());
    }

    #[test]
    fn decode_si_binary_suffixes() {
        assert_eq!(decode_si("1Ki").unwrap(), 1024);
        assert_eq!(decode_si("8ki").unwrap(), 8192);
        assert_eq!(decode_si("16Mi").unwrap(), 16 * 1024 * 1024);
        assert_eq!(decode_si("0.5Mi").unwrap(), 512 * 1024);
        assert_eq!(decode_si("2GI").unwrap(), 2 * (1i64 << 30));
        assert_eq!(decode_si("3Ti").unwrap(), 3 * (1i64 << 40));
        assert_eq!(decode_si("1Pi").unwrap(), 1i64 << 50);
        assert_eq!(decode_si("4Ei").unwrap(), 1i64 << 62);
        assert!(decode_si("8Ei").is_err()); // 2^63 does not fit in i64
        assert!(decode_si("i").is_err());
        assert!(decode_si("5i").is_err()); // 'i' alone is not a suffix
        assert!(decode_si("Ki").is_err());
    }

    #[test]
    fn split_command_words_like_runtime_exec() {
        assert_eq!(split_command_words("sum.sh 4 5"), vec!["sum.sh", "4", "5"]);
        assert_eq!(split_command_words("  ls\t-l \n"), vec!["ls", "-l"]);
        assert!(split_command_words(" \t ").is_empty());
        // no quoting, as in Java
        assert_eq!(split_command_words("echo \"a b\""), vec!["echo", "\"a", "b\""]);
    }

    #[test]
    fn java_read_lines_cases() {
        assert_eq!(java_read_lines("a\nb\n"), vec!["a", "b"]);
        assert_eq!(java_read_lines("a\r\nb\rc"), vec!["a", "b", "c"]);
        assert_eq!(java_read_lines("\n\nx"), vec!["", "", "x"]);
        assert!(java_read_lines("").is_empty());
    }
}
