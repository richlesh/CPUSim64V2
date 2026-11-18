mod geometry {
    pub const UNIT: f64 = 1.0;
    pub struct Rect { pub w: f64, pub h: f64 }
    impl Rect {
        pub fn area(&self) -> f64 { self.w * self.h }
        pub fn square(s: f64) -> Rect { Rect { w: s, h: s } }
    }
    pub mod util {
        use super::Rect;
        pub fn perimeter(r: &Rect) -> f64 { 2.0 * (r.w + r.h) }
    }
}
use geometry::{Rect, util::perimeter};
use std::io::{self, Write};
fn read_numbers() -> Vec<i64> {
    let mut line = String::new();
    io::stdin().read_line(&mut line).unwrap();
    line.split_whitespace().map(|t| t.parse().unwrap()).collect()
}
fn bits(x: u32) -> u32 { (x & 0xFF) | (x << 4) ^ (x >> 2) }
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let r = Rect::square(2.0);
    println!("{} {} {}", r.area(), perimeter(&r), geometry::UNIT);
    let nums = read_numbers();
    let mx = nums.iter().max();
    let mn = nums.iter().copied().min().unwrap_or(0);
    println!("{:?} {}", mx, mn);
    let mut input = String::new();
    io::stdin().read_to_string(&mut input)?;
    let lines: Vec<&str> = input.lines().collect();
    println!("{}", lines.len());
    let big: u64 = 1 << 40;
    let h = (big % 1_000_007) as u32;
    println!("{} {}", bits(h), u64::MAX / 3);
    if nums.is_empty() { std::process::exit(2); }
    let n: i32 = "42".parse()?;
    println!("{}", n);
    io::stdout().flush()?;
    Ok(())
}
