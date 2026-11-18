//! hashcode2.rs
//!
//! Like hashcode, but reads the data in blocks of 8 bytes and prints the bytes
//! of the last, partial block as it hashes them.  Rust version of
//! documentation/projects/hashcode2.asm.
//!
//! CPUSim64 has no command line and no files: the first line of standard input
//! is the file name that is printed after the hash, the data is the rest of
//! standard input.

use std::io::Read;

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("hashcode2")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

fn hashcode(data: &[u8]) -> u64 {
    let full = data.len() / 8 * 8;
    let mut hash = 0u64;
    for &b in &data[..full] {
        hash = hash.wrapping_mul(31).wrapping_add(b as u64);
    }
    for &b in &data[full..] {
        println!("tail byte: {:02x}", b);
        hash = hash.wrapping_mul(31).wrapping_add(b as u64);
    }
    hash
}

fn main() {
    let argv = args();
    let mut data = Vec::new();
    std::io::stdin().read_to_end(&mut data).unwrap();
    let hash = hashcode(&data);
    if argv.len() < 2 {
        println!("{:016x}", hash);
    } else {
        println!("{:016x} {}", hash, argv[1]);
    }
}
