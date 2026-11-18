trait T { fn f(&self, a: i32) -> bool; }
struct S;
impl T for S {
    fn f(&self, a: u8) -> bool { true }
}
fn main() {}
