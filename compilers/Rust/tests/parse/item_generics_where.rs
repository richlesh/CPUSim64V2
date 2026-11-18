fn a<T>() {}
fn b<T,>() {}
fn c<T: Clone>() {}
fn d<T: Clone + Copy + 'static>() {}
fn e<T: ?Sized>() {}
fn f<'a>() {}
fn g<'a, 'b: 'a>() {}
fn h<'a, 'b: 'a + 'static, T>() {}
fn i<T = i32>() {}
fn j<T: Clone = i32>() {}
fn k<const N: usize>() {}
fn l<const N: usize = 3>() {}
fn m<const N: i32 = -1>() {}
fn n<const B: bool = true>() {}
fn o<const N: usize = { 1 + 2 }>() {}
fn p<const N: usize = X>() {}
fn q<'a, T: 'a, const N: usize>() {}
fn r<T: for<'a> Fn(&'a i32) -> &'a i32>() {}
fn s<T: Iterator<Item = i32>>() {}
fn t<T: Into<Vec<Vec<u8>>>>() {}
fn u<T:>() {}
fn v<F>(f: F) where F: Fn(i32) -> i32 {}
fn w<T, U>() where T: Clone, U: Copy {}
fn x<T>() where T: Clone, {}
fn y<'a, T>() where 'a: 'static, T: 'a {}
fn z<'a, 'b>() where 'a: 'b, 'b: {}
fn aa<T>() where for<'a> T: Fn(&'a i32) {}
fn bb<T>() where T::Item: Clone {}
fn cc<T>() where Vec<T>: Clone, [T; 3]: Copy, &'static T: Sized {}
fn dd<T>() where T: {}
fn ee<T>() where T: ?Sized + Clone {}
fn ff<T>() -> T where T: Default { T::default() }
fn gg<T>() where for<'a, 'b> T: Fn(&'a i32, &'b i32) -> bool {}
