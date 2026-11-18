macro_rules! calc {
    (@go $acc:expr;) => { $acc };
    (@go $acc:expr; plus $n:literal $($rest:tt)*) => { calc!(@go $acc + $n; $($rest)*) };
    (@go $acc:expr; times $n:literal $($rest:tt)*) => { calc!(@go ($acc) * $n; $($rest)*) };
    ($start:literal $($rest:tt)*) => { calc!(@go $start; $($rest)*) };
}

macro_rules! enum_str {
    ($(#[$meta:meta])* $vis:vis enum $name:ident { $($variant:ident = $val:expr),+ $(,)? }) => {
        $(#[$meta])*
        $vis enum $name { $($variant = $val),+ }
        impl $name {
            fn name(&self) -> &'static str {
                match self { $($name::$variant => stringify!($variant)),+ }
            }
        }
    };
}

macro_rules! opt {
    ($a:expr $(, $b:expr)?) => { $a $(+ $b)? };
}

macro_rules! matrix {
    ($([$($x:expr),*]),*) => { [$([$($x),*]),*] };
}

macro_rules! kv {
    ($($k:ident = $v:expr);* $(;)?) => { $(let $k = $v;)* };
}

macro_rules! tuple_sum {
    ($($i:tt : $t:ident),*) => { |t: ($($t,)*)| 0 $(+ t.$i)* };
}

macro_rules! strs {
    ("a" => $x:expr) => { $x };
    ("b" => $x:expr) => { $x + 1 };
}

macro_rules! chars {
    ('x') => { 1 };
    ('y') => { 2 };
}

macro_rules! with_ty {
    ($v:ident : $t:ty = $e:expr) => { let $v: $t = $e; };
}

macro_rules! call_with {
    ($f:path, $($a:expr),*) => { $f($($a),*) };
}

macro_rules! items {
    ($($i:item)*) => { $($i)* };
}

macro_rules! sum_stmts {
    ($($s:stmt),*) => { $($s;)* };
}

macro_rules! forward {
    ($($t:tt)*) => { inner_fwd!($($t)*) };
}
macro_rules! inner_fwd {
    ($a:expr, $b:expr) => { $a * $b };
}

macro_rules! idents {
    ($($i:ident)*) => { [$(stringify!($i)),*] };
}

macro_rules! early {
    ($x:expr) => {
        if $x > 0 { return 1; }
    };
}

enum_str! {
    #[derive(Debug)]
    pub enum Color { Red = 1, Green = 2, Blue = 3, }
}
items! {
    fn one() -> i32 { 1 }
    struct W(i32);
}

fn main() {
    let a = calc!(1 plus 2 times 3 plus 4);
    let b = opt!(1);
    let c = opt!(1, 2);
    let m = matrix!([1, 2], [3, 4]);
    kv!(p = 1; q = 2;);
    let f = tuple_sum!(0: i32, 1: i32);
    let s = strs!("a" => 1) + strs!("b" => 1);
    let ch = chars!('x') + chars!('y');
    with_ty!(w: u8 = 3);
    let r = call_with!(std::cmp::max, 1, 2);
    sum_stmts!(let z = 1, let zz = 2);
    let fw = forward!(3, 4);
    let ids = idents!(alpha beta);
    early!(a);
    println!("{}", Color::Red.name());
}
