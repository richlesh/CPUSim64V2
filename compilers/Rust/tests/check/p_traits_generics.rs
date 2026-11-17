use std::fmt::Display;
use std::cmp::Ordering;
use std::error::Error;

trait Describe { fn describe(&self) -> String; }
impl<T: Display> Describe for T { fn describe(&self) -> String { format!("<{}>", self) } }

struct Parser<'a> { s: &'a str, pos: usize }
impl<'a> Parser<'a> {
    fn new(s: &'a str) -> Parser<'a> { Parser { s, pos: 0 } }
    fn next_char(&mut self) -> Option<char> {
        let c = self.s[self.pos..].chars().next()?;
        self.pos += c.len_utf8();
        Some(c)
    }
}
struct Btn { f: Box<dyn Fn(i32) -> i32>, name: String }
impl Btn { fn click(&self, x: i32) -> i32 { (self.f)(x) } }
fn double(x: i32) -> i32 { x * 2 }
enum List { Cons(i32, Box<List>), Nil }
use List::{Cons, Nil};
fn sum_list(l: &List) -> i32 { match l { Cons(v, rest) => v + sum_list(rest), Nil => 0 } }
fn parse_sum(a: &str, b: &str) -> Result<i32, Box<dyn Error>> {
    let x: i32 = a.parse()?;
    let y = b.parse::<i32>()?;
    if x < 0 { return Err("negative".into()); }
    Ok(x + y)
}
fn slice_desc(v: &[i32]) -> String {
    match v {
        [] => "empty".to_string(),
        [x] => format!("one {}", x),
        [first, .., last] => format!("{}..{}", first, last),
    }
}
fn main() {
    println!("{} {}", 5.describe(), "x".describe());
    let mut p = Parser::new("héllo");
    while let Some(c) = p.next_char() { print!("{} ", c); }
    let b = Btn { f: Box::new(|x| x + 1), name: "ok".to_string() };
    println!("{} {}", b.click(1), b.name);
    let fp: fn(i32) -> i32 = double;
    println!("{}", fp(4));
    let l = Cons(1, Box::new(Cons(2, Box::new(Nil))));
    println!("{}", sum_list(&l));
    println!("{:?}", parse_sum("1", "2").is_ok());
    println!("{}", slice_desc(&[1, 2, 3]));
    let mut v = vec![3, 1, 2];
    v.sort();
    v.sort_by(|a, b| b.cmp(a));
    v.sort_by_key(|x| -x);
    let o = v.iter().position(|&x| x == 2).map(|i| i + 1).unwrap_or(0);
    let m = v.iter().max().unwrap();
    println!("{} {}", o, m);
    let rev: String = "abc".chars().rev().collect();
    let up: String = rev.chars().map(|c| c.to_ascii_uppercase()).collect();
    println!("{} {}", rev, up);
    let mut a = 1; let mut bb = 2;
    std::mem::swap(&mut a, &mut bb);
    match a.cmp(&bb) { Ordering::Less => println!("lt"), Ordering::Equal => println!("eq"), Ordering::Greater => println!("gt") }
    let s: String = "a".into();
    let n = i64::from(3i32);
    let words = ["x", "y"];
    let joined = words.join("-");
    println!("{} {} {}", s, n, joined);
}
