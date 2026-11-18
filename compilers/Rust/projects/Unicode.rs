//! Unicode.rs
//!
//! Prints ranges of Unicode characters, 32 per line: ASCII, Latin, math
//! symbols, box drawing, symbols and icons (emoji).  The console writes them
//! as UTF-8.  Rust version of documentation/projects/Unicode.asm.

fn print_range(first: u32, last: u32) {
    let mut out = String::new();
    for c in first..=last {
        out.push(char::from_u32(c).unwrap());
        if c & 31 == 31 {
            out.push('\n');
        }
    }
    print!("{}", out);
}

fn main() {
    println!("ASCII");
    print_range(0x20, 0x7f);
    println!("Basic Multilingual");
    print_range(0xa0, 0x1ff);
    println!("Math Symbols");
    print_range(0x2200, 0x22ff);
    println!("Drawing");
    print_range(0x2500, 0x25ff);
    println!("Symbols");
    print_range(0x2600, 0x27ff);
    println!("Icons");
    print_range(0x1f300, 0x1f4ff);
}
