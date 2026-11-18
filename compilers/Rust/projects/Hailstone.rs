//! Hailstone.rs
//!
//! Length of the hailstone (Collatz) sequence of n: with an argument it prints
//! that number's length, without one it prints the lengths of 1..=100 and
//! exits with status 1.  Rust version of documentation/projects/Hailstone.asm.
//!
//! The program has no command line on CPUSim64: the arguments are the first
//! line of standard input (see run_projects.sh).

use std::io::BufRead;

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("Hailstone")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

fn compute_hailstone(n: i64) -> i64 {
    if n <= 1 {
        1
    } else if n & 1 == 0 {
        compute_hailstone(n / 2) + 1
    } else {
        compute_hailstone(3 * n + 1) + 1
    }
}

fn main() {
    let argv = args();
    if argv.len() >= 2 {
        let n: i64 = argv[1].parse().unwrap_or(0);
        println!("{}", compute_hailstone(n));
        return;
    }
    for i in 1..=100 {
        println!("{}: {}", i, compute_hailstone(i));
    }
    std::process::exit(1);
}
