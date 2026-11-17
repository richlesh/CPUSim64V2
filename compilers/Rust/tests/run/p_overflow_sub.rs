// unsigned subtraction underflow
fn diff(a: usize, b: usize) -> usize {
    a - b
}

fn main() {
    println!("{}", diff(5, 3));
    println!("{}", diff(3, 5));
}
