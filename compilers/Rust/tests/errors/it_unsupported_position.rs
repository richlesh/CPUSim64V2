use std::fmt::Display;

fn main() {
    let x: impl Display = 5;
    println!("{}", x);
}
