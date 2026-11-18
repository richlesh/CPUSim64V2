use std::fmt::Display;

fn total(it: impl Iterator<Item = i32>) -> i32 {
    let mut s = 0;
    for x in it {
        s += x;
    }
    s
}

fn show(x: impl Display) -> String {
    format!("<{}>", x)
}

fn apply(f: impl Fn(i32) -> i32, x: i32) -> i32 {
    f(x) + f(x + 1)
}

fn both(a: impl Display, b: impl Display) {
    println!("{} and {}", a, b);
}

fn count(v: &[i32], pred: impl Fn(&i32) -> bool) -> usize {
    let mut n = 0;
    for x in v {
        if pred(x) {
            n += 1;
        }
    }
    n
}

fn main() {
    println!("{}", total(vec![1, 2, 3].into_iter()));
    println!("{}", total((1..5).map(|x| x * 2)));
    println!("{}", show(5));
    println!("{}", show("hi"));
    println!("{}", show(2.5f64));
    let k = 10;
    println!("{}", apply(|x| x * k, 3));
    println!("{}", apply(|x| x + 1, 3));
    both(1, "two");
    println!("{}", count(&[1, 2, 3, 4, 5, 6], |x| *x % 2 == 0));
}
