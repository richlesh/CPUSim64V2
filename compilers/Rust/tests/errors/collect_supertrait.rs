trait Base { fn b(&self); }
trait Sub: Base { fn s(&self); }
struct S;
impl Sub for S { fn s(&self) {} }
fn main() {}
