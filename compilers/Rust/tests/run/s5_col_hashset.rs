use std::collections::HashSet;

fn sorted<'a, I: Iterator<Item = &'a i32>>(it: I) -> Vec<i32> {
    let mut v: Vec<i32> = it.copied().collect();
    v.sort();
    v
}

fn main() {
    let mut s: HashSet<i32> = HashSet::new();
    println!("{} {}", s.insert(3), s.insert(5));
    println!("{} {}", s.insert(3), s.len());
    println!("{} {}", s.contains(&5), s.contains(&6));
    println!("{} {}", s.remove(&5), s.remove(&5));
    println!("{:?} {}", s, s.is_empty());

    let a: HashSet<i32> = (1..=6).collect();
    let b: HashSet<i32> = [4, 5, 6, 7, 8].into_iter().collect();
    println!("{:?}", sorted(a.union(&b)));
    println!("{:?}", sorted(a.intersection(&b)));
    println!("{:?}", sorted(a.difference(&b)));
    println!("{:?}", sorted(b.difference(&a)));
    println!("{:?}", sorted(a.symmetric_difference(&b)));
    println!("{}", a.union(&b).count());
    let c: HashSet<i32> = [1, 2].into_iter().collect();
    println!("{} {} {}", c.is_subset(&a), a.is_superset(&c), a.is_disjoint(&b));
    println!("{}", c.is_disjoint(&b));
    let u = &a | &b;
    let i = &a & &b;
    println!("{} {}", u.len(), i.len());

    // strings, &str lookups
    let mut words: HashSet<String> = HashSet::new();
    for w in "a b c a b d e a".split(' ') {
        words.insert(w.to_string());
    }
    let mut ws: Vec<&String> = words.iter().collect();
    ws.sort();
    println!("{:?} {}", ws, words.len());
    println!("{} {}", words.contains("a"), words.contains("z"));
    println!("{:?}", words.get("b"));
    println!("{:?}", words.take("c"));
    println!("{}", words.len());

    // a single element prints deterministically
    let one: HashSet<&str> = ["only"].into_iter().collect();
    println!("{:?}", one);
    println!("{:#?}", one);

    // equality, clone, extend, retain
    let mut d = a.clone();
    println!("{}", d == a);
    d.insert(100);
    println!("{}", d == a);
    d.extend(vec![200, 300, 1]);
    d.retain(|x| *x > 5);
    println!("{:?}", sorted(d.iter()));
    let mut total = 0;
    for x in &d { total += *x; }
    println!("{}", total);
    let mut v: Vec<i32> = d.into_iter().collect();
    v.sort_by(|x, y| y.cmp(x));
    println!("{:?}", v);

    // many elements
    let mut big = HashSet::new();
    for i in 0..1000 { big.insert((i * 7) % 301); }
    println!("{}", big.len());
    for i in 0..150 { big.remove(&i); }
    println!("{}", big.len());
    let mut t: HashSet<(i32, char)> = HashSet::new();
    t.insert((1, 'a'));
    t.insert((1, 'b'));
    t.insert((1, 'a'));
    println!("{}", t.len());
    let e: HashSet<u8> = HashSet::default();
    println!("{:?}", e);
}
