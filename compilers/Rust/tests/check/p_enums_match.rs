#[derive(Debug)]
enum Expr {
    Num(i64),
    Add(Box<Expr>, Box<Expr>),
    Mul(Box<Expr>, Box<Expr>),
    Neg(Box<Expr>),
}
fn eval(e: &Expr) -> i64 {
    match e {
        Expr::Num(n) => *n,
        Expr::Add(a, b) => eval(a) + eval(b),
        Expr::Mul(a, b) => eval(a) * eval(b),
        Expr::Neg(a) => -eval(a),
    }
}
enum Color { Red, Green, Blue }
fn code(c: Color) -> u8 {
    match c { Color::Red => 1, Color::Green => 2, Color::Blue => 3 }
}
fn classify(n: i32) -> &'static str {
    match n {
        0 => "zero",
        1..=9 => "small",
        x if x < 0 => "neg",
        _ => "big",
    }
}
fn main() {
    let e = Expr::Add(Box::new(Expr::Num(2)), Box::new(Expr::Mul(Box::new(Expr::Num(3)), Box::new(Expr::Num(4)))));
    println!("{}", eval(&e));
    println!("{}", code(Color::Green));
    println!("{}", classify(5));
    let o: Option<i32> = Some(3);
    if let Some(v) = o { println!("{}", v); }
    match o { Some(x) => println!("{}", x), None => println!("none") }
}
