struct Point { x: i32, y: i32 }
struct Wrapper<T> { v: T, n: usize }
struct Pair(i32, f64);
struct Unit;

impl Point {
    fn new(x: i32, y: i32) -> Point { Point { x, y } }
    fn dist2(&self) -> i32 { self.x * self.x + self.y * self.y }
    fn shift(&mut self, dx: i32) -> &mut Point { self.x += dx; self }
    fn origin() -> Self { Self { x: 0, y: 0 } }
}

impl<T> Wrapper<T> {
    fn new(v: T) -> Wrapper<T> { Wrapper { v, n: 1 } }
    fn get(&self) -> &T { &self.v }
    fn into_inner(self) -> T { self.v }
}

impl std::ops::Add for Point {
    type Output = Point;
    fn add(self, o: Point) -> Point { Point { x: self.x + o.x, y: self.y + o.y } }
}

fn total(p: Point, q: Point) -> i32 {
    let mut r = p + q;
    r.shift(2).shift(3);
    let w = Wrapper::new(r.dist2());
    let pair = Pair(1, 2.5);
    let _u = Unit;
    let base = Point { x: 9, ..Point::origin() };
    *w.get() + w.n as i32 + pair.0 + base.y + w.into_inner()
}
