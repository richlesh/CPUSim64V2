// generics, traits, impls, Self, qualified paths, where clauses, const generics
use std::ops::Add;
trait Shape { const SIDES: i32; type Unit; fn area(&self) -> Self::Unit; fn name() -> i32 { Self::SIDES } }
struct Sq<T>(T);
struct Arr<const N: usize> { d: [i32; N] }
impl<const N: usize> Arr<N> { fn len(&self) -> usize { N } fn new() -> Self { Arr { d: [0; N] } } }
impl<T: Add<Output = T> + Copy> Shape for Sq<T> where T: Sized {
    const SIDES: i32 = 4;
    type Unit = T;
    fn area(&self) -> T { self.0 + self.0 }
}
trait Named: Sized { fn make() -> Self; fn dup(self) -> (Self, Self) where Self: Copy { (self, self) } }
fn apply<F: Fn(i32) -> i32, G>(f: F, g: G) -> i32 where G: Fn(i32) -> i32 { g(f(1)) }
fn dynamic(x: &dyn Shape<Unit = i32>, y: impl Add<i32>, z: fn(i32) -> i32, w: *const i32, v: [u8; 4], s: &[u8]) {}
fn qualified<T: Shape>(t: &T) -> <T as Shape>::Unit { t.area() }
fn assoc<T: Shape>() -> i32 { T::SIDES + <T as Shape>::SIDES + T::name() }
fn selfy() -> Sq<i32> { let s = Sq::<i32>(3); Sq(4) }
struct Holder<T = i32> { t: T }
fn main() {
    let a = Arr::<3>::new();
    let n = a.len();
    let k = assoc::<Sq<i32>>();
    let f = apply(|x| x + 1, |y| y * 2);
    let m = <Sq<i32> as Shape>::SIDES;
    let sz = std::mem::size_of::<u64>();
}
