struct S;
impl S { fn m(&self, a: i32) {} }
fn main() {
    let s = S;
    s.m();
}
