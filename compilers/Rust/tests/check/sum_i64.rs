struct Point { x: i64, y: i64 }
impl Point { fn len2(&self) -> i64 { self.x * self.x + self.y * self.y } }
fn main() {
    let p = Point { x: 3, y: 4 };
    let v = vec![1, 2, 3];
    let s: i64 = v.iter().map(|a| a * 2).sum();
    let name = String::from("hi");
    println!("{} {} {}", p.len2(), s, name.len());
}
fn more() -> f64 {
    let f = vec![1.5, 2.5];
    let b: f64 = f.iter().map(|x| x * 2.0).sum();
    let w = vec![1, 2, 3];
    let d: i32 = w.iter().map(|&x| x + 1).product();
    let c: i64 = vec![1, 2].iter().sum();
    b + d as f64 + c as f64
}
