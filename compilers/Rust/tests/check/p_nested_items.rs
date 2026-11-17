fn outer(n: i32) -> i32 {
    const K: i32 = 3;
    fn helper(x: i32) -> i32 { x * K }
    struct Local { v: i32 }
    impl Local { fn get(&self) -> i32 { self.v } }
    let l = Local { v: helper(n) };
    l.get()
}
static mut COUNTER: u32 = 0;
fn bump() -> u32 { unsafe { COUNTER += 1; COUNTER } }
fn is_even(n: u32) -> bool { if n == 0 { true } else { is_odd(n - 1) } }
fn is_odd(n: u32) -> bool { if n == 0 { false } else { is_even(n - 1) } }
fn main() {
    println!("{} {} {}", outer(2), bump(), is_even(4));
    let x;
    if outer(1) > 2 { x = 1; } else { x = 2; }
    let label = 'a: { if x > 1 { break 'a "big"; } "small" };
    println!("{} {}", x, label);
    let v = vec![1, 2, 3];
    let idx = 1;
    println!("{} {}", v[idx], v[idx as usize + 1]);
    let mut grid = vec![vec![0; 3]; 2];
    grid[1][2] = 7;
    let row: &Vec<i32> = &grid[1];
    println!("{} {}", row[2], grid.len());
    let t: (i32, String) = (1, "a".to_string());
    let (n, ref s) = t;
    println!("{} {}", n, s);
}
