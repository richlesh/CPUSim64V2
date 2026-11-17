enum Shape { Circle(f64), Rect { w: f64, h: f64 }, Tri(f64, f64, f64) }
impl Shape {
    fn area(&self) -> f64 {
        match self {
            Shape::Circle(r) => 3.14159 * r * r,
            Shape::Rect { w, h } => w * h,
            Shape::Tri(a, b, c) => {
                let s = (a + b + c) / 2.0;
                (s * (s - a) * (s - b) * (s - c)).sqrt()
            }
        }
    }
    fn name(&self) -> &str {
        match *self { Shape::Circle(_) => "circle", Shape::Rect { .. } => "rect", Shape::Tri(..) => "tri" }
    }
    fn scale(&mut self, k: f64) {
        match self {
            Shape::Circle(r) => *r *= k,
            Shape::Rect { w, h } => { *w *= k; *h *= k; }
            Shape::Tri(a, b, c) => { *a *= k; *b *= k; *c *= k; }
        }
    }
}
fn main() {
    let mut shapes = vec![Shape::Circle(1.0), Shape::Rect { w: 2.0, h: 3.0 }, Shape::Tri(3.0, 4.0, 5.0)];
    for s in shapes.iter_mut() { s.scale(2.0); }
    for s in &shapes { println!("{} {:.2}", s.name(), s.area()); }
    let biggest = shapes.iter().map(|s| s.area()).fold(0.0, |a, b| if b > a { b } else { a });
    println!("{}", biggest);
}
