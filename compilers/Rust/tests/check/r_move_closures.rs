fn make_counter() -> impl_free::Counter { impl_free::Counter { n: 0 } }
mod impl_free { pub struct Counter { pub n: u32 } }
fn make_adder(k: i32) -> Box<dyn Fn(i32) -> i32> { Box::new(move |x| x + k) }
fn make_multiplier(k: i32) -> Box<dyn FnMut(i32) -> i32> {
    let mut calls = 0;
    Box::new(move |x| { calls += 1; x * k + calls })
}
fn apply_n<F: FnMut() -> i32>(mut f: F, n: usize) -> Vec<i32> {
    let mut out = Vec::new();
    for _ in 0..n { out.push(f()); }
    out
}
fn main() {
    let c = make_counter();
    let name = String::from("zed");
    let greet = move || format!("hi {}", name);
    let mut total = 0;
    let mut add = |x: i32| total += x;
    add(3); add(4);
    let adders: Vec<Box<dyn Fn(i32) -> i32>> = vec![make_adder(1), make_adder(10)];
    let results: Vec<i32> = adders.iter().map(|f| f(5)).collect();
    let mut mul = make_multiplier(3);
    let mut state = 0;
    let seq = apply_n(|| { state += 2; state }, 4);
    println!("{} {} {} {:?} {} {:?}", c.n, greet(), total, results, mul(2), seq);
}
