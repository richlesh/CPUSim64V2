//! holy_grail.rs
//!
//! The Bridge of Death scene: asks three questions and answers with ANSI colors.
//! Rust version of documentation/projects/holy_grail.asm.

use std::io::Write;

const ANSI_RED: &str = "\x1b[31m";
const ANSI_BLINK: &str = "\x1b[5m";
const ANSI_RESET_BLINK: &str = "\x1b[25m";
const ANSI_RESET_COLOR: &str = "\x1b[39m";

/// prints the prompt and reads a line without its newline
fn ask(prompt: &str) -> String {
    print!("{}", prompt);
    std::io::stdout().flush().unwrap();
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    match line.strip_suffix('\n') {
        Some(rest) => rest.to_string(),
        None => line,
    }
}

fn main() {
    println!("STOP! Who would cross the Bridge of Death must answer me\nthese questions three, ere the other side he see.");
    println!();
    let _name = ask("BRIDGEKEEPER: What... is your name? ");
    let _quest = ask("BRIDGEKEEPER: What... is your quest? ");
    let speed = ask("BRIDGEKEEPER: What... is the air-speed velocity of an unladen swallow? ").to_lowercase();
    println!();
    let african = speed.contains("african");
    let european = speed.contains("european");
    print!("{}{}", ANSI_RED, ANSI_BLINK);
    if african && european {
        print!("BRIDGEKEEPER: Huh? I-- I don't know that... AAAAARGH!\n\n");
    } else {
        print!("BRIDGEKEEPER: You are cast into the Gorge of Eternal Peril!\n\n");
    }
    print!("{}{}", ANSI_RESET_BLINK, ANSI_RESET_COLOR);
}
