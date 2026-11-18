trait T { fn f(&self); }
struct S;
fn g<X: T>(x: X) { x.f(); }
fn main() {
    g(S);
}
