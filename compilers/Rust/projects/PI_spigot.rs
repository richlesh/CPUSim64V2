//! PI_spigot.rs
//!
//! Prints n digits of pi after the decimal point with the Rabinowitz-Wagon
//! spigot algorithm:
//!     len = floor(10 * D / 3) + 1,  a[1..len] = 2
//!     repeat D times:
//!         q = 0
//!         for i = len down to 1:
//!             x = 10 * a[i] + q * i;  a[i] = x mod (2i - 1);  q = x div (2i - 1)
//!         a[1] = q mod 10,  q = q div 10
//! A digit 9 is held back (it may become 0 after a carry of 10).  A few guard
//! digits are computed past the requested ones.
//! Rust version of documentation/projects/PI_spigot.asm.
//!
//! Input (first line of standard input): n

use std::io::BufRead;

const GUARD_DIGITS: i64 = 10;

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("PI_spigot")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

/// collects the digits of pi that are printed
struct Emitter {
    index: i64, // the index of the next digit: -1 is the predigit 0, 0 is the 3
    end: i64,   // the digits with index < end are printed
    text: String,
}

impl Emitter {
    fn digit(&mut self, d: i64) {
        self.index += 1;
        if self.index >= 0 && self.index < self.end {
            self.text.push((b'0' + d as u8) as char);
            if self.index == 0 {
                self.text.push('.');
            }
        }
    }

    fn run(&mut self, d: i64, count: i64) {
        for _ in 0..count {
            self.digit(d);
        }
    }
}

fn main() {
    let argv = args();
    if argv.len() != 2 {
        println!("Usage: PI_spigot <n>   (n = number of digits after the decimal point)");
        std::process::exit(1);
    }
    let n: i64 = argv[1].parse().unwrap_or(0);
    if n < 1 {
        println!("n must be >= 1");
        std::process::exit(1);
    }
    let mut em = Emitter { index: -2, end: n + 1, text: String::new() };
    let total = n + 1 + GUARD_DIGITS;
    let len = (total * 10 / 3 + 1) as usize;
    let mut a = vec![2i64; len + 1];
    let (mut predigit, mut nines) = (0i64, 0i64);
    for _ in 0..total {
        let mut q = 0i64;
        for i in (1..=len).rev() {
            let den = 2 * i as i64 - 1;
            let x = a[i] * 10 + q * i as i64;
            q = x / den;
            a[i] = x % den;
        }
        a[1] = q % 10;
        q /= 10;
        if q == 9 {
            nines += 1;
        } else if q == 10 {
            em.digit(predigit + 1);
            em.run(0, nines);
            predigit = 0;
            nines = 0;
        } else {
            em.digit(predigit);
            predigit = q;
            em.run(9, nines);
            nines = 0;
        }
        if em.index + 1 >= em.end {
            break;
        }
    }
    println!("{}", em.text);
}
