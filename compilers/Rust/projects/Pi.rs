//! Pi.rs
//!
//! Computes pi with the Bailey-Borwein-Plouffe series
//!     pi = sum over k of 1/16^k * (4/(8k+1) - 2/(8k+4) - 1/(8k+5) - 1/(8k+6))
//! Rust version of documentation/projects/Pi.asm.
//!
//! Input (first line of standard input): the number of series terms, 1-11.

use std::io::BufRead;

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("Pi")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

fn compute_pi(terms: i64) -> f64 {
    (0..terms)
        .map(|k| {
            let kf = k as f64;
            let t = 4.0 / (8.0 * kf + 1.0) - 2.0 / (8.0 * kf + 4.0) - 1.0 / (8.0 * kf + 5.0) - 1.0 / (8.0 * kf + 6.0);
            t / 16f64.powi(k as i32)
        })
        .fold(0.0, |sum, x| sum + x)
}

fn main() {
    let argv = args();
    if argv.len() != 2 {
        println!("You must supply the number of terms 1-11");
        std::process::exit(1);
    }
    let terms: i64 = argv[1].parse().unwrap_or(0);
    if !(0 < terms && terms <= 11) {
        println!("You must supply the number of terms in the range [1-11]");
        std::process::exit(1);
    }
    println!("PI({}): {:.16}", terms, compute_pi(terms));
}
