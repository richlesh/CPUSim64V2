// closures: capture by ref, by mut ref, by move; passing and returning closures
fn apply<F: Fn(i32) -> i32>(f: F, x: i32) -> i32 {
    f(x)
}

fn apply_twice<F: Fn(i32) -> i32>(f: F, x: i32) -> i32 {
    f(f(x))
}

fn call_mut<F>(mut f: F, n: usize)
where
    F: FnMut(),
{
    for _ in 0..n {
        f();
    }
}

fn consume<F: FnOnce() -> String>(f: F) -> String {
    f()
}

fn make_adder(n: i32) -> Box<dyn Fn(i32) -> i32> {
    Box::new(move |x| x + n)
}

fn make_boxed(kind: u8) -> Box<dyn Fn(i32) -> i32> {
    match kind {
        0 => Box::new(|x| x + 1),
        1 => Box::new(|x| x * x),
        _ => Box::new(|x| -x),
    }
}

fn compose<A, B, C, F: Fn(A) -> B, G: Fn(B) -> C>(f: F, g: G) -> impl_free::Composed<F, G> {
    impl_free::Composed { f, g }
}

mod impl_free {
    pub struct Composed<F, G> {
        pub f: F,
        pub g: G,
    }
    impl<F, G> Composed<F, G> {
        pub fn call<A, B, C>(&self, x: A) -> C
        where
            F: Fn(A) -> B,
            G: Fn(B) -> C,
        {
            (self.g)((self.f)(x))
        }
    }
}

fn main() {
    let k = 10;
    let add_k = |x| x + k;
    println!("{}", apply(add_k, 5));
    println!("{}", apply(|x| x * 3, 5));
    println!("{}", apply_twice(|x| x * 3, 5));

    let mut count = 0;
    call_mut(|| count += 2, 5);
    println!("{}", count);

    let mut log = Vec::new();
    let mut record = |s: &str| log.push(s.to_string());
    record("a");
    record("b");
    println!("{:?}", log);

    let name = String::from("owned");
    let greet = move || format!("hi {}", name);
    println!("{}", consume(greet));

    let add5 = make_adder(5);
    println!("{} {}", add5(1), add5(2));
    let fs: Vec<Box<dyn Fn(i32) -> i32>> = (0..3).map(make_boxed).collect();
    for f in fs.iter() {
        print!("{} ", f(7));
    }
    println!();
    let inc_then_double = compose(|x: i32| x + 1, |x: i32| x * 2);
    println!("{}", inc_then_double.call(4));
    let to_str = compose(|x: i32| x * x, |x: i32| format!("<{}>", x));
    println!("{}", to_str.call(9));

    let mut fib = {
        let mut a = 0u64;
        let mut b = 1u64;
        move || {
            let r = a;
            a = b;
            b += r;
            r
        }
    };
    let mut seq = Vec::new();
    for _ in 0..10 {
        seq.push(fib());
    }
    println!("{:?}", seq);

    fn local_helper(x: i32) -> i32 {
        x - 1
    }
    fn neg(x: f64) -> f64 {
        -x
    }
    let fp: fn(i32) -> i32 = local_helper;
    println!("{}", apply(fp, 10));
    println!("{:?}", [1, 2, 3].iter().map(|&x| local_helper(x)).collect::<Vec<_>>());
    let table: [fn(f64) -> f64; 2] = [f64::sqrt, neg];
    for f in table.iter() {
        println!("{}", f(16.0));
    }
}
