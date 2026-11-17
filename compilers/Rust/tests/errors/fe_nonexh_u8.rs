fn a(x: u8) -> i32 {
    match x {
        0..=9 => 1,
    }
}
fn main() {
    println!("{}", a(3));
}
