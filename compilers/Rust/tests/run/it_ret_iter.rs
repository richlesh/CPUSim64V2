fn evens(n: u32) -> impl Iterator<Item = u32> {
    (0..n).filter(|x| x % 2 == 0)
}

fn doubled<T: Clone>(v: Vec<T>) -> impl Iterator<Item = T> {
    let w = v.clone();
    v.into_iter().chain(w.into_iter())
}

fn squares(n: i32) -> impl Iterator<Item = i32> {
    (1..=n).map(|x| x * x)
}

fn main() {
    println!("{}", evens(10).map(|x| x * x).sum::<u32>());
    for x in evens(7) {
        println!("{}", x);
    }
    let d: Vec<i32> = doubled(vec![1, 2, 3]).collect();
    println!("{:?}", d);
    let s: Vec<String> = doubled(vec![String::from("a"), String::from("b")]).collect();
    println!("{:?}", s);
    let mut it = squares(4);
    println!("{:?}", it.next());
    println!("{}", it.sum::<i32>());
}
