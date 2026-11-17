trait T { fn f(&self); }
struct S;
impl T for S {
    const f: i32 = 1;
}
fn main() {}
