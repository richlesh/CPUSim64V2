trait T { fn f(&self); fn g(&self); fn h(&self) {} }
struct S;
impl T for S {
    fn h(&self) {}
}
fn main() {}
