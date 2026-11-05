//! Java `Formatter`-compatible number/string formatting.
//!
//! Strategy for floats: obtain correctly-rounded decimal digits from Rust's own
//! formatter (which uses round-half-to-even for `{:e}`, but Java uses HALF_UP).
//! To match Java's HALF_UP we generate one extra digit and round manually.

/// A finite-float decimal breakdown: sign, the significant digits (no leading
/// zeros except a single "0" for zero), and the power-of-ten exponent such that
/// value = 0.d1d2... * 10^(exp+1), i.e. `exp` is the exponent of the first
/// significant digit (scientific exponent).
struct Decimal {
    negative: bool,
    digits: Vec<u8>, // significant decimal digits, most significant first
    exp: i32,        // exponent of the first digit (value ~= d[0].d[1..] * 10^exp)
}

/// Produce `sig` significant digits for |x| (x finite, != 0), sourced from
/// Java's shortest decimal (Schubfach) and then rounded HALF_UP or zero-padded
/// to `sig` digits. This matches `java.util.Formatter`, which formats the
/// `Double.toString` digits rather than the exact binary value.
fn decimal_digits(x: f64, sig: usize) -> Decimal {
    let negative = x.is_sign_negative();
    let ax = x.abs();
    if ax == 0.0 {
        return Decimal {
            negative,
            digits: vec![0; sig.max(1)],
            exp: 0,
        };
    }
    let fp = crate::javacompat::schubfach::to_decimal(ax)
        .expect("finite non-zero has a decimal");
    // Shortest digits of f (len digits), scientific exponent of first digit:
    //   value = f * 10^e, f has `len` digits => first digit exponent = e + len - 1.
    let mut digits: Vec<u8> = fp.f.to_string().bytes().map(|b| b - b'0').collect();
    // Trailing zeros in f are significant positions for the shortest repr; keep
    // them (Schubfach's len already excludes implicit trailing structure).
    // Scientific exponent of the most significant digit:
    let mut sci_exp = fp.e + digits.len() as i32 - 1;

    if digits.len() > sig {
        // Round HALF_UP to `sig` digits.
        let round_digit = digits[sig];
        digits.truncate(sig);
        if round_digit >= 5 {
            let mut i = sig as isize - 1;
            loop {
                if i < 0 {
                    digits.insert(0, 1);
                    sci_exp += 1;
                    digits.truncate(sig);
                    break;
                }
                if digits[i as usize] == 9 {
                    digits[i as usize] = 0;
                    i -= 1;
                } else {
                    digits[i as usize] += 1;
                    break;
                }
            }
        }
    }
    while digits.len() < sig {
        digits.push(0);
    }
    Decimal {
        negative,
        digits,
        exp: sci_exp,
    }
}

/// Render using fixed notation with `sig` significant digits whose first digit
/// has scientific exponent `exp`. Produces e.g. "123.4500" or "0.001234".
fn render_fixed_sig(d: &Decimal, sig: usize) -> String {
    let mut out = String::new();
    if d.negative {
        out.push('-');
    }
    let exp = d.exp;
    if exp >= 0 {
        let int_digits = (exp + 1) as usize;
        if int_digits >= sig {
            // All significant digits are integer digits; pad with zeros.
            for i in 0..sig {
                out.push((b'0' + d.digits[i]) as char);
            }
            for _ in sig..int_digits {
                out.push('0');
            }
            // No fractional part and no decimal point (matches Java for e.g.
            // %.17g of 1e16 -> "10000000000000000").
        } else {
            for i in 0..int_digits {
                out.push((b'0' + d.digits[i]) as char);
            }
            out.push('.');
            for i in int_digits..sig {
                out.push((b'0' + d.digits[i]) as char);
            }
        }
    } else {
        // 0.000ddd...
        out.push('0');
        out.push('.');
        let zeros = (-exp - 1) as usize;
        for _ in 0..zeros {
            out.push('0');
        }
        for i in 0..sig {
            out.push((b'0' + d.digits[i]) as char);
        }
    }
    out
}

