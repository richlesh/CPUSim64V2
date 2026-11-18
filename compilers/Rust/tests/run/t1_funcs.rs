// functions, recursion, mutable references, early return, consts, statics
const LIMIT: u32 = 10;
static GREETING: &str = "hi";

fn add(a: i32, b: i32) -> i32 {
    a + b
}

fn factorial(n: u64) -> u64 {
    if n <= 1 {
        1
    } else {
        n * factorial(n - 1)
    }
}

fn gcd(a: u32, b: u32) -> u32 {
    if b == 0 {
        a
    } else {
        gcd(b, a % b)
    }
}

fn is_prime(n: u32) -> bool {
    if n < 2 {
        return false;
    }
    let mut d = 2;
    while d * d <= n {
        if n % d == 0 {
            return false;
        }
        d += 1;
    }
    true
}

fn bump(x: &mut i32) {
    *x += 1;
}

fn swap_pair(a: &mut i32, b: &mut i32) {
    let t = *a;
    *a = *b;
    *b = t;
}

fn minmax(a: i32, b: i32) -> (i32, i32) {
    if a < b {
        (a, b)
    } else {
        (b, a)
    }
}

fn is_even(n: u32) -> bool {
    if n == 0 {
        true
    } else {
        is_odd(n - 1)
    }
}

fn is_odd(n: u32) -> bool {
    if n == 0 {
        false
    } else {
        is_even(n - 1)
    }
}

fn main() {
    println!("{}", add(2, 3));
    println!("{}", factorial(15));
    println!("{}", gcd(48, 18));
    let mut primes = 0;
    for i in 0..LIMIT * 10 {
        if is_prime(i) {
            primes += 1;
        }
    }
    println!("primes below 100: {}", primes);
    let mut v = 5;
    bump(&mut v);
    bump(&mut v);
    println!("{}", v);
    let mut p = 1;
    let mut q = 2;
    swap_pair(&mut p, &mut q);
    println!("{} {}", p, q);
    let (lo, hi) = minmax(9, 4);
    println!("{} {}", lo, hi);
    println!("{} {}", is_even(10), is_odd(10));
    println!("{}", GREETING);
}
