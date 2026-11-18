fn apply<F: Fn(i32) -> i32>(f: F, x: i32) -> i32 { f(x) }
fn make_adder(n: i32) -> Box<dyn Fn(i32) -> i32> { Box::new(move |x| x + n) }
fn main() {
    let k = 10;
    let add = |x| x + k;
    println!("{}", add(5));
    println!("{}", apply(|x| x * 2, 4));
    let v: Vec<i32> = vec![1, 2, 3, 4, 5];
    let doubled: Vec<i32> = v.iter().map(|x| x * 2).collect();
    let evens: Vec<&i32> = v.iter().filter(|x| **x % 2 == 0).collect();
    let sum: i32 = v.iter().sum();
    let total = v.iter().fold(0, |a, b| a + b);
    println!("{} {} {}", doubled.len(), evens.len(), sum + total);
    let f = make_adder(3);
    println!("{}", f(4));
    let mut names = Vec::new();
    names.push(String::from("bob"));
    names.push("al".to_string());
    let lens: Vec<usize> = names.iter().map(|s| s.len()).collect();
    println!("{:?}", lens);
    let s: String = names.iter().map(|s| s.as_str()).collect::<Vec<&str>>().join(",");
    println!("{}", s);
}
