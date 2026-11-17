// Fibonacci with u64 overflow checks, checked/wrapping/saturating/overflowing arithmetic
fn fib_checked(n: u32) -> Option<u64> {
    let mut a: u64 = 0;
    let mut b: u64 = 1;
    for _ in 0..n {
        let c = a.checked_add(b)?;
        a = b;
        b = c;
    }
    Some(a)
}

fn fib_wrapping(n: u32) -> u64 {
    let mut a: u64 = 0;
    let mut b: u64 = 1;
    for _ in 0..n {
        let c = a.wrapping_add(b);
        a = b;
        b = c;
    }
    a
}

fn fib_overflowing(n: u32) -> (u64, bool) {
    let mut a: u64 = 0;
    let mut b: u64 = 1;
    let mut over = false;
    for _ in 0..n {
        let (c, o) = a.overflowing_add(b);
        over |= o;
        a = b;
        b = c;
    }
    (a, over)
}

fn first_overflow() -> u32 {
    let mut n = 0;
    while fib_checked(n).is_some() {
        n += 1;
    }
    n
}

fn factorial_checked(n: u64) -> Option<u64> {
    let mut r: u64 = 1;
    for i in 2..=n {
        r = r.checked_mul(i)?;
    }
    Some(r)
}

fn pow_checked(base: i64, exp: u32) -> Option<i64> {
    let mut r: i64 = 1;
    for _ in 0..exp {
        r = r.checked_mul(base)?;
    }
    Some(r)
}

fn main() {
    for n in [0, 1, 2, 10, 50, 90, 93] {
        println!("fib({}) = {:?}", n, fib_checked(n));
    }
    println!("first n whose fib overflows u64: {}", first_overflow());
    println!("{}", fib_wrapping(94));
    println!("{:?}", fib_overflowing(93));
    println!("{:?}", fib_overflowing(94));
    println!("{:?} {:?}", factorial_checked(20), factorial_checked(21));
    println!("{:?} {:?}", pow_checked(2, 62), pow_checked(2, 63));
    println!("{:?}", pow_checked(-2, 63));
    println!("{}", u64::MAX.saturating_add(1));
    println!("{}", i64::MIN.saturating_sub(1));
    println!("{}", i64::MAX.wrapping_add(1));
    println!("{:?}", (-7i64).checked_rem(0));
    println!("{}", u32::MAX.wrapping_mul(u32::MAX));
    println!("{:?}", 255u8.overflowing_mul(2));
    println!("{}", (u64::MAX / 2 + 1).wrapping_mul(2));
    println!("{:?}", 12345678901234u64.checked_mul(1000000));
    println!("{:?}", 12345678901u64.checked_mul(1000000));
}
