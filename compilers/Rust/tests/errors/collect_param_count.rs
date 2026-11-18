trait T { fn f(&self, a: i32); }
struct S;
impl T for S {
    fn f(&self) {}
}
fn main() {}
