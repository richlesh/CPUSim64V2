use std::rc::Rc;

enum List {
    Cons(i32, Rc<List>),
    Nil,
}
use List::{Cons, Nil};

fn sum(l: &Rc<List>) -> i32 {
    match &**l {
        Cons(v, rest) => *v + sum(rest),
        Nil => 0,
    }
}

fn to_vec(l: &Rc<List>) -> Vec<i32> {
    let mut out = Vec::new();
    let mut cur = l.clone();
    loop {
        let next = match &*cur {
            Cons(v, rest) => {
                out.push(*v);
                rest.clone()
            }
            Nil => break,
        };
        cur = next;
    }
    out
}

fn main() {
    let tail = Rc::new(Cons(3, Rc::new(Cons(4, Rc::new(Nil)))));
    println!("tail count = {}", Rc::strong_count(&tail));
    let a = Rc::new(Cons(1, tail.clone()));
    println!("after a: {}", Rc::strong_count(&tail));
    let b = Rc::new(Cons(2, tail.clone()));
    println!("after b: {}", Rc::strong_count(&tail));
    {
        let c = Rc::new(Cons(5, tail.clone()));
        println!("in scope: {} sum c = {}", Rc::strong_count(&tail), sum(&c));
    }
    println!("after scope: {}", Rc::strong_count(&tail));
    println!("a = {:?} sum {}", to_vec(&a), sum(&a));
    println!("b = {:?} sum {}", to_vec(&b), sum(&b));
    drop(a);
    println!("after drop a: {}", Rc::strong_count(&tail));
    drop(b);
    println!("after drop b: {}", Rc::strong_count(&tail));
    let mut long = Rc::new(Nil);
    for i in 0..100 {
        long = Rc::new(Cons(i, long));
    }
    println!("long sum = {}", sum(&long));
}
