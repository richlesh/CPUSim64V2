//! BBP_hex_digits.rs
//!
//! Prints the 32 hex digits of pi that start at hex position n after the point,
//! computed with the Bailey-Borwein-Plouffe formula without the earlier digits
//! (n = 1 is the first digit after the point).  The fractions are 192-bit fixed
//! point numbers.  Rust version of documentation/projects/BBP_hex_digits.asm.
//!
//! Input (first line of standard input): n

use std::io::BufRead;
use std::io::Write;

const MAX_POSITION: i64 = 10000000;

fn args() -> Vec<String> {
    let v: Vec<String> = std::env::args().collect();
    if v.len() > 1 {
        return v; // real command-line arguments (host rustc)
    }
    // CPUSim64: no arguments, so read them from the first line of stdin
    print!("n? ");
    let _ = std::io::stdout().flush();
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("BBP_hex_digits")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

/// a fraction in [0, 1): hi holds the first 64 bits
#[derive(Clone, Copy)]
struct U192 {
    hi: u64,
    mid: u64,
    lo: u64,
}

impl U192 {
    /// num / den as a 192-bit fraction (num < den < 2^32): long division 32 bits at a time
    fn frac(num: u64, den: u64) -> U192 {
        let mut words = [0u64; 3];
        let mut r = num;
        for w in words.iter_mut() {
            let a = (r << 32) / den;
            r = (r << 32) % den;
            let b = (r << 32) / den;
            r = (r << 32) % den;
            *w = (a << 32) | b;
        }
        U192 { hi: words[0], mid: words[1], lo: words[2] }
    }

    fn add(&mut self, b: U192) {
        let lo = self.lo.wrapping_add(b.lo);
        let c1 = (lo < self.lo) as u64;
        let mid = self.mid.wrapping_add(b.mid);
        let mut c2 = (mid < self.mid) as u64;
        let mid2 = mid.wrapping_add(c1);
        if mid2 < mid {
            c2 = 1;
        }
        self.lo = lo;
        self.mid = mid2;
        self.hi = self.hi.wrapping_add(b.hi).wrapping_add(c2);
    }

    fn sub(&mut self, b: U192) {
        let lo = self.lo.wrapping_sub(b.lo);
        let c1 = (self.lo < b.lo) as u64;
        let mid = self.mid.wrapping_sub(b.mid);
        let mut c2 = (self.mid < b.mid) as u64;
        if c1 == 1 && mid == 0 {
            c2 = 1;
        }
        self.lo = lo;
        self.mid = mid.wrapping_sub(c1);
        self.hi = self.hi.wrapping_sub(b.hi).wrapping_sub(c2);
    }

    /// shifts left by n (0 < n < 64)
    fn shl(&mut self, n: u32) {
        self.hi = (self.hi << n) | (self.mid >> (64 - n));
        self.mid = (self.mid << n) | (self.lo >> (64 - n));
        self.lo <<= n;
    }
}

/// 16^e mod m
fn powmod16(mut e: u64, m: u64) -> u64 {
    if m == 1 {
        return 0;
    }
    let mut r = 1;
    let mut b = 16 % m;
    while e > 0 {
        if e & 1 == 1 {
            r = r * b % m;
        }
        b = b * b % m;
        e >>= 1;
    }
    r
}

/// the fractional part of 16^d * S(j)
fn series(j: u64, d: u64) -> U192 {
    let mut s = U192 { hi: 0, mid: 0, lo: 0 };
    for k in 0..=d {
        let m = 8 * k + j;
        s.add(U192::frac(powmod16(d - k, m), m));
    }
    // the tail: 16^-t / m for the next terms, shifted right by 4t bits
    for t in 1..48u64 {
        let m = 8 * (d + t) + j;
        let mut f = U192::frac(1, m);
        let mut sh = 4 * t;
        while sh >= 64 {
            f.lo = f.mid;
            f.mid = f.hi;
            f.hi = 0;
            sh -= 64;
        }
        if sh != 0 {
            f.lo = (f.lo >> sh) | (f.mid << (64 - sh));
            f.mid = (f.mid >> sh) | (f.hi << (64 - sh));
            f.hi >>= sh;
        }
        s.add(f);
    }
    s
}

fn main() {
    let argv = args();
    if argv.len() != 2 {
        println!("Usage: BBP_hex_digits <n>   (n = 1 is the first hex digit after the point)");
        std::process::exit(1);
    }
    let n: i64 = argv[1].parse().unwrap_or(0);
    if n < 1 {
        println!("n must be >= 1");
        std::process::exit(1);
    }
    if n > MAX_POSITION {
        println!("n must be <= {}", MAX_POSITION);
        std::process::exit(1);
    }
    let d = (n - 1) as u64;
    let mut acc = series(1, d);
    acc.shl(2); // 4 * S(1)
    let mut t = series(4, d);
    t.shl(1); // 2 * S(4)
    acc.sub(t);
    acc.sub(series(5, d));
    acc.sub(series(6, d));
    println!("Hex digits of pi at position {}: {:016X}{:016X}", n, acc.hi, acc.mid);
    // the top 32 guard bits: all 0s or all 1s mean the last digits may be off by one
    let guard = acc.lo >> 32;
    if guard == 0 {
        println!("Warning: guard bits are all 0s; the last digit(s) may be 1 too high.");
    } else if guard == 0xFFFFFFFF {
        println!("Warning: guard bits are all 1s; the last digit(s) may be 1 too low.");
    }
}
