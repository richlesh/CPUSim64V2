type A = fn();
type B = fn(i32, i32) -> i32;
type C = fn(x: i32, _: u8, &str,) -> !;
type D = unsafe extern "C" fn(*const u8, ...) -> i32;
type E = extern fn(a: u8);
type F = unsafe fn();
type G = for<'a> fn(&'a u8) -> &'a u8;
type H = fn(fn(i32) -> i32) -> fn() -> u8;
type I = Box<dyn Fn(i32, i32) -> i32>;
type J = Box<dyn Fn(&str) -> Vec<u8> + Send + 'static>;
type K = Box<dyn A + B + 'a>;
type L = Box<dyn for<'a> Fn(&'a u8) + Send>;
type N1 = &dyn Trait;
type O = &'a mut dyn Trait;
type P = &(dyn A + B);
type Q = *const dyn Trait;
type R = impl Iterator<Item = u8> + 'a;
type S = impl Trait;
type T1 = Box<Trait + Send>;
type U = Box<for<'a> Fn(&'a u8)>;
type V = Box<dyn (Fn() -> u8) + Send>;
type W = Box<dyn Iterator<Item: Display, Foo = Vec<u8>>>;
type X1 = Box<dyn Fn() -> Box<dyn Fn() -> u8>>;
type Y1 = Box<dyn for<'a, 'b: 'a> Tr<'a, 'b>>;
fn f(x: &dyn A, y: impl A + B) -> impl Iterator<Item = &'a T> + 'a {}
fn g() where F: Fn(u8) -> u8 + Copy {}
