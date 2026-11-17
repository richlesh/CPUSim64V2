use std::fmt::Display;
trait Sh { fn show(&self, x: impl Display) -> String; }
struct A;
impl Sh for A { fn show(&self, x: impl Display) -> String { format!("A{}", x) } }
fn f(v: Vec<impl Display>) -> usize { v.len() }
fn g() -> Vec<impl Display> { vec![1, 2] }
fn h() -> Option<impl Fn() -> i32> { Some(|| 3) }
fn t(x: &impl Display) -> String { format!("{}", x) }
fn pr() -> (impl Display, impl Display) { (1, "b") }
fn main() {
    println!("{}", A.show(5));
    println!("{}", f(vec![1, 2, 3]));
    for x in g() { println!("{}", x); }
    println!("{}", h().unwrap()());
    println!("{}", t(&9));
    let p = pr();
    println!("{} {}", p.0, p.1);
}
