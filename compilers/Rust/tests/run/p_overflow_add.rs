// integer overflow in i32 addition panics in debug builds
fn id(x: i32) -> i32 {
    x
}

fn main() {
    println!("before");
    let a = id(i32::MAX - 1);
    let b = a + 1;
    println!("{}", b);
    let c = b + id(1);
    println!("unreachable {}", c);
}
