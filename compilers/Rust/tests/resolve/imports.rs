// use trees, globs, renames, re-exports, chains and `as _`
mod a {
    pub mod b {
        pub fn f() -> i32 { 1 }
        pub struct S { pub v: i32 }
        pub const K: i32 = 7;
        pub trait T { fn t(&self) -> i32; }
        impl T for S { fn t(&self) -> i32 { self.v } }
    }
    pub use self::b::f as g;
    pub use self::b::*;
    pub(crate) fn only_crate() {}
    pub(super) fn only_super() {}
}

mod chain {
    pub use crate::a::g as h;
    pub use crate::chain2::h2;
}
mod chain2 {
    pub use crate::chain::h as h2;
}

use a::b::{self, S, T as _};
use a::{g, K};
use chain::h2;
use std::mem::swap;
use ::core::mem;
use core::option::Option::{Some as Yes, None as No};

enum Dir { Up, Down }
use Dir::*;
use self::Dir::Up as Climb;

fn main() {
    let x = b::f() + g() + K + h2() + chain::h();
    let s = S { v: 3 };
    let t = s.t();
    let d = Up;
    let e = Climb;
    match d { Up => {} Down => {} }
    let mut p = 1;
    let mut q = 2;
    swap(&mut p, &mut q);
    mem::swap(&mut p, &mut q);
    let y = Yes(1);
    let n: Option<i32> = No;
    a::only_crate();
    a::only_super();
    {
        use a::b::f as inner_f;
        fn nested() -> i32 { inner_f() }
        let z = inner_f() + nested();
    }
}
