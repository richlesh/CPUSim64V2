//! Thin adapter over the `regex` crate to approximate the subset of
//! `java.util.regex` used by the interpreter's string interrupts.
//!
//! Known deviations (surface, do not silently differ):
//!   * Java regex supports backreferences and lookaround; the `regex` crate does
//!     not. If a validation script uses those, the adapter returns an error that
//!     the caller maps to a CPUException, making the deviation visible.
//!   * Java replacement strings use `$1`/`${name}` group syntax; the `regex`
//!     crate uses the same `$1`/`${name}`, so replacements pass through.

use regex::Regex;

pub fn compile(pattern: &str) -> Result<Regex, String> {
    Regex::new(pattern).map_err(|e| format!("regex compile error: {e}"))
}

/// Java `Matcher.find()` — true if the pattern matches anywhere in the input.
pub fn find(pattern: &str, input: &str) -> Result<bool, String> {
    Ok(compile(pattern)?.is_match(input))
}

/// Java `String.replaceFirst`.
pub fn replace_first(input: &str, pattern: &str, repl: &str) -> Result<String, String> {
    Ok(compile(pattern)?.replacen(input, 1, repl).into_owned())
}

/// Java `String.replaceAll`.
pub fn replace_all(input: &str, pattern: &str, repl: &str) -> Result<String, String> {
    Ok(compile(pattern)?.replace_all(input, repl).into_owned())
}

/// Java `String.split(regex)` (limit -1 = no limit; Java drops trailing empty
/// strings when limit == 0, but the interpreter passes limit explicitly).
pub fn split(input: &str, pattern: &str, limit: i32) -> Result<Vec<String>, String> {
    let re = compile(pattern)?;
    if limit > 0 {
        let parts: Vec<String> = re
            .splitn(input, limit as usize)
            .map(|s| s.to_string())
            .collect();
        Ok(parts)
    } else {
        // limit <= 0: split all. Java's limit==0 trims trailing empties; limit<0
        // keeps them. The interpreter uses -1 (keep).
        let mut parts: Vec<String> = re.split(input).map(|s| s.to_string()).collect();
        if limit == 0 {
            while parts.last().map(|s| s.is_empty()).unwrap_or(false) {
                parts.pop();
            }
        }
        Ok(parts)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn find_and_replace() {
        assert_eq!(find(r"\d+", "abc123").unwrap(), true);
        assert_eq!(find(r"^\d+$", "abc123").unwrap(), false);
        assert_eq!(replace_first("aaa", "a", "b").unwrap(), "baa");
        assert_eq!(replace_all("aaa", "a", "b").unwrap(), "bbb");
    }

    #[test]
    fn split_works() {
        assert_eq!(split("a,b,c", ",", -1).unwrap(), vec!["a", "b", "c"]);
    }
}
