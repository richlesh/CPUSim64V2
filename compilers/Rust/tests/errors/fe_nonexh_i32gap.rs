fn a(x: i32) -> i32 {
    match x {
        i32::MIN..=-1 => 1,
        1..=i32::MAX => 2,
    }
}
fn main() {
    println!("{}", a(3));
}
