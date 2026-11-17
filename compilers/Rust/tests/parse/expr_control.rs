fn main() {
    if a { 1 } else if b { 2 } else { 3 };
    if let Some(x) = y { x } else { 0 };
    if let Some(x) = y && x > 1 && let Ok(z) = w { z }
    while i < 10 { i += 1; }
    while let Some(x) = it.next() { f(x); }
    loop { break; }
    'outer: loop { break 'outer; }
    'a: while x { continue 'a; }
    'b: for i in 0..10 { if i == 3 { continue 'b; } }
    for (i, x) in v.iter().enumerate() { f(i, x); }
    for i in 0.. { break; }
    let v = loop { break 5; };
    let w = 'l: loop { break 'l 7; };
    let x = 'blk: { if c { break 'blk 1; } 2 };
    let y = if a == B { 1 } else { 2 };
    let z = match x {
        1 => 10,
        2 | 3 => { 20 }
        n if n > 5 => 30,
        Some(v) if let Ok(q) = v => q,
        _ => return,
    };
    match x { _ => {} _ => () }
    match (S { a: 1 }).a { _ => {} }
    unsafe { f(); }
    let u = unsafe { g() };
    match x { 1 => if a { 1 } else { 2 } 2 => break, 3 => continue, _ => 4 }
    return;
}
fn g() -> i32 { return 1 + 2 }
