fn main() {
    let a = x.f();
    let b = x.f::<u8>(1, 2);
    let c = x.collect::<Vec<_>>();
    let d = x.0;
    let e = x.0.1;
    let f = x.0.1.foo;
    let g = x?;
    let h = x.await;
    let i = f(1)(2)[3].g()?.h;
    let j = Vec::<i32>::new();
    let k = a.b.c.d(e).f;
    let l = x.iter().map(|v| v + 1).sum::<i32>();
    let m = t.0 .1;
    let n = a[1..];
    let o = a[..2];
    let p = a[..];
    let q = <T as Tr>::f(1);
    let r = self.x;
    let s = Self::new();
    let t = crate::a::b(super::c);
}
