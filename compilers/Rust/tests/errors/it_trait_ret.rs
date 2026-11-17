use std::fmt::Display;

trait Shape {
    fn name(&self) -> impl Display;
}

fn main() {}
