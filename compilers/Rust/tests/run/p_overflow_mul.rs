// u64 multiplication overflow in a factorial loop
fn factorial(n: u64) -> u64 {
    let mut r: u64 = 1;
    for i in 1..=n {
        r *= i;
    }
    r
}

fn main() {
    println!("{}", factorial(20));
    println!("{}", factorial(21));
}
