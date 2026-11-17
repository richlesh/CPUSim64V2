struct Point {
    x: i32,
    y: i32,
}

struct Pair(i32, i32);

enum Shape {
    Circle(i32),
    Rect { w: i32, h: i32 },
    Empty,
}

const LIMIT: i32 = 100;

fn classify(n: i32) -> i32 {
    match n {
        i32::MIN..=-1 => -1,
        0 => 0,
        1 | 2 | 3 => 1,
        4..=9 => 2,
        10..20 => 3,
        LIMIT..=1000 => 4,
        x @ 21..=99 => x,
        big @ (1001 | 1002) => big,
        _ => 5,
    }
}

fn chars(c: char) -> i32 {
    match c {
        'a'..='z' => 1,
        'A'..='Z' => 2,
        '0' | '1' => 3,
        _ => 0,
    }
}

fn shapes(s: &Shape) -> i32 {
    match s {
        Shape::Circle(r) => *r,
        Shape::Rect { w, h: 0 } => *w,
        Shape::Rect { w, h } => w * h,
        Shape::Empty => 0,
    }
}

fn destructure() {
    let (a, b) = (1, 2);
    let (c,) = (3,);
    let () = ();
    let Point { x, y: py } = Point { x: 1, y: 2 };
    let Point { x: ref rx, .. } = Point { x: 3, y: 4 };
    let Pair(first, _) = Pair(5, 6);
    let Pair(..) = Pair(7, 8);
    let &r = &9;
    let &&rr = &&10;
    let &mut m = &mut 11;
    let ref a2 = a;
    let ref mut b2 = 12;
    let mut z = 13;
    let [p, q, r2] = [1, 2, 3];
    let [head, .., tail] = [1, 2, 3, 4];
    let [one, rest @ ..] = [1, 2, 3];
}

fn options(o: Option<Option<i32>>, v: &[i32]) -> i32 {
    if let Some(Some(n)) = o {
        return n;
    }
    while let Some(_) = o {
        break;
    }
    match v {
        [] => 0,
        [x] => *x,
        [x, y] => x + y,
        [first, .., last] => first + last,
    }
}

fn main() {
    let t = (1, 2, 3);
    match t {
        (0, ..) => {}
        (.., 9) => {}
        (a, _, b) => {}
    }
    for (i, x) in [1, 2].iter().enumerate() {}
    let f = |(a, b): (i32, i32), &c| a + b;
}
