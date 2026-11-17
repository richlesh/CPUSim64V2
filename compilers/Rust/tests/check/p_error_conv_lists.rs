use std::fmt;
#[derive(Debug)]
enum MyErr { Parse(std::num::ParseIntError), Neg(i32) }
impl From<std::num::ParseIntError> for MyErr { fn from(e: std::num::ParseIntError) -> MyErr { MyErr::Parse(e) } }
impl fmt::Display for MyErr {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self { MyErr::Parse(e) => write!(f, "parse: {:?}", e), MyErr::Neg(n) => write!(f, "neg {}", n) }
    }
}
fn parse(s: &str) -> Result<i32, MyErr> {
    let n = s.parse::<i32>()?;
    if n < 0 { Err(MyErr::Neg(n)) } else { Ok(n) }
}
enum Level { Low = 1, High = 10 }
struct Node { val: i32, next: Option<Box<Node>> }
struct LinkedList { head: Option<Box<Node>>, len: usize }
impl LinkedList {
    fn new() -> Self { LinkedList { head: None, len: 0 } }
    fn push(&mut self, v: i32) {
        let old = self.head.take();
        self.head = Some(Box::new(Node { val: v, next: old }));
        self.len += 1;
    }
    fn pop(&mut self) -> Option<i32> {
        self.head.take().map(|n| { self.head = n.next; self.len -= 1; n.val })
    }
    fn peek(&self) -> Option<&i32> { self.head.as_ref().map(|n| &n.val) }
    fn sum(&self) -> i32 {
        let mut cur = &self.head;
        let mut t = 0;
        while let Some(n) = cur { t += n.val; cur = &n.next; }
        t
    }
}
trait Named { fn name(&self) -> String; }
trait Greeter: Named { fn greet(&self) -> String { format!("hi {}", self.name()) } }
struct Bob;
impl Named for Bob { fn name(&self) -> String { "bob".to_string() } }
impl Greeter for Bob {}
trait Conv<T> { fn conv(&self) -> T; }
impl Conv<i64> for i32 { fn conv(&self) -> i64 { *self as i64 } }
impl Conv<String> for i32 { fn conv(&self) -> String { format!("{}", self) } }
struct Cache<F: Fn(u32) -> u32> { f: F, last: Option<u32> }
impl<F: Fn(u32) -> u32> Cache<F> {
    fn new(f: F) -> Cache<F> { Cache { f, last: None } }
    fn get(&mut self, x: u32) -> u32 { let r = (self.f)(x); self.last = Some(r); r }
}
fn main() {
    match parse("12") { Ok(n) => println!("{}", n), Err(e) => println!("{}", e) }
    println!("{} {}", Level::High as i32, Level::Low as u8);
    let mut l = LinkedList::new();
    l.push(1); l.push(2);
    println!("{:?} {} {:?}", l.peek(), l.sum(), l.pop());
    println!("{}", Bob.greet());
    let a: i64 = 5.conv();
    let b: String = 5.conv();
    println!("{} {}", a, b);
    let mut c = Cache::new(|x| x * x);
    println!("{}", c.get(4));
    let mut v = Vec::new();
    v.push(1u8);
    let f = |a, b| a + b;
    println!("{} {}", f(1, 2), v.len());
    let xs = vec![1, 2, 3];
    let sq: Vec<i32> = xs.iter().map(|x| x * x).collect();
    let tot: i32 = xs.iter().sum();
    let (mut p, mut q) = (1, 2);
    p += q; q += p;
    println!("{:?} {} {} {}", sq, tot, p, q);
    let ch = 'c';
    let idx = (ch as u8 - b'a') as usize;
    let nb = b'a' + 1;
    let w = 250u8.wrapping_add(10);
    println!("{} {} {}", idx, nb, w);
    let s = String::from("hello");
    match s.as_str() { "hello" => println!("hi"), "bye" => println!("bye"), _ => println!("?") }
    let x = 5; let x = x as f64 * 1.5; let x = format!("{}", x);
    println!("{}", x);
    let r = &xs[1..];
    let first: &i32 = &r[0];
    println!("{} {}", first, &s[1..3]);
}
