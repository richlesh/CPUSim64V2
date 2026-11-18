macro_rules! needs_two {
    ($a:expr, $b:expr) => { $a + $b };
}

fn main() {
    let x = needs_two!(1,);
}
