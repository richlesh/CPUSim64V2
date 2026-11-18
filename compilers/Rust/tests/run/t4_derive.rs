// derive(Debug, Clone, PartialEq, PartialOrd, Default, Copy, Eq, Ord)
#[derive(Debug, Clone, PartialEq, PartialOrd)]
struct Version {
    major: u32,
    minor: u32,
    tag: String,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
enum Level {
    Low = 1,
    Mid = 5,
    High = 10,
}

#[derive(Debug, Clone, PartialEq)]
enum Msg {
    Quit,
    Move { x: i32, y: i32 },
    Write(String),
    Color(u8, u8, u8),
}

#[derive(Debug, Default, Clone, PartialEq)]
struct Config {
    name: String,
    retries: u32,
    verbose: bool,
    ratio: f64,
    tags: Vec<String>,
    limit: Option<u32>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
struct Id(u32);

#[derive(Debug)]
struct Unit;

#[derive(Debug, Clone, PartialEq)]
struct Wrapper<T> {
    inner: T,
}

fn main() {
    let v1 = Version { major: 1, minor: 2, tag: "a".to_string() };
    let v2 = Version { major: 1, minor: 10, tag: "a".to_string() };
    let v3 = v1.clone();
    println!("{:?}", v1);
    println!("{} {}", v1 == v3, v1 == v2);
    println!("{} {}", v1 < v2, v2 <= v1);
    println!("{:?}", v1.partial_cmp(&v2));

    println!("{:?} {}", Level::Mid, Level::High as i32);
    println!("{}", Level::Low < Level::High);
    println!("{:?}", Level::Mid.cmp(&Level::Mid));
    println!("{:?}", Level::High.max(Level::Low));
    let mut lv = vec![Level::High, Level::Low, Level::Mid];
    lv.sort();
    println!("{:?}", lv);

    let msgs = vec![
        Msg::Quit,
        Msg::Move { x: 1, y: -1 },
        Msg::Write("hi".to_string()),
        Msg::Color(1, 2, 3),
    ];
    for m in &msgs {
        println!("{:?}", m);
    }
    println!("{}", msgs[1] == Msg::Move { x: 1, y: -1 });
    println!("{}", msgs[2] == Msg::Write("ho".to_string()));
    println!("{:#?}", msgs[1]);

    let c = Config::default();
    println!("{:?}", c);
    let c2 = Config { name: "x".to_string(), retries: 3, ..Default::default() };
    println!("{:?}", c2);
    println!("{}", c == c2);
    println!("{:#?}", c2);

    let a = Id(5);
    let b = a;
    println!("{:?} {:?} {}", a, b, a == b);
    println!("{:?}", Id(1).max(Id(9)));
    println!("{:?}", Unit);
    let w = Wrapper { inner: vec![Some(1), None] };
    println!("{:?}", w.clone());
    println!("{}", w == w.clone());
    println!("{:?}", (Id(1), Level::Low));
}
