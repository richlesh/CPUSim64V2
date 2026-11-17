use std::cell::RefCell;

fn main() {
    let c = RefCell::new(1);
    let a = c.borrow_mut();
    println!("first {}", *a);
    let b = c.borrow_mut();
    println!("unreachable {}", *b);
}
