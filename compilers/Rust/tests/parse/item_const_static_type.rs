const A: i32 = 1;
const _: () = ();
const B: [u8; 3] = [1, 2, 3];
pub const C: &str = "x";
pub(crate) const D: usize = 4;
static S: i32 = 5;
static mut M: u32 = 0;
pub static P: &str = "p";
static ARR: [i32; 2] = [1, 2];
type Alias = i32;
type Gen<T> = Vec<T>;
type Bounded<T: Clone> = Option<T>;
type Where<T> where T: Copy = Option<T>;
type Where2<T> = Option<T> where T: Copy;
pub type Pub = (i32, i32);
type Fnp = fn(i32) -> i32;
type Lt<'a> = &'a str;
type Nested = Vec<Vec<Option<i32>>>;
fn f() {
    const INNER: i32 = 1;
    static INNER_S: i32 = 2;
    type Local = u8;
    struct Local2;
    fn nested() {}
    use std::fmt;
    let x = const { 1 };
}
