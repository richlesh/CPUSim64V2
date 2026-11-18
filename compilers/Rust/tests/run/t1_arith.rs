// integer and float arithmetic, casts, let, shadowing
fn main() {
    let a = 17;
    let b = 5;
    println!("{} {} {} {} {}", a + b, a - b, a * b, a / b, a % b);
    println!("{} {}", -a / b, -a % b);
    let c: i64 = 1 << 40;
    println!("{}", c);
    let d: u8 = 250;
    println!("{}", d as i8);
    println!("{}", -1i32 as u32);
    println!("{}", 300i32 as u8);
    println!("{}", 3.99f64 as i32);
    println!("{}", -3.99f64 as i32);
    let x = 10;
    let x = x * 2;
    let x = x + 1;
    println!("{}", x);
    let f = 7.0 / 2.0;
    println!("{}", f);
    println!("{}", 1.5 + 2.25);
    println!("{}", (2.0f64).sqrt() * (2.0f64).sqrt() > 1.99);
    println!("{} {}", 0xffu32, 0b1010_1010u8);
    println!("{} {}", 5 & 3, 5 | 3);
    println!("{} {}", 5 ^ 3, !5i32);
    println!("{} {}", 1u32 << 31, -16i32 >> 2);
    let mut n = 3;
    n += 4;
    n *= 2;
    n -= 1;
    n /= 3;
    n %= 3;
    println!("{}", n);
    println!("{}", i32::MAX);
    println!("{}", i64::MIN);
    println!("{}", u64::MAX);
    println!("{}", 'a' as u32 + 1);
    println!("{}", (b'a' + 2) as char);
}
