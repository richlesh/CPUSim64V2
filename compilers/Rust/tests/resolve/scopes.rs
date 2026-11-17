// block scopes, shadowing, patterns, closures, labels
#[derive(Clone)]
enum E { A(i32), B { x: i32, y: i32 }, C }
const ZERO: i32 = 0;
struct Unit;
fn takes(u: Unit, (a, b): (i32, i32), E::A(c): E) -> i32 { a + b + c }
fn main() {
    let x = 1;
    let x = x + 1;
    {
        let x = x * 2;
        let _ = x;
    }
    let e = E::B { x: 1, y: 2 };
    let n = match e {
        E::A(v) | E::B { x: v, .. } if v > ZERO => v,
        E::A(0) => ZERO,
        ZERO => 0,
        E::C => 1,
        other => 2,
    };
    let (mut a, ref b, ref mut c) = (1, 2, 3);
    let [first, .., last] = [1, 2, 3];
    let u = Unit;
    let Unit = u;
    if let E::A(k) = e { let _ = k; } else { let _ = x; }
    let v = Some(2);
    while let Some(i) = v { let _ = i; break; }
    let Some(w) = v else { return; };
    let r = 'blk: { if w > 1 { break 'blk 1; } 2 };
    let add = |p: i32, q| p + q + x;
    let nested = |p| move |q| p + q;
    let mut count = 0;
    'a: for i in 0..3 {
        'b: loop {
            for j in 0..i { if j == 1 { continue 'a; } else { break 'b; } }
        }
    }
    fn helper(q: i32) -> i32 { q + ZERO }
    struct Local { f: i32 }
    impl Local { fn get(&self) -> i32 { self.f + helper(1) } }
    let l = Local { f: 1 };
    let l2 = Local { ..l };
    const INNER: i32 = ZERO + 1;
    let arr = [INNER; 3];
    x_is_used(x);
}
fn x_is_used(_v: i32) {}
