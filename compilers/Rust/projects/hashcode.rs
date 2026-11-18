//! hashcode.rs
//!
//! Computes a Java-style hash code (hash = hash * 31 + byte, 64 bits) of the
//! data.  Rust version of documentation/projects/hashcode.asm.
//!
//! CPUSim64 has no command line and no files: the first line of standard input
//! is the file name that is printed after the hash, the data is the rest of
//! standard input.

use std::io::Read;

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("hashcode")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

fn hashcode(data: &[u8]) -> u64 {
    data.iter().fold(0u64, |hash, &c| hash.wrapping_mul(31).wrapping_add(c as u64))
}

fn main() {
    let argv = args();
    let mut data = Vec::new();
    std::io::stdin().read_to_end(&mut data).unwrap();
    if argv.len() < 2 {
        println!("{:016x}", hashcode(&data));
    } else {
        println!("{:016x} {}", hashcode(&data), argv[1]);
    }
}
