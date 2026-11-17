const N: usize = 4;
static NAME: &str = "n";
fn run(n: u64) -> f64 {
    let mut arr = [0i32; N];
    let mut grid = [[0u8; 3]; 2];
    grid[1][2] = 5;
    for i in 0..N { arr[i] = i as i32 * 2; }
    let mut t = 0u64;
    let mut i = 0;
    while i < n { t += i; i += 1; }
    let r = loop { t += 1; if t > 100 { break t; } };
    let label = 'a: { if t > 5 { break 'a 1; } 2 };
    let sl = &arr[1..3];
    let s = &NAME[0..1];
    let c = (b'a' + 1) as char;
    let x = 3.7f64 as i32 + (c as u8) as i32 + true as i32;
    let tup = (1, "two", 3.0);
    let (a, b, d) = tup;
    let ok = a < 2 && b.len() > 1 || d >= 1.0;
    let y = if ok { 1.5 } else { 2.5 };
    let neg = -y * 2.0 + (x as f64) / 2.0 - (sl.len() + s.len()) as f64;
    neg + label as f64 + r as f64 + grid[1][2] as f64
}