/// Render scientific notation: d.dddde±XX (ports Java `%e`/`%g` scientific).
/// `frac` = digits after the decimal point.
fn render_scientific(d: &Decimal, frac: usize) -> String {
    let mut out = String::new();
    if d.negative {
        out.push('-');
    }
    out.push((b'0' + d.digits[0]) as char);
    if frac > 0 {
        out.push('.');
        for i in 1..=frac {
            out.push((b'0' + *d.digits.get(i).unwrap_or(&0)) as char);
        }
    }
    out.push('e');
    if d.exp < 0 {
        out.push('-');
    } else {
        out.push('+');
    }
    let ae = d.exp.unsigned_abs();
    if ae < 10 {
        out.push('0');
        out.push((b'0' + ae as u8) as char);
    } else {
        out.push_str(&ae.to_string());
    }
    out
}

/// Java `%g` with `precision` significant digits (default 6; 0 treated as 1).
pub fn format_g(x: f64, precision: usize) -> String {
    let sig = precision.max(1);
    if x.is_nan() {
        return "NaN".to_string();
    }
    if x.is_infinite() {
        return if x < 0.0 { "-Infinity" } else { "Infinity" }.to_string();
    }
    if x == 0.0 {
        let d = Decimal {
            negative: x.is_sign_negative(),
            digits: vec![0; sig],
            exp: 0,
        };
        // exp 0 is in [-4, sig) so fixed notation.
        return render_fixed_sig(&d, sig);
    }
    let d = decimal_digits(x, sig);
    if d.exp < -4 || d.exp >= sig as i32 {
        render_scientific(&d, sig - 1)
    } else {
        render_fixed_sig(&d, sig)
    }
}

/// Java `%e` with `precision` fractional digits (default 6).
pub fn format_e(x: f64, precision: usize) -> String {
    if x.is_nan() {
        return "NaN".to_string();
    }
    if x.is_infinite() {
        return if x < 0.0 { "-Infinity" } else { "Infinity" }.to_string();
    }
    let sig = precision + 1;
    let d = if x == 0.0 {
        Decimal {
            negative: x.is_sign_negative(),
            digits: vec![0; sig],
            exp: 0,
        }
    } else {
        decimal_digits(x, sig)
    };
    render_scientific(&d, precision)
}

/// Java `%f` with `precision` fractional digits (default 6).
pub fn format_f(x: f64, precision: usize) -> String {
    if x.is_nan() {
        return "NaN".to_string();
    }
    if x.is_infinite() {
        return if x < 0.0 { "-Infinity" } else { "Infinity" }.to_string();
    }
    let mut out = String::new();
    if x.is_sign_negative() {
        out.push('-');
    }
    let ax = x.abs();
    // Number of significant digits needed = integer digits + precision. Use a
    // high-precision scientific expansion then place the decimal point.
    if ax == 0.0 {
        out.push('0');
        if precision > 0 {
            out.push('.');
            for _ in 0..precision {
                out.push('0');
            }
        }
        return out;
    }
    // Determine exponent first.
    let probe = decimal_digits(ax, 1);
    let exp = probe.exp;
    let int_digits = if exp >= 0 { exp + 1 } else { 0 } as i32;
    let sig = (int_digits + precision as i32).max(1) as usize;
    let d = decimal_digits(ax, sig);
    // Rebuild possibly shifted exponent (rounding may have grown it).
    let exp = d.exp;
    let int_digits = if exp >= 0 { (exp + 1) as usize } else { 0 };
    // Integer part.
    if int_digits == 0 {
        out.push('0');
    } else {
        for i in 0..int_digits.min(d.digits.len()) {
            out.push((b'0' + d.digits[i]) as char);
        }
        for _ in d.digits.len()..int_digits {
            out.push('0');
        }
    }
    if precision > 0 {
        out.push('.');
        if exp < 0 {
            let zeros = (-exp - 1) as usize;
            for _ in 0..zeros.min(precision) {
                out.push('0');
            }
            let mut written = zeros.min(precision);
            let mut di = 0;
            while written < precision {
                let ch = *d.digits.get(di).unwrap_or(&0);
                out.push((b'0' + ch) as char);
                di += 1;
                written += 1;
            }
        } else {
            let mut written = 0;
            let mut di = int_digits;
            while written < precision {
                let ch = *d.digits.get(di).unwrap_or(&0);
                out.push((b'0' + ch) as char);
                di += 1;
                written += 1;
            }
        }
    }
    out
}

