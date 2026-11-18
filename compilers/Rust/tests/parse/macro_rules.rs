macro_rules! square {
    ($x:expr) => {
        $x * $x
    };
}

macro_rules! maxm {
    ($x:expr) => ($x);
    ($x:expr, $($rest:expr),+) => {{
        let a = $x;
        let b = maxm!($($rest),+);
        if a > b { a } else { b }
    }};
}

macro_rules! make_struct {
    ($name:ident { $($field:ident : $ty:ty),* $(,)? }) => {
        struct $name {
            $($field: $ty),*
        }
    };
}

macro_rules! unit [
    () => [ () ];
];

macro_rules! paren_form (
    ($a:tt) => { $a };
);

make_struct!(Pt { x: i32, y: i32 });

fn main() {
    let a = square!(3);
    let b = maxm!(1, 2, 3);
    let p = Pt { x: 1, y: 2 };
    println!("{} {} {}", a, b, p.x + p.y);
}
