struct S { x: i32 }
macro_rules! show {
    ($e:expr) => { stringify!($e) };
}
macro_rules! check {
    ($e:expr) => { assert!($e) };
}
macro_rules! lit_fmt {
    ($fmt:expr, $($arg:tt)*) => { println!(concat!("[log] ", $fmt), $($arg)*) };
}
fn main() {
    let v = vec![1, 2, 3];
    let x = 5;
    let s = S { x: 1 };
    assert!(v.iter().all(|e| *e > 0));
    assert!(v == [1, 2, 3]);
    assert!(x as i64 >= -1 && !v.is_empty());
    assert!(matches!(v.get(0), Some(_)));
    assert!(s.x != 2 || (x - 1) * 2 == 8);
    assert!(&v[..] == &[1, 2, 3][..]);
    assert!(x==5);
    assert!(Vec::<i32>::new().is_empty());
    assert!("a" < "b");
    assert!(v.len() >= 1, "len was {}", v.len());
    check!(1 + 1 == 2);
    let a = show!(1+2);
    let b = show!(x * (y + 1));
    lit_fmt!("{} {}", 1, x);
    println!("{}", stringify!(a   +   b));
}
