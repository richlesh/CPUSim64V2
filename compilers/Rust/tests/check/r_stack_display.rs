use std::fmt;
struct Stack<T> { items: Vec<T> }
impl<T> Stack<T> {
    fn new() -> Self { Stack { items: Vec::new() } }
    fn push(&mut self, t: T) { self.items.push(t); }
    fn pop(&mut self) -> Option<T> { self.items.pop() }
    fn peek(&self) -> Option<&T> { self.items.last() }
    fn len(&self) -> usize { self.items.len() }
    fn is_empty(&self) -> bool { self.items.is_empty() }
}
impl<T: fmt::Display> fmt::Display for Stack<T> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "[")?;
        for (i, it) in self.items.iter().enumerate() {
            if i > 0 { write!(f, ", ")?; }
            write!(f, "{}", it)?;
        }
        write!(f, "]")
    }
}
fn main() {
    let mut s: Stack<i32> = Stack::new();
    for i in 1..=4 { s.push(i * i); }
    println!("{} len={} top={:?}", s, s.len(), s.peek());
    while let Some(x) = s.pop() { if x < 5 { break; } }
    let mut names = Stack::new();
    names.push(String::from("a"));
    println!("{} {}", names, s.is_empty());
}
