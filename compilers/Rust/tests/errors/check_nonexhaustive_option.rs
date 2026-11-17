fn main() {
    let o: Option<i32> = Some(1);
    match o {
        Some(x) => println!("{}", x),
    }
}
