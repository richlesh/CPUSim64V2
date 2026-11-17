trait Shape {
    fn area(&self) -> f64;
    fn name(&self) -> String { String::from("shape") }
}
struct Circle { r: f64 }
struct Rect { w: f64, h: f64 }
impl Shape for Circle {
    fn area(&self) -> f64 { 3.14 * self.r * self.r }
}
impl Shape for Rect {
    fn area(&self) -> f64 { self.w * self.h }
    fn name(&self) -> String { "rect".to_string() }
}
fn total_area(shapes: &Vec<Box<dyn Shape>>) -> f64 {
    let mut t = 0.0;
    for s in shapes.iter() { t += s.area(); }
    t
}
fn largest<T: PartialOrd + Copy>(xs: &[T]) -> T {
    let mut m = xs[0];
    for &x in xs { if x > m { m = x; } }
    m
}
fn main() {
    let mut shapes: Vec<Box<dyn Shape>> = Vec::new();
    shapes.push(Box::new(Circle { r: 1.0 }));
    shapes.push(Box::new(Rect { w: 2.0, h: 3.0 }));
    println!("{}", total_area(&shapes));
    println!("{}", largest(&[1, 5, 3]));
    println!("{}", largest(&[1.5, 0.5]));
    for s in &shapes { println!("{}", s.name()); }
}
