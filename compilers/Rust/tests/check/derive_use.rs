#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Default)]
struct Point { x: i32, y: i32 }

#[derive(Debug, Clone, PartialEq, PartialOrd, Default)]
struct Pair<T, U>(T, U);

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Default)]
struct Unit;

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
enum Shape<T> { Empty, Circle(T), Rect { w: T, h: T } }

#[derive(Debug, Default, Clone, Copy, PartialEq)]
enum Mode { Fast, #[default] Normal }

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord, Default)]
struct Named { name: String, tags: Vec<i32>, p: Point }

fn main() {
    let a = Point { x: 1, y: 2 };
    let b = a;
    let c = a.clone();
    println!("{:?} {:#?}", a, b);
    let eq = a == c;
    let ne = a != Point::default();
    let lt = a < Point { x: 1, y: 3 };
    let ord = a.cmp(&c);
    let pc = a.partial_cmp(&c);
    let p: Pair<i32, String> = Pair::default();
    let p2 = p.clone();
    println!("{:?} {}", p2, p == p2);
    let u = Unit;
    println!("{:?} {}", u, u == Unit::default());
    let s: Shape<i32> = Shape::Rect { w: 1, h: 2 };
    let t = Shape::Circle(3);
    let e0: Shape<i32> = Shape::Empty;
    println!("{:?} {} {}", s, s < t, e0 == Shape::Empty);
    let m = Mode::default();
    println!("{:?}", m);
    let mut v = vec![Point { x: 2, y: 1 }, Point { x: 1, y: 5 }, Point { x: 1, y: 2 }];
    v.sort();
    println!("{:?} {:?}", v, v.iter().max());
    let mut ns = vec![Named::default(), Named { name: String::from("z"), tags: vec![1], p: a }];
    ns.sort();
    let n2 = ns.clone();
    println!("{}", ns == n2);
}
