//! Hailstone2.rs
//!
//! Finds the number up to a limit with the longest hailstone sequence.
//! Rust version of documentation/projects/Hailstone2.asm.
//!
//! The limit is the first line of standard input (CPUSim64 has no command line).

use std::io::BufRead;

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("Hailstone2")];
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
    if argv.len() < 2 {
        println!("You must supply a positive integer argument.");
        std::process::exit(1);
    }
    let limit: i64 = argv[1].parse().unwrap_or(0);
    let (imax, max) = (2..=limit)
        .map(|i| (i, compute_hailstone(i)))
        .fold((1, 1), |best, (i, len)| if len > best.1 { (i, len) } else { best });
    println!("{}: {}", imax, max);
}
