fn f() {
    let a = Vec::<u8>::new();
    let b = <T as Default>::default();
    let c = <Vec<u8>>::with_capacity(3);
    let d = ::std::mem::swap::<u8>;
    let e = x as usize + 1;
    let g = p as *const u8 as usize;
    let h = r as &dyn Trait;
    let i = HashMap::<String, Vec<(u8, u8)>>::new();
    let j = Iterator::sum::<i32>(it);
    let k = foo::<{ 3 }, -1, 'a', N>();
    let l = self::helper();
    let m = <<T as A>::B as C>::D::new();
    let n = a as u8 <= b;
    let o: Vec<Vec<u8>>= Vec::new();
}
