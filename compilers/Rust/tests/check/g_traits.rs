trait Shape {
    fn area(&self) -> f64;
    fn name(&self) -> String { String::new() }
}
trait Container {
    type Item;
    fn first(&self) -> Option<&Self::Item>;
    fn count(&self) -> usize;
}
struct Circle { r: f64 }
struct Bag { items: Vec<i32> }
impl Shape for Circle { fn area(&self) -> f64 { 3.0 * self.r * self.r } }
impl Container for Bag {
    type Item = i32;
    fn first(&self) -> Option<&i32> { self.items.first() }
    fn count(&self) -> usize { self.items.len() }
}
trait Named { fn id(&self) -> u32; }
impl<T: Shape> Named for T { fn id(&self) -> u32 { 7 } }

fn largest<T: PartialOrd + Copy>(xs: &[T]) -> T {
    let mut m = xs[0];
    for &x in xs { if x > m { m = x; } }
    m
}
fn area_of(s: &dyn Shape) -> f64 { s.area() }
fn first_or<C: Container>(c: &C, d: C::Item) -> C::Item where C::Item: Copy {
    match c.first() { Some(x) => *x, None => d }
}
fn run() -> f64 {
    let c = Circle { r: 2.0 };
    let shapes: Vec<Box<dyn Shape>> = vec![Box::new(Circle { r: 1.0 })];
    let b = Bag { items: vec![4, 5] };
    let k = first_or(&b, 0) + b.count() as i32 + largest(&[1, 5, 3]) + c.id() as i32;
    area_of(&c) + shapes[0].area() + largest(&[1.5, 0.5]) + k as f64
}
