// Option and Result: combinators, ?, if let, while let, let else-free
fn find(v: &[i32], target: i32) -> Option<usize> {
    for i in 0..v.len() {
        if v[i] == target {
            return Some(i);
        }
    }
    None
}

fn parse_sum(a: &str, b: &str) -> Result<i32, std::num::ParseIntError> {
    let x: i32 = a.parse()?;
    let y: i32 = b.parse()?;
    Ok(x + y)
}

fn half(n: i32) -> Result<i32, String> {
    if n % 2 == 0 {
        Ok(n / 2)
    } else {
        Err(format!("{} is odd", n))
    }
}

fn quarter(n: i32) -> Result<i32, String> {
    let h = half(n)?;
    half(h)
}

fn first_even(v: &[i32]) -> Option<i32> {
    let f = v.iter().find(|x| **x % 2 == 0)?;
    Some(*f * 10)
}

fn main() {
    let v = [4, 8, 15, 16, 23, 42];
    println!("{:?} {:?}", find(&v, 15), find(&v, 99));
    println!("{:?}", parse_sum("20", "22"));
    println!("{}", parse_sum("20", "x").is_err());
    println!("{:?} {:?}", quarter(12), quarter(6));
    println!("{:?} {:?}", first_even(&[1, 3, 6]), first_even(&[1, 3]));

    let a: Option<i32> = Some(5);
    let b: Option<i32> = None;
    println!("{}", a.unwrap_or(0) + b.unwrap_or(100));
    println!("{:?}", a.map(|x| x * 2));
    println!("{:?}", b.map(|x| x * 2));
    println!("{:?}", a.and_then(|x| if x > 3 { Some(x - 3) } else { None }));
    println!("{} {}", a.is_some(), b.is_none());
    println!("{}", b.unwrap_or_else(|| 77));
    println!("{:?}", a.ok_or("none"));
    println!("{:?}", b.ok_or("none"));
    println!("{:?}", a.filter(|x| *x > 10));

    let r: Result<i32, String> = Ok(3);
    let e: Result<i32, String> = Err("bad".to_string());
    println!("{:?} {:?}", r.clone().ok(), e.clone().ok());
    println!("{}", r.clone().unwrap_or(0));
    println!("{}", e.clone().unwrap_or(-1));
    println!("{:?}", r.clone().map(|x| x + 1));
    println!("{:?}", e.clone().map_err(|s| s.len()));

    if let Some(i) = find(&v, 23) {
        println!("found at {}", i);
    }
    if let Err(msg) = half(7) {
        println!("error: {}", msg);
    }
    let mut stack = vec![1, 2, 3];
    while let Some(top) = stack.pop() {
        print!("{} ", top);
    }
    println!();
    let mut o = Some(3);
    let taken = o.take();
    println!("{:?} {:?}", o, taken);
}