/// Apply a field width with optional left-justify and zero-pad to a rendered
/// value `body`. `width` 0 means no padding.
pub fn pad(body: &str, width: usize, left_justify: bool, zero_pad: bool) -> String {
    if body.len() >= width {
        return body.to_string();
    }
    let padding = width - body.len();
    if left_justify {
        format!("{}{}", body, " ".repeat(padding))
    } else if zero_pad {
        // Zero padding goes after a leading sign.
        if let Some(stripped) = body.strip_prefix('-') {
            format!("-{}{}", "0".repeat(padding), stripped)
        } else if let Some(stripped) = body.strip_prefix('+') {
            format!("+{}{}", "0".repeat(padding), stripped)
        } else {
            format!("{}{}", "0".repeat(padding), body)
        }
    } else {
        format!("{}{}", " ".repeat(padding), body)
    }
}

/// A single formatting argument for `java_printf`.
#[derive(Debug, Clone)]
pub enum Arg {
    Int(i64),
    Float(f64),
    Str(String),
    Char(i32),
}

/// Java `Formatter`/`String.format` for the conversion subset used by the
/// interpreter: `%[flags][width][.precision][cdxefgs]` plus `%%`/`%n`.
pub fn java_printf(fmt: &str, args: &[Arg]) -> String {
    let bytes: Vec<char> = fmt.chars().collect();
    let mut out = String::new();
    let mut i = 0;
    let mut arg_idx = 0;
    while i < bytes.len() {
        let c = bytes[i];
        if c != '%' {
            out.push(c);
            i += 1;
            continue;
        }
        i += 1;
        if i < bytes.len() && bytes[i] == '%' {
            out.push('%');
            i += 1;
            continue;
        }
        if i < bytes.len() && bytes[i] == 'n' {
            out.push('\n');
            i += 1;
            continue;
        }
        let mut left = false;
        let mut plus = false;
        let mut zero = false;
        let mut space = false;
        while i < bytes.len() {
            match bytes[i] {
                '-' => left = true,
                '+' => plus = true,
                '0' => zero = true,
                ' ' => space = true,
                _ => break,
            }
            i += 1;
        }
        let mut width = 0usize;
        while i < bytes.len() && bytes[i].is_ascii_digit() {
            width = width * 10 + (bytes[i] as usize - '0' as usize);
            i += 1;
        }
        let mut precision: Option<usize> = None;
        if i < bytes.len() && bytes[i] == '.' {
            i += 1;
            let mut p = 0usize;
            while i < bytes.len() && bytes[i].is_ascii_digit() {
                p = p * 10 + (bytes[i] as usize - '0' as usize);
                i += 1;
            }
            precision = Some(p);
        }
        if i >= bytes.len() {
            break;
        }
        let conv = bytes[i];
        i += 1;
        let arg = args.get(arg_idx).cloned();
        arg_idx += 1;
        let body = render_conv(conv, &arg, precision, plus, space);
        out.push_str(&pad(&body, width, left, zero && !left));
    }
    out
}

