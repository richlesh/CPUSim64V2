// enums with data, match with guards, bindings, ranges, nested patterns
enum Shape {
    Circle(f64),
    Rect { w: f64, h: f64 },
    Tri(f64, f64, f64),
    Empty,
}

enum Dir {
    North,
    East,
    South,
    West,
}

impl Dir {
    fn turn_right(&self) -> Dir {
        match self {
            Dir::North => Dir::East,
            Dir::East => Dir::South,
            Dir::South => Dir::West,
            Dir::West => Dir::North,
        }
    }
    fn name(&self) -> &str {
        match *self {
            Dir::North => "north",
            Dir::East => "east",
            Dir::South => "south",
            Dir::West => "west",
        }
    }
}

fn area(s: &Shape) -> f64 {
    match s {
        Shape::Circle(r) => 3.0 * r * r,
        Shape::Rect { w, h } => w * h,
        Shape::Tri(a, b, c) => {
            let p = (a + b + c) / 2.0;
            (p * (p - a) * (p - b) * (p - c)).sqrt()
        }
        Shape::Empty => 0.0,
    }
}

fn classify(n: i32) -> &'static str {
    match n {
        i32::MIN..=-1 => "negative",
        0 => "zero",
        1 | 2 | 3 => "small",
        x if x % 2 == 0 => "big even",
        _ => "big odd",
    }
}

fn describe(p: (i32, i32)) -> String {
    match p {
        (0, 0) => String::from("origin"),
        (x, 0) => format!("x axis at {}", x),
        (0, y) => format!("y axis at {}", y),
        (x, y) if x == y => format!("diagonal {}", x),
        (x, y) => format!("({}, {})", x, y),
    }
}

fn main() {
    let shapes = [
        Shape::Circle(2.0),
        Shape::Rect { w: 3.0, h: 4.5 },
        Shape::Tri(3.0, 4.0, 5.0),
        Shape::Empty,
    ];
    for s in shapes.iter() {
        println!("{}", area(s));
    }
    let mut d = Dir::North;
    for _ in 0..5 {
        print!("{} ", d.name());
        d = d.turn_right();
    }
    println!();
    for n in [-5, 0, 2, 10, 11] {
        println!("{} {}", n, classify(n));
    }
    println!("{}", describe((0, 0)));
    println!("{}", describe((4, 0)));
    println!("{}", describe((0, -2)));
    println!("{}", describe((5, 5)));
    println!("{}", describe((1, 2)));
    let c = 'k';
    let kind = match c {
        'a' | 'e' | 'i' | 'o' | 'u' => "vowel",
        'a'..='z' => "consonant",
        _ => "other",
    };
    println!("{}", kind);
    let v = 15u8;
    match v {
        n @ 10..=19 => println!("teen {}", n),
        _ => println!("no"),
    }
}
