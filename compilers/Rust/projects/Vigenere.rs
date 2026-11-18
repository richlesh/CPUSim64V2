//! Vigenere.rs
//!
//! Vigenere cipher over the 94 printable ASCII characters ('!' to '~'): each
//! such character is shifted by the next character of the keyword.  Spaces and
//! other characters pass through unchanged and do not use up a key character.
//! Rust version of documentation/projects/Vigenere.asm.
//!
//! CPUSim64 has no command line and no files: the keyword is the first line of
//! standard input (a keyword starting with '-' decrypts), the text to convert
//! is the rest of standard input and the result goes to standard output.

use std::io::{Read, Write};

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("Vigenere")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

fn vigenere(keyword: &str, input: &[u8]) -> Vec<u8> {
    let (decrypt, key) = match keyword.strip_prefix('-') {
        Some(rest) => (true, rest.as_bytes()),
        None => (false, keyword.as_bytes()),
    };
    let mut out = Vec::new();
    if key.is_empty() {
        return out;
    }
    let mut index = 0;
    for &c in input {
        if c > b' ' && c <= b'~' {
            let offset = (key[index] - b'!') as i64;
            let mut v = (c - b'!') as i64 + if decrypt { -offset } else { offset };
            while v < 0 {
                v += 94;
            }
            while v >= 94 {
                v -= 94;
            }
            out.push((v + b'!' as i64) as u8);
            index += 1;
            if index >= key.len() {
                index = 0;
            }
        } else {
            out.push(c);
        }
    }
    out
}

fn main() {
    let argv = args();
    if argv.len() < 2 {
        println!("Syntax: Vigenere [-]keyword   (the text is read from standard input)");
        println!("    a keyword starting with '-' decrypts");
        std::process::exit(1);
    }
    let mut input = Vec::new();
    std::io::stdin().read_to_end(&mut input).unwrap();
    let output = vigenere(&argv[1], &input);
    std::io::stdout().write_all(&output).unwrap();
}
