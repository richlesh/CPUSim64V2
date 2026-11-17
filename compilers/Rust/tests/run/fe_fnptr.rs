fn neg(x: f64) -> f64 {
    -x
}
fn main() {
    let t: [(&str, fn(f64) -> f64); 2] = [("sqrt", f64::sqrt), ("neg", neg)];
    for (n, f) in t.iter() {
        println!("{} {}", n, f(4.0));
    }
    let u: (fn(f64) -> f64, i32) = (neg, 1);
    println!("{}", (u.0)(2.0) + u.1 as f64);
    let w: Vec<(&str, fn(f64) -> f64)> = vec![("a", neg), ("b", f64::abs)];
    for (n, f) in &w {
        println!("{} {}", n, f(-3.0));
    }
    let o: Option<fn(f64) -> f64> = Some(neg);
    println!("{}", o.unwrap()(5.0));
}
