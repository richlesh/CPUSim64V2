trait Shape {
    fn area(&self) -> f64;
}
struct Circle {
    r: f64,
}
struct Sq {
    s: f64,
}
impl Shape for Circle {
    fn area(&self) -> f64 {
        3.0 * self.r * self.r
    }
}
impl Shape for Sq {
    fn area(&self) -> f64 {
        self.s * self.s
    }
}
fn f(k: i32) -> Option<Box<dyn Shape>> {
    if k == 0 {
        None
    } else if k == 1 {
        Some(Box::new(Circle { r: 2.0 }))
    } else {
        Some(Box::new(Sq { s: 3.0 }))
    }
}
fn g(b: Box<dyn Shape>) -> f64 {
    b.area()
}
fn h() -> Result<Box<dyn Shape>, String> {
    Ok(Box::new(Sq { s: 1.5 }))
}
fn main() {
    println!("{}", f(1).unwrap().area());
    println!("{}", f(2).unwrap().area());
    println!("{}", f(0).is_none());
    println!("{}", g(Box::new(Circle { r: 1.0 })));
    println!("{}", h().unwrap().area());
    let v: Vec<Box<dyn Shape>> = vec![Box::new(Circle { r: 1.0 }), Box::new(Sq { s: 2.0 })];
    let t: f64 = v.iter().map(|s| s.area()).sum();
    println!("{}", t);
    let o: Option<&dyn Shape> = Some(&Sq { s: 4.0 });
    println!("{}", o.unwrap().area());
    let p: (Box<dyn Shape>, i32) = (Box::new(Sq { s: 1.0 }), 3);
    println!("{}", p.0.area() + p.1 as f64);
    let a: [Box<dyn Shape>; 2] = [Box::new(Sq { s: 2.0 }), Box::new(Circle { r: 1.0 })];
    println!("{}", a[0].area() + a[1].area());
}
