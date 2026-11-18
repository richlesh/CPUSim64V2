fn make_adder(n: i32) -> impl Fn(i32) -> i32 {
    move |x| x + n
}

fn make_counter() -> impl FnMut() -> u32 {
    let mut c = 0;
    move || {
        c += 1;
        c
    }
}

fn compose(f: impl Fn(i32) -> i32, g: impl Fn(i32) -> i32) -> impl Fn(i32) -> i32 {
    move |x| g(f(x))
}

fn main() {
    let a = make_adder(5);
    println!("{}", a(1));
    println!("{}", make_adder(3)(4));
    let mut c = make_counter();
    c();
    c();
    println!("{}", c());
    let h = compose(make_adder(1), |x| x * 10);
    println!("{}", h(2));
}
