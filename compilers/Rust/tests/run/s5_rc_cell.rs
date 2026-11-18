use std::cell::{Cell, RefCell};

struct Counter {
    hits: Cell<u32>,
    last: Cell<i64>,
    log: RefCell<Vec<String>>,
}

impl Counter {
    fn new() -> Counter {
        Counter { hits: Cell::new(0), last: Cell::new(-1), log: RefCell::new(Vec::new()) }
    }
    fn touch(&self, v: i64) {
        self.hits.set(self.hits.get() + 1);
        let old = self.last.replace(v);
        self.log.borrow_mut().push(format!("{}->{}", old, v));
    }
}

fn main() {
    let c = Counter::new();
    for i in 0..5 {
        c.touch(i * i);
    }
    println!("hits={} last={}", c.hits.get(), c.last.get());
    println!("{:?}", c.log.borrow());
    let t = c.hits.take();
    println!("took {} now {}", t, c.hits.get());
    let cell = Cell::new(7);
    println!("{:?}", cell); println!("{}", cell.into_inner());

    let r = RefCell::new(vec![1, 2, 3]);
    println!("{:?}", r);
    {
        let a = r.borrow();
        let b = r.borrow();
        println!("two readers {} {}", a.len(), b.len());
        println!("try_borrow_mut err = {}", r.try_borrow_mut().is_err());
        println!("try_borrow ok = {}", r.try_borrow().is_ok());
    }
    {
        let mut w = r.borrow_mut();
        w.push(4);
        println!("try_borrow err = {}", r.try_borrow().is_err());
    }
    println!("{:?}", r);
    let old = r.replace(vec![9]);
    println!("{:?} {:?}", old, r.borrow());
    let taken = r.take();
    println!("{:?} {:?}", taken, r.borrow());
    println!("{}", r.into_inner().len());
    let n = RefCell::new(String::from("abc"));
    n.borrow_mut().push('d');
    println!("{} {:?}", n.borrow(), n);
    match n.try_borrow_mut() {
        Ok(mut g) => { g.push('e'); println!("ok {}", *g); }
        Err(e) => println!("err {}", e),
    }
    let g = n.borrow();
    match n.try_borrow_mut() {
        Ok(_) => println!("unexpected"),
        Err(e) => println!("err {} / {:?}", e, e),
    }
    println!("{}", g);
}
