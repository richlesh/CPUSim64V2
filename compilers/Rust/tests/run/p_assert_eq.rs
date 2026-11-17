// assert! and assert_eq! failures
fn main() {
    let a = 2 + 2;
    assert!(a == 4, "math works");
    assert_eq!(a, 4);
    println!("asserts passed");
    let b = a * 2;
    assert_eq!(b, 9, "b should be nine");
    println!("unreachable");
}
