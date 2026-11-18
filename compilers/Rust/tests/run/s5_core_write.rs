use std::fmt::Write;
use std::fmt;
struct P(i32);
impl fmt::Display for P {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result { write!(f, "P<{}>", self.0) }
}
fn add(r: &mut String) { writeln!(r, "via ref {}", 2).unwrap(); }
fn main() {
    let mut s = String::new();
    write!(s, "a{}", 1).unwrap();
    add(&mut s);
    write!(&mut s, "{}", P(7)).unwrap();
    print!("{}", s);
    println!();
}
