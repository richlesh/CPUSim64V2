use std::fmt::Debug;
enum Tok { Num(i32), Plus }
impl Debug for Tok { fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result { f.write_str("tok") } }
trait Maker { fn make() -> Self; fn id(&self) -> u32; }
struct A; struct B;
impl Maker for A { fn make() -> A { A } fn id(&self) -> u32 { 1 } }
impl Maker for B { fn make() -> B { B } fn id(&self) -> u32 { 2 } }
fn build<T: Maker>() -> T { T::make() }
fn build2<T: Maker>() -> T { <T as Maker>::make() }
struct Pt { x: i32 }
impl Pt {
    fn origin() -> Self { Self { x: 0 } }
    fn twice() -> Self { let p = Self::origin(); Pt { x: p.x + 2 } }
}
fn show<T: Debug>(t: &T) -> String { format!("{:?}", t) }
fn get(o: Option<i32>) -> i32 {
    let Some(v) = o else { return -1; };
    v
}
fn main() {
    let toks: Vec<Tok> = vec![1, 2].into_iter().map(Tok::Num).collect();
    let somes: Vec<Option<i32>> = vec![1, 2].into_iter().map(Some).collect();
    println!("{} {}", toks.len(), somes.len());
    let a: A = build();
    let b = build2::<B>();
    println!("{} {}", a.id(), Maker::id(&b));
    println!("{}", <A as Maker>::id(&a) + A::make().id());
    println!("{}", Pt::twice().x);
    println!("{}", show(&Tok::Plus));
    println!("{}", get(Some(3)));
    let p: *const i32 = &5;
    let x = 5;
    let q = &x as *const i32;
    let r = 3.7f64 as i32;
    let t = true as i32 + 'a' as i32;
    println!("{} {}", r, t);
    let nested = ((1, 2), [3, 4]);
    let ((n1, _), [_, n4]) = nested;
    println!("{}", n1 + n4);
    let v = vec![Some(1), None, Some(3)];
    let flat: Vec<i32> = v.iter().filter_map(|x| *x).collect();
    let total: i32 = v.iter().flatten_sum();
    println!("{:?}", flat);
}
trait FlattenSum { fn flatten_sum(self) -> i32; }
impl<'a, I: Iterator<Item = &'a Option<i32>>> FlattenSum for I {
    fn flatten_sum(self) -> i32 { let mut t = 0; for x in self { if let Some(v) = x { t += v; } } t }
}
