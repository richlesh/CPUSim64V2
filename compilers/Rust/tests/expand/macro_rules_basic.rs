macro_rules! square {
    ($x:expr) => { $x * $x };
}

macro_rules! max {
    ($a:expr) => { $a };
    ($a:expr, $($rest:expr),+) => {{
        let r = max!($($rest),+);
        if $a > r { $a } else { r }
    }};
}

macro_rules! count {
    () => { 0usize };
    ($head:tt $($tail:tt)*) => { 1usize + count!($($tail)*) };
}

macro_rules! make_fn {
    ($name:ident, $ret:ty, $val:expr) => {
        fn $name() -> $ret { $val }
    };
}

macro_rules! my_vec {
    () => { Vec::new() };
    ($($x:expr),+ $(,)?) => {{
        let mut v = Vec::new();
        $( v.push($x); )+
        v
    }};
}

macro_rules! point {
    ($name:ident { $($field:ident : $ty:ty),* }) => {
        struct $name { $($field: $ty),* }
        impl $name {
            fn describe(&self) -> usize { count!($($field)*) }
        }
    };
}

macro_rules! hashmap {
    ($($k:expr => $v:expr),* $(,)?) => {{
        let mut m = HashMap::new();
        $( m.insert($k, $v); )*
        m
    }};
}

macro_rules! op {
    ($a:literal plus $b:literal) => { $a + $b };
    ($a:literal minus $b:literal) => { $a - $b };
}

macro_rules! pat_test {
    ($p:pat) => { match 5 { $p => true, _ => false } };
}

macro_rules! block_test {
    ($b:block) => { { let r = $b; r + 1 } };
}

macro_rules! stmts {
    ($($s:stmt;)*) => { $($s;)* };
}

macro_rules! lifetimes {
    ($l:lifetime) => { fn lt<$l>(x: &$l i32) -> &$l i32 { x } };
}

macro_rules! vis_test {
    ($v:vis struct $n:ident;) => { $v struct $n; };
}

macro_rules! nested {
    ($($outer:ident: [$($inner:expr),*]);*) => {
        $( let $outer = [$($inner),*]; )*
    };
}

macro_rules! tycall {
    ($t:ty) => { <$t>::default() };
}

make_fn!(five, i32, 5);
make_fn!(name, &'static str, "five");
point!(Pt { x: i32, y: i32, z: i32 });
lifetimes!('a);
vis_test!(pub struct Unit;);

fn main() {
    let a = square!(3 + 1);
    let m = max!(1, 5, 3);
    let n = count!(a b c d);
    let v = my_vec![1, 2, 3,];
    let e: Vec<i32> = my_vec![];
    let h = hashmap!{ "a" => 1, "b" => 2 };
    let s = op!(1 plus 2) + op!(5 minus 3);
    let p = pat_test!(1..=9);
    let q = block_test!({ 10 });
    stmts! { let x = 1; let y = x + 2; }
    nested!(aa: [1, 2]; bb: [3]);
    let d = tycall!(u8);
    println!("{}", square!(2));
}
