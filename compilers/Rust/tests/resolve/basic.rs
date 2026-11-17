mod shapes {
    pub struct Point { pub x: i32, pub y: i32 }
    pub struct Unit;
    pub struct Pair(pub i32, pub i32);
    pub enum Shape { Dot, Circle(i32), Rect { w: i32, h: i32 } }
    pub fn area(s: &Shape) -> i32 {
        match s {
            Shape::Dot => 0,
            Shape::Circle(r) => r * r * 3,
            Shape::Rect { w, h } => w * h,
        }
    }
    pub mod inner {
        pub fn up() -> i32 { super::helper() + crate::top() }
    }
    fn helper() -> i32 { 1 }
}

use shapes::{Point, Shape::{self, *}, area as compute};
use shapes::inner::up;

const LIMIT: i32 = 10;
static mut COUNTER: i32 = 0;

fn top() -> i32 { LIMIT }

struct Wrapper<T> { v: T }

impl<T> Wrapper<T> {
    fn new(v: T) -> Self { Wrapper { v } }
    fn get(&self) -> &T { &self.v }
}

trait Speak {
    type Out;
    fn speak(&self) -> Self::Out;
    fn twice(&self) -> i32 where Self: Sized { 2 }
}

impl Speak for Point {
    type Out = i32;
    fn speak(&self) -> Self::Out { self.x }
}

fn main() {
    let p = Point { x: 1, y: 2 };
    let q = p;
    let x = 5;
    let x = x + 1;
    let s = Circle(3);
    let n = compute(&s) + up();
    match s {
        Dot => {}
        Circle(r) | Circle(r) => { let _ = r; }
        Rect { w, .. } => { let _ = w; }
    }
    let mut total = 0;
    for i in 0..10 { total += i; }
    'outer: loop {
        while total > 3 {
            if let Shape::Circle(k) = s { total -= k; break 'outer; }
            total -= 1;
            continue 'outer;
        }
    }
    let f = |a: i32, b| a + b + x;
    let w = Wrapper::new(3);
    let u = shapes::Unit;
    let pr = shapes::Pair(1, 2);
    let v: Vec<i32> = Vec::new();
    let o = Some(1);
    let m = i32::MAX;
    let c = <Point as Speak>::speak(&q);
    unsafe { COUNTER += 1; }
    fn local(a: i32) -> i32 { a + LIMIT }
    let r = local(3);
}
