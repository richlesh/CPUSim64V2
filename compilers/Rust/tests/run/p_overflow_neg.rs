// negating i32::MIN overflows
fn neg(x: i32) -> i32 {
    -x
}

fn main() {
    println!("{}", neg(5));
    println!("{}", neg(i32::MIN));
}
