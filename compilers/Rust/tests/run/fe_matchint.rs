const LIM: u8 = 100;
const NEG: i32 = -5;
fn a(x: u8) -> i32 {
    match x {
        0..=9 => 1,
        10..=u8::MAX => 2,
    }
}
fn b(x: i32) -> i32 {
    match x {
        i32::MIN..=-1 => -1,
        0 => 0,
        1..=i32::MAX => 1,
    }
}
fn c(x: u8) -> i32 {
    match x {
        0..=LIM => 1,
        101..=255 => 2,
    }
}
fn d(x: i8) -> i32 {
    match x {
        -128..=-1 => 1,
        0..=127 => 2,
    }
}
fn e(x: (u8, bool)) -> i32 {
    match x {
        (0..=127, true) => 1,
        (128..=255, true) => 2,
        (_, false) => 3,
    }
}
fn f(x: &str) -> i32 {
    match x {
        "a" => 1,
        "bc" => 2,
        _ => 3,
    }
}
fn g(x: char) -> i32 {
    match x {
        '\0'..='z' => 1,
        '{'..=char::MAX => 2,
    }
}
fn h(x: u8) -> i32 {
    match x {
        0 => 1,
        1..=254 => 2,
        255 => 3,
    }
}
fn i(x: Option<u8>) -> i32 {
    match x {
        Some(0..=5) => 1,
        Some(6..) => 2,
        None => 3,
    }
}
fn j(x: u8) -> i32 {
    match x {
        ..=5 => 1,
        6.. => 2,
    }
}
fn k(x: u64) -> i32 {
    match x {
        0..=9 => 1,
        10..=u64::MAX => 2,
    }
}
fn l(x: i64) -> i32 {
    match x {
        i64::MIN..=0 => 1,
        1.. => 2,
    }
}
fn m(x: i32) -> i32 {
    match x {
        i32::MIN..=NEG => 1,
        -4..=3 | 4 | 5..=i32::MAX => 2,
    }
}
fn n(x: u8, y: bool) -> i32 {
    match (x, y) {
        (0, _) => 1,
        (1..=255, true) => 2,
        (1..=100, false) => 3,
        (101..=255, false) => 4,
    }
}
fn main() {
    println!("{} {} {}", a(3), a(10), a(255));
    println!("{} {} {}", b(-4), b(0), b(7));
    println!("{} {} {}", c(7), c(100), c(101));
    println!("{} {}", d(-1), d(1));
    println!("{} {} {}", e((200, true)), e((1, true)), e((9, false)));
    println!("{} {} {}", f("a"), f("bc"), f("zz"));
    println!("{} {}", g('a'), g('~'));
    println!("{} {} {}", h(0), h(77), h(255));
    println!("{} {} {}", i(Some(9)), i(Some(5)), i(None));
    println!("{} {}", j(3), j(6));
    println!("{} {}", k(9), k(u64::MAX));
    println!("{} {}", l(i64::MIN), l(1));
    println!("{} {} {}", m(-100), m(-4), m(9));
    println!("{} {} {}", n(0, true), n(50, false), n(200, false));
}
