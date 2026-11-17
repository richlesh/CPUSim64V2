struct Point { x: i32, y: i32 }

impl Point {
    fn new(x: i32, y: i32) -> Point { Point { x, y } }
    fn dist2(&self) -> i32 { self.x * self.x + self.y * self.y }
    fn shift(&mut self, dx: i32) { self.x += dx; }
}

fn add(a: i32, b: i32) -> i32 { a + b }

fn main() {
    let mut p = Point::new(3, 4);
    p.shift(2);
    let d = p.dist2();
    let s = add(d, 1);
    println!("{} {}", s, p.x);
    let v = vec![1, 2, 3];
    let mut total = 0;
    for x in v.iter() {
        total += *x;
    }
    println!("{}", total);
}
