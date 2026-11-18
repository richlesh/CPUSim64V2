//! Renard2.rs
//!
//! Prints the Renard series for every series value given.
//! Rust version of documentation/projects/Renard2.asm.
//!
//! Input (first line of standard input): one or more series values

use std::io::BufRead;

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("Renard2")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

fn print_renard(s: i64) {
    let factor = 10.0f64.powf(1.0 / s as f64);
    let mut line = format!("{}:", s);
    for i in 0..s {
        line.push_str(&format!(" {:.2}", factor.powf(i as f64)));
    }
    println!("{}", line);
}

fn main() {
    let argv = args();
    if argv.len() < 2 {
        println!("You must supply a series value 5, 10, 20 or 40");
        std::process::exit(1);
    }
    for a in &argv[1..] {
        print_renard(a.parse().unwrap_or(0));
    }
}
