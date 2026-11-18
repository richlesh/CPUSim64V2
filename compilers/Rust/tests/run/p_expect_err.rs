// expect on Err
fn half(n: i32) -> Result<i32, String> {
    if n % 2 == 0 {
        Ok(n / 2)
    } else {
        Err(format!("{} is odd", n))
    }
}

fn main() {
    println!("{}", half(10).expect("even input"));
    println!("{}", half(7).expect("even input"));
}
