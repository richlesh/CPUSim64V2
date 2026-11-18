// iterator adapters and consumers
fn main() {
    let v = vec![3, 1, 4, 1, 5, 9, 2, 6, 5, 3];
    let doubled: Vec<i32> = v.iter().map(|x| x * 2).collect();
    println!("{:?}", doubled);
    let evens: Vec<&i32> = v.iter().filter(|x| **x % 2 == 0).collect();
    println!("{:?}", evens);
    println!("{}", v.iter().sum::<i32>());
    println!("{}", v.iter().product::<i32>());
    println!("{}", v.iter().fold(0, |acc, x| acc * 2 + x));
    println!("{:?} {:?}", v.iter().max(), v.iter().min());
    println!("{}", v.iter().count());
    println!("{}", v.iter().any(|&x| x > 8));
    println!("{}", v.iter().all(|&x| x > 0));
    println!("{:?}", v.iter().find(|&&x| x > 4));
    println!("{:?}", v.iter().position(|&x| x == 9));
    for (i, x) in v.iter().enumerate().skip(7) {
        print!("{}:{} ", i, x);
    }
    println!();
    let names = ["a", "b", "c"];
    let nums = [1, 2, 3, 4];
    let pairs: Vec<(&str, i32)> = names.iter().cloned().zip(nums.iter().cloned()).collect();
    println!("{:?}", pairs);
    println!("{:?}", (1..=4).map(|x| x * x).collect::<Vec<u32>>());
    println!("{}", (1..=10).filter(|n| n % 3 == 0).sum::<u32>());
    println!("{:?}", (0..20).step_by(5).collect::<Vec<_>>());
    println!("{:?}", v.iter().take(3).collect::<Vec<_>>());
    println!("{:?}", v.iter().skip(8).collect::<Vec<_>>());
    println!("{:?}", v.iter().take_while(|&&x| x < 5).collect::<Vec<_>>());
    println!("{:?}", v.iter().skip_while(|&&x| x < 5).count());
    println!("{:?}", [1, 2].iter().chain([3].iter()).collect::<Vec<_>>());
    println!("{:?}", v.iter().rev().copied().collect::<Vec<i32>>());
    println!("{:?}", v.iter().max_by_key(|&&x| (x as i32 - 4).abs()));
    println!("{:?}", v.iter().min_by_key(|&&x| (x as i32 - 4).abs()));
    println!("{:?}", v.windows(2).filter(|w| w[0] < w[1]).count());
    let parsed: Vec<i32> = "1 2 3".split(' ').map(|s| s.parse().unwrap()).collect();
    println!("{:?}", parsed);
    let good: Result<Vec<i32>, _> = "1,2,x".split(',').map(|s| s.parse::<i32>()).collect();
    println!("{}", good.is_err());
    let all: Option<Vec<i32>> = vec![Some(1), Some(2)].into_iter().collect();
    println!("{:?}", all);
    let fm: Vec<i32> = vec!["1", "a", "3"].iter().filter_map(|s| s.parse().ok()).collect();
    println!("{:?}", fm);
    let s: String = vec!["x", "y", "z"].into_iter().collect();
    println!("{}", s);
    let mut it = v.iter().peekable();
    let mut runs = 0;
    while let Some(x) = it.next() {
        if let Some(&&nx) = it.peek() {
            if nx > *x {
                runs += 1;
            }
        }
    }
    println!("{}", runs);
    let mut counter = 0;
    (0..5).for_each(|i| counter += i);
    println!("{}", counter);
    println!("{:?}", std::iter::repeat(7).take(3).collect::<Vec<_>>());
    println!("{:?}", v.iter().last());
    println!("{:?}", v.iter().nth(2));
    let small: Vec<i32> = v.iter().cloned().filter(|&x| x < 4).collect();
    let big: Vec<i32> = v.iter().cloned().filter(|&x| x >= 4).collect();
    println!("{:?} {:?}", small, big);
    println!("{:?}", (1..4).map(|x| x * 10).collect::<Vec<_>>());
}
