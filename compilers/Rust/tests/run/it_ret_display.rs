use std::fmt::Display;

fn label() -> impl Display {
    42
}

fn name(b: bool) -> impl Display {
    if b {
        String::from("yes")
    } else {
        String::from("no")
    }
}

fn pair<T: Display>(x: T) -> impl Display {
    format!("[{}]", x)
}

fn main() {
    println!("{}", label());
    let l = label();
    println!("{} {}", l, l);
    println!("{}", name(true));
    println!("{}", name(false));
    println!("{}", pair(7));
    println!("{}", pair("s"));
}
