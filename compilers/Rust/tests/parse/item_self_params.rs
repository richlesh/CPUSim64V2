struct S;
impl S {
    fn a(self) {}
    fn b(mut self) {}
    fn c(&self) {}
    fn d(&mut self) {}
    fn e(&'a self) {}
    fn f(&'a mut self, x: i32) {}
    fn g(self: Box<Self>) {}
    fn h(mut self: Self, y: u8) {}
    fn i(self: &Self) {}
    fn j(&self, other: &Self) -> bool { true }
    fn k(x: i32, self_like: i32) {}
    fn l(&mut x: &mut i32) {}
}
trait T {
    fn m(&self);
    fn n(&mut self, v: Vec<i32>) -> Option<i32>;
    fn o(self) -> Self where Self: Sized;
}
