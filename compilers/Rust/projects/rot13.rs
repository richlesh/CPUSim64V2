//! rot13.rs
//!
//! ROT13: rotates every letter by 13 places (running it twice restores the
//! text).  Reads standard input and writes standard output.
//! Rust version of documentation/projects/rot13.asm.

use std::io::{Read, Write};

fn rot13(c: u8) -> u8 {
    match c {
        b'A'..=b'Z' => b'A' + (c - b'A' + 13) % 26,
        b'a'..=b'z' => b'a' + (c - b'a' + 13) % 26,
        _ => c,
    }
}

fn main() {
    let mut input = Vec::new();
    std::io::stdin().read_to_end(&mut input).unwrap();
    let output: Vec<u8> = input.iter().map(|&c| rot13(c)).collect();
    std::io::stdout().write_all(&output).unwrap();
}
