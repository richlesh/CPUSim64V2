//! Cubic.rs
//!
//! Solves A*x^3 + B*x^2 + C*x + D = 0 with Cardano's formula (one real root and
//! a complex pair, three real roots through the trigonometric form).
//! Rust version of documentation/projects/Cubic.asm.
//!
//! Input (first line of standard input): A B C D

use std::f64::consts::PI;
use std::io::BufRead;

const EPSILON: f64 = 0.000000001;

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("Cubic")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

fn real_cbrt(x: f64) -> f64 {
    let r = x.abs().powf(1.0 / 3.0);
    if x < 0.0 {
        -r
    } else {
        r
    }
}

fn solve_cubic(a: f64, mut b: f64, mut c: f64, mut d: f64) {
    b /= a;
    c /= a;
    d /= a;
    let shift = b / 3.0;
    let p = c - b * b / 3.0;
    let q = b * b * b * 2.0 / 27.0 - b * c / 3.0 + d;
    let disc = (q / 2.0) * (q / 2.0) + (p / 3.0) * (p / 3.0) * (p / 3.0);
    if disc > EPSILON {
        let s = disc.sqrt();
        let h = -(q / 2.0);
        let u = real_cbrt(h + s);
        let v = real_cbrt(h - s);
        let real = -((u + v) / 2.0) - shift;
        let imag = (u - v) * 3.0f64.sqrt() / 2.0;
        println!("x1 = {:.6}", u + v - shift);
        println!("x2 = {:.6} + {:.6}i", real, imag);
        println!("x3 = {:.6} - {:.6}i", real, imag);
    } else if disc < -EPSILON {
        let m = (-(p / 3.0)).sqrt() * 2.0;
        let theta = (q * 3.0 / (p * 2.0) * (-(3.0 / p)).sqrt()).acos() / 3.0;
        for k in 0..3 {
            println!("x{} = {:.6}", k + 1, (theta - PI * 2.0 * k as f64 / 3.0).cos() * m - shift);
        }
    } else {
        let u = real_cbrt(-(q / 2.0));
        println!("x1 = {:.6}", u * 2.0 - shift);
        println!("x2 = {:.6}", -u - shift);
        println!("x3 = {:.6}", -u - shift);
    }
}

fn main() {
    let argv = args();
    if argv.len() != 5 {
        println!("Syntax: Cubic A B C D   (solves A*x^3 + B*x^2 + C*x + D = 0)");
        std::process::exit(1);
    }
    let v: Vec<f64> = argv[1..].iter().map(|s| s.parse().unwrap_or(0.0)).collect();
    let (a, b, c, d) = (v[0], v[1], v[2], v[3]);
    if a == 0.0 {
        println!("Error: coefficient A must be non-zero for a cubic equation.");
        std::process::exit(1);
    }
    println!("Solving {:.6}*x^3 + {:.6}*x^2 + {:.6}*x + {:.6} = 0", a, b, c, d);
    solve_cubic(a, b, c, d);
}
