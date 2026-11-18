// recursive enum with Box, impl on enums: expression tree and linked list
enum Expr {
    Num(i64),
    Add(Box<Expr>, Box<Expr>),
    Mul(Box<Expr>, Box<Expr>),
    Neg(Box<Expr>),
}

use Expr::*;

fn eval(e: &Expr) -> i64 {
    match e {
        Num(n) => *n,
        Add(a, b) => eval(a) + eval(b),
        Mul(a, b) => eval(a) * eval(b),
        Neg(a) => -eval(a),
    }
}

fn show(e: &Expr) -> String {
    match e {
        Num(n) => n.to_string(),
        Add(a, b) => format!("({} + {})", show(a), show(b)),
        Mul(a, b) => format!("({} * {})", show(a), show(b)),
        Neg(a) => format!("-{}", show(a)),
    }
}

enum List {
    Cons(i32, Box<List>),
    Nil,
}

impl List {
    fn push_front(self, v: i32) -> List {
        List::Cons(v, Box::new(self))
    }
    fn len(&self) -> usize {
        match self {
            List::Cons(_, rest) => 1 + rest.len(),
            List::Nil => 0,
        }
    }
    fn sum(&self) -> i32 {
        match self {
            List::Cons(v, rest) => *v + rest.sum(),
            List::Nil => 0,
        }
    }
}

struct Node {
    val: i32,
    left: Option<Box<Node>>,
    right: Option<Box<Node>>,
}

impl Node {
    fn new(val: i32) -> Node {
        Node { val, left: None, right: None }
    }
    fn insert(&mut self, v: i32) {
        if v < self.val {
            match self.left {
                Some(ref mut l) => l.insert(v),
                None => self.left = Some(Box::new(Node::new(v))),
            }
        } else {
            match self.right {
                Some(ref mut r) => r.insert(v),
                None => self.right = Some(Box::new(Node::new(v))),
            }
        }
    }
    fn walk(&self, out: &mut Vec<i32>) {
        if let Some(l) = &self.left {
            l.walk(out);
        }
        out.push(self.val);
        if let Some(r) = &self.right {
            r.walk(out);
        }
    }
    fn height(&self) -> usize {
        let l = match &self.left {
            Some(n) => n.height(),
            None => 0,
        };
        let r = match &self.right {
            Some(n) => n.height(),
            None => 0,
        };
        1 + if l > r { l } else { r }
    }
}

fn main() {
    let e = Add(
        Box::new(Mul(Box::new(Num(6)), Box::new(Num(7)))),
        Box::new(Neg(Box::new(Num(2)))),
    );
    println!("{} = {}", show(&e), eval(&e));
    let l = List::Nil.push_front(3).push_front(2).push_front(1);
    println!("{} {}", l.len(), l.sum());
    let mut root = Node::new(50);
    for v in [30, 70, 20, 40, 60, 80, 35, 45, 10] {
        root.insert(v);
    }
    let mut out = Vec::new();
    root.walk(&mut out);
    println!("{:?}", out);
    println!("{}", root.height());
}
