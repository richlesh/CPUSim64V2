// f64 arithmetic and methods
fn main() {
    let a = 2.5f64;
    let b = 0.5f64;
    println!("{} {} {} {}", a + b, a - b, a * b, a / b);
    println!("{}", 0.1 + 0.2);
    println!("{}", 1.0 / 3.0);
    println!("{}", 100.0f64);
    println!("{:?}", 100.0f64);
    println!("{}", 1e10f64);
    println!("{}", 1.5e-3f64);
    println!("{}", (16.0f64).sqrt());
    println!("{}", (2.7f64).floor());
    println!("{}", (2.2f64).ceil());
    println!("{}", (-2.5f64).abs());
    println!("{}", (2.0f64).powi(10));
    println!("{:.4}", std::f64::consts::PI);
    println!("{:.3}", (2.0f64).sqrt());
    println!("{}", f64::MAX > 1e300);
    println!("{}", (0.0f64 / 0.0).is_nan());
    println!("{}", 1.0f64 / 0.0);
    println!("{}", -1.0f64 / 0.0);
    println!("{}", 7.0f64 % 3.0);
    println!("{}", (5.0f64).max(9.0));
    println!("{}", (5.0f64).min(9.0));
    println!("{}", 9.99f64 as u8);
    println!("{}", -1.0f64 as u8);
    println!("{}", 3i32 as f64 / 2.0);
    let mut total = 0.0;
    for i in 1..=10 {
        total += 1.0 / (i as f64);
    }
    println!("{:.6}", total);
    println!("{}", 1.0f64 < 2.0 && 2.0 >= 2.0);
}
