// sieve of Eratosthenes, prime factorization, twin primes
fn sieve(n: usize) -> Vec<bool> {
    let mut is = vec![true; n + 1];
    is[0] = false;
    if n >= 1 {
        is[1] = false;
    }
    let mut i = 2;
    while i * i <= n {
        if is[i] {
            let mut j = i * i;
            while j <= n {
                is[j] = false;
                j += i;
            }
        }
        i += 1;
    }
    is
}

fn factorize(mut n: u64) -> Vec<(u64, u32)> {
    let mut out = Vec::new();
    let mut p = 2;
    while p * p <= n {
        if n % p == 0 {
            let mut e = 0;
            while n % p == 0 {
                n /= p;
                e += 1;
            }
            out.push((p, e));
        }
        p += 1;
    }
    if n > 1 {
        out.push((n, 1));
    }
    out
}

fn show_factors(n: u64) -> String {
    let parts: Vec<String> = factorize(n)
        .iter()
        .map(|&(p, e)| if e == 1 { p.to_string() } else { format!("{}^{}", p, e) })
        .collect();
    parts.join(" * ")
}

fn main() {
    let limit = 10000;
    let is = sieve(limit);
    let primes: Vec<usize> = (0..=limit).filter(|&i| is[i]).collect();
    println!("{} primes up to {}", primes.len(), limit);
    println!("{:?}", &primes[..15]);
    println!("last {}", primes[primes.len() - 1]);
    let mut twins = 0;
    for w in primes.windows(2) {
        if w[1] - w[0] == 2 {
            twins += 1;
        }
    }
    println!("twin pairs {}", twins);
    let mut max_gap = 0;
    let mut at = 0;
    for w in primes.windows(2) {
        if w[1] - w[0] > max_gap {
            max_gap = w[1] - w[0];
            at = w[0];
        }
    }
    println!("max gap {} after {}", max_gap, at);
    let sum: usize = primes.iter().sum();
    println!("sum {}", sum);
    for n in [360u64, 97, 1001, 65536, 600851475143] {
        println!("{} = {}", n, show_factors(n));
    }
}
