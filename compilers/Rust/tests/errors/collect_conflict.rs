trait T { fn f(&self); }
struct S;
impl T for S { fn f(&self) {} }
impl T for S { fn f(&self) {} }
fn main() {}
