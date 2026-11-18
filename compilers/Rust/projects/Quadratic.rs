//! Quadratic.rs
//!
//! Solves A*x^2 + B*x + C = 0 (real and complex roots).
//! Rust version of documentation/projects/Quadratic.asm.
//!
//! Input (first line of standard input): A B C

use std::io::BufRead;

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("Quadratic")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

fn solve_quadratic(a: f64, b: f64, c: f64) {
    if a == 0.0 {
        println!("Not a quadratic equation: coefficient A must be non-zero.");
        return;
    }
    let disc = b * b - a * c * 4.0;
    let two_a = a * 2.0;
    if disc > 0.0 {
        let sqrt_d = disc.sqrt();
        println!("Two real roots: x1 = {:.6}, x2 = {:.6}", (-b + sqrt_d) / two_a, (-b - sqrt_d) / two_a);
    } else if disc == 0.0 {
        println!("One repeated (double) root: x = {:.6}", -b / two_a);
    } else {
        let imag = ((-disc).sqrt() / two_a).abs();
        let real = -b / two_a;
        println!("Two complex roots: x1 = {:.6} + {:.6}i, x2 = {:.6} - {:.6}i", real, imag, real, imag);
    }
}

fn main() {
    let argv = args();
    if argv.len() != 4 {
        println!("Syntax: Quadratic A B C   (solves A*x^2 + B*x + C = 0)");
        std::process::exit(1);
    }
    let v: Vec<f64> = argv[1..].iter().map(|s| s.parse().unwrap_or(0.0)).collect();
    solve_quadratic(v[0], v[1], v[2]);
}
