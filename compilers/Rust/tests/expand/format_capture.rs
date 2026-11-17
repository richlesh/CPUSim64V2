// captured identifiers, explicit and implicit arguments mixed, repeated use of one argument
#[derive(Debug)]
struct S(i32);

impl S {
    fn show(&self) {
        println!("{self:?} {self:#?}");
    }
}

fn main() {
    let a = 1;
    let b = "two";
    let c = 3.5;
    println!("{a} {b} {c}");
    println!("{a:>4}|{b:<6}|{c:^8.1}|");
    println!("{} {a} {}", 10, 20);
    println!("{0} {0:?} {a} {a:?}", b);
    println!("{b} {}", a + 1);
    let width = 7;
    let prec = 2;
    println!("{c:width$.prec$}|{c:>width$}|{:1$}", 5, width);
    S(1).show();
}
