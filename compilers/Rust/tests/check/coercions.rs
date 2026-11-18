trait Tr { fn f(&self) -> i32; }
struct S;
impl Tr for S { fn f(&self) -> i32 { 1 } }
fn takes_str(s: &str) -> usize { s.len() }
fn takes_slice(s: &[i32]) -> usize { s.len() }
fn takes_ref(s: &i32) -> i32 { *s }
fn takes_dyn(t: &dyn Tr) -> i32 { t.f() }
fn nv() -> i32 { return 1; }
fn main() {
    let st = String::from("a");
    let v = vec![1];
    let arr = [1, 2];
    let mut n = 5;
    takes_str(&st);
    takes_slice(&v);
    takes_slice(&arr);
    takes_ref(&mut n);
    takes_dyn(&S);
    let b: Box<dyn Tr> = Box::new(S);
    let r = &&st;
    takes_str(r);
    let x: i32 = if n > 1 { 5 } else { panic!() };
    st.len();
    arr.len();
    let bx = Box::new(S);
    bx.f();
}
