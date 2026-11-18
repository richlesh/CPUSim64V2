struct Counter { n: u32 }
impl Iterator for Counter {
    type Item = u32;
    fn next(&mut self) -> Option<u32> {
        if self.n < 5 { self.n += 1; Some(self.n) } else { None }
    }
}
struct Stack<T> { items: Vec<T> }
impl<T> Stack<T> {
    fn new() -> Stack<T> { Stack { items: Vec::new() } }
    fn push(&mut self, t: T) { self.items.push(t); }
    fn pop(&mut self) -> Option<T> { self.items.pop() }
    fn peek(&self) -> Option<&T> { self.items.last() }
    fn len(&self) -> usize { self.items.len() }
    fn is_empty(&self) -> bool { self.items.is_empty() }
}
trait Animal { fn speak(&self) -> String; }
struct Dog;
struct Cat { lives: u8 }
impl Animal for Dog { fn speak(&self) -> String { "woof".to_string() } }
impl Animal for Cat { fn speak(&self) -> String { format!("meow{}", self.lives) } }
fn describe(a: &dyn Animal) -> String { a.speak() }
fn generic_speak<A: Animal>(a: &A) -> String { a.speak() }
fn main() {
    let c = Counter { n: 0 };
    let s: u32 = c.zip(Counter { n: 2 }).map(|(a, b)| a * b).filter(|x| x % 3 == 0).sum();
    println!("{}", s);
    let mut st: Stack<i32> = Stack::new();
    st.push(1); st.push(2);
    println!("{:?} {}", st.peek(), st.len());
    while let Some(x) = st.pop() { println!("{}", x); }
    let d = Dog; let k = Cat { lives: 9 };
    println!("{} {}", describe(&d), generic_speak(&k));
    let animals: Vec<Box<dyn Animal>> = vec![Box::new(Dog), Box::new(Cat { lives: 7 })];
    for a in animals.iter() { println!("{}", a.speak()); }
    let mut sum = 0;
    for i in 0..10 { if i % 2 == 0 { continue; } sum += i; }
    for (i, v) in [10, 20, 30].iter().enumerate() { sum += i as i32 * *v; }
    println!("{}", sum);
}
