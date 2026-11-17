fn main() {
    let nums = vec![5, 12, 7, 20, 3, 8];
    let names = vec!["a", "b", "c"];
    let big: Vec<i32> = nums.iter().filter(|&&n| n > 6).map(|n| n * 2).collect();
    let idx: Vec<(usize, &i32)> = nums.iter().enumerate().filter(|(i, _)| i % 2 == 0).collect();
    let pairs: Vec<(i32, &str)> = nums.iter().cloned().zip(names.iter().cloned()).collect();
    let labels: Vec<String> = pairs.iter().map(|(n, s)| format!("{}{}", s, n)).collect();
    let squares: Vec<u64> = (1..=5u64).map(|x| x * x).collect();
    let total: i32 = nums.iter().sum();
    let maxv = nums.iter().max().unwrap();
    let evens = nums.iter().filter(|n| *n % 2 == 0).count();
    let anybig = nums.iter().any(|&n| n > 15);
    let mut sorted = nums.clone();
    sorted.sort_by(|a, b| b.cmp(a));
    let firsts: Vec<i32> = sorted.iter().take(3).cloned().collect();
    println!("{:?} {:?} {:?} {:?}", big, idx, labels, squares);
    println!("{} {} {} {} {:?}", total, maxv, evens, anybig, firsts);
}
