// expect on None prints the message
fn lookup(k: u32) -> Option<&'static str> {
    match k {
        1 => Some("one"),
        2 => Some("two"),
        _ => None,
    }
}

fn main() {
    println!("{}", lookup(1).expect("key 1 must exist"));
    println!("{}", lookup(5).expect("key 5 must exist"));
}
