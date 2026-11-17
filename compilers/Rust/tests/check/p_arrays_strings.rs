const MAX: usize = 10;
static GREETING: &str = "hi";
struct Grid { cells: [[u8; 3]; 3] }
impl Grid {
    fn new() -> Grid { Grid { cells: [[0; 3]; 3] } }
    fn set(&mut self, r: usize, c: usize, v: u8) { self.cells[r][c] = v; }
    fn get(&self, r: usize, c: usize) -> u8 { self.cells[r][c] }
    fn total(&self) -> u32 {
        let mut t = 0u32;
        for r in 0..3 { for c in 0..3 { t += self.cells[r][c] as u32; } }
        t
    }
}
fn fib(n: u64) -> u64 { if n < 2 { n } else { fib(n - 1) + fib(n - 2) } }
fn gcd(a: u64, b: u64) -> u64 { if b == 0 { a } else { gcd(b, a % b) } }
fn main() {
    let mut g = Grid::new();
    g.set(1, 1, 5);
    println!("{} {}", g.get(1, 1), g.total());
    let mut arr = [5, 3, 8, 1];
    let n = arr.len();
    for i in 0..n { for j in 0..n - 1 - i { if arr[j] > arr[j + 1] { arr.swap(j, j + 1); } } }
    println!("{:?} {} {}", arr, MAX, GREETING);
    println!("{} {}", fib(10), gcd(48, 18));
    let x = 5u8; let y = x as i32 - 10; let z = y as f64 / 3.0;
    let c = 'a'; let d = (c as u8 + 1) as char;
    println!("{} {} {}", y, z, d);
    let s = String::from("hello world");
    let words: Vec<&str> = s.split(' ').collect();
    let up = s.to_uppercase();
    println!("{} {} {}", words.len(), up, s.len());
    'outer: for i in 0..5 { for j in 0..5 { if i * j > 6 { break 'outer; } } }
    let t = if sum(&[1, 2, 3]) > 3 { "big" } else { "small" };
    println!("{}", t);
}
fn sum(v: &[i32]) -> i32 { let mut s = 0; for x in v { s += x; } s }
