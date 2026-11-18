fn apply<F: Fn(i32) -> i32>(f: F, x: i32) -> i32 { f(x) }
fn make(n: i32) -> Box<dyn Fn(i32) -> i32> { Box::new(move |x| x + n) }
fn run() -> i32 {
    let k = 10;
    let add = |x| x + k;
    let mut count = 0;
    let mut inc = || count += 1;
    inc();
    let v: Vec<i32> = vec![1, 2, 3, 4];
    let doubled: Vec<i32> = v.iter().map(|x| x * 2).collect();
    let evens = v.iter().filter(|x| **x % 2 == 0).count();
    let sum: i32 = v.iter().sum();
    let folded = v.iter().fold(0, |a, b| a + b);
    let strs: Vec<String> = v.iter().map(|n| n.to_string()).collect();
    let longest = strs.iter().map(|s| s.len()).max().unwrap_or(0);
    let pos = v.iter().position(|&x| x == 3);
    let pairs: Vec<(usize, &i32)> = v.iter().enumerate().collect();
    let f = make(3);
    add(1) + apply(|x| x * 2, 4) + doubled.len() as i32 + evens as i32 + sum + folded
        + longest as i32 + pos.unwrap() as i32 + pairs.len() as i32 + f(2) + count
}
