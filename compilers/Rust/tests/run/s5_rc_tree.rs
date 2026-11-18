use std::cell::RefCell;
use std::rc::{Rc, Weak};

struct Node {
    value: i32,
    parent: RefCell<Weak<Node>>,
    children: RefCell<Vec<Rc<Node>>>,
}

impl Drop for Node {
    fn drop(&mut self) {
        println!("drop node {}", self.value);
    }
}

fn new_node(value: i32) -> Rc<Node> {
    Rc::new(Node { value, parent: RefCell::new(Weak::new()), children: RefCell::new(vec![]) })
}

fn add_child(parent: &Rc<Node>, child: Rc<Node>) {
    *child.parent.borrow_mut() = Rc::downgrade(parent);
    parent.children.borrow_mut().push(child);
}

fn sum(n: &Rc<Node>) -> i32 {
    let mut s = n.value;
    for c in n.children.borrow().iter() {
        s += sum(c);
    }
    s
}

fn path_to_root(n: &Rc<Node>) -> Vec<i32> {
    let mut out = vec![n.value];
    let mut cur = n.parent.borrow().upgrade();
    while let Some(p) = cur {
        out.push(p.value);
        cur = p.parent.borrow().upgrade();
    }
    out
}

fn main() {
    let root = new_node(1);
    let a = new_node(2);
    let b = new_node(3);
    let c = new_node(4);
    add_child(&root, a.clone());
    add_child(&root, b.clone());
    add_child(&a, c.clone());
    println!("sum = {}", sum(&root));
    println!("path c = {:?}", path_to_root(&c));
    println!("root strong={} weak={}", Rc::strong_count(&root), Rc::weak_count(&root));
    println!("a strong={} weak={}", Rc::strong_count(&a), Rc::weak_count(&a));
    println!("c parent = {}", c.parent.borrow().upgrade().map(|p| p.value).unwrap_or(-1));
    drop(a);
    drop(b);
    println!("dropped a, b handles");
    println!("children of root: {}", root.children.borrow().len());
    drop(root);
    println!("after root dropped, c parent alive = {}", c.parent.borrow().upgrade().is_some());
    println!("end");
}
