use std::fmt;
struct P { x: i32, y: i32 }
impl Clone for P { fn clone(&self) -> P { P { x: self.x, y: self.y } } }
impl PartialEq for P { fn eq(&self, o: &P) -> bool { self.x == o.x && self.y == o.y } }
impl fmt::Debug for P {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result { write!(f, "P({}, {})", self.x, self.y) }
}
impl fmt::Display for P {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result { write!(f, "({}, {})", self.x, self.y) }
}
impl std::ops::Add for P {
    type Output = P;
    fn add(self, o: P) -> P { P { x: self.x + o.x, y: self.y + o.y } }
}
fn parse(s: &str) -> Result<i32, std::num::ParseIntError> {
    let n: i32 = s.trim().parse()?;
    Ok(n * 2)
}
fn first(v: &[i32]) -> Option<i32> {
    let x = v.get(0)?;
    Some(*x + 1)
}
fn main() {
    let a = P { x: 1, y: 2 };
    let b = a.clone() + P { x: 3, y: 4 };
    println!("{} {}", a, b);
    println!("{}", a == b);
    match parse("21") { Ok(n) => println!("{}", n), Err(e) => println!("err {:?}", e) }
    println!("{:?}", first(&[1, 2]));
    let mut i = 0;
    while i < 3 { i += 1; }
    let r = loop { i += 1; if i > 10 { break i; } };
    println!("{}", r);
    let t = (1, "a", 2.5);
    let (x, y, z) = t;
    println!("{} {} {}", x, y, z);
    let arr = [1, 2, 3];
    let sl = &arr[1..];
    println!("{}", sl.len() + arr.len());
}
