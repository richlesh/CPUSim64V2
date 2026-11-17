use std::fmt::Display;

fn base(n: u32) -> impl Iterator<Item = u32> {
    0..n
}

fn middle(n: u32) -> impl Iterator<Item = u32> {
    base(n).map(|x| x + 1)
}

fn top(n: u32) -> impl Iterator<Item = u32> {
    middle(n).filter(|x| x % 2 == 1)
}

fn adder(n: i32) -> impl Fn(i32) -> i32 {
    move |x| x + n
}

fn twice(n: i32) -> impl Fn(i32) -> i32 {
    let a = adder(n);
    move |x| a(a(x))
}

fn shown(n: u32) -> impl Display {
    top(n).sum::<u32>()
}

fn main() {
    let v: Vec<u32> = top(9).collect();
    println!("{:?}", v);
    println!("{}", twice(10)(1));
    println!("{}", shown(10));
}
