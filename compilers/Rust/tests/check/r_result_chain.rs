use std::fmt;
enum AppError { Empty, BadNumber(String), Negative(i64) }
impl fmt::Display for AppError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            AppError::Empty => write!(f, "empty input"),
            AppError::BadNumber(s) => write!(f, "bad number '{}'", s),
            AppError::Negative(n) => write!(f, "negative: {}", n),
        }
    }
}
fn parse_one(s: &str) -> Result<i64, AppError> {
    let n = s.trim().parse::<i64>().map_err(|_| AppError::BadNumber(s.to_string()))?;
    if n < 0 { return Err(AppError::Negative(n)); }
    Ok(n)
}
fn parse_all(line: &str) -> Result<Vec<i64>, AppError> {
    if line.is_empty() { return Err(AppError::Empty); }
    let mut out = Vec::new();
    for part in line.split(',') { out.push(parse_one(part)?); }
    Ok(out)
}
fn sum_line(line: &str) -> Result<i64, AppError> {
    let v = parse_all(line)?;
    Ok(v.iter().sum())
}
fn first_even(v: &[i64]) -> Option<i64> {
    let x = v.iter().find(|n| **n % 2 == 0)?;
    Some(*x * 10)
}
fn main() {
    for line in ["1,2,3", "", "4,x", "5,-6"] {
        match sum_line(line) { Ok(n) => println!("ok {}", n), Err(e) => println!("error: {}", e) }
    }
    println!("{:?}", first_even(&[1, 3, 4]));
}
