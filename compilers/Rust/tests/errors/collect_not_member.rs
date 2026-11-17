trait T { fn f(&self); }
struct S;
impl T for S {
    fn f(&self) {}
    fn g(&self) {}
}
fn main() {}
