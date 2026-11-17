macro_rules! empty { () => {}; }
macro_rules! two { ($a:expr) => { $a }; ($a:expr, $b:expr) => { $a + $b }; }
macro_rules! paren ( () => {} );
macro_rules! bracket [ () => {} ];
#[macro_export]
macro_rules! exported { () => {} }
foo!();
foo!(a, b);
foo![1, 2];
foo! { a b c }
foo!{}
a::b::c!(x);
crate::mac!();
::std::println!("hi");
#[derive_all]
mk_struct!(S);
pub struct After;
thread_local! { static X: i32 = 1; }
lazy! {
    static Y: i32 = 2;
}
mod m { inner!(); macro_rules! local { () => {} } }
fn f() {
    macro_rules! in_fn { () => {} }
    struct Inner;
}
