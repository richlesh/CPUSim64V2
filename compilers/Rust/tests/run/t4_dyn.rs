// trait objects: Box<dyn Trait>, &dyn Trait, Vec of boxes, dyn with generics
trait Shape {
    fn area(&self) -> f64;
    fn name(&self) -> String;
    fn scale(&mut self, k: f64);
}

struct Circle {
    r: f64,
}
struct Square {
    s: f64,
}
struct Rect {
    w: f64,
    h: f64,
}

impl Shape for Circle {
    fn area(&self) -> f64 {
        3.0 * self.r * self.r
    }
    fn name(&self) -> String {
        "circle".to_string()
    }
    fn scale(&mut self, k: f64) {
        self.r *= k;
    }
}

impl Shape for Square {
    fn area(&self) -> f64 {
        self.s * self.s
    }
    fn name(&self) -> String {
        "square".to_string()
    }
    fn scale(&mut self, k: f64) {
        self.s *= k;
    }
}

impl Shape for Rect {
    fn area(&self) -> f64 {
        self.w * self.h
    }
    fn name(&self) -> String {
        String::from("rect")
    }
    fn scale(&mut self, k: f64) {
        self.w *= k;
        self.h *= k;
    }
}

fn print_area(s: &dyn Shape) {
    println!("{} {}", s.name(), s.area());
}

fn grow(s: &mut dyn Shape) {
    s.scale(2.0);
}

fn make(kind: &str, x: f64) -> Option<Box<dyn Shape>> {
    let b: Box<dyn Shape> = match kind {
        "circle" => Box::new(Circle { r: x }),
        "square" => Box::new(Square { s: x }),
        "rect" => Box::new(Rect { w: x, h: x + 1.0 }),
        _ => return None,
    };
    Some(b)
}

trait Visitor {
    fn visit(&mut self, n: i32);
}

struct SumV(i32);
struct MaxV(i32);

impl Visitor for SumV {
    fn visit(&mut self, n: i32) {
        self.0 += n;
    }
}

impl Visitor for MaxV {
    fn visit(&mut self, n: i32) {
        if n > self.0 {
            self.0 = n;
        }
    }
}

fn walk(data: &[i32], v: &mut dyn Visitor) {
    for &x in data {
        v.visit(x);
    }
}

fn main() {
    let mut shapes: Vec<Box<dyn Shape>> = Vec::new();
    for (k, x) in [("circle", 1.0), ("square", 2.0), ("rect", 3.0), ("hex", 1.0)] {
        match make(k, x) {
            Some(s) => shapes.push(s),
            None => println!("unknown shape {}", k),
        }
    }
    for s in shapes.iter() {
        print_area(&**s);
    }
    for s in shapes.iter_mut() {
        grow(&mut **s);
    }
    let mut total = 0.0;
    for s in &shapes {
        total += s.area();
    }
    println!("total {}", total);
    let biggest = shapes
        .iter()
        .max_by(|a, b| a.area().partial_cmp(&b.area()).unwrap())
        .unwrap();
    println!("biggest {}", biggest.name());
    let c = Circle { r: 2.0 };
    print_area(&c);

    let data = [4, -2, 9, 3];
    let mut s = SumV(0);
    let mut m = MaxV(i32::MIN);
    walk(&data, &mut s);
    walk(&data, &mut m);
    println!("{} {}", s.0, m.0);
    let mut visitors: Vec<Box<dyn Visitor>> = vec![Box::new(SumV(100)), Box::new(MaxV(0))];
    for v in visitors.iter_mut() {
        v.visit(5);
    }
    println!("{}", visitors.len());
}
