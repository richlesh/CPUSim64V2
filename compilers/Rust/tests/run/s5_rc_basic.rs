use std::rc::{Rc, Weak};

struct Noisy {
    name: String,
}

impl Drop for Noisy {
    fn drop(&mut self) {
        println!("drop {}", self.name);
    }
}

fn make(name: &str) -> Noisy {
    Noisy { name: name.to_string() }
}

fn main() {
    let a = Rc::new(make("a"));
    println!("strong={} weak={}", Rc::strong_count(&a), Rc::weak_count(&a));
    let b = Rc::clone(&a);
    println!("strong={} name={}", Rc::strong_count(&a), b.name);
    {
        let c = a.clone();
        println!("inner strong={} ptr_eq={}", Rc::strong_count(&c), Rc::ptr_eq(&a, &c));
    }
    println!("after inner strong={}", Rc::strong_count(&a));
    let w: Weak<Noisy> = Rc::downgrade(&a);
    println!("weak={} upgrade={}", Rc::weak_count(&a), w.upgrade().is_some());
    println!("strong after upgrade temp={}", Rc::strong_count(&a));
    drop(a);
    println!("dropped a, strong={}", Rc::strong_count(&b));
    drop(b);
    println!("upgrade after drop: {}", w.upgrade().is_none());

    // values: Display, Debug, comparisons, default
    let x = Rc::new(10);
    let y = Rc::new(20);
    println!("{} {:?} {}", x, y, x < y);
    println!("{}", x == Rc::new(10));
    let d: Rc<i32> = Default::default();
    println!("{}", d);
    let s = Rc::new(String::from("hello"));
    println!("{} len={}", s, s.len());

    // get_mut / try_unwrap / make_mut
    let mut m = Rc::new(1);
    *Rc::get_mut(&mut m).unwrap() += 1;
    let m2 = m.clone();
    println!("get_mut shared: {}", Rc::get_mut(&mut m).is_none());
    drop(m2);
    println!("{:?}", Rc::try_unwrap(m));
    let mut v = Rc::new(vec![1, 2, 3]);
    let v2 = v.clone();
    Rc::make_mut(&mut v).push(4);
    println!("{:?} {:?} {}", v, v2, Rc::ptr_eq(&v, &v2));
    let t = Rc::new(make("t"));
    let t2 = t.clone();
    match Rc::try_unwrap(t) {
        Ok(_) => println!("unwrapped"),
        Err(back) => println!("still shared {}", Rc::strong_count(&back)),
    }
    let e: Weak<i32> = Weak::new();
    println!("{}", e.upgrade().is_none());
    println!("end {}", t2.name);
}
