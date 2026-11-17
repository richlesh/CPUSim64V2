//! Renard.rs
//!
//! Prints a Renard series (preferred numbers), the series 5, 10, 20 or 40:
//! the powers of the 10th root-of-ten ratio rounded to two decimals.
//! Rust version of documentation/projects/Renard.asm.
//!
//! Input (first line of standard input): the series value

use std::io::BufRead;

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("Renard")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

fn main() {
    let argv = args();
    if argv.len() != 2 {
        println!("You must supply a series value 5, 10, 20 or 40");
        std::process::exit(1);
    }
    let series: i64 = argv[1].parse().unwrap_or(0);
    if series < 2 {
        println!("You must supply a series value > 2");
        std::process::exit(2);
    }
    let factor = 10.0f64.powf(1.0 / series as f64); // the common ratio
    let line: String = (0..series).map(|i| format!("{:.2} ", factor.powf(i as f64))).collect();
    println!("{}", line);
}