fn render_conv(
    conv: char,
    arg: &Option<Arg>,
    precision: Option<usize>,
    plus: bool,
    space: bool,
) -> String {
    let sign_prefix = |neg: bool, body: String| -> String {
        if neg {
            format!("-{}", body)
        } else if plus {
            format!("+{}", body)
        } else if space {
            format!(" {}", body)
        } else {
            body
        }
    };
    match conv {
        'd' => {
            let v = match arg {
                Some(Arg::Int(v)) => *v,
                Some(Arg::Float(f)) => *f as i64,
                _ => 0,
            };
            sign_prefix(v < 0, v.unsigned_abs().to_string())
        }
        'x' => {
            let v = match arg {
                Some(Arg::Int(v)) => *v,
                _ => 0,
            };
            format!("{:x}", v as u64)
        }
        'X' => {
            let v = match arg {
                Some(Arg::Int(v)) => *v,
                _ => 0,
            };
            format!("{:X}", v as u64)
        }
        'f' => {
            let v = match arg {
                Some(Arg::Float(f)) => *f,
                Some(Arg::Int(v)) => *v as f64,
                _ => 0.0,
            };
            let prec = precision.unwrap_or(6);
            sign_prefix(v.is_sign_negative() && !v.is_nan(), format_f(v.abs(), prec))
        }
        'e' | 'E' => {
            let v = match arg {
                Some(Arg::Float(f)) => *f,
                Some(Arg::Int(v)) => *v as f64,
                _ => 0.0,
            };
            let prec = precision.unwrap_or(6);
            let mut body = format_e(v.abs(), prec);
            if conv == 'E' {
                body = body.to_uppercase();
            }
            sign_prefix(v.is_sign_negative(), body)
        }
        'g' | 'G' => {
            let v = match arg {
                Some(Arg::Float(f)) => *f,
                Some(Arg::Int(v)) => *v as f64,
                _ => 0.0,
            };
            let prec = precision.unwrap_or(6);
            let mut body = format_g(v.abs(), prec);
            if conv == 'G' {
                body = body.to_uppercase();
            }
            sign_prefix(v.is_sign_negative(), body)
        }
        's' | 'S' => {
            let mut s = match arg {
                Some(Arg::Str(s)) => s.clone(),
                Some(Arg::Int(v)) => v.to_string(),
                Some(Arg::Float(f)) => f.to_string(),
                Some(Arg::Char(c)) => char::from_u32(*c as u32)
                    .map(|c| c.to_string())
                    .unwrap_or_default(),
                None => "null".to_string(),
            };
            if let Some(p) = precision {
                s = s.chars().take(p).collect();
            }
            if conv == 'S' {
                s = s.to_uppercase();
            }
            s
        }
        'c' | 'C' => {
            let cp = match arg {
                Some(Arg::Char(c)) => *c,
                Some(Arg::Int(v)) => *v as i32,
                _ => 0,
            };
            char::from_u32(cp as u32)
                .map(|c| c.to_string())
                .unwrap_or_default()
        }
        other => other.to_string(),
    }
}

/// Java `MessageFormat.format` over `{n}` placeholders (positional strings).
pub fn message_format(template: &str, args: &[String]) -> String {
    let numbered = number_placeholders(template);
    let chars: Vec<char> = numbered.chars().collect();
    let mut out = String::new();
    let mut i = 0;
    while i < chars.len() {
        if chars[i] == '{' {
            let mut j = i + 1;
            let mut num = String::new();
            while j < chars.len() && chars[j] != '}' {
                num.push(chars[j]);
                j += 1;
            }
            if j < chars.len() && chars[j] == '}' {
                if let Ok(idx) = num.trim().parse::<usize>() {
                    if let Some(a) = args.get(idx) {
                        out.push_str(a);
                    }
                    i = j + 1;
                    continue;
                }
            }
            out.push('{');
            i += 1;
        } else {
            out.push(chars[i]);
            i += 1;
        }
    }
    out
}

