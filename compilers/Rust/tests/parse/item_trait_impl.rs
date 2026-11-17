trait Empty {}
pub trait Sup: Clone + Send + 'static {}
trait Gen<T, U = i32>: Into<T> where U: Copy {
    type Item;
    type Bounded: Clone + Default;
    type WithDefault = i32;
    type Both: Copy = u8;
    type Gat<'a> where Self: 'a;
    const N: usize;
    const D: usize = 4;
    fn required(&self) -> Self::Item;
    fn provided(&self) -> i32 { 1 }
    fn generic<V: Into<T>>(v: V);
    unsafe fn dangerous();
    #[inline]
    fn attr();
    /// doc
    fn documented();
    my_macro!(a, b);
    other! { x }
}
unsafe trait Unsafe {}
auto trait Auto {}
unsafe auto trait UA {}
trait Inner {
    #![allow(dead_code)]
    fn f();
}
impl Foo {}
impl Foo { fn f() {} const C: i32 = 1; type T = i32; pub fn g(&self) {} pub(crate) const D: u8 = 2; }
impl<T> Foo<T> where T: Clone { }
impl<'a, T: 'a, const N: usize> Foo<'a, T, N> {}
impl Trait for Foo {}
impl<T> Trait for Vec<T> {}
impl<T: Display> fmt::Display for Wrapper<T> { fn fmt(&self, f: &mut Formatter) -> Result { Ok(()) } }
impl<T> From<T> for Foo<T> where T: Copy { fn from(t: T) -> Self { Foo } }
unsafe impl Send for Foo {}
unsafe impl<T> Sync for Foo<T> {}
impl !Send for Foo {}
impl<T> !Sync for Foo<T> {}
impl<T> Trait for &T {}
impl Trait for [u8; 4] {}
impl Trait for (i32, i32) {}
impl Foo { #![allow(unused)] fn f() {} }
impl Outer { macro_in_impl!(); }
impl<T> Iterator for Counter<T> {
    type Item = T;
    fn next(&mut self) -> Option<Self::Item> { None }
}
