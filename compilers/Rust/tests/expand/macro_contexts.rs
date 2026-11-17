// macros in every position: items, impl items, trait items, statements, tail,
// patterns, types, nested modules, scoping, #[macro_export], macro-defining macros
macro_rules! getter {
    ($name:ident, $field:ident, $t:ty) => {
        fn $name(&self) -> $t { self.$field }
    };
}

macro_rules! ty_of {
    () => { i64 };
}

macro_rules! one_pat {
    () => { Some(1) | Some(2) };
}

macro_rules! tail_value {
    ($x:expr) => {
        let y = $x;
        y * 2
    };
}

macro_rules! define_adder {
    ($name:ident, $n:expr) => {
        macro_rules! $name {
            ($v:expr) => { $v + $n };
        }
    };
}

#[macro_export]
macro_rules! exported {
    ($x:expr) => { $x + 100 };
}

struct S { a: i32, b: i64 }

impl S {
    getter!(get_a, a, i32);
    getter!(get_b, b, ty_of!());
}

trait T {
    fn f(&self) -> i32;
}

macro_rules! impl_t {
    ($($t:ty),*) => { $(impl T for $t { fn f(&self) -> i32 { 1 } })* };
}
impl_t!(u8, u16, u32);

define_adder!(add5, 5);
define_adder!(add7, 7);

mod inner {
    pub fn g() -> i32 { exported!(1) + crate::exported!(2) }
    macro_rules! local_only { () => { 42 }; }
    pub fn h() -> i32 { local_only!() }
}

#[macro_use]
mod with_use {
    macro_rules! from_mod { () => { 7 }; }
}

fn tail() -> i32 {
    tail_value!(3 + 4)
}

fn nested_block() -> i32 {
    macro_rules! scoped { () => { 9 }; }
    let r = { scoped!() };
    r + scoped!()
}

fn pats(o: Option<i32>) -> bool {
    match o {
        one_pat!() => true,
        _ => false,
    }
}

fn types() -> Vec<ty_of!()> {
    let x: ty_of!() = 5;
    vec![x]
}

fn main() {
    let s = S { a: 1, b: 2 };
    let t = add5!(1) + add7!(2) + from_mod!();
    let u = tail();
    println!("{} {} {}", t, u, nested_block());
    if matches!(s.get_a(), 1) {
        println!("yes {}", vec![1, 2].len());
    }
    for i in vec![1, 2, 3] {
        println!("{}", i);
    }
    let z = [format!("a"), format!("b")];
    let c = || println!("closure {}", 1);
    let r = (println!("x"), vec![1u8; 3]);
}
