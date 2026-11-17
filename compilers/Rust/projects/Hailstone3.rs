//! Hailstone3.rs
//!
//! Like Hailstone2, with a cache of the lengths already computed, and a
//! progress line every 1000 numbers.  Rust version of
//! documentation/projects/Hailstone3.asm.
//!
//! The limit is the first line of standard input (CPUSim64 has no command line).

use std::io::BufRead;

const CACHE_SIZE: usize = 3000000; // number of memoized entries

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("Hailstone3")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

struct Hailstone {
    cache: Vec<i64>, // cache[n] = length for n, 0 = not computed yet
}

impl Hailstone {
    fn new() -> Hailstone {
        let mut cache = vec![0i64; CACHE_SIZE];
        cache[1] = 1; // the sequence of 1 has length 1
        Hailstone { cache }
    }

    fn length(&mut self, n: i64) -> i64 {
        let cached = n >= 0 && (n as usize) < CACHE_SIZE;
        if cached && self.cache[n as usize] != 0 {
            return self.cache[n as usize];
        }
        let len = if n & 1 == 0 {
            self.length(n / 2) + 1
        } else {
            self.length(3 * n + 1) + 1
        };
        if cached {
            self.cache[n as usize] = len;
        }
        len
    }
}

fn main() {
    let argv = args();
    if argv.len() < 2 {
        println!("You must supply a positive integer argument.");
        std::process::exit(1);
    }
    let limit: i64 = argv[1].parse().unwrap_or(0);
    let mut h = Hailstone::new();
    let (mut imax, mut max) = (1i64, 1i64);
    for i in 2..=limit {
        let len = h.length(i);
        if len > max {
            imax = i;
            max = len;
        }
        if i % 1000 == 0 {
            println!("{}...{}:{}", i, imax, max);
        }
    }
    println!("{}: {}", imax, max);
}
