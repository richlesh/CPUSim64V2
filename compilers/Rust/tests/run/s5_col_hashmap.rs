use std::collections::HashMap;
use std::collections::hash_map::Entry;

fn main() {
    let text = "the quick brown fox jumps over the lazy dog the fox barks and the dog runs";
    let mut counts: HashMap<String, usize> = HashMap::new();
    for w in text.split_whitespace() {
        *counts.entry(w.to_string()).or_insert(0) += 1;
    }
    let mut keys: Vec<&String> = counts.keys().collect();
    keys.sort();
    for k in keys {
        println!("{:<6}{}", k, counts[k]);
    }
    println!("len {} empty {}", counts.len(), counts.is_empty());
    println!("{:?} {:?}", counts.get("the"), counts.get("cat"));
    println!("{} {}", counts.contains_key("fox"), counts.contains_key("zebra"));

    // insert returns the old value
    let mut m: HashMap<i32, String> = HashMap::new();
    println!("{:?}", m.insert(1, "one".to_string()));
    println!("{:?}", m.insert(1, "uno".to_string()));
    m.insert(2, "two".to_string());
    m.insert(3, "three".to_string());
    println!("{} {}", m[&1], m[&3]);
    if let Some(v) = m.get_mut(&2) {
        v.push_str("!!");
    }
    println!("{:?}", m.remove(&2));
    println!("{:?}", m.remove(&2));
    println!("len {}", m.len());

    // entry API
    let mut e: HashMap<&str, Vec<i32>> = HashMap::new();
    e.entry("a").or_default().push(1);
    e.entry("a").or_default().push(2);
    e.entry("b").or_insert_with(Vec::new).push(3);
    e.entry("b").and_modify(|v| v.push(30)).or_insert_with(Vec::new);
    e.entry("c").and_modify(|v| v.push(99)).or_insert_with(|| vec![7]);
    let mut es: Vec<(&&str, &Vec<i32>)> = e.iter().collect();
    es.sort();
    println!("{:?}", es);
    match e.entry("a") {
        Entry::Occupied(o) => println!("occupied {} {:?}", o.key(), o.get()),
        Entry::Vacant(_) => println!("vacant"),
    }
    match e.entry("zz") {
        Entry::Occupied(_) => println!("occupied"),
        Entry::Vacant(v) => { println!("vacant {}", v.key()); v.insert(vec![0]); }
    }
    println!("{}", e.len());

    // many entries: growth and removal
    let mut big: HashMap<u32, u32> = HashMap::new();
    for i in 0..500u32 {
        big.insert(i, i * i);
    }
    let mut sum = 0u64;
    for (k, v) in &big {
        sum += (*k as u64) + (*v as u64);
    }
    println!("big {} {}", big.len(), sum);
    for i in 0..500u32 {
        if i % 3 != 0 {
            big.remove(&i);
        }
    }
    let mut ks: Vec<u32> = big.keys().copied().collect();
    ks.sort();
    println!("left {} first {:?} last {:?}", big.len(), &ks[..4], ks.last());
    println!("{:?} {:?}", big.get(&9), big.get(&10));
    for v in big.values_mut() {
        *v += 1;
    }
    let total: u32 = big.values().sum();
    println!("total {}", total);
    big.retain(|k, _| k % 2 == 0);
    println!("retained {}", big.len());

    // collect, extend, clone, eq, into_iter
    let a: HashMap<i32, i32> = (1..=5).map(|x| (x, x * 10)).collect();
    let mut b = a.clone();
    println!("{}", a == b);
    b.insert(6, 60);
    println!("{}", a == b);
    b.extend(vec![(7, 70), (1, 11)]);
    let mut items: Vec<(i32, i32)> = b.into_iter().collect();
    items.sort();
    println!("{:?}", items);
    let d: HashMap<String, i32> = HashMap::default();
    println!("{:?} {}", d, d.len());
    let mut one = HashMap::new();
    one.insert("k", 1);
    println!("{:?}", one);
    println!("{:#?}", one);
    let from = HashMap::from([(1, 'a'), (2, 'b')]);
    println!("{}", from.len());
    for (_, v) in from.iter() {
        assert!(*v == 'a' || *v == 'b');
    }
    let mut tup: HashMap<(i32, i32), &str> = HashMap::new();
    tup.insert((1, 2), "x");
    tup.insert((2, 1), "y");
    println!("{:?} {:?} {:?}", tup.get(&(1, 2)), tup.get(&(2, 1)), tup.get(&(3, 3)));
    let mut cleared = a.clone();
    cleared.clear();
    println!("{} {}", cleared.len(), cleared.get(&1).is_none());
}
