// unwrap on Err prints the message and the Debug of the error
fn parse(s: &str) -> Result<i32, String> {
    match s.parse::<i32>() {
        Ok(n) => Ok(n),
        Err(_) => Err(format!("not a number: {}", s)),
    }
}

fn main() {
    println!("{}", parse("12").unwrap());
    println!("{}", parse("1x").unwrap());
}
