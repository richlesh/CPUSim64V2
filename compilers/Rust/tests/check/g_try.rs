struct Err1(i32);
enum MyErr { Parse(std::num::ParseIntError), Neg(i32) }
impl From<std::num::ParseIntError> for MyErr {
    fn from(e: std::num::ParseIntError) -> MyErr { MyErr::Parse(e) }
}
impl From<Err1> for MyErr {
    fn from(e: Err1) -> MyErr { MyErr::Neg(e.0) }
}
fn check(n: i32) -> Result<i32, Err1> { if n < 0 { Err(Err1(n)) } else { Ok(n) } }
fn parse(s: &str) -> Result<i32, MyErr> {
    let n = s.trim().parse::<i32>()?;
    let m = check(n)?;
    Ok(m * 2)
}
fn first(v: &[i32]) -> Option<i32> {
    let x = v.get(0)?;
    let y = v.get(1)?;
    Some(*x + *y)
}
fn dynerr(s: &str) -> Result<i32, Box<dyn std::error::Error>> {
    let n: i32 = s.parse()?;
    if n == 0 { return Err("zero".into()); }
    Ok(n)
}
fn unwrap_or(o: Option<i32>) -> i32 {
    let Some(v) = o else { return -1 };
    v
}
