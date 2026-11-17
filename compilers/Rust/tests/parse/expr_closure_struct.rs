struct P { a: i32, b: i32 }
fn main() {
    let f = |a, b: i32| a + b;
    let g = || 1;
    let h = move || x;
    let i = |x: i32| -> i32 { x + 1 };
    let j = || -> () { };
    let k = |&x| *x;
    let l = |(a, b)| a * b;
    let m = |a| |b| a + b;
    let p = P { a: 1, b: 2 };
    let q = P { a, b };
    let r = P { a: 1, ..base };
    let s = P { b: 2, ..Default::default() };
    let t = path::to::P { a: 1, b: f(P { a: 1, b: 2 }), };
    let u = Tup { 0: 1, 1: 2 };
    if x == (P { a: 1, b: 2 }) { }
    if x == f(P { a: 1, b: 2 }) { }
    while let Some(P { a, b }) = it.next() { }
    for p in [P { a: 1, b: 2 }] { }
    match (P { a: 1, b: 2 }) { _ => {} }
    let v = async { 1 };
    let w = async move { 2 };
    let x = unsafe { *p };
    let y = f(|x| x, 3);
    let z = x.map(|v| { v + 1 });
}
