use std::collections::{HashMap, HashSet};
use std::hash::{Hash, Hasher};
use std::collections::hash_map::DefaultHasher;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
struct Point { x: i32, y: i32 }

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
struct Person { name: String, age: u32 }

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
enum Color { Red, Green, Blue }

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
enum Shape {
    Dot,
    Circle(i32),
    Rect { w: u32, h: u32 },
    Labeled(String, Color),
}

#[derive(Debug, PartialEq, Eq, Hash)]
struct Wrapper<T>(T, Vec<T>);

#[derive(PartialEq, Eq, Hash, Clone, Copy, Debug)]
struct Unit;

fn hash_of<T: Hash + ?Sized>(t: &T) -> u64 {
    let mut h = DefaultHasher::new();
    t.hash(&mut h);
    h.finish()
}

fn main() {
    let mut grid: HashMap<Point, &str> = HashMap::new();
    grid.insert(Point { x: 0, y: 0 }, "origin");
    grid.insert(Point { x: 1, y: 2 }, "a");
    grid.insert(Point { x: 2, y: 1 }, "b");
    grid.insert(Point { x: 1, y: 2 }, "a2");
    println!("{} {:?} {:?} {:?}", grid.len(), grid.get(&Point { x: 1, y: 2 }), grid.get(&Point { x: 2, y: 1 }), grid.get(&Point { x: 9, y: 9 }));

    let mut seen: HashSet<Point> = HashSet::new();
    let mut dups = 0;
    for i in 0..40 {
        let p = Point { x: i % 5, y: i % 3 };
        if !seen.insert(p) { dups += 1; }
    }
    println!("unique {} dups {}", seen.len(), dups);

    let mut ages: HashMap<Person, u32> = HashMap::new();
    ages.insert(Person { name: "ann".to_string(), age: 30 }, 1);
    ages.insert(Person { name: "bob".to_string(), age: 25 }, 2);
    *ages.entry(Person { name: "ann".to_string(), age: 30 }).or_insert(0) += 10;
    println!("{:?} {:?}", ages.get(&Person { name: "ann".to_string(), age: 30 }), ages.get(&Person { name: "ann".to_string(), age: 31 }));

    let mut by_color: HashMap<Color, Vec<u32>> = HashMap::new();
    for i in 0..9u32 {
        let c = match i % 3 { 0 => Color::Red, 1 => Color::Green, _ => Color::Blue };
        by_color.entry(c).or_default().push(i);
    }
    let mut cs: Vec<_> = by_color.iter().collect();
    cs.sort();
    println!("{:?}", cs);

    let shapes = vec![
        Shape::Dot, Shape::Circle(3), Shape::Circle(4), Shape::Rect { w: 1, h: 2 },
        Shape::Rect { w: 2, h: 1 }, Shape::Labeled("a".to_string(), Color::Red),
        Shape::Labeled("a".to_string(), Color::Blue), Shape::Dot, Shape::Circle(3),
    ];
    let set: HashSet<Shape> = shapes.iter().cloned().collect();
    println!("shapes {} of {}", set.len(), shapes.len());
    println!("{}", set.contains(&Shape::Rect { w: 2, h: 1 }));
    println!("{}", set.contains(&Shape::Rect { w: 3, h: 1 }));

    // equal values hash equal; different variants differ
    println!("{}", hash_of(&Point { x: 3, y: 4 }) == hash_of(&Point { x: 3, y: 4 }));
    println!("{}", hash_of(&Point { x: 3, y: 4 }) == hash_of(&Point { x: 4, y: 3 }));
    println!("{}", hash_of(&Shape::Circle(1)) == hash_of(&Shape::Circle(1)));
    println!("{}", hash_of(&Shape::Dot) == hash_of(&Shape::Circle(0)));
    println!("{}", hash_of(&Color::Red) == hash_of(&Color::Green));
    println!("{}", hash_of(&Unit) == hash_of(&Unit));
    println!("{}", hash_of(&"abc".to_string()) == hash_of(&"abc"));
    println!("{}", hash_of(&(1, "x")) == hash_of(&(1, "x")));
    println!("{}", hash_of(&vec![1, 2, 3]) == hash_of(&[1, 2, 3][..]));

    let mut w: HashSet<Wrapper<i32>> = HashSet::new();
    w.insert(Wrapper(1, vec![2, 3]));
    w.insert(Wrapper(1, vec![2, 3]));
    w.insert(Wrapper(1, vec![3, 2]));
    println!("{}", w.len());

    let mut u: HashSet<Unit> = HashSet::new();
    u.insert(Unit);
    u.insert(Unit);
    println!("{}", u.len());

    let mut nested: HashMap<(Color, Option<Point>), i32> = HashMap::new();
    nested.insert((Color::Red, None), 1);
    nested.insert((Color::Red, Some(Point { x: 1, y: 1 })), 2);
    println!("{:?} {:?}", nested.get(&(Color::Red, None)), nested.get(&(Color::Red, Some(Point { x: 1, y: 1 }))));
}
