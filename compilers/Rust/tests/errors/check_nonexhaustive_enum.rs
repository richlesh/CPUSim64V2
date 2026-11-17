enum E { A, B, C }
fn f(e: E) -> i32 {
    match e {
        E::A => 1,
        E::B => 2,
    }
}
fn main() { f(E::A); }
