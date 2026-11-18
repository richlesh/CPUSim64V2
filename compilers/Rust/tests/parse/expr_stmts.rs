fn main() {
    let x = 1;
    let y: i32;
    let z: u8 = 2;
    let (a, b) = (1, 2);
    let Some(v) = opt else { return; };
    let Ok(w) = res else { panic!("bad") };
    ;
    ;;
    if x {} - 1;
    if x { 1 } else { 2 }
    match x { _ => {} }
    { let inner = 1; }
    unsafe { f() };
    fn inner_fn(a: i32) -> i32 { a }
    struct Local { v: i32 }
    const N: i32 = 3;
    use std::fmt;
    println!("hello {}", x);
    vec![1, 2, 3];
    m! { a b c }
    m!{} - 1;
    println!("x").len();
    vec![1, 2].len();
    #[allow(unused)]
    let q = 1;
    #[inline]
    fn attr_fn() {}
    x += 1;
    x = if a { 1 } else { 2 } + 3;
    match x { _ => 1 }.foo();
    if a { b } else { c }.to_string();
    f(x)?;
    loop {}
    let t = 1.0 + 2.5e3 + 3f32 + 4_000i64 + 0xffu8;
    let s = "str" == r"raw" || b"bytes" == br"raw";
    let c = 'c';
    let by = b'x';
    let tr = true && !false;
    let e = { 1 };
    let r = 10;
    r
}
fn tail() -> i32 {
    if a { 1 } else { 2 }
}
fn tail_macro() -> Vec<i32> {
    vec![1, 2]
}
fn tail_block() -> i32 {
    { 3 }
}
