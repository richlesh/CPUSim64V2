enum E { A(i32, i32) }
fn main() {
    let e = E::A(1, 2);
    match e { E::A(x) => {} }
}
