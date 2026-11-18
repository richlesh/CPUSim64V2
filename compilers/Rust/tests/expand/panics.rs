fn f(a: i32, v: Vec<i32>) {
    panic!();
    panic!("plain {{braces}}");
    panic!("value {} {x}", a, x = 3);
    assert!(a == 1);
    assert!(a > 0 && !v.is_empty(), "bad a: {}", a);
    assert!(v.len() == 3);
    assert_eq!(a, 2);
    assert_ne!(a, 3, "not {}", 3);
    debug_assert!(a != 0);
    debug_assert_eq!(a + 1, 2);
    unreachable!();
    unreachable!("state {}", a);
    todo!();
    todo!("later {}", a);
    unimplemented!();
    unimplemented!("nope");
    assert!(foo::bar(1, 2) == Some(3));
    assert!(-a < 0);
    assert!(&v[0] == &1);
    assert!(a as u8 == 1);
}
fn main() {}
