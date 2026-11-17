//! Base64.rs
//!
//! Base64 encoder and decoder.  The encoder writes lines of 76 characters (57
//! input bytes); the decoder ignores characters that are not in the alphabet
//! and stops at the padding.  Rust version of documentation/projects/Base64.asm.
//!
//! CPUSim64 has no command line and no files: the mode (e or d) is the first
//! line of standard input, the data is the rest of standard input and the
//! result goes to standard output.

use std::io::{Read, Write};

const PAD_CHAR: u8 = b'=';
const ALPHABET: &[u8] = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("Base64")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

/// the value of a Base64 character: 0-63, 64 for the padding, None for anything else
fn decode_value(c: u8) -> Option<u32> {
    match c {
        b'A'..=b'Z' => Some((c - b'A') as u32),
        b'a'..=b'z' => Some((c - b'a') as u32 + 26),
        b'0'..=b'9' => Some((c - b'0') as u32 + 52),
        b'+' => Some(62),
        b'/' => Some(63),
        PAD_CHAR => Some(64),
        _ => None,
    }
}

fn encode(input: &[u8]) -> Vec<u8> {
    let mut out = Vec::new();
    for line in input.chunks(57) {
        for group in line.chunks(3) {
            let b0 = group[0] as usize;
            let b1 = if group.len() > 1 { group[1] as usize } else { 0 };
            let b2 = if group.len() > 2 { group[2] as usize } else { 0 };
            out.push(ALPHABET[b0 >> 2]);
            out.push(ALPHABET[((b0 & 0x3) << 4) | (b1 >> 4)]);
            if group.len() > 1 {
                out.push(ALPHABET[((b1 & 0xF) << 2) | (b2 >> 6)]);
            } else {
                out.push(PAD_CHAR);
            }
            if group.len() > 2 {
                out.push(ALPHABET[b2 & 0x3F]);
            } else {
                out.push(PAD_CHAR);
            }
        }
        out.push(b'\n');
    }
    out
}

fn decode(input: &[u8]) -> Vec<u8> {
    let mut out = Vec::new();
    let mut buffer: u32 = 0;
    let mut bits = 0;
    for &c in input {
        match decode_value(c) {
            Some(64) => break, // the padding ends the data
            None => continue,  // a newline or another character
            Some(v) => {
                buffer = ((buffer << 6) | v) & 0xFFFF;
                bits += 6;
                if bits >= 8 {
                    bits -= 8;
                    out.push(((buffer >> bits) & 0xFF) as u8);
                }
            }
        }
    }
    out
}

fn main() {
    let argv = args();
    if argv.len() < 2 {
        println!("Syntax: Base64 <e|d>   (the data is read from standard input)");
        std::process::exit(1);
    }
    let mode = argv[1].chars().next().unwrap().to_ascii_lowercase();
    if mode != 'e' && mode != 'd' {
        println!("Mode can only be E or D!");
        std::process::exit(1);
    }
    let mut input = Vec::new();
    std::io::stdin().read_to_end(&mut input).unwrap();
    let output = if mode == 'e' { encode(&input) } else { decode(&input) };
    std::io::stdout().write_all(&output).unwrap();
}
