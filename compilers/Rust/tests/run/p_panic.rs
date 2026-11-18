// explicit panic! with a formatted message after some output
fn check(n: u32) {
    if n > 2 {
        panic!("value too large: {}", n);
    }
    println!("ok {}", n);
}

fn main() {
    for i in 0..5 {
        check(i);
    }
}
