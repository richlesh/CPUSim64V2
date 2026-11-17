struct S { v: String }
impl Clone for S { fn clone(&self) -> S { S { v: self.v.clone() } } }
impl Copy for S {}
fn main() {}
