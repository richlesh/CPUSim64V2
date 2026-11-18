// structs: named, tuple, unit; methods; update syntax; nested
struct Point {
    x: i32,
    y: i32,
}

struct Rect {
    top_left: Point,
    w: u32,
    h: u32,
}

struct Meters(f64);
struct Marker;

impl Point {
    fn new(x: i32, y: i32) -> Point {
        Point { x, y }
    }
    fn origin() -> Self {
        Point { x: 0, y: 0 }
    }
    fn manhattan(&self) -> i32 {
        self.x.abs() + self.y.abs()
    }
    fn shift(&mut self, dx: i32, dy: i32) {
        self.x += dx;
        self.y += dy;
    }
    fn into_pair(self) -> (i32, i32) {
        (self.x, self.y)
    }
}

impl Rect {
    fn area(&self) -> u32 {
        self.w * self.h
    }
    fn contains(&self, p: &Point) -> bool {
        p.x >= self.top_left.x
            && p.y >= self.top_left.y
            && p.x < self.top_left.x + self.w as i32
            && p.y < self.top_left.y + self.h as i32
    }
}

impl Marker {
    fn id(&self) -> u8 {
        7
    }
}

fn main() {
    let mut p = Point::new(3, -4);
    println!("{} {}", p.x, p.y);
    println!("{}", p.manhattan());
    p.shift(2, 2);
    println!("{} {}", p.x, p.y);
    let o = Point::origin();
    println!("{}", o.manhattan());
    let r = Rect { top_left: Point { x: 1, y: 1 }, w: 4, h: 3 };
    println!("{}", r.area());
    println!("{} {}", r.contains(&p), r.contains(&Point::new(2, 2)));
    let q = Point { x: 100, ..Point::new(1, 2) };
    println!("{} {}", q.x, q.y);
    let m = Meters(2.5);
    println!("{}", m.0 * 2.0);
    println!("{}", Marker.id());
    let Point { x, y } = Point::new(8, 9);
    println!("{} {}", x, y);
    println!("{:?}", q.into_pair());
}
