#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
enum Color { Red, Green, Blue }

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
enum Shape<T> {
    Empty,
    Circle(T),
    Rect { w: T, h: T },
    Tri(T, T, T),
}

#[derive(Debug, Default, Clone, Copy, PartialEq)]
enum Mode { Fast, #[default] Normal, Slow }

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord)]
enum One { Only(i32) }

fn main() {}
