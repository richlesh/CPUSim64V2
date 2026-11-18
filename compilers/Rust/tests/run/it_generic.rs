use std::fmt::Display;
fn tagger<T: Display + Clone>(t: T) -> impl Fn(i32) -> String {
    move |x| format!("{}#{}", t, x)
}
fn wrap<U: Display + Clone>(u: U) -> impl Fn(i32) -> String {
    let f = tagger(u);
    move |x| format!("({})", f(x))
}
fn id_fn(f: impl Fn(i32) -> i32) -> impl Fn(i32) -> i32 { f }
fn scaled<T: Copy + std::ops::Mul<Output = T>>(v: Vec<T>, k: T) -> impl Iterator<Item = T> {
    v.into_iter().map(move |x| x * k)
}
fn recur(n: u32) -> impl Display {
    if n == 0 { return 0u32; }
    n
}
fn main() {
    println!("{}", tagger(1)(2));
    println!("{}", tagger("a")(3));
    println!("{}", wrap(2.5)(4));
    println!("{}", wrap('c')(5));
    let k = 7;
    println!("{}", id_fn(move |x| x + k)(1));
    let a: Vec<i32> = scaled(vec![1, 2], 3).collect();
    let b: Vec<f64> = scaled(vec![1.5], 2.0).collect();
    println!("{:?} {:?}", a, b);
    println!("{}", recur(3));
}
