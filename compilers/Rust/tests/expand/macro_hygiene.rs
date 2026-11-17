macro_rules! add_one_to {
    ($v:expr) => {{
        let x = 1;
        $v + x
    }};
}

macro_rules! swap_sum {
    ($a:expr, $b:expr) => {{
        let tmp = $a;
        let (p, q) = (tmp, $b);
        p + q
    }};
}

macro_rules! each {
    ($v:expr, $f:expr) => {{
        let mut total = 0;
        for item in $v {
            total += $f(item);
        }
        total
    }};
}

macro_rules! with_closure {
    ($n:expr) => {{
        let base = $n;
        let add = |k: i32| k + base;
        add(1)
    }};
}

macro_rules! declare {
    ($name:ident) => { let $name = 7; };
}

macro_rules! max_of {
    ($a:expr) => { $a };
    ($a:expr, $($rest:expr),+) => {{
        let r = max_of!($($rest),+);
        if $a > r { $a } else { r }
    }};
}

macro_rules! named {
    () => {{
        let value = 3;
        (stringify!(value), value)
    }};
}

fn main() {
    let x = 10;
    let a = add_one_to!(x);
    let tmp = 5;
    let b = swap_sum!(tmp, tmp * 2);
    let item = 2;
    let total = each!(vec![1, 2, 3], |n| n * item);
    let base = 100;
    let c = with_closure!(base);
    declare!(visible);
    let r = 3;
    let m = max_of!(r, 9, 4);
    let nm = named!();
}
