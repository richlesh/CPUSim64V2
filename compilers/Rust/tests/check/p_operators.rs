#[derive(Clone, Copy)]
struct V2 { x: f64, y: f64 }
impl std::ops::Add for V2 { type Output = V2; fn add(self, o: V2) -> V2 { V2 { x: self.x + o.x, y: self.y + o.y } } }
impl std::ops::Mul<f64> for V2 { type Output = V2; fn mul(self, k: f64) -> V2 { V2 { x: self.x * k, y: self.y * k } } }
impl std::ops::Neg for V2 { type Output = V2; fn neg(self) -> V2 { V2 { x: -self.x, y: -self.y } } }
impl std::ops::AddAssign for V2 { fn add_assign(&mut self, o: V2) { self.x += o.x; self.y += o.y; } }
impl std::ops::Index<usize> for V2 { type Output = f64; fn index(&self, i: usize) -> &f64 { if i == 0 { &self.x } else { &self.y } } }
fn main() {
    let a = V2 { x: 1.0, y: 2.0 };
    let mut b = a + a * 2.0;
    b += -a;
    println!("{} {} {}", b.x, b.y, b[1]);
    let s1 = String::from("a");
    let s2 = s1 + "b" + &String::from("c");
    println!("{}", s2);
    let mut t = String::new();
    t += "x";
    t.push('y');
    t.push_str("zz");
    println!("{}", t);
    let n: Option<&str> = None;
    println!("{}", n.unwrap_or("dflt"));
    let r: Result<i32, String> = Err("bad".to_string());
    match r { Ok(v) => println!("{}", v), Err(ref e) if e.len() > 5 => println!("long"), Err(e) => println!("{}", e) }
}
