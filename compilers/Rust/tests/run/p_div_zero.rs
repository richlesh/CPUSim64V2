// integer division by zero
fn div(a: i32, b: i32) -> i32 {
    a / b
}

fn main() {
    println!("{}", div(10, 3));
    println!("{}", div(10, 0));
}
