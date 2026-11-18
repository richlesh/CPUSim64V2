struct W(i32);
struct Meters(f64);
trait Zero { const ZERO: Self; fn is_zero(&self) -> bool; }
impl Zero for i32 { const ZERO: i32 = 0; fn is_zero(&self) -> bool { *self == 0 } }
struct Cfg { a: i32, b: bool, c: String }
impl Default for Cfg { fn default() -> Cfg { Cfg { a: 1, b: false, c: String::new() } } }
struct Acc { total: i64 }
impl Acc {
    const LIMIT: i64 = 100;
    fn new() -> Self { Self { total: 0 } }
    fn add(&mut self, v: i64) -> &mut Self { self.total += v; self }
    fn full(&self) -> bool { self.total >= Self::LIMIT }
}
fn largest_pair<T>(a: T, b: T) -> T where T: PartialOrd { if a > b { a } else { b } }
fn main() {
    let x = 5;
    println!("{}", x.pow(2));
    let y = 2.0_f64.sqrt() + (x as f64).sqrt();
    println!("{} {} {}", y, i32::MAX, u8::MAX);
    println!("{}", std::f64::consts::PI);
    let w = W(5);
    let m = Meters(2.5);
    println!("{} {}", w.0, m.0);
    println!("{} {}", i32::ZERO, 3.is_zero());
    let c = Cfg { a: 5, ..Default::default() };
    let d: Cfg = Default::default();
    println!("{} {} {}", c.a, d.b, c.c);
    let mut acc = Acc::new();
    acc.add(5).add(10);
    println!("{}", acc.full());
    let mut count = 0;
    let mut inc = || count += 1;
    inc(); inc();
    println!("{}", count);
    for i in (1..=5).rev() { print!("{}", i); }
    for i in (0..10).step_by(2) { print!("{}", i); }
    let sq: Vec<i32> = (0..5).map(|n| n * n).collect();
    let tot = (1..=10).sum::<i32>();
    let evens = (0..20).filter(|n| n % 2 == 0).collect::<Vec<_>>();
    println!("{:?} {} {}", sq, tot, evens.len());
    let arr = [1, 2];
    let [a, b] = arr;
    println!("{}", a + b);
    let pair = (3, -3);
    match pair { (x, y) if x + y == 0 => println!("zero"), (x, _) if x % 2 == 1 => println!("odd"), _ => println!("other") }
    let n = 7;
    let kind = match n { 1 | 2 | 3 => "low", 4..=6 => "mid", big @ 7..=100 => { println!("{}", big); "high" } _ => "huge" };
    let o = Some(String::from("s"));
    if let Some(ref s) = o { println!("{}", s); }
    let mut s = String::from("a");
    { let r = &mut s; r.push('b'); }
    let mut v = vec![1, 2, 3];
    { let first = &mut v[0]; *first += 10; }
    for e in v.iter_mut() { *e *= 2; }
    println!("{} {:?} {}", s, v, kind);
    println!("{}", largest_pair(1.5, 2.5));
    let t = vec![(1, "a"), (2, "b")];
    for &(n, s) in &t { println!("{}{}", n, s); }
    for (n, s) in t.iter() { println!("{}{}", n, s); }
}
