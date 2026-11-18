//! Caesar.rs
//!
//! Caesar cipher: shifts the 95 printable ASCII characters (' ' to '~') by a key,
//! wrapping around; other characters pass through unchanged.
//! Rust version of documentation/projects/Caesar.asm.
//!
//! CPUSim64 has no command line and no files: the key is the first line of
//! standard input, the text to convert is the rest of standard input and the
//! result goes to standard output.
//!
//! Usage:
//!     key in [1, 94] encrypts, key in [-94, -1] decrypts, 0 does nothing

use std::io::{Read, Write};

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("Caesar")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

fn caesar(key: i64, input: &[u8]) -> Vec<u8> {
    input
        .iter()
        .map(|&c| {
            if c >= b' ' && c <= b'~' {
                let mut v = (c - b' ') as i64 + key;
                while v < 0 {
                    v += 95;
                }
                while v >= 95 {
                    v -= 95;
                }
                (v + b' ' as i64) as u8
            } else {
                c
            }
        })
        .collect()
}

fn main() {
    let argv = args();
    if argv.len() < 2 {
        println!("Syntax: Caesar <key>   (the text is read from standard input)");
        println!("    key can be in the range [1,94] to encrypt.");
        println!("    key can be in the range [-94,-1] to decrypt.");
        println!("    key == 0 is a no-op");
        std::process::exit(1);
    }
    let key: i64 = argv[1].parse().unwrap_or(0);
    let mut input = Vec::new();
    std::io::stdin().read_to_end(&mut input).unwrap();
    let output = caesar(key % 95, &input);
    std::io::stdout().write_all(&output).unwrap();
}
