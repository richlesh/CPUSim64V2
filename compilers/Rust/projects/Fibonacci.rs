//! Fibonacci.rs
//!
//! Prints the first 45 Fibonacci numbers, one per line.
//! Rust version of documentation/projects/Fibonacci.asm.

struct Fib {
    a: i64,
    b: i64,
}

impl Iterator for Fib {
    type Item = i64;

    fn next(&mut self) -> Option<i64> {
        let c = self.a + self.b;
        let out = self.a;
        self.a = self.b;
        self.b = c;
        Some(out)
    }
}

fn main() {
    for n in (Fib { a: 1, b: 1 }).take(45) {
        println!("{}", n);
    }
}
