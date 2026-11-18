// integer methods: pow, checked, wrapping, saturating, abs, min/max
fn main() {
    println!("{}", 2i32.pow(10));
    println!("{}", 3u64.pow(30));
    println!("{}", (-5i32).abs());
    println!("{}", (-5i32).signum());
    println!("{:?}", 200u8.checked_add(100));
    println!("{:?}", 200u8.checked_add(50));
    println!("{:?}", 5u32.checked_sub(6));
    println!("{:?}", i32::MAX.checked_mul(2));
    println!("{:?}", 10i32.checked_div(0));
    println!("{}", 250u8.wrapping_add(10));
    println!("{}", 0u32.wrapping_sub(1));
    println!("{}", i32::MAX.wrapping_add(1));
    println!("{}", 100u8.saturating_add(200));
    println!("{}", 5u8.saturating_sub(10));
    println!("{}", (-7i32).rem_euclid(3));
    println!("{}", 7u32.abs_diff(10));
    println!("{}", 0b1011u32.count_ones());
    println!("{}", std::cmp::max(3, 9));
    println!("{}", std::cmp::min(3, 9));
    println!("{}", 5.max(2).min(4));
    let (v, o) = 255u8.overflowing_add(1);
    println!("{} {}", v, o);
    println!("{}", u8::MAX as u32 + 1);
    println!("{} {}", i8::MIN, i8::MAX);
    println!("{} {}", i16::MIN, u16::MAX);
    println!("{}", usize::MAX == u64::MAX as usize);
    println!("{:?}", "123".parse::<i32>());
    println!("{:?}", "-9".parse::<i64>());
    println!("{}", "abc".parse::<i32>().is_err());
    println!("{:?}", "2.5".parse::<f64>());
}
