use std::fmt::Write;
struct P { x: i32 }
impl std::fmt::Display for P {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "P({})", self.x)?;
        writeln!(f)?;
        writeln!(f, "{:>3}", self.x)
    }
}
fn main() {
    let mut s = String::new();
    write!(s, "{}-{}", 1, 2).unwrap();
    writeln!(&mut s, "end").unwrap();
    let v: Vec<i32> = vec![];
    let w = vec![1, 2, 3];
    let z = vec![0u8; 16];
    let t = vec![1, 2, 3,];
    let nested = vec![vec![1], vec![2, 3]];
    let b = matches!(w.len(), 3);
    let c = matches!(Some(1), Some(n) if n > 0);
    let d = matches!('x', 'a'..='z' | 'A'..='Z');
    let l = line!();
    let col = column!();
    let f = file!();
    let st = stringify!(a + b, c);
    let st2 = stringify!(Vec<u8>);
    let cc = concat!("a", 1, 2.5, 'c', true, "z");
    let n = concat!("x", -3, line!());
    println!(concat!("fmt ", "{}"), 1);
    println!("{}", stringify!(x));
}
