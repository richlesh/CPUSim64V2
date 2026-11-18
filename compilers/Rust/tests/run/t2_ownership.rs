// ownership: move, clone, borrow, mutable borrow, Drop order, mem::swap/replace
struct Noisy(&'static str);

impl Drop for Noisy {
    fn drop(&mut self) {
        println!("drop {}", self.0);
    }
}

fn consume(v: Vec<i32>) -> usize {
    v.len()
}

fn longest<'a>(a: &'a str, b: &'a str) -> &'a str {
    if a.len() >= b.len() {
        a
    } else {
        b
    }
}

fn append(s: &mut String, t: &str) {
    s.push_str(t);
}

fn main() {
    let v = vec![1, 2, 3];
    let w = v.clone();
    println!("{}", consume(v));
    println!("{:?}", w);
    let s1 = String::from("hello");
    let s2 = s1;
    println!("{}", s2);
    let mut s3 = s2.clone();
    append(&mut s3, " world");
    println!("{} {}", s2, s3);
    println!("{}", longest("abc", "de"));

    let _a = Noisy("a");
    {
        let _b = Noisy("b");
        let _c = Noisy("c");
        println!("inner scope");
    }
    let d = Noisy("d");
    drop(d);
    let e = Noisy("e");
    let moved = e;
    println!("moved {}", moved.0);

    let mut x = String::from("x");
    let mut y = String::from("y");
    std::mem::swap(&mut x, &mut y);
    println!("{} {}", x, y);
    let old = std::mem::replace(&mut x, String::from("new"));
    println!("{} {}", old, x);
    let t = std::mem::take(&mut y);
    println!("[{}] [{}]", t, y);
    println!("end of main");
}
