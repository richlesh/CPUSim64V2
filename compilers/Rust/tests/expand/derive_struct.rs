#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Default)]
struct Point { x: i32, y: i32 }

#[derive(Debug, Clone, PartialEq, PartialOrd, Default)]
struct Pair<T, U>(T, U) where T: Copy;

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Default)]
struct Unit;

#[derive(Clone, Debug)]
#[derive(PartialEq)]
struct Wrapper<'a, T> { name: &'a str, items: Vec<T> }

fn main() {}
