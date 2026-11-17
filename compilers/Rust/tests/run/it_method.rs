use std::fmt::Display;

struct Bag {
    items: Vec<i32>,
    base: i32,
}

impl Bag {
    fn new() -> Bag {
        Bag { items: vec![3, 1, 4, 1, 5], base: 100 }
    }

    fn iter_big(&self, min: i32) -> impl Iterator<Item = &i32> + '_ {
        self.items.iter().filter(move |x| **x >= min)
    }

    fn offset(&self) -> impl Fn(i32) -> i32 {
        let b = self.base;
        move |x| x + b
    }

    fn describe(&self) -> impl Display {
        self.items.len()
    }

    fn scaled(&self, k: i32) -> impl Iterator<Item = i32> {
        self.items.clone().into_iter().map(move |x| x * k)
    }
}

fn main() {
    let b = Bag::new();
    let v: Vec<&i32> = b.iter_big(3).collect();
    println!("{:?}", v);
    println!("{}", b.offset()(5));
    println!("{}", b.describe());
    println!("{}", b.scaled(2).sum::<i32>());
}
