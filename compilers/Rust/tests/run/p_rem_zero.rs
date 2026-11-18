// remainder with a zero divisor
fn modulo(a: u64, b: u64) -> u64 {
    a % b
}

fn main() {
    println!("{}", modulo(10, 4));
    println!("{}", modulo(10, 0));
}
