//! Congruent.rs
//!
//! Tests whether n is a congruent number with Tunnell's theorem (the result is
//! conditional on the Birch and Swinnerton-Dyer conjecture).
//! Rust version of documentation/projects/Congruent.asm.
//!
//! Input (first line of standard input): n

use std::io::BufRead;

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("Congruent")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

/// n with every square factor divided out
fn squarefree_kernel(n: i64) -> i64 {
    let mut m = n;
    let mut i = 2;
    while i * i <= m {
        if m % (i * i) == 0 {
            m /= i * i;
        } else {
            i += 1;
        }
    }
    m
}

fn isqrt(n: i64) -> i64 {
    (n as f64).sqrt().floor() as i64
}

fn compute_limit(target: i64, divisor: i64) -> i64 {
    (target as f64 / divisor as f64).sqrt().floor() as i64
}

/// the number of integer solutions of a*x^2 + b*y^2 + c*z^2 = target
fn count_solutions(target: i64, a: i64, b: i64, c: i64) -> i64 {
    let mut count = 0;
    let xlim = compute_limit(target, a);
    for x in -xlim..=xlim {
        let mut rem_x = a * x * x;
        if rem_x > target {
            continue;
        }
        rem_x = target - rem_x;
        let ylim = compute_limit(rem_x, b);
        for y in -ylim..=ylim {
            let mut rem_xy = b * y * y;
            if rem_xy > rem_x {
                continue;
            }
            rem_xy = rem_x - rem_xy;
            if rem_xy % c == 0 {
                let zsq = rem_xy / c;
                let z = isqrt(zsq);
                if z * z == zsq {
                    count += if z == 0 { 1 } else { 2 }; // z and -z
                }
            }
        }
    }
    count
}

fn main() {
    let argv = args();
    if argv.len() < 2 {
        println!("Usage: Congruent <n>");
        std::process::exit(1);
    }
    let n: i64 = argv[1].parse().unwrap_or(0);
    if n <= 0 {
        println!("n must be a positive integer");
        std::process::exit(1);
    }
    let m = squarefree_kernel(n);
    let (t1, t2) = if m & 1 == 1 {
        (count_solutions(m, 2, 1, 8), count_solutions(m, 2, 1, 32))
    } else {
        (count_solutions(m, 8, 2, 16), count_solutions(m, 8, 2, 64))
    };
    println!("n = {} (squarefree kernel = {})", n, m);
    println!("t1 = {}, 2*t2 = {}", t1, 2 * t2);
    if t1 == 2 * t2 {
        println!("{} IS a congruent number (assuming BSD)", n);
    } else {
        println!("{} is NOT a congruent number", n);
    }
}
