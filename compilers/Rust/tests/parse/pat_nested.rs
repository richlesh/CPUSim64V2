mod m {
    pub enum E {
        A,
        B(i32),
        C { v: i32 },
    }
}

use m::E;

fn leading_bar(e: E) -> i32 {
    match e {
        | E::A | E::B(_) => 0,
        E::C { v: 1 | 2 } => 1,
        E::C { .. } => 2,
    }
}

fn nested(x: Option<(i32, Result<i32, i32>)>) -> i32 {
    match x {
        Some((0, Ok(n))) | Some((1, Err(n))) => n,
        Some((a, Ok(_) | Err(_))) => a,
        None => -1,
    }
}

fn refs(v: &Vec<(i32, i32)>) -> i32 {
    let mut s = 0;
    for &(a, b) in v.iter() {
        s += a + b;
    }
    for &&(a, _) in [&(1, 2)].iter() {
        s += a;
    }
    s
}

fn qualified_and_global() {
    let x = 5;
    match x {
        ::std::i32::MAX => {}
        std::i32::MIN..=0 => {}
        _ => {}
    }
}

fn negatives(x: i64, f: f64) {
    match x {
        -1 => {}
        -10..=-2 => {}
        i64::MIN..=-11 => {}
        _ => {}
    }
    match f {
        -1.5 => {}
        _ => {}
    }
}

fn strings(s: &str, b: bool) {
    match s {
        "yes" | "y" => {}
        "" => {}
        _ => {}
    }
    match b {
        true => {}
        false => {}
    }
    match b'a' {
        b'a'..=b'z' => {}
        _ => {}
    }
}
