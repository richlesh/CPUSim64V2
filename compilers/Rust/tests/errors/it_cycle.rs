use std::fmt::Display;

fn a(n: u32) -> impl Display {
    b(n)
}

fn b(n: u32) -> impl Display {
    a(n)
}

fn main() {
    println!("{}", a(1));
}
