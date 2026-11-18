struct Unit;
struct Empty {}
struct Point { x: i32, y: i32 }
struct Trailing { x: i32, y: i32, }
pub struct Vis { pub x: i32, pub(crate) y: i32, pub(super) z: i32, w: i32 }
struct Pair(i32, i64);
struct PairVis(pub i32, pub(crate) T, pub (A, B));
struct One(i32,);
struct EmptyTuple();
struct Gen<T, U: Clone = i32> { t: T, u: U }
struct GenTuple<T>(T, Vec<T>) where T: Copy;
struct GenWhere<T> where T: Clone { t: T }
struct GenUnit<T>;
struct Life<'a, 'b: 'a> { r: &'a i32, s: &'b mut i32 }
struct Arr<const N: usize> { a: [u8; N] }
struct Attrs {
    #[allow(dead_code)]
    a: i32,
    /// doc on field
    pub b: i32,
}
enum E { A, B, C }
enum Trail { A, B, }
enum Data {
    Unit,
    Tuple(i32, String),
    Named { x: i32, y: i32 },
    Empty {},
    EmptyT(),
    #[doc = "hidden"] Attr(u8),
    Disc = 5,
    Neg = -1,
}
enum Gen2<T, E> { Ok(T), Err(E) }
enum WithWhere<T> where T: Clone { V(T) }
enum Discr { A = 1, B = 2, C }
pub enum PubEnum { V(pub i32) }
union U { a: u32, b: f32 }
union Gen3<T: Copy> where T: Sized { t: T }
fn uses_union_ident() { let union = 3; }
