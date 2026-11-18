// u8 overflow through an `as` cast result plus arithmetic
fn bump(x: u8) -> u8 {
    x + 100
}

fn main() {
    let mut v: u8 = 50;
    for _ in 0..5 {
        v = bump(v);
        println!("{}", v);
    }
}
