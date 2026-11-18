macro_rules! forever {
    ($x:expr) => { forever!($x + 1) };
}

fn main() {
    let n = forever!(0);
}
