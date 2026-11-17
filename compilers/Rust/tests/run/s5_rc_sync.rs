use std::rc::Rc;
use std::sync::{Arc, Mutex};

fn main() {
    let m = Arc::new(Mutex::new(vec![1, 2]));
    let m2 = Arc::clone(&m);
    m2.lock().unwrap().push(3);
    {
        let mut g = m.lock().unwrap();
        g.push(4);
        println!("{:?}", *g);
    }
    println!("{} {:?}", Arc::strong_count(&m), m.lock().unwrap());
    let c = Arc::new(Mutex::new(0));
    for i in 1..=4 {
        let c = Arc::clone(&c);
        *c.lock().unwrap() += i;
    }
    println!("{}", *c.lock().unwrap());
    let mut r = Rc::new(String::from("x"));
    let w = Rc::downgrade(&r);
    Rc::make_mut(&mut r).push('y');
    println!("{} {}", r, w.upgrade().is_none());
}
