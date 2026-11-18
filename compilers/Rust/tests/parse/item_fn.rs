fn plain() {}
fn with_args(a: i32, b: &str, (c, d): (u8, u8), mut e: u64,) -> i32 { 0 }
pub fn public() -> bool { true }
pub(crate) fn in_crate() {}
pub(super) fn in_super() {}
pub(self) fn in_self() {}
pub(in crate::a::b) fn in_path() {}
const fn const_fn() -> usize { 3 }
async fn async_fn() {}
unsafe fn unsafe_fn() {}
const unsafe fn const_unsafe() {}
async unsafe fn async_unsafe() {}
extern "C" fn c_abi(x: i32) -> i32 { x }
extern fn bare_extern() {}
pub unsafe extern "C" fn all_quals() {}
fn generic<T, 'a, const N: usize>(x: &'a [T; N]) -> &'a T where T: Copy { x }
fn ret_impl() -> impl Fn(i32) -> i32 + 'static { loop {} }
fn trailing_where<T>(t: T) -> T
where
    T: Clone + 'static,
    for<'a> &'a T: Copy,
{
    t
}
fn never() -> ! { loop {} }
fn nested_generics(v: Vec<Vec<i32>>) -> Option<Vec<Vec<u8>>> { None }
