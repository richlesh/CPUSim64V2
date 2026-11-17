fn a(x: &str) -> i32 {
    match x {
        "a" => 1,
        "b" => 2,
    }
}
fn main() {
    println!("{}", a("a"));
}
