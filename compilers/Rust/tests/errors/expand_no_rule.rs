macro_rules! pair {
    ($a:ident, $b:ident) => { ($a, $b) };
    ($a:ident) => { $a };
}

fn main() {
    let x = 1;
    let p = pair!(x; x);
}
