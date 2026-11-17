struct NoClone;
#[derive(Clone)]
struct P { x: NoClone }
fn main() {}
