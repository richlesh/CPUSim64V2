// if/else, while, loop, for, break with value, labels
fn main() {
    let n = 7;
    if n % 2 == 0 {
        println!("even");
    } else if n > 5 {
        println!("odd and big");
    } else {
        println!("odd and small");
    }
    let label = if n < 10 { "small" } else { "large" };
    println!("{}", label);

    let mut i = 0;
    let mut sum = 0;
    while i < 10 {
        sum += i;
        i += 1;
    }
    println!("sum {}", sum);

    let mut k = 1;
    let r = loop {
        k *= 2;
        if k > 100 {
            break k;
        }
    };
    println!("loop {}", r);

    for j in 0..5 {
        if j == 3 {
            continue;
        }
        print!("{} ", j);
    }
    println!();
    for j in (1..=4).rev() {
        print!("{} ", j);
    }
    println!();
    for j in (0..20).step_by(7) {
        print!("{} ", j);
    }
    println!();

    let mut count = 0;
    'outer: for a in 0..10 {
        for b in 0..10 {
            if a * b > 20 {
                break 'outer;
            }
            if b > a {
                continue 'outer;
            }
            count += 1;
        }
    }
    println!("count {}", count);

    let mut x = 27u64;
    let mut steps = 0;
    while x != 1 {
        x = if x % 2 == 0 { x / 2 } else { 3 * x + 1 };
        steps += 1;
    }
    println!("collatz {}", steps);
}
