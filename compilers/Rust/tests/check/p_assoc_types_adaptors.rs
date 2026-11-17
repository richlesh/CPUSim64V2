use std::fmt::{self, Debug, Display};
struct Wrapper<T> { v: T }
impl<T: Display> Wrapper<T> { fn show(&self) -> String { format!("[{}]", self.v) } }
impl<T: Debug> Debug for Wrapper<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result { write!(f, "W({:?})", self.v) }
}
trait Container {
    type Item;
    fn get(&self, i: usize) -> Option<&Self::Item>;
    fn first(&self) -> Option<&Self::Item> { self.get(0) }
    fn size(&self) -> usize;
}
struct MyVec { d: Vec<i32> }
impl Container for MyVec {
    type Item = i32;
    fn get(&self, i: usize) -> Option<&i32> { self.d.get(i) }
    fn size(&self) -> usize { self.d.len() }
}
fn print_all<C: Container>(c: &C) where C::Item: Debug {
    for i in 0..c.size() { println!("{:?}", c.get(i).unwrap()); }
}
fn compose<A, B, C, F: Fn(A) -> B, G: Fn(B) -> C>(f: F, g: G) -> Box<dyn Fn(A) -> C> where F: 'static, G: 'static, A: 'static { Box::new(move |x| g(f(x))) }
fn main() {
    let w = Wrapper { v: 3 };
    println!("{} {:?}", w.show(), w);
    let mv = MyVec { d: vec![1, 2] };
    print_all(&mv);
    println!("{:?}", mv.first());
    let v = vec![1, 2, 3, 4, 5, 6];
    let w2: Vec<i32> = v.windows(2).map(|p| p[0] + p[1]).collect();
    let ch: Vec<Vec<i32>> = v.chunks(2).map(|c| c.to_vec()).collect();
    let z: Vec<(i32, char)> = v.iter().cloned().zip("abc".chars()).collect();
    let any = v.iter().any(|&x| x > 5);
    let all = v.iter().all(|x| *x > 0);
    let m = v.iter().min_by_key(|x| (**x - 3).abs());
    let tw: Vec<&i32> = v.iter().take_while(|x| **x < 3).collect();
    let ch2: Vec<i32> = v.iter().copied().chain(vec![9, 9]).skip(1).take(4).collect();
    let fs = [1.5, 2.5];
    let fsum: f64 = fs.iter().sum();
    let prod: i32 = v.iter().product();
    println!("{:?} {} {} {:?} {} {:?} {} {}", w2, ch.len(), z.len(), m, tw.len(), ch2, fsum, prod);
    println!("{:?} {:?} {}", any, all, v.iter().rev().last().unwrap());
    let ow: Option<i32> = Some(2);
    let r = ow.map(|x| x + 1).and_then(|x| if x > 2 { Some(x * 2) } else { None }).unwrap_or_else(|| 0);
    let rr: Result<i32, String> = ow.ok_or("none".to_string());
    println!("{} {:?}", r, rr);
    let adder = compose(|x: i32| x + 1, |y: i32| y * 2);
    println!("{}", adder(3));
}
