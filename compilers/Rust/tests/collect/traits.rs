// collect: generics, bounds, supertraits, associated types, impls, Copy
use std::fmt::Debug;
use std::ops::Add;

trait Shape { const SIDES: i32; type Unit; fn area(&self) -> Self::Unit; fn name() -> i32 { Self::SIDES } }
trait Named: Debug + Clone { fn make() -> Self; fn dup(self) -> (Self, Self) where Self: Copy { (self, self) } }
trait Container { type Item: Clone; type Iter: Iterator<Item = Self::Item>; fn items(&self) -> Self::Iter; }

#[derive(Debug)]
struct Sq<T>(T);
struct Arr<const N: usize> { d: [i32; N] }
struct Holder<T = i32> { t: T }
enum Pick<A, B> { Left(A), Right { b: B }, Neither }
union Bits { i: i32, f: f64 }

impl<const N: usize> Arr<N> {
    fn len(&self) -> usize { N }
    fn new() -> Self { Arr { d: [0; N] } }
    fn first(self: Box<Self>) -> i32 { 0 }
}
impl<T: Add<Output = T> + Copy> Shape for Sq<T> where T: Sized {
    const SIDES: i32 = 4;
    type Unit = T;
    fn area(&self) -> T { self.0 + self.0 }
}
impl<T> Container for Holder<T> where T: Clone {
    type Item = T;
    type Iter = std::iter::Once<T>;
    fn items(&self) -> Self::Iter { std::iter::once(self.t.clone()) }
}
impl Clone for Bits { fn clone(&self) -> Bits { Bits { i: 0 } } }
impl Copy for Bits {}

fn apply<F: Fn(i32) -> i32, G>(f: F, g: G) -> i32 where G: Fn(i32) -> i32 { g(f(1)) }
fn dynamic(x: &dyn Shape<Unit = i32>, y: impl Add<i32>, z: fn(i32) -> i32, w: *const i32, v: [u8; 4], s: &[u8]) {}
fn qualified<T: Shape>(t: &T) -> <T as Shape>::Unit { t.area() }
fn assoc<C: Container>(c: &C) -> C::Item where C::Item: Debug { c.items().next().unwrap() }
fn unsized_ok<T: ?Sized + Debug>(t: &T) {}
fn pair<A, B: Into<A>>(a: A, b: B) -> Pick<A, B> { Pick::Neither }
type Table<K> = Vec<(K, i32)>;
fn use_alias(t: Table<u8>) -> Table<char> { t.len(); Vec::new() }
const LIMIT: usize = 3 * 4;
static NAMES: [&str; 2] = ["a", "b"];
fn main() {}
