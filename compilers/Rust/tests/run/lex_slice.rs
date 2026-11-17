#!/usr/bin/env run-rust
//! crate doc
/// doc comment before main is skipped
fn main() {
    /** block doc */
    println!("héllo wörld 😀 日本"); // unicode text survives
    println!(r#"raw "quoted" \n kept"#);
}