/// Ports `numberPlaceholders`: replaces each bare `{}` with `{0}`, `{1}`, ...
pub fn number_placeholders(template: &str) -> String {
    let chars: Vec<char> = template.chars().collect();
    let mut out = String::new();
    let mut count = 0;
    let mut i = 0;
    while i < chars.len() {
        if chars[i] == '{' && i + 1 < chars.len() && chars[i + 1] == '}' {
            out.push('{');
            out.push_str(&count.to_string());
            out.push('}');
            count += 1;
            i += 2;
        } else {
            out.push(chars[i]);
            i += 1;
        }
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    fn g16(x: f64) -> String {
        pad(&format_g(x, 16), 23, false, false)
    }

    #[test]
    fn g16_matches_java_corpus() {
        assert_eq!(g16(0.0), "      0.000000000000000");
        assert_eq!(g16(2.0), "      2.000000000000000");
        assert_eq!(g16(26.0), "      26.00000000000000");
        assert_eq!(g16(53.0), "      53.00000000000000");
        assert_eq!(g16(123456.789), "      123456.7890000000");
        assert_eq!(g16(1.0e-5), "  1.000000000000000e-05");
        assert_eq!(g16(1.0e-4), "  0.0001000000000000000");
        assert_eq!(g16(1.0e16), "  1.000000000000000e+16");
        assert_eq!(g16(100000.0), "      100000.0000000000");
        assert_eq!(g16(1000000.0), "      1000000.000000000");
        assert_eq!(g16(-2.5), "     -2.500000000000000");
        assert_eq!(g16(1.0 / 3.0), "     0.3333333333333333");
        assert_eq!(g16(123.456), "      123.4560000000000");
    }

    #[test]
    fn g17_matches_java_corpus() {
        assert_eq!(format_g(0.0, 17), "0.0000000000000000");
        assert_eq!(format_g(2.0, 17), "2.0000000000000000");
        assert_eq!(format_g(26.0, 17), "26.000000000000000");
        assert_eq!(format_g(1.0e16, 17), "10000000000000000");
        assert_eq!(format_g(1.0e17, 17), "1.0000000000000000e+17");
        assert_eq!(format_g(100000.0, 17), "100000.00000000000");
    }

    #[test]
    fn f_and_e_match_java() {
        assert_eq!(format_f(3.14159265358979, 6), "3.141593");
        assert_eq!(format_f(0.0, 6), "0.000000");
        assert_eq!(format_f(1.0e-5, 6), "0.000010");
        assert_eq!(format_f(1.0e-4, 6), "0.000100");
        assert_eq!(format_e(0.0, 6), "0.000000e+00");
        assert_eq!(format_e(26.0, 6), "2.600000e+01");
        assert_eq!(format_e(9999999.5, 6), "1.000000e+07");
        assert_eq!(format_e(123456.789, 6), "1.234568e+05");
    }

    #[test]
    fn printf_matches_java() {
        assert_eq!(
            java_printf("%d items, %5.2f%% done", &[Arg::Int(3), Arg::Float(66.6667)]),
            "3 items, 66.67% done"
        );
        assert_eq!(java_printf("x=%08x", &[Arg::Int(255)]), "x=000000ff");
        assert_eq!(java_printf("%-10s|", &[Arg::Str("hi".into())]), "hi        |");
        assert_eq!(java_printf("%+d %+d", &[Arg::Int(5), Arg::Int(-5)]), "+5 -5");
        assert_eq!(
            java_printf("%c%c%c", &[Arg::Char(72), Arg::Char(105), Arg::Char(33)]),
            "Hi!"
        );
        assert_eq!(java_printf("%g", &[Arg::Float(3.14159)]), "3.14159");
    }

    #[test]
    fn message_format_matches_java() {
        assert_eq!(
            message_format("Hello {0}, you are {1}", &["Bob".into(), "42".into()]),
            "Hello Bob, you are 42"
        );
        assert_eq!(number_placeholders("a {} b {} c"), "a {0} b {1} c");
    }
}
