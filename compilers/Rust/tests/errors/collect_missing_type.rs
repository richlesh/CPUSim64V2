trait T { type Out; fn f(&self) -> i32; }
struct S;
impl T for S {
    fn f(&self) -> i32 { 1 }
}
fn main() {}
