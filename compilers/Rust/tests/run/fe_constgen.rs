#[derive(Debug, PartialEq)]
struct W<T, const N: usize> {
    a: [T; N],
}
impl<T: Clone, const N: usize> Clone for W<T, N> {
    fn clone(&self) -> Self {
        W { a: self.a.clone() }
    }
}
impl<T: Copy + Default, const N: usize> W<T, N> {
    fn new() -> Self {
        W { a: [T::default(); N] }
    }
    fn len(&self) -> usize {
        N
    }
}
struct V<const N: usize>(pub [u8; N]);
fn main() {
    let w = W { a: [1, 2, 3] };
    let c = w.clone();
    println!("{:?} {}", c, c == w);
    let z: W<i64, 4> = W::new();
    println!("{:?} {}", z, z.len());
    let v = V([1, 2]);
    println!("{}", v.0.len());
    let x = [1, 2, 3];
    let y = [1, 2, 4];
    println!("{} {} {}", x < y, x == y, x.cmp(&y) == std::cmp::Ordering::Less);
    println!("{:?}", x.partial_cmp(&y));
    println!("{}", x <= y && y >= x && x != y);
    let m = [[1, 2], [0, 5]];
    println!("{}", m[0] > m[1]);
    let mx = [3u8, 1].max([2, 9]);
    println!("{:?}", mx);
    let mut v = vec![[2, 1], [1, 9], [1, 2]];
    v.sort();
    println!("{:?}", v);
    println!("{}", ["a", "b"] < ["a", "c"]);
}
