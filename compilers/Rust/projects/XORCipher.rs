//! XORCipher.rs
//!
//! XORs the input with a repeating keyword; running it twice with the same
//! keyword restores the input.  Rust version of documentation/projects/XORCipher.asm.
//!
//! CPUSim64 has no command line and no files: the keyword is the first line of
//! standard input, the data is the rest of standard input and the result goes
//! to standard output.

use std::io::{Read, Write};

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("XORCipher")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

fn main() {
    let argv = args();
    if argv.len() < 2 {
        println!("Syntax: XORCipher keyword   (the data is read from standard input)");
        return;
    }
    let key = argv[1].as_bytes();
    let mut input = Vec::new();
    std::io::stdin().read_to_end(&mut input).unwrap();
    let output: Vec<u8> = input.iter().enumerate().map(|(i, &c)| c ^ key[i % key.len()]).collect();
    std::io::stdout().write_all(&output).unwrap();
}
