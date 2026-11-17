fn main() {
    let a = 1;
    let mut b = 2;
    let s = String::from("x");
    let t = String::from("y");
    let f1 = || a + 1;
    let mut f2 = || b += 1;
    let f3 = || s.len();
    let f4 = move || t.len();
    let u = String::from("z");
    let f5 = || { let v = u; v.len() };
    let f6 = |x: i32| { let g = |y: i32| x + y + a; g(1) };
    f2();
}
