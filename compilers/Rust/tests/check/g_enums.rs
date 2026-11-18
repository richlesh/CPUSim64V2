enum Expr { Num(i64), Add(Box<Expr>, Box<Expr>), Neg(Box<Expr>), Var { name: String, scale: i64 } }
enum Color { Red, Green, Blue }
fn eval(e: &Expr) -> i64 {
    match e {
        Expr::Num(n) => *n,
        Expr::Add(a, b) => eval(a) + eval(b),
        Expr::Neg(a) => -eval(a),
        Expr::Var { scale, .. } => *scale,
    }
}
fn code(c: Color) -> u8 {
    match c { Color::Red => 1, Color::Green => 2, Color::Blue => 3 }
}
fn classify(n: i32) -> i32 {
    match n {
        0 => 0,
        1 | 2 => 1,
        3..=9 => 2,
        x if x < 0 => -1,
        _ => 3,
    }
}
fn opt(o: Option<i32>, r: Result<i32, String>) -> i32 {
    let a = match o { Some(x) => x, None => 0 };
    let b = if let Ok(v) = r { v } else { 0 };
    let t = (a, b);
    match t { (0, y) => y, (x, 0) => x, (x, y) => x + y }
}
fn refs(v: &Vec<(i32, bool)>) -> i32 {
    let mut s = 0;
    for &(n, flag) in v { if flag { s += n; } }
    for (n, _) in v { s += *n; }
    s
}
fn slices(v: &[i32]) -> i32 {
    match v { [] => 0, [x] => *x, [first, .., last] => first + last }
}
